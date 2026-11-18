#ifndef GARDEN_AUDIO_PLAYER_H
#define GARDEN_AUDIO_PLAYER_H
#include "audio/audio_types.h"
#include "game/game.h"

namespace garden {
class AudioPlayer {
public:
    void start(const AudioSettings& settings);
    void ensure_music(Phase phase, const AudioSettings& settings);
    void process(const AudioCommands& commands, const AudioSettings& settings);
    void update(const Game& game, Phase previous_phase);

private:
    void apply(const AudioSettings& settings);
    int _bite_sound_cooldown = 0;
    int _combat_sound_cooldown = 0;
};
} // namespace garden
#endif
