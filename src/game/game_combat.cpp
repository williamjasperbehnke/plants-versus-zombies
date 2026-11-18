#include "game/game.h"
#include "game/game_internal.h"

namespace garden {
namespace {
bool shot_reaches(int target_x, int previous_x, int next_x) {
    constexpr int hit_radius = 7 * scale;
    return target_x + hit_radius >= previous_x && target_x - hit_radius <= next_x;
}
} // namespace

void Game::update_tombstones() {
    for (auto& stone : _tombstones) {
        if (stone.flash > 0) {
            --stone.flash;
        }
        if (stone.crumble_frames > 0) {
            --stone.crumble_frames;
        }
    }
}

bool Game::shoot(int row, int x, bool frozen) {
    auto* shot = detail::free_slot(_shots);
    if (!shot) {
        return false;
    }
    *shot = {true, row, x, frozen};
    _events.shot = true;
    return true;
}

void Game::spawn_zombie_effect(const Zombie& zombie, ZombieEffectType type) {
    auto* effect = detail::free_slot(_zombie_effects);
    if (!effect) {
        // Keep combat independent of the cosmetic pool; replace the effect nearest expiry.
        effect = &_zombie_effects[0];
        for (auto& candidate : _zombie_effects) {
            if (candidate.remaining < effect->remaining) {
                effect = &candidate;
            }
        }
    }
    *effect = {true, type, zombie.row, zombie.x, effect_duration(type), zombie.arm_lost};
}

void Game::update_zombie_effects() {
    for (auto& effect : _zombie_effects) {
        if (effect.active && --effect.remaining <= 0) {
            effect.active = false;
        }
    }
}

void Game::damage(Zombie& zombie, int amount, DamageType type) {
    const bool swallowed = type == DamageType::swallow;
    const int old_health = zombie.health;
    const int old_shield = zombie.shield_health;
    bool killed = take_zombie_damage(zombie, amount, type);
    _events.hit = true;
    zombie.flash = 6;
    if (!swallowed && zombie.type == ZombieType::cone && old_health > 6 && (zombie.health <= 6 || killed)) {
        spawn_zombie_effect(zombie, ZombieEffectType::cone);
    }
    if (!swallowed && old_shield > 0 && (zombie.shield_health == 0 || killed)) {
        switch (zombie.type) {
            case ZombieType::newspaper:
                spawn_zombie_effect(zombie, ZombieEffectType::newspaper);
                break;
            case ZombieType::bucket:
                spawn_zombie_effect(zombie, ZombieEffectType::bucket);
                break;
            case ZombieType::screen_door:
                spawn_zombie_effect(zombie, ZombieEffectType::door);
                break;
            case ZombieType::football:
                spawn_zombie_effect(zombie, ZombieEffectType::football_helmet);
                break;
            default:
                break;
        }
    }
    if (killed) {
        if (!swallowed && zombie.type == ZombieType::vaulter && !zombie.pole_used) {
            spawn_zombie_effect(zombie, ZombieEffectType::pole);
        }
        if (zombie.type == ZombieType::flag) {
            // A mower or edge blast can defeat an entering leader before its first visible step.
            enter_horde(zombie);
            if (!swallowed) {
                spawn_zombie_effect(zombie, ZombieEffectType::flag);
            }
        }
        auto death = ZombieEffectType::death;
        switch (zombie.type) {
            case ZombieType::flag:
                death = ZombieEffectType::flag_death;
                break;
            case ZombieType::newspaper:
                death = ZombieEffectType::newspaper_death;
                break;
            case ZombieType::football:
                death = ZombieEffectType::football_death;
                break;
            case ZombieType::vaulter:
                death = ZombieEffectType::vault_death;
                break;
            default:
                break;
        }
        if (!swallowed) {
            spawn_zombie_effect(zombie, death);
        }
        zombie.active = false;
        ++_kills;
        auto& total = _progress.zombie_kills[static_cast<int>(zombie.type)];
        if (total < UINT32_MAX) {
            ++total;
        }
    } else if (!zombie.arm_lost && zombie.health <= arm_loss_health(zombie.type)) {
        zombie.arm_lost = true;
        auto arm = ZombieEffectType::arm;
        if (zombie.type == ZombieType::newspaper) {
            arm = ZombieEffectType::newspaper_arm;
        } else if (zombie.type == ZombieType::football || zombie.type == ZombieType::vaulter) {
            arm = ZombieEffectType::football_arm;
        }
        spawn_zombie_effect(zombie, arm);
    }
}

void Game::damage(Tombstone& tombstone, int amount) {
    tombstone.health -= amount;
    if (tombstone.health <= 0) {
        tombstone.health = 0;
        tombstone.crumble_frames = 30;
    }
    tombstone.flash = 6;
    _events.hit = true;
}

void Game::update_shots() {
    for (auto& shot : _shots) {
        if (!shot.active) {
            continue;
        }
        const int previous_x = shot.x;
        shot.x += 640;
        Zombie* closest = nullptr;
        for (auto& zombie : _zombies) {
            if (zombie.active && zombie.vault_frames == 0 && zombie.row == shot.row &&
                shot_reaches(zombie.x, previous_x, shot.x) && (!closest || zombie.x < closest->x)) {
                closest = &zombie;
            }
        }
        Tombstone* obstacle = nullptr;
        for (auto& tombstone : _tombstones) {
            int x = cell_x(tombstone.column) * scale;
            if (tombstone.active() && tombstone.row == shot.row && shot_reaches(x, previous_x, shot.x) &&
                (!obstacle || x < cell_x(obstacle->column) * scale)) {
                obstacle = &tombstone;
            }
        }
        if (obstacle && (!closest || cell_x(obstacle->column) * scale <= closest->x)) {
            damage(*obstacle, 1);
            shot.active = false;
        } else if (closest) {
            damage(*closest, 1, shot.frozen ? DamageType::frost : DamageType::projectile);
            shot.active = false;
        } else if (shot.x > 245 * scale) {
            shot.active = false;
        }
    }
}

void Game::update_mowers() {
    for (int row = 0; row < rows; ++row) {
        auto& mower = _mowers[row];
        if (!mower.moving) {
            continue;
        }
        mower.advance();
        for (auto& tombstone : _tombstones) {
            if (tombstone.active() && tombstone.row == row &&
                cell_x(tombstone.column) * scale <= mower.x + 12 * scale) {
                damage(tombstone, 100);
            }
        }
        for (auto& zombie : _zombies) {
            if (zombie.active && zombie.row == row && zombie.x <= mower.x + 12 * scale) {
                damage(zombie, 100, DamageType::crush);
            }
        }
    }
}

bool Game::flag_buffed(const Zombie& zombie) const {
    if (!zombie.active || zombie.type == ZombieType::flag) {
        return false;
    }
    constexpr int radius = 80 * scale;
    for (const auto& leader : _zombies) {
        if (!leader.active || leader.type != ZombieType::flag) {
            continue;
        }
        int dx = detail::absolute(leader.x - zombie.x);
        int dy = detail::absolute(leader.row - zombie.row) * cell_height * scale;
        if (dx <= radius && dy <= radius && dx * dx + dy * dy <= radius * radius) {
            return true;
        }
    }
    return false;
}

int Game::zombie_speed(const Zombie& zombie) const {
    int speed = movement_speed(zombie);
    return flag_buffed(zombie) ? speed * 9 / 8 : speed;
}

void Game::update_zombies() {
    for (int i = 0; i < zombie_capacity; ++i) {
        auto& zombie = _zombies[i];
        if (!zombie.active) {
            continue;
        }
        if (zombie.flash > 0) {
            --zombie.flash;
        }
        if (zombie.emergence_frames > 0) {
            --zombie.emergence_frames;
            zombie.eating = false;
            continue;
        }
        if (zombie.vault_frames > 0) {
            if (zombie.frost_frames == 0 || _frame % 2 == 0) {
                --zombie.vault_frames;
                zombie.x = zombie.vault_origin + (zombie.vault_target - zombie.vault_origin) *
                                                     (vault_duration - zombie.vault_frames) / vault_duration;
            }
            zombie.eating = false;
            continue;
        }
        Plant* target = nullptr;
        for (int column = columns - 1; column >= 0; --column) {
            auto& plant = _plants[cell_index(zombie.row, column)];
            int distance = zombie.x - cell_x(column) * scale;
            if (zombie.type == ZombieType::vaulter && !zombie.pole_used && plant.active && plant.health > 0 &&
                !plant.committed() && plant.type != PlantType::tallnut && distance > 0 &&
                distance <= 26 * scale) {
                zombie.pole_used = true;
                zombie.vault_frames = vault_duration;
                zombie.vault_origin = zombie.x;
                zombie.vault_target = (cell_x(column) - 22) * scale;
                spawn_zombie_effect(zombie, ZombieEffectType::pole);
                break;
            }
            if (plant.active && !plant.committed() && plant.type != PlantType::cherry && plant.health > 0 &&
                !(plant.type == PlantType::potato_mine && plant.timer == 0) && distance >= -5 * scale &&
                distance <= 15 * scale) {
                target = &plant;
                break;
            }
        }
        if (zombie.vault_frames > 0) {
            zombie.eating = false;
            continue;
        }
        zombie.eating = target != nullptr;
        if (target) {
            if (--zombie.bite_timer <= 0) {
                --target->health;
                _events.bite = true;
                target->flash = 8;
                zombie.bite_timer = bite_delay(zombie);
                if (target->health <= 0) {
                    target->active = false;
                }
            }
        } else {
            zombie.x -= zombie_speed(zombie);
            zombie.bite_timer = bite_delay(zombie);
        }
        if (zombie.type == ZombieType::flag && zombie.x < flag_entry_x * scale) {
            enter_horde(zombie);
        }
        auto& mower = _mowers[zombie.row];
        if (zombie.x < 29 * scale && mower.available) {
            mower.available = false;
            mower.moving = true;
            _events.mower = true;
            damage(zombie, 100, DamageType::crush);
        } else if (zombie.x < 8 * scale) {
            _breached_zombie = i;
            _phase = Phase::lost;
            return;
        }
    }
}
} // namespace garden
