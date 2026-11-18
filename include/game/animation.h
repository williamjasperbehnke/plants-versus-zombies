#ifndef GARDEN_ANIMATION_H
#define GARDEN_ANIMATION_H
#include <array>
#include <cassert>
#include <cstdint>

namespace garden {
struct Plant;
struct ZombieEffect;
class Game;
enum class PlantType : uint8_t;
enum class ZombieType : uint8_t;

// Fixed-size clips share timing and frame selection without allocation or virtual dispatch.
template <std::size_t Size>
struct AnimationClip {
    static_assert(Size > 0, "Animations need at least one frame");
    std::array<int, Size> frames;
    int ticks_per_frame;

    constexpr int frame_at(int clock, int phase = 0) const {
        assert(clock >= 0 && phase >= 0 && ticks_per_frame > 0);
        return frames[(clock / ticks_per_frame + phase) % Size];
    }

    // One-shot actions hold their last pose instead of restarting the animation.
    constexpr int frame_once(int clock) const {
        assert(clock >= 0 && ticks_per_frame > 0);
        int index = clock / ticks_per_frame;
        return frames[index < int(Size) ? index : int(Size) - 1];
    }
};

// A frame is local to a named sheet; no global numeric ranges or decoding.
enum class ActorSheet : uint8_t {
    sunflower,
    peashooter,
    walnut,
    cherry,
    potato_mine,
    snow_pea,
    squash,
    repeater,
    tallnut,
    chomper,
    normal_zombie,
    cone_zombie,
    newspaper_zombie,
    flag_zombie,
    bucket_zombie,
    screen_door_zombie,
    football_zombie,
    vaulter_zombie,
    wilted_plant,
    explosion,
    emerging_zombie,
    field_items,
    zombie_death,
    newspaper_death,
    flag_death,
    football_death,
    vault_death,
    zombie_debris
};

struct ActorFrame {
    ActorSheet sheet;
    int index;

    constexpr bool operator==(const ActorFrame& other) const {
        return sheet == other.sheet && index == other.index;
    }
};

struct ActorPose {
    ActorFrame frame;
    int x;
    int y;
    int depth = 0;
    int scale_percent = 100;
};

ActorPose plant_animation(const Plant& plant, int clock, int cell);
ActorPose zombie_animation(const Game& game, int index);
ActorPose zombie_effect_animation(const ZombieEffect& effect);

inline constexpr int wilted_plant_poses = 4;
ActorFrame wilted_display_frame(int clock, int cell);
int chomper_idle_pose(int clock);
int chomper_chew_pose(int clock);
int damage_stage(int health, int full_health);
ActorFrame nut_display_frame(PlantType type, int health, bool alternate);
ActorFrame plant_idle_frame(PlantType type, bool alternate);
ActorFrame plant_display_frame(PlantType type, int clock);
inline constexpr int screen_door_armor_frames = 6; // Two walking, four biting poses.
inline constexpr int screen_door_buff_frames = screen_door_armor_frames * 2;
int screen_door_bite_pose(int remaining, int interval);
ActorFrame zombie_display_frame(ZombieType type, int clock);

// Shared by simulation, poses and audio so action boundaries cannot drift apart.
inline constexpr int shooter_recoil_frames = 6;
inline constexpr int chomper_bite_duration = 36;
inline constexpr int chomper_pull_start = chomper_bite_duration * 2 / 3;
inline constexpr int chomper_swallow_frame = chomper_bite_duration / 3;
inline constexpr int chomper_pull_duration = chomper_pull_start - chomper_swallow_frame;
inline constexpr int vault_duration = 36;
inline constexpr std::array<int, 4> reward_bounce_durations{{48, 36, 28, 20}};
inline constexpr std::array<int, 4> reward_bounce_heights{{35, 22, 12, 5}};
inline constexpr int reward_zoom_start = 150;
inline constexpr int reward_reveal_frame = 180;
inline constexpr int reward_ready_frame = reward_reveal_frame + 30;

constexpr int reward_bounce_end(int bounce) {
    int frame = 0;
    for (int i = 0; i <= bounce; ++i) {
        frame += reward_bounce_durations[i];
    }
    return frame;
}

constexpr bool reward_landing(int frame) {
    for (int i = 0; i < int(reward_bounce_durations.size()); ++i) {
        if (frame == reward_bounce_end(i)) {
            return true;
        }
    }
    return false;
}

struct RewardPose {
    int x;
    int y;
    int scale;
    int height;
    int squash;
};

RewardPose reward_pose(int frame, int lift = 0);
} // namespace garden
#endif
