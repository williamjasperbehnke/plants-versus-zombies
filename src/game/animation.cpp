#include "game/animation.h"

#include "game/game.h"

namespace garden {
namespace {
constexpr AnimationClip<2> idle_clip{{0, 1}, 30};
constexpr AnimationClip<2> mine_clip{{1, 2}, 20};
constexpr AnimationClip<4> football_clip{{0, 1, 2, 3}, 6};
constexpr AnimationClip<4> flag_clip{{0, 4, 8, 12}, 8};
constexpr AnimationClip<6> chomper_bite_clip{{2, 2, 3, 3, 4, 5}, chomper_bite_duration / 6};
constexpr AnimationClip<3> repeater_shot_clip{{2, 3, 5}, shooter_recoil_frames / 3};
constexpr AnimationClip<3> repeater_recoil_clip{{2, 1, 0}, shooter_recoil_frames / 3};
constexpr AnimationClip<4> vault_clip{{8, 9, 10, 11}, vault_duration / 4};
// PlantType order; sheet-local resting poses.
constexpr std::array<ActorFrame, plant_count> plant_idle_frames{
    {{ActorSheet::sunflower, 0},
     {ActorSheet::peashooter, 0},
     {ActorSheet::walnut, 0},
     {ActorSheet::cherry, 0},
     {ActorSheet::potato_mine, 1},
     {ActorSheet::snow_pea, 0},
     {ActorSheet::squash, 0},
     {ActorSheet::repeater, 0},
     {ActorSheet::tallnut, 0},
     {ActorSheet::chomper, 0}}
};

int debris_index(ZombieEffectType type) {
    switch (type) {
        case ZombieEffectType::arm:
            return 0;
        case ZombieEffectType::newspaper_arm:
            return 1;
        case ZombieEffectType::cone:
            return 2;
        case ZombieEffectType::newspaper:
            return 3;
        case ZombieEffectType::bucket:
            return 4;
        case ZombieEffectType::door:
            return 5;
        case ZombieEffectType::flag:
            return 6;
        case ZombieEffectType::football_arm:
            return 7;
        case ZombieEffectType::football_helmet:
            return 8;
        case ZombieEffectType::pole:
            return 9;
        default:
            return 0; // Corpses use their own sheets, not the debris sheet.
    }
}

} // namespace

ActorFrame wilted_display_frame(int clock, int cell) {
    constexpr AnimationClip<6> clip{{0, 1, 2, 1, 0, 3}, 30};
    return {ActorSheet::wilted_plant, clip.frame_at(clock, cell % 3)};
}

int chomper_idle_pose(int clock) {
    constexpr AnimationClip<4> clip{{0, 1, 8, 1}, 16};
    return clip.frame_at(clock);
}

int chomper_chew_pose(int clock) {
    constexpr AnimationClip<4> clip{{5, 9, 6, 10}, 10};
    return clip.frame_at(clock);
}

int damage_stage(int health, int full_health) {
    if (health <= full_health / 3) {
        return 2;
    }
    if (health <= full_health * 2 / 3) {
        return 1;
    }
    return 0;
}

ActorFrame nut_display_frame(PlantType type, int health, bool alternate) {
    int damage = damage_stage(health, spec(type).health);
    if (type == PlantType::tallnut) {
        return {ActorSheet::tallnut, damage * 2 + int(alternate)};
    }
    return {ActorSheet::walnut, damage + int(alternate) * 3};
}

ActorFrame plant_idle_frame(PlantType type, bool alternate) {
    int index = static_cast<int>(type);
    if (index >= plant_count) {
        return {ActorSheet::sunflower, 0};
    }
    auto frame = plant_idle_frames[index];
    if (type == PlantType::peashooter) {
        frame.index += int(alternate) * 2;
    } else if (type == PlantType::walnut) {
        frame.index += int(alternate) * 3;
    } else {
        frame.index += int(alternate);
    }
    return frame;
}

ActorFrame plant_display_frame(PlantType type, int clock) {
    if (type == PlantType::chomper) {
        return {ActorSheet::chomper, chomper_idle_pose(clock)};
    }
    if (type == PlantType::potato_mine) {
        return {ActorSheet::potato_mine, mine_clip.frame_at(clock)};
    }
    return plant_idle_frame(type, idle_clip.frame_at(clock));
}

int screen_door_bite_pose(int remaining, int interval) {
    int age = interval - remaining;
    if (age < interval / 4) {
        return 3; // Recoil after the bite.
    }
    if (age < interval / 2) {
        return 0; // Rest with the upright door.
    }
    if (age < interval * 3 / 4) {
        return 1; // Shift the door aside.
    }
    return 2; // Open mouth with the door held aside.
}

ActorFrame zombie_display_frame(ZombieType type, int clock) {
    int pose = clock / 16 % 2;
    switch (type) {
        case ZombieType::normal:
            return {ActorSheet::normal_zombie, pose};
        case ZombieType::cone:
            return {ActorSheet::cone_zombie, pose};
        case ZombieType::newspaper:
            return {ActorSheet::newspaper_zombie, pose};
        case ZombieType::flag:
            return {ActorSheet::flag_zombie, flag_clip.frame_at(clock) + pose};
        case ZombieType::bucket:
            return {ActorSheet::bucket_zombie, pose};
        case ZombieType::screen_door:
            return {ActorSheet::screen_door_zombie, pose};
        case ZombieType::football:
            return {ActorSheet::football_zombie, football_clip.frame_at(clock)};
        case ZombieType::vaulter:
            return {ActorSheet::vaulter_zombie, pose};
        default:
            return {ActorSheet::normal_zombie, 0};
    }
}

RewardPose reward_pose(int frame, int lift) {
    if (frame < reward_zoom_start) {
        int tick = frame;
        int height = 0;
        int squash = 0;
        for (int i = 0; i < int(reward_bounce_durations.size()); ++i) {
            if (tick < reward_bounce_durations[i]) {
                int length = reward_bounce_durations[i];
                height = 4 * reward_bounce_heights[i] * tick * (length - tick) / (length * length);
                squash = tick < 4 ? 4 - tick : 0;
                break;
            }
            tick -= reward_bounce_durations[i];
        }
        int travel = frame < reward_bounce_end(3) ? reward_bounce_end(3) - frame : 0;
        return {
            120 + 136 * travel * travel / (reward_bounce_end(3) * reward_bounce_end(3)),
            112 - height,
            50,
            height,
            squash
        };
    }
    int zoom =
        frame < reward_reveal_frame ? frame - reward_zoom_start : reward_reveal_frame - reward_zoom_start;
    int remaining = reward_reveal_frame - reward_zoom_start - zoom;
    int duration = reward_reveal_frame - reward_zoom_start;
    int squared_duration = duration * duration;
    int eased = squared_duration - remaining * remaining;
    return {120, 112 - (14 + lift) * eased / squared_duration, 50 + 50 * eased / squared_duration, 0, 0};
}

ActorPose plant_animation(const Plant& plant, int clock, int cell) {
    constexpr AnimationClip<2> lawn_idle_clip{{0, 1}, 24};
    int idle_pose = lawn_idle_clip.frame_at(clock, cell);
    auto frame = plant_idle_frame(plant.type, idle_pose);
    if (plant.type == PlantType::peashooter &&
        plant.timer > spec(plant.type).action_delay - shooter_recoil_frames) {
        frame = {ActorSheet::peashooter, 1};
    }
    if (plant.type == PlantType::cherry && plant.health == 0) {
        frame = {ActorSheet::explosion, (18 - plant.timer) / 6};
    }
    if (plant.type == PlantType::potato_mine) {
        if (plant.health == 0) {
            frame = {ActorSheet::potato_mine, 3};
        } else if (plant.timer > 0) {
            frame = {ActorSheet::potato_mine, 0};
        } else {
            frame = {ActorSheet::potato_mine, mine_clip.frame_at(clock)};
        }
    }
    if (plant.type == PlantType::walnut || plant.type == PlantType::tallnut) {
        frame = nut_display_frame(plant.type, plant.health, idle_pose);
    }
    bool shooter = plant.type == PlantType::peashooter || plant.type == PlantType::snow_pea ||
                   plant.type == PlantType::repeater;
    bool firing = shooter && plant.timer > spec(plant.type).action_delay - shooter_recoil_frames;
    if (plant.type == PlantType::snow_pea && firing) {
        frame = {ActorSheet::snow_pea, 2};
    }
    int repeater_recoil = 0;
    if (plant.type == PlantType::repeater) {
        int remaining = plant.shot_frames;
        if (remaining > 0) {
            frame = {ActorSheet::repeater, repeater_shot_clip.frame_once(shooter_recoil_frames - remaining)};
            repeater_recoil = repeater_recoil_clip.frame_once(shooter_recoil_frames - remaining);
        } else if (plant.burst_frames > 0 && plant.burst_frames <= 2) {
            frame = {ActorSheet::repeater, 4};
        }
    }
    if (plant.type == PlantType::chomper) {
        int pose = chomper_idle_pose(clock + cell * 5);
        if (plant.bite_frames > 0) {
            pose = chomper_bite_clip.frame_once(chomper_bite_duration - plant.bite_frames);
        } else if (plant.timer > 0) {
            pose = plant.timer < 20 ? 7 : chomper_chew_pose(clock + cell * 5);
        }
        frame = {ActorSheet::chomper, pose};
    }
    int plant_x = cell_x(cell % columns);
    int plant_y = cell_y(cell / columns) - 1;
    if (plant.type == PlantType::chomper && plant.bite_frames > 0) {
        if (plant.bite_frames > chomper_pull_start) {
            --plant_x;
        } else if (plant.bite_frames > chomper_swallow_frame) {
            plant_x += (chomper_pull_start - plant.bite_frames) / 2;
        } else {
            plant_x += plant.bite_frames / 2;
        }
    }
    if (plant.type == PlantType::squash) {
        if (plant.committed()) {
            int age = 30 - plant.timer;
            frame = {ActorSheet::squash, age < 6 ? 2 : 3};
            if (plant.health == 0) {
                frame = {ActorSheet::squash, 4};
                plant_x = plant.target_x / scale;
            } else if (age >= 6) {
                int time = age - 6;
                plant_x += (plant.target_x / scale - plant_x) * time / 24;
                plant_y -= 120 * time * (24 - time) / (24 * 24);
            }
        }
    }
    int recoil = firing ? 1 : 0;
    if (plant.type == PlantType::repeater) {
        recoil = repeater_recoil;
    }
    if (plant.wilted) {
        frame = wilted_display_frame(clock, cell);
    }
    int depth = 2 - (cell / columns) * 4;
    if (plant.type == PlantType::chomper && plant.bite_frames > 0) {
        depth = -24;
    }
    if (plant.exploding() || plant.committed()) {
        depth = -28; // Blasts cross lanes and must draw above every plant.
    }
    return {frame, plant_x - recoil, plant_y, depth};
}

ActorPose zombie_animation(const Game& game, int i) {
    const auto& zombie = game.zombies()[i];
    bool breached = game.phase() == Phase::lost && i == game.breached_zombie();
    int clock = game.elapsed() + (breached ? game.loss_frames() : 0);
    int slow = zombie.frost_frames > 0 ? 2 : 1;
    const AnimationClip<2> bob_clip{{0, 1}, 14 * slow};
    int bob = bob_clip.frame_at(clock, i);
    int cadence = zombie.type == ZombieType::newspaper && zombie.shield_health == 0 ? 6 : 12;
    const AnimationClip<2> walk_clip{{0, 1}, cadence * slow};
    int pose = walk_clip.frame_at(clock, i);
    bool chewing = zombie.eating && !breached;
    if (chewing) {
        bob = 0;
        pose = zombie.bite_timer < 12 * slow ? 1 : 0;
    }
    int bite_offset = chewing ? 2 : 0;
    ActorFrame frame{ActorSheet::normal_zombie, bite_offset + pose};
    if (zombie.type == ZombieType::cone && zombie.health > 6) {
        frame.sheet = ActorSheet::cone_zombie;
    }
    switch (zombie.type) {
        case ZombieType::newspaper:
            frame = {ActorSheet::newspaper_zombie, bite_offset + pose};
            if (zombie.shield_health == 0) {
                frame.index += 4;
            }
            break;
        case ZombieType::flag:
            frame = {ActorSheet::flag_zombie, flag_clip.frame_at(clock, i) + bite_offset + pose};
            break;
        case ZombieType::bucket:
            if (zombie.shield_health > 0) {
                frame = {ActorSheet::bucket_zombie, bite_offset + pose};
                if (zombie.shield_health * 2 <= spec(zombie.type).shield_health) {
                    frame.index += 4;
                }
            }
            break;
        case ZombieType::screen_door:
            if (zombie.shield_health > 0) {
                frame = {ActorSheet::screen_door_zombie, pose};
                if (chewing) {
                    frame.index = 2 + screen_door_bite_pose(zombie.bite_timer, game.bite_delay(zombie));
                }
                if (zombie.shield_health * 2 <= spec(zombie.type).shield_health) {
                    frame.index += screen_door_armor_frames;
                }
            }
            break;
        case ZombieType::football:
            frame = {ActorSheet::football_zombie, football_clip.frame_at(clock / slow)};
            if (chewing) {
                frame.index = 4 + pose;
            }
            if (zombie.shield_health <= 0) {
                frame.index += 12;
            } else if (zombie.shield_health <= 20) {
                frame.index += 6;
            }
            break;
        case ZombieType::vaulter:
            frame = {ActorSheet::vaulter_zombie, bite_offset + pose};
            if (zombie.vault_frames > 0) {
                frame.index = vault_clip.frame_once(vault_duration - zombie.vault_frames);
            } else if (zombie.pole_used) {
                frame.index += 4;
            }
            break;
        default:
            break;
    }
    if (game.flag_buffed(zombie)) {
        switch (frame.sheet) {
            case ActorSheet::screen_door_zombie:
                frame.index += screen_door_buff_frames;
                break;
            case ActorSheet::vaulter_zombie:
                frame.index += 12;
                break;
            case ActorSheet::football_zombie:
                frame.index += 18;
                break;
            case ActorSheet::bucket_zombie:
                frame.index += 8;
                break;
            case ActorSheet::newspaper_zombie:
                frame.index += 8;
                break;
            case ActorSheet::flag_zombie:
                frame.index += 32;
                break;
            default:
                frame.index += 4;
                break;
        }
    }
    if (zombie.emergence_frames > 0) {
        int stage = (emergence_duration - zombie.emergence_frames) * emergence_stages / emergence_duration;
        frame = {ActorSheet::emerging_zombie, stage};
        if (game.flag_buffed(zombie)) {
            frame.index += emergence_stages;
        }
        bob = 0;
    }
    int x = zombie.x / scale;
    if (breached) {
        int advance = game.loss_frames() < 90 ? game.loss_frames() : 90;
        x -= advance * 24 / 90;
    }
    int y = cell_y(zombie.row) + bob - 2;
    if (zombie.vault_frames > 0) {
        int height = 88 * zombie.vault_frames * (vault_duration - zombie.vault_frames) /
                     (vault_duration * vault_duration);
        y -= height;
    }
    int pull = 0;
    for (int column = 0; column < columns; ++column) {
        const auto& plant = game.plants()[cell_index(zombie.row, column)];
        if (plant.active && plant.type == PlantType::chomper && plant.bite_target == i &&
            plant.bite_frames <= chomper_pull_start && plant.bite_frames > chomper_swallow_frame) {
            pull = chomper_pull_start - plant.bite_frames;
            x += (cell_x(column) + 14 - x) * pull / chomper_pull_duration;
            y += pull / 3;
            break;
        }
    }
    int depth = -zombie.row * 4;
    if (zombie.vault_frames > 0) {
        depth = -24;
    }
    if (zombie.type == ZombieType::flag) {
        --depth;
    }
    if (pull > 0) {
        depth = -23;
    }
    return {frame, x, y, depth, 100 - pull * 5};
}

ActorPose zombie_effect_animation(const ZombieEffect& effect) {
    int duration = effect_duration(effect.type);
    int age = duration - effect.remaining;
    bool corpse = corpse_effect(effect.type);
    int stages = effect_stages(effect.type);
    int first_frame = 0;
    if (corpse) {
        if (effect.wounded) {
            first_frame = stages;
        }
    } else {
        first_frame = debris_index(effect.type) * stages;
    }
    int frame = first_frame + age * stages / duration;
    int x = effect.x / scale;
    int y = cell_y(effect.row) - 2;
    if (!corpse) {
        bool arm = effect.type == ZombieEffectType::arm || effect.type == ZombieEffectType::newspaper_arm ||
                   effect.type == ZombieEffectType::football_arm;
        bool hat = effect.type == ZombieEffectType::cone || effect.type == ZombieEffectType::bucket ||
                   effect.type == ZombieEffectType::football_helmet;
        bool flag = effect.type == ZombieEffectType::flag;
        int origin = 3;
        int ground = 12;
        int drift = -7 + age / 3;
        if (arm) {
            origin = 5;
            ground = 10;
            drift = -9 - age / 9;
        } else if (flag) {
            origin = -3;
            ground = 4;
            drift = 3 + age / 3;
        } else if (hat) {
            origin = -13;
            drift = age / 3;
        }
        x += drift;
        y += origin + (ground - origin) * age * age / (duration * duration);
        if (!arm) {
            y -= 32 * age * (duration - age) / (duration * duration);
        }
    }
    ActorSheet sheet = ActorSheet::zombie_debris;
    switch (effect.type) {
        case ZombieEffectType::death:
            sheet = ActorSheet::zombie_death;
            break;
        case ZombieEffectType::newspaper_death:
            sheet = ActorSheet::newspaper_death;
            break;
        case ZombieEffectType::flag_death:
            sheet = ActorSheet::flag_death;
            break;
        case ZombieEffectType::football_death:
            sheet = ActorSheet::football_death;
            break;
        case ZombieEffectType::vault_death:
            sheet = ActorSheet::vault_death;
            break;
        default:
            break;
    }
    int depth = (corpse ? 1 : -3) - effect.row * 4;
    return {{sheet, frame}, x, y, depth};
}
} // namespace garden
