#ifndef GARDEN_AUDIO_SETTINGS_H
#define GARDEN_AUDIO_SETTINGS_H

#include <array>

namespace garden
{
enum class AudioChannel { master, music, effects };

struct AudioSettings
{
    std::array<int, 3> levels{10, 5, 10};

    bool adjust(AudioChannel channel, int delta)
    {
        int& level = levels[static_cast<int>(channel)];
        int next = level + delta;
        if(next < 0) { next = 0; }
        if(next > 10) { next = 10; }
        bool changed = next != level;
        level = next;
        return changed;
    }

    int gain(AudioChannel channel) const
    { return levels[0] * levels[static_cast<int>(channel)]; }

    int key() const { return levels[0] * 121 + levels[1] * 11 + levels[2]; }
};
}
#endif
