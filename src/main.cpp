#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "game.h"
#include "presentation.h"

namespace
{
// Immediate response on press, then controlled repeat for comfortable grid navigation.
bool repeat(bool held, int& frames)
{
    if(! held) { frames = 0; return false; }
    ++frames;
    return frames == 1 || (frames > 16 && (frames - 16) % 5 == 0);
}

class Application
{
public:
    void run()
    {
        while(true)
        {
            handle_input();
            const auto old_phase = _game.phase();
            _game.tick();
            play_events(old_phase);
            if(_game.phase() == garden::Phase::playing && _view.feedback_frames > 0)
            { --_view.feedback_frames; }
            _view.shoveling = bn::keypad::select_held();
            _presentation.update(_game, _view);
            bn::core::update();
        }
    }

private:
    void play_events(garden::Phase previous_phase)
    {
        using garden::Phase;
        if(previous_phase != _game.phase())
        {
            if(_game.phase() == Phase::won) { bn::sound_items::win.play(); return; }
            if(_game.phase() == Phase::lost) { bn::sound_items::lose.play(); return; }
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
        if(events.explosion) { bn::sound_items::boom.play(); }
        else if(events.mower) { bn::sound_items::mower.play(); }
        else if(events.wave_started) { bn::sound_items::wave_start.play(); }
        else if(events.wave_cleared) { bn::sound_items::wave_clear.play(); }
        else if(! _combat_sound_cooldown && (events.hit || events.shot))
        {
            if(events.hit) { bn::sound_items::hit.play(); }
            else { bn::sound_items::shot.play(); }
            _combat_sound_cooldown = 8;
        }
    }

    void start_game()
    {
        _game.start();
        _view = garden::ViewState();
        _repeat = {};
        _combat_sound_cooldown = 0;
        _bite_sound_cooldown = 0;
    }

    void show_title()
    {
        _game = garden::Game();
        _view = garden::ViewState();
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
        using namespace garden;
        if(_game.phase() == Phase::title || _game.phase() == Phase::won || _game.phase() == Phase::lost)
        {
            if(bn::keypad::start_pressed() || bn::keypad::a_pressed()) { start_game(); }
            else if(bn::keypad::b_pressed() && _game.phase() != Phase::title) { show_title(); }
            return;
        }
        if(_game.phase() == Phase::paused)
        {
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
            if(bn::keypad::a_pressed())
            {
                switch(_view.pause_choice)
                {
                case PauseChoice::resume: _game.toggle_pause(); _repeat = {}; break;
                case PauseChoice::restart: start_game(); break;
                case PauseChoice::title: show_title(); break;
                default: break;
                }
            }
            return;
        }
        if(bn::keypad::start_pressed())
        {
            _game.toggle_pause();
            _view.pause_choice = PauseChoice::resume;
            _repeat = {};
            return;
        }
        if(_game.phase() != Phase::playing) { return; }
        if(repeat(bn::keypad::up_held(), _repeat[0]) && _view.cursor.row > 0) { --_view.cursor.row; }
        if(repeat(bn::keypad::down_held(), _repeat[1]) && _view.cursor.row < rows - 1) { ++_view.cursor.row; }
        if(repeat(bn::keypad::left_held(), _repeat[2]) && _view.cursor.column > 0) { --_view.cursor.column; }
        if(repeat(bn::keypad::right_held(), _repeat[3]) && _view.cursor.column < columns - 1) { ++_view.cursor.column; }
        int seed = static_cast<int>(_view.cursor.seed);
        if(bn::keypad::l_pressed()) { _view.cursor.seed = static_cast<PlantType>((seed + plant_count - 1) % plant_count); }
        if(bn::keypad::r_pressed()) { _view.cursor.seed = static_cast<PlantType>((seed + 1) % plant_count); }
        if(seed != static_cast<int>(_view.cursor.seed)) { bn::sound_items::navigate.play(); }
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

    garden::Game _game;
    garden::ViewState _view;
    garden::Presentation _presentation;
    std::array<int, 4> _repeat{};
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
