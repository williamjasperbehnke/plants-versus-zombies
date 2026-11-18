#include "audio/audio_player.h"

#include "bn_music.h"
#include "bn_music_items.h"
#include "bn_sound.h"
#include "bn_sound_items.h"

namespace garden {
void AudioPlayer::apply(const AudioSettings& settings) {
    bn::sound::set_master_volume(bn::fixed(settings.gain(AudioChannel::effects)) / 100);
    bn::music::set_volume(bn::fixed(settings.gain(AudioChannel::music)) / 100);
}

void AudioPlayer::start(const AudioSettings& settings) {
    bn::music_items::garden_theme.play();
    apply(settings);
}

void AudioPlayer::ensure_music(Phase phase, const AudioSettings& settings) {
    if (phase != Phase::lost && !bn::music::playing()) {
        start(settings);
    }
}

void AudioPlayer::process(const AudioCommands& commands, const AudioSettings& settings) {
    for (int i = 0; i < commands.size(); ++i) {
        switch (commands[i]) {
            case AudioCommand::navigate:
                bn::sound_items::navigate.play();
                break;
            case AudioCommand::error:
                bn::sound_items::error.play();
                break;
            case AudioCommand::collect:
                bn::sound_items::collect.play();
                break;
            case AudioCommand::plant:
                bn::sound_items::plant.play();
                break;
            case AudioCommand::apply_settings:
                apply(settings);
                break;
            case AudioCommand::reset_combat:
                _combat_sound_cooldown = 0;
                _bite_sound_cooldown = 0;
                break;
            default:
                break;
        }
    }
}

void AudioPlayer::update(const Game& game, Phase previous_phase) {
    if (previous_phase != game.phase()) {
        if (game.phase() == Phase::lost) {
            bn::music::stop();
            bn::sound::stop_all();
            bn::sound_items::record_scratch.play();
            return;
        }
    }
    if (game.phase() == Phase::won) {
        int frame = game.reward_frames();
        if (frame == 1) {
            bn::sound_items::win.play();
        }
        if (reward_landing(frame)) {
            bn::sound_items::plant.play(bn::fixed(0.35));
        }
        if (frame == garden::reward_reveal_frame) {
            bn::sound_items::collect.play();
        }
    }
    if (game.phase() == Phase::lost) {
        if (game.loss_frames() == garden::defeat_audio::scream_frame) {
            bn::sound_items::scream.play();
        }
        return;
    }
    if (game.phase() != Phase::playing) {
        return;
    }
    if (_combat_sound_cooldown > 0) {
        --_combat_sound_cooldown;
    }
    const auto& events = game.events();
    if (_bite_sound_cooldown > 0) {
        --_bite_sound_cooldown;
    }
    if (events.bite && _bite_sound_cooldown == 0) {
        bn::sound_items::chew.play(bn::fixed(0.72));
        _bite_sound_cooldown = 12;
    }
    if (events.chomp) {
        bn::sound_items::chew.play(bn::fixed(0.9));
    }
    if (events.explosion) {
        bn::sound_items::boom.play();
    } else if (events.mower) {
        bn::sound_items::mower.play();
    } else if (events.final_wave_approaching || events.horde_started) {
        bn::sound_items::wave_start.play();
    } else if (!_combat_sound_cooldown && (events.hit || events.shot)) {
        if (events.hit) {
            bn::sound_items::hit.play();
        } else {
            bn::sound_items::shot.play();
        }
        _combat_sound_cooldown = 8;
    }
}

} // namespace garden
