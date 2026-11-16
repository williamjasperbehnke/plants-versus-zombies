#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_sound.h"
#include "bn_music.h"
#include "bn_music_items.h"
#include "game.h"
#include "presentation.h"
#include "save_store.h"
#include "menu_layout.h"

namespace
{
// Immediate response on press, then controlled repeat for comfortable grid navigation.
bool repeat(bool held, int& frames, int delay = 16, int interval = 5)
{
    if(! held) { frames = 0; return false; }
    ++frames;
    return frames == 1 || (frames > delay && (frames - delay) % interval == 0);
}

class Application
{
public:
    void run()
    {
        _game.load_progress(_save.load());
        _save.update(_game.progress());
        bn::music_items::garden_theme.play();
        apply_audio();
        while(true)
        {
            handle_input();
            if(_game.phase() != garden::Phase::lost && ! bn::music::playing())
            { bn::music_items::garden_theme.play(); apply_audio(); }
            const auto old_phase = _game.phase();
            _game.tick();
            play_events(old_phase);
            _save.update(_game.progress());
            if(_game.phase() == garden::Phase::playing && _view.feedback_frames > 0)
            { --_view.feedback_frames; }
            if(_view.seed_error_frames > 0) { --_view.seed_error_frames; }
            _view.shoveling = bn::keypad::select_held();
            _presentation.update(_game, _view);
            bn::core::update();
        }
    }

private:
    void apply_audio()
    {
        bn::sound::set_master_volume(bn::fixed(_view.audio.gain(garden::AudioChannel::effects)) / 100);
        bn::music::set_volume(bn::fixed(_view.audio.gain(garden::AudioChannel::music)) / 100);
    }

    void reset_view()
    {
        auto audio = _view.audio;
        _view = garden::ViewState();
        _view.audio = audio;
        _seed_repeat = {};
    }

    void play_events(garden::Phase previous_phase)
    {
        using garden::Phase;
        if(previous_phase != _game.phase())
        {
            if(_game.phase() == Phase::lost)
            {
                bn::music::stop();
                bn::sound::stop_all();
                bn::sound_items::record_scratch.play();
                return;
            }
        }
        if(_game.phase() == Phase::won)
        {
            int frame = _game.reward_frames();
            if(frame == 1) { bn::sound_items::win.play(); }
            if(frame == 48 || frame == 84 || frame == 112 || frame == 132)
            { bn::sound_items::plant.play(bn::fixed(0.35)); }
            if(frame == garden::reward_reveal_frame) { bn::sound_items::collect.play(); }
        }
        if(_game.phase() == Phase::lost)
        {
            if(_game.loss_frames() == garden::defeat_audio::scream_frame) { bn::sound_items::scream.play(); }
            return;
        }
        if(_game.phase() != Phase::playing) { return; }
        if(_combat_sound_cooldown > 0) { --_combat_sound_cooldown; }
        const auto& events = _game.events();
        if(_bite_sound_cooldown > 0) { --_bite_sound_cooldown; }
        if(events.bite && _bite_sound_cooldown == 0)
        {
            bn::sound_items::chew.play(bn::fixed(0.72));
            _bite_sound_cooldown = 12;
        }
        if(events.chomp) { bn::sound_items::chew.play(bn::fixed(0.9)); }
        if(events.explosion) { bn::sound_items::boom.play(); }
        else if(events.mower) { bn::sound_items::mower.play(); }
        else if(events.final_wave_approaching || events.horde_started) { bn::sound_items::wave_start.play(); }
        else if(! _combat_sound_cooldown && (events.hit || events.shot))
        {
            if(events.hit) { bn::sound_items::hit.play(); }
            else { bn::sound_items::shot.play(); }
            _combat_sound_cooldown = 8;
        }
    }

    void start_game()
    {
        _game.prepare_level();
        reset_view();
        _repeat = {};
        _combat_sound_cooldown = 0;
        _bite_sound_cooldown = 0;
    }

    void show_title()
    {
        _game.return_to_title();
        reset_view();
        _repeat = {};
    }

    void show_level_select()
    {
        _game.open_level_select();
        reset_view();
        _repeat = {};
    }

    void feedback(garden::Feedback kind)
    {
        _view.feedback = kind;
        _view.feedback_frames = 30;
        _view.feedback_seed = _view.cursor.seed;
        if(kind != garden::Feedback::collected)
        {
            _view.feedback_x = garden::cell_x(_view.cursor.column);
            _view.feedback_y = garden::cell_y(_view.cursor.row);
        }
    }

    void handle_input()
    {
        using garden::Phase;
        if(_game.phase() != Phase::playing) { _seed_repeat = {}; }
        switch(_game.phase())
        {
        case Phase::almanac: handle_almanac_input(); break;
        case Phase::preview: handle_preview_input(); break;
        case Phase::seed_select: handle_seed_select_input(); break;
        case Phase::lost: handle_lost_input(); break;
        case Phase::level_select: handle_level_select_input(); break;
        case Phase::title: handle_title_input(); break;
        case Phase::won: handle_won_input(); break;
        case Phase::paused: handle_paused_input(); break;
        case Phase::playing: handle_playing_input(); break;
        default: break;
        }
    }

    void handle_almanac_input()
    {
        using namespace garden;
        if(bn::keypad::b_pressed())
        {
            if(_view.almanac_details) { _view.almanac_details = false; }
            else { _game.close_almanac(); }
            _repeat = {};
            return;
        }
        if(_view.almanac_details) { return; }
        if(bn::keypad::a_pressed())
        { _view.almanac_details = true; _repeat = {}; bn::sound_items::navigate.play(); return; }
        if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
        {
            constexpr int plant_pages = (plant_count + 5) / 6;
            constexpr int pages = plant_pages + (zombie_type_count + 5) / 6;
            int page = (_view.almanac_zombies ? plant_pages : 0) + _view.almanac_entry / 6;
            page = (page + pages + (bn::keypad::r_pressed() ? 1 : -1)) % pages;
            _view.almanac_zombies = page >= plant_pages;
            _view.almanac_entry = (page - (_view.almanac_zombies ? plant_pages : 0)) * 6;
            bn::sound_items::navigate.play();
        }
        int horizontal = 0, vertical = 0;
        if(repeat(bn::keypad::left_held(), _repeat[2])) { --horizontal; }
        if(repeat(bn::keypad::right_held(), _repeat[3])) { ++horizontal; }
        if(repeat(bn::keypad::up_held(), _repeat[0])) { --vertical; }
        if(repeat(bn::keypad::down_held(), _repeat[1])) { ++vertical; }
        if(horizontal || vertical)
        {
            int count = _view.almanac_zombies ? zombie_type_count : plant_count;
            _view.almanac_entry = menu_layout::almanac_move(_view.almanac_entry, count, horizontal, vertical);
            bn::sound_items::navigate.play();
        }
    }

    void handle_preview_input()
    {
        if(_game.preview_returning()) { return; }
        if(bn::keypad::b_pressed())
        { show_level_select(); }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            _game.choose_seeds();
            if(_game.preview_returning()) { _view.cursor.seed = _game.seed_at(0); }
            _repeat = {};
        }
    }

    void handle_seed_select_input()
    {
        using namespace garden;
        if(bn::keypad::b_pressed()) { _game.cancel_seed_selection(); _view.seed_error_frames = 0; _repeat = {}; return; }
        int delta = 0;
        if(repeat(bn::keypad::left_held(), _repeat[2])) { --delta; }
        if(repeat(bn::keypad::right_held(), _repeat[3])) { ++delta; }
        if(repeat(bn::keypad::up_held(), _repeat[0])) { delta -= menu_layout::seed_columns; }
        if(repeat(bn::keypad::down_held(), _repeat[1])) { delta += menu_layout::seed_columns; }
        if(delta) { _view.seed_choice = (_view.seed_choice + plant_count + delta) % plant_count; bn::sound_items::navigate.play(); }
        if(bn::keypad::a_pressed())
        {
            if(_game.toggle_seed(seed_order[_view.seed_choice])) { _view.seed_error_frames = 0; bn::sound_items::navigate.play(); }
            else { bn::sound_items::error.play(); }
        }
        if(bn::keypad::start_pressed())
        {
            _game.begin_level();
            if(_game.phase() == Phase::preview) { _view.cursor.seed = _game.seed_at(0); _repeat = {}; }
            else { _view.seed_error_frames = 120; bn::sound_items::error.play(); }
        }
    }

    void handle_lost_input()
    {
        if(! _game.loss_ready()) { return; }
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed()) { start_game(); }
        else if(bn::keypad::b_pressed())
        { show_level_select(); }
    }

    void handle_level_select_input()
    {
        using namespace garden;
        int delta = 0;
        if(repeat(bn::keypad::left_held(), _repeat[2]) && _game.level() % menu_layout::level_columns > 0) { --delta; }
        if(repeat(bn::keypad::right_held(), _repeat[3]) && _game.level() % menu_layout::level_columns < menu_layout::level_columns - 1) { ++delta; }
        if(repeat(bn::keypad::up_held(), _repeat[0]) && _game.level() % levels_per_world >= menu_layout::level_columns) { delta -= menu_layout::level_columns; }
        if(repeat(bn::keypad::down_held(), _repeat[1]) && _game.level() % levels_per_world < menu_layout::level_columns) { delta += menu_layout::level_columns; }
        if(delta) { _game.select_level(delta); bn::sound_items::navigate.play(); }
        if(bn::keypad::b_pressed()) { show_title(); }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            if(_game.level_unlocked(_game.level())) { start_game(); }
            else { bn::sound_items::error.play(); }
        }
    }

    void handle_title_input()
    {
        using namespace garden;
        int choice = static_cast<int>(_view.title_choice);
        bool up = repeat(bn::keypad::up_held(), _repeat[0]);
        bool down = repeat(bn::keypad::down_held(), _repeat[1]);
        if(up != down)
        {
            choice = (choice + 1) % static_cast<int>(TitleChoice::count);
            _view.title_choice = static_cast<TitleChoice>(choice);
            bn::sound_items::navigate.play();
        }
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            if(_view.title_choice == TitleChoice::almanac) { _game.open_almanac(); }
            else { show_level_select(); }
            _repeat = {};
        }
    }

    void handle_won_input()
    {
        if(_game.reward_ready() && bn::keypad::any_pressed()) { show_level_select(); }
    }

    void handle_paused_input()
    {
        using namespace garden;
        if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            _game.toggle_pause();
            _repeat = {};
            return;
        }
        constexpr int choices = static_cast<int>(PauseChoice::count);
        int choice = static_cast<int>(_view.pause_choice);
        if(repeat(bn::keypad::up_held(), _repeat[0])) { choice = (choice + choices - 1) % choices; }
        if(repeat(bn::keypad::down_held(), _repeat[1])) { choice = (choice + 1) % choices; }
        if(choice != static_cast<int>(_view.pause_choice)) { bn::sound_items::navigate.play(); }
        _view.pause_choice = static_cast<PauseChoice>(choice);
        int delta = 0;
        if(repeat(bn::keypad::left_held(), _repeat[2])) { --delta; }
        if(repeat(bn::keypad::right_held(), _repeat[3])) { ++delta; }
        if(choice >= static_cast<int>(PauseChoice::master) && delta)
        {
            auto channel = static_cast<AudioChannel>(choice - static_cast<int>(PauseChoice::master));
            if(_view.audio.adjust(channel, delta))
            {
                apply_audio();
                bn::sound_items::navigate.play();
            }
        }
        if(bn::keypad::a_pressed())
        {
            switch(_view.pause_choice)
            {
            case PauseChoice::resume: _game.toggle_pause(); _repeat = {}; break;
            case PauseChoice::restart: start_game(); break;
            case PauseChoice::choose_levels: show_level_select(); break;
            default: break;
            }
        }
    }

    void handle_playing_input()
    {
        using namespace garden;
        if(bn::keypad::start_pressed())
        {
            _game.toggle_pause();
            _view.pause_choice = PauseChoice::resume;
            _repeat = {};
            return;
        }
        if(repeat(bn::keypad::up_held(), _repeat[0]) && _view.cursor.row > _game.first_row()) { --_view.cursor.row; }
        if(repeat(bn::keypad::down_held(), _repeat[1]) && _view.cursor.row < _game.last_row()) { ++_view.cursor.row; }
        if(repeat(bn::keypad::left_held(), _repeat[2]) && _view.cursor.column > 0) { --_view.cursor.column; }
        if(repeat(bn::keypad::right_held(), _repeat[3]) && _view.cursor.column < columns - 1) { ++_view.cursor.column; }
        int seed = _game.bank_slot(_view.cursor.seed);
        int count = _game.available_plants();
        int seed_delta = 0;
        if(repeat(bn::keypad::l_held(), _seed_repeat[0], 20, 10)) { --seed_delta; }
        if(repeat(bn::keypad::r_held(), _seed_repeat[1], 20, 10)) { ++seed_delta; }
        _view.cursor.seed = _game.seed_at((seed + count + seed_delta) % count);
        if(seed != _game.bank_slot(_view.cursor.seed)) { bn::sound_items::navigate.play(); }
        if(bn::keypad::b_pressed() || (bn::keypad::b_held() && _game.elapsed() % 15 == 0))
        {
            int collected = _game.collect_sun();
            if(collected)
            {
                bn::sound_items::collect.play();
                feedback(Feedback::collected);
            }
        }
        if(! bn::keypad::a_pressed()) { return; }
        if(bn::keypad::select_held())
        {
            if(_game.shovel(_view.cursor.row, _view.cursor.column))
            {
                bn::sound_items::plant.play();
                feedback(Feedback::removed);
            }
            return;
        }
        switch(_game.plant(_view.cursor.row, _view.cursor.column, _view.cursor.seed))
        {
        case PlantResult::planted:
            bn::sound_items::plant.play();
            feedback(Feedback::planted);
            break;
        case PlantResult::occupied: bn::sound_items::error.play(); feedback(Feedback::occupied); break;
        case PlantResult::insufficient_sun: bn::sound_items::error.play(); feedback(Feedback::insufficient_sun); break;
        case PlantResult::cooling_down: bn::sound_items::error.play(); feedback(Feedback::cooling_down); break;
        default: break;
        }
    }

    garden::SaveStore _save;
    garden::Game _game;
    garden::ViewState _view;
    garden::Presentation _presentation;
    std::array<int, 4> _repeat{};
    std::array<int, 2> _seed_repeat{};
    int _bite_sound_cooldown = 0;
    int _combat_sound_cooldown = 0;
};
}

int main()
{
    bn::core::init();
    Application application;
    application.run();
}
