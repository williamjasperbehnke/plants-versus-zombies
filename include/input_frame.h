#ifndef GARDEN_INPUT_FRAME_H
#define GARDEN_INPUT_FRAME_H
#include <cstdint>

namespace garden
{
// GBA key bits are translated once by the hardware adapter, never by the model.
enum class Button : uint16_t
{
    a = 1, b = 2, select = 4, start = 8, right = 16, left = 32,
    up = 64, down = 128, r = 256, l = 512
};
constexpr uint16_t button_mask(Button button) { return static_cast<uint16_t>(button); }
struct InputFrame
{
    uint16_t held_keys = 0;
    uint16_t pressed_keys = 0;
    bool held(Button button) const { return (held_keys & button_mask(button)) != 0; }
    bool pressed(Button button) const { return (pressed_keys & button_mask(button)) != 0; }
    bool any_pressed() const { return pressed_keys != 0; }
};

class KeyRepeat
{
public:
    bool update(bool held, int delay = 16, int interval = 5)
    {
        if(! held) { _frames = 0; return false; }
        ++_frames;
        return _frames == 1 || (_frames > delay && (_frames - delay) % interval == 0);
    }
private:
    int _frames = 0;
};
}
#endif
