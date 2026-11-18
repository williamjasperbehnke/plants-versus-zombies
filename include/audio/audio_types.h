#ifndef GARDEN_AUDIO_TYPES_H
#define GARDEN_AUDIO_TYPES_H
#include <array>
#include <cassert>
#include <cstdint>

namespace garden {
enum class AudioChannel {
    master,
    music,
    effects
};

struct AudioSettings {
    std::array<int, 3> levels{10, 10, 10};

    bool adjust(AudioChannel channel, int delta) {
        int& level = levels[static_cast<int>(channel)];
        int next = level + delta;
        if (next < 0) {
            next = 0;
        }
        if (next > 10) {
            next = 10;
        }
        bool changed = next != level;
        level = next;
        return changed;
    }

    int gain(AudioChannel channel) const {
        return levels[0] * levels[static_cast<int>(channel)];
    }

    int key() const {
        return levels[0] * 121 + levels[1] * 11 + levels[2];
    }
};

enum class AudioCommand : uint8_t {
    navigate,
    error,
    collect,
    plant,
    apply_settings,
    reset_combat
};

// Ordered UI cues preserve input-side playback order without audio dependencies.
class AudioCommands {
public:
    void clear() {
        _size = 0;
    }

    void push(AudioCommand command) {
        assert(_size < int(_commands.size()));
        _commands[_size++] = command;
    }

    int size() const {
        return _size;
    }

    AudioCommand operator[](int index) const {
        return _commands[index];
    }

private:
    std::array<AudioCommand, 8> _commands{};
    int _size = 0;
};
} // namespace garden
#endif
