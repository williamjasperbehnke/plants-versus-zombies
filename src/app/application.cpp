#include "app/application.h"

#include "bn_core.h"
#include "bn_keypad.h"

namespace garden {
namespace {
InputFrame read_input() {
    InputFrame input;
    for (int bit = 0; bit < 10; ++bit) {
        auto mask = uint16_t(1 << bit);
        auto key = static_cast<bn::keypad::key_type>(mask);
        if (bn::keypad::held(key)) {
            input.held_keys |= mask;
        }
        if (bn::keypad::pressed(key)) {
            input.pressed_keys |= mask;
        }
    }
    return input;
}
} // namespace

void Application::run() {
    _save.load(_slots);
    _save.update(_slots);
    _audio.start(_controller.view().audio);
    while (true) {
        _controller.update(read_input());
        _audio.process(_controller.audio_commands(), _controller.view().audio);
        _audio.ensure_music(_game.phase(), _controller.view().audio);
        Phase previous_phase = _game.phase();
        _game.tick(_controller.holding_waves(), _controller.holding_sky_sun());
        _audio.update(_game, previous_phase);
        _slots.update_active(_game.progress());
        _save.update(_slots);
        _controller.finish_frame();
        _presentation.update(_game, _controller.view());
        bn::core::update();
    }
}
} // namespace garden
