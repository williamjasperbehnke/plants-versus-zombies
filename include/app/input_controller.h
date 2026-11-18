#ifndef GARDEN_INPUT_CONTROLLER_H
#define GARDEN_INPUT_CONTROLLER_H
#include "audio/audio_types.h"
#include "save/save_slots.h"
#include "ui/view_state.h"

#include <cstdint>

namespace garden {
// GBA key bits are translated once by the hardware adapter, never by the model.
enum class Button : uint16_t {
    a = 1,
    b = 2,
    select = 4,
    start = 8,
    right = 16,
    left = 32,
    up = 64,
    down = 128,
    r = 256,
    l = 512
};

constexpr uint16_t button_mask(Button button) {
    return static_cast<uint16_t>(button);
}

struct InputFrame {
    uint16_t held_keys = 0;
    uint16_t pressed_keys = 0;

    bool held(Button button) const {
        return (held_keys & button_mask(button)) != 0;
    }

    bool pressed(Button button) const {
        return (pressed_keys & button_mask(button)) != 0;
    }

    bool any_pressed() const {
        return pressed_keys != 0;
    }
};

class KeyRepeat {
public:
    bool update(bool held, int delay = 16, int interval = 5) {
        if (!held) {
            _frames = 0;
            return false;
        }
        ++_frames;
        return _frames == 1 || (_frames > delay && (_frames - delay) % interval == 0);
    }

private:
    int _frames = 0;
};

// Owns screen interaction and transient view state, with no hardware or renderer.
class InputController {
public:
    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    InputController(Game& game, SaveSlots& slots) : _game(game), _slots(slots) {
    }

    void update(const InputFrame& input);
    void finish_frame();

    bool holding_sky_sun() const {
        return _tutorial.holds_sky_sun();
    }

    bool holding_waves() const {
        return _tutorial.active();
    }

    const ViewState& view() const {
        return _view;
    }

    const AudioCommands& audio_commands() const {
        return _commands;
    }

private:
    void reset_view();
    void handle_save_input();
    void refresh_saves();
    void start_game();
    void show_title();
    void show_level_select();
    void handle_input();
    void handle_almanac_input();
    void handle_preview_input();
    void handle_seed_select_input();
    void handle_lost_input();
    void handle_level_select_input();
    void handle_title_input();
    void handle_won_input();
    void handle_paused_input();
    void handle_playing_input();
    void feedback(Feedback kind);
    TutorialGuide _tutorial;
    bool _collected = false;
    Game& _game;
    SaveSlots& _slots;
    ViewState _view;
    InputFrame _input;
    AudioCommands _commands;
    std::array<KeyRepeat, 4> _repeat{};
    std::array<KeyRepeat, 2> _seed_repeat{};
};
} // namespace garden
#endif
