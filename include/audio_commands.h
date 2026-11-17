#ifndef GARDEN_AUDIO_COMMANDS_H
#define GARDEN_AUDIO_COMMANDS_H
#include <array>
#include <cassert>
#include <cstdint>

namespace garden
{
enum class AudioCommand : uint8_t { navigate, error, collect, plant, apply_settings, reset_combat };

// Ordered UI cues preserve input-side playback order without audio dependencies.
class AudioCommands
{
public:
    void clear() { _size = 0; }
    void push(AudioCommand command)
    {
        assert(_size < int(_commands.size()));
        _commands[_size++] = command;
    }
    int size() const { return _size; }
    AudioCommand operator[](int index) const { return _commands[index]; }
private:
    std::array<AudioCommand, 8> _commands{};
    int _size = 0;
};
}
#endif
