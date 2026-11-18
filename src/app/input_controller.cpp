#include "app/input_controller.h"

#include "ui/almanac.h"
#include "ui/menu_layout.h"

namespace garden {
void InputController::update(const InputFrame& input) {
    _input = input;
    _collected = false;
    _commands.clear();
    handle_input();
    refresh_saves();
}

void InputController::finish_frame() {
    if (_game.phase() == Phase::playing && _view.feedback_frames > 0) {
        --_view.feedback_frames;
    }
    if (_view.seed_error_frames > 0) {
        --_view.seed_error_frames;
    }
    auto previous_step = _tutorial.prompt().step;
    _tutorial.update(_game, _collected);
    auto step = _tutorial.prompt().step;
    if (step != previous_step && (step == TutorialStep::collect_sun || step == TutorialStep::sunflower_sun)) {
        _game.hasten_sun(tutorial_sun_delay, step == TutorialStep::sunflower_sun);
    }
    _view.tutorial = _tutorial.prompt();
    _view.shoveling = _input.held(Button::select);
}

void InputController::reset_view() {
    auto audio = _view.audio;
    _view = garden::ViewState();
    _view.audio = audio;
    _tutorial = {};
    _seed_repeat = {};
    _repeat = {};
}

void InputController::start_game() {
    _game.prepare_level();
    reset_view();
    _tutorial.begin(_game);
    _view.tutorial = _tutorial.prompt();
    _commands.push(AudioCommand::reset_combat);
}

void InputController::show_title() {
    _game.return_to_title();
    reset_view();
}

void InputController::show_level_select() {
    _game.open_level_select();
    reset_view();
}

void InputController::feedback(garden::Feedback kind) {
    _view.feedback = kind;
    _view.feedback_frames = 30;
    _view.feedback_seed = _view.cursor.seed;
    if (kind != garden::Feedback::collected) {
        _view.feedback_x = garden::cell_x(_view.cursor.column);
        _view.feedback_y = garden::cell_y(_view.cursor.row);
    }
}

void InputController::handle_input() {
    if (_game.phase() != Phase::playing) {
        _seed_repeat = {};
    }
    switch (_game.phase()) {
        case Phase::save_select:
            handle_save_input();
            break;
        case Phase::almanac:
            handle_almanac_input();
            break;
        case Phase::preview:
            handle_preview_input();
            break;
        case Phase::seed_select:
            handle_seed_select_input();
            break;
        case Phase::lost:
            handle_lost_input();
            break;
        case Phase::level_select:
            handle_level_select_input();
            break;
        case Phase::title:
            handle_title_input();
            break;
        case Phase::won:
            handle_won_input();
            break;
        case Phase::paused:
            handle_paused_input();
            break;
        case Phase::playing:
            handle_playing_input();
            break;
        default:
            break;
    }
}

void InputController::handle_almanac_input() {
    if (_input.pressed(Button::b)) {
        if (_view.almanac.details) {
            _view.almanac.details = false;
            _view.almanac.scroll = 0;
        } else {
            _game.close_almanac();
        }
        _repeat = {};
        return;
    }
    if (_view.almanac.details) {
        int limit = almanac_scroll_limit(_view.almanac.zombies, _view.almanac.entry);
        if (_repeat[0].update(_input.held(Button::up)) && _view.almanac.scroll > 0) {
            --_view.almanac.scroll;
        }
        if (_repeat[1].update(_input.held(Button::down)) && _view.almanac.scroll < limit) {
            ++_view.almanac.scroll;
        }
        return;
    }
    if (_input.pressed(Button::a)) {
        _view.almanac.details = true;
        _repeat = {};
        _commands.push(AudioCommand::navigate);
        return;
    }
    if (_input.pressed(Button::l) || _input.pressed(Button::r)) {
        constexpr int plant_pages = menu_layout::almanac_pages(plant_count);
        constexpr int pages = plant_pages + menu_layout::almanac_pages(zombie_type_count);
        int page_offset = _view.almanac.zombies ? plant_pages : 0;
        int page = page_offset + _view.almanac.entry / menu_layout::almanac_page_size;
        page = menu_layout::wrap_index(page + (_input.pressed(Button::r) ? 1 : -1), pages);
        _view.almanac.zombies = page >= plant_pages;
        page_offset = _view.almanac.zombies ? plant_pages : 0;
        _view.almanac.entry = (page - page_offset) * menu_layout::almanac_page_size;
        _commands.push(AudioCommand::navigate);
    }
    int horizontal = 0, vertical = 0;
    if (_repeat[2].update(_input.held(Button::left))) {
        --horizontal;
    }
    if (_repeat[3].update(_input.held(Button::right))) {
        ++horizontal;
    }
    if (_repeat[0].update(_input.held(Button::up))) {
        --vertical;
    }
    if (_repeat[1].update(_input.held(Button::down))) {
        ++vertical;
    }
    if (horizontal || vertical) {
        int count = _view.almanac.zombies ? zombie_type_count : plant_count;
        _view.almanac.entry = menu_layout::almanac_move(_view.almanac.entry, count, horizontal, vertical);
        _commands.push(AudioCommand::navigate);
    }
}

void InputController::handle_preview_input() {
    if (_game.preview_returning()) {
        return;
    }
    if (_input.pressed(Button::b)) {
        show_level_select();
    } else if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
        _game.choose_seeds();
        if (_game.preview_returning()) {
            _view.cursor.seed = _game.seed_at(0);
        }
        _repeat = {};
    }
}

void InputController::handle_seed_select_input() {
    if (_input.pressed(Button::b)) {
        _game.cancel_seed_selection();
        _view.seed_error_frames = 0;
        _repeat = {};
        return;
    }
    int delta = 0;
    if (_repeat[2].update(_input.held(Button::left))) {
        --delta;
    }
    if (_repeat[3].update(_input.held(Button::right))) {
        ++delta;
    }
    if (_repeat[0].update(_input.held(Button::up))) {
        delta -= menu_layout::seed_columns;
    }
    if (_repeat[1].update(_input.held(Button::down))) {
        delta += menu_layout::seed_columns;
    }
    if (delta) {
        _view.seed_choice = menu_layout::wrap_index(_view.seed_choice + delta, plant_count);
        _commands.push(AudioCommand::navigate);
    }
    if (_input.pressed(Button::a)) {
        if (_game.toggle_seed(seed_order[_view.seed_choice])) {
            _view.seed_error_frames = 0;
            _commands.push(AudioCommand::navigate);
        } else {
            _commands.push(AudioCommand::error);
        }
    }
    if (_input.pressed(Button::start)) {
        _game.begin_level();
        if (_game.phase() == Phase::preview) {
            _view.cursor.seed = _game.seed_at(0);
            _repeat = {};
        } else {
            _view.seed_error_frames = 120;
            _commands.push(AudioCommand::error);
        }
    }
}

void InputController::handle_lost_input() {
    if (!_game.loss_ready()) {
        return;
    }
    if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
        start_game();
    } else if (_input.pressed(Button::b)) {
        show_level_select();
    }
}

void InputController::handle_level_select_input() {
    int delta = 0;
    if (_repeat[2].update(_input.held(Button::left)) && _game.level() % menu_layout::level_columns > 0) {
        --delta;
    }
    if (_repeat[3].update(_input.held(Button::right)) &&
        _game.level() % menu_layout::level_columns < menu_layout::level_columns - 1) {
        ++delta;
    }
    if (_repeat[0].update(_input.held(Button::up)) &&
        _game.level() % levels_per_world >= menu_layout::level_columns) {
        delta -= menu_layout::level_columns;
    }
    if (_repeat[1].update(_input.held(Button::down)) &&
        _game.level() % levels_per_world < menu_layout::level_columns) {
        delta += menu_layout::level_columns;
    }
    if (delta) {
        _game.select_level(delta);
        _commands.push(AudioCommand::navigate);
    }
    if (_input.pressed(Button::b)) {
        show_title();
    } else if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
        if (_game.level_unlocked(_game.level())) {
            start_game();
        } else {
            _commands.push(AudioCommand::error);
        }
    }
}

void InputController::handle_title_input() {
    int choice = static_cast<int>(_view.title_choice);
    bool up = _repeat[0].update(_input.held(Button::up));
    bool down = _repeat[1].update(_input.held(Button::down));
    if (up != down) {
        choice = (choice + 1) % static_cast<int>(TitleChoice::count);
        _view.title_choice = static_cast<TitleChoice>(choice);
        _commands.push(AudioCommand::navigate);
    }
    if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
        if (_view.title_choice == TitleChoice::almanac) {
            _game.open_almanac();
        } else {
            _game.open_save_select();
            _view.save_menu.slot = _slots.active() >= 0 ? _slots.active() : 0;
        }
        _repeat = {};
    }
}

void InputController::refresh_saves() {
    for (int i = 0; i < save_slot_count; ++i) {
        const auto& progress = _slots.progress(i);
        _view.save_menu.slots[i] = {_slots.occupied(i), progress.unlocked, progress.cleared_levels()};
    }
}

void InputController::handle_save_input() {
    if (_view.save_menu.confirming_delete) {
        if (_input.pressed(Button::b)) {
            _view.save_menu.confirming_delete = false;
            return;
        }
        if (_input.pressed(Button::left) || _input.pressed(Button::right)) {
            _view.save_menu.confirm_delete = !_view.save_menu.confirm_delete;
        }
        if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
            if (_view.save_menu.confirm_delete) {
                _slots.erase(_view.save_menu.slot);
                if (_slots.active() < 0) {
                    _game.load_progress(Progress());
                }
            }
            _view.save_menu.confirming_delete = false;
            _view.save_menu.delete_selected = false;
            _repeat = {};
        }
        return;
    }
    if (_input.pressed(Button::b)) {
        show_title();
        return;
    }
    int delta = 0;
    if (_repeat[0].update(_input.held(Button::up))) {
        --delta;
    }
    if (_repeat[1].update(_input.held(Button::down))) {
        ++delta;
    }
    if (delta) {
        _view.save_menu.slot = menu_layout::wrap_index(_view.save_menu.slot + delta, save_slot_count);
        _view.save_menu.delete_selected = false;
        _commands.push(AudioCommand::navigate);
    }
    if ((_input.pressed(Button::left) || _input.pressed(Button::right)) &&
        _slots.occupied(_view.save_menu.slot)) {
        _view.save_menu.delete_selected = !_view.save_menu.delete_selected;
        _commands.push(AudioCommand::navigate);
    }
    if (_input.pressed(Button::a) || _input.pressed(Button::start)) {
        if (_view.save_menu.delete_selected && _slots.occupied(_view.save_menu.slot)) {
            _view.save_menu.confirming_delete = true;
            _view.save_menu.confirm_delete = false;
        } else {
            _game.load_progress(_slots.activate(_view.save_menu.slot));
            show_level_select();
        }
        _repeat = {};
    }
}

void InputController::handle_won_input() {
    if (_game.reward_ready() && _input.any_pressed()) {
        show_level_select();
    }
}

void InputController::handle_paused_input() {
    if (_input.pressed(Button::start) || _input.pressed(Button::b)) {
        _game.toggle_pause();
        _repeat = {};
        return;
    }
    constexpr int choices = static_cast<int>(PauseChoice::count);
    int choice = static_cast<int>(_view.pause_choice);
    if (_repeat[0].update(_input.held(Button::up))) {
        choice = menu_layout::wrap_index(choice - 1, choices);
    }
    if (_repeat[1].update(_input.held(Button::down))) {
        choice = menu_layout::wrap_index(choice + 1, choices);
    }
    if (choice != static_cast<int>(_view.pause_choice)) {
        _commands.push(AudioCommand::navigate);
    }
    _view.pause_choice = static_cast<PauseChoice>(choice);
    int delta = 0;
    if (_repeat[2].update(_input.held(Button::left))) {
        --delta;
    }
    if (_repeat[3].update(_input.held(Button::right))) {
        ++delta;
    }
    if (choice >= static_cast<int>(PauseChoice::master) && delta) {
        auto channel = static_cast<AudioChannel>(choice - static_cast<int>(PauseChoice::master));
        if (_view.audio.adjust(channel, delta)) {
            _commands.push(AudioCommand::apply_settings);
            _commands.push(AudioCommand::navigate);
        }
    }
    if (_input.pressed(Button::a)) {
        switch (_view.pause_choice) {
            case PauseChoice::resume:
                _game.toggle_pause();
                _repeat = {};
                break;
            case PauseChoice::restart:
                start_game();
                break;
            case PauseChoice::choose_levels:
                show_level_select();
                break;
            default:
                break;
        }
    }
}

void InputController::handle_playing_input() {
    if (_input.pressed(Button::start)) {
        _game.toggle_pause();
        _view.pause_choice = PauseChoice::resume;
        _repeat = {};
        return;
    }
    if (_repeat[0].update(_input.held(Button::up)) && _view.cursor.row > _game.first_row()) {
        --_view.cursor.row;
    }
    if (_repeat[1].update(_input.held(Button::down)) && _view.cursor.row < _game.last_row()) {
        ++_view.cursor.row;
    }
    if (_repeat[2].update(_input.held(Button::left)) && _view.cursor.column > 0) {
        --_view.cursor.column;
    }
    if (_repeat[3].update(_input.held(Button::right)) && _view.cursor.column < columns - 1) {
        ++_view.cursor.column;
    }
    int seed = _game.bank_slot(_view.cursor.seed);
    int count = _game.available_plants();
    int seed_delta = 0;
    if (_seed_repeat[0].update(_input.held(Button::l), 20, 10)) {
        --seed_delta;
    }
    if (_seed_repeat[1].update(_input.held(Button::r), 20, 10)) {
        ++seed_delta;
    }
    _view.cursor.seed = _game.seed_at(menu_layout::wrap_index(seed + seed_delta, count));
    if (seed != _game.bank_slot(_view.cursor.seed)) {
        _commands.push(AudioCommand::navigate);
    }
    if (_input.pressed(Button::b) || (_input.held(Button::b) && _game.elapsed() % 15 == 0)) {
        int collected = 0;
        if (_tutorial.allows_collect()) {
            collected = _game.collect_sun(_tutorial.prompt().step == TutorialStep::sunflower_sun);
        }
        if (collected) {
            _collected = true;
            _commands.push(AudioCommand::collect);
            feedback(Feedback::collected);
        }
    }
    if (!_input.pressed(Button::a)) {
        return;
    }
    if (_input.held(Button::select)) {
        if (!_tutorial.allows_shovel(_view.cursor.row, _view.cursor.column)) {
            _commands.push(AudioCommand::error);
            return;
        }
        if (_game.shovel(_view.cursor.row, _view.cursor.column)) {
            _commands.push(AudioCommand::plant);
            feedback(Feedback::removed);
        }
        return;
    }
    if (!_tutorial.allows_plant(_view.cursor.row, _view.cursor.column, _view.cursor.seed)) {
        _commands.push(AudioCommand::error);
        return;
    }
    Feedback result;
    switch (_game.plant(_view.cursor.row, _view.cursor.column, _view.cursor.seed)) {
        case PlantResult::planted:
            result = Feedback::planted;
            break;
        case PlantResult::occupied:
            result = Feedback::occupied;
            break;
        case PlantResult::insufficient_sun:
            result = Feedback::insufficient_sun;
            break;
        case PlantResult::cooling_down:
            result = Feedback::cooling_down;
            break;
        default:
            return;
    }
    _commands.push(result == Feedback::planted ? AudioCommand::plant : AudioCommand::error);
    feedback(result);
}

} // namespace garden
