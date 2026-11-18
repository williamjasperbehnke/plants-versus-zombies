#include "game/game.h"
#include "game/game_internal.h"

namespace garden {
namespace {
bool valid_cell(int row, int column) {
    return row >= 0 && row < rows && column >= 0 && column < columns;
}
} // namespace

PlantResult Game::plant(int row, int column, PlantType type) {
    if (_phase != Phase::playing || !valid_cell(row, column) || !row_active(row) || !plant_available(type)) {
        return PlantResult::inactive;
    }
    auto& target = _plants[cell_index(row, column)];
    if (target.active || tombstone_at(row, column)) {
        return PlantResult::occupied;
    }
    if (cooldown(type)) {
        return PlantResult::cooling_down;
    }
    const auto& definition = spec(type);
    if (_sun < definition.cost) {
        return PlantResult::insufficient_sun;
    }
    target = {true, type, definition.health, type == PlantType::chomper ? 0 : definition.action_delay, 0};
    _sun -= definition.cost;
    _cooldowns[static_cast<int>(type)] = definition.recharge;
    return PlantResult::planted;
}

bool Game::tombstone_at(int row, int column) const {
    for (const auto& tombstone : _tombstones) {
        if (tombstone.active() && tombstone.row == row && tombstone.column == column) {
            return true;
        }
    }
    return false;
}

bool Game::shovel(int row, int column) {
    if (_phase != Phase::playing || !valid_cell(row, column)) {
        return false;
    }
    auto& target = _plants[cell_index(row, column)];
    bool removed = target.active;
    target.active = false;
    return removed;
}

bool Game::lane_has_target(int row, int x) const {
    for (const auto& zombie : _zombies) {
        if (zombie.active && zombie.row == row && zombie.x > x) {
            return true;
        }
    }
    for (const auto& tombstone : _tombstones) {
        if (tombstone.active() && tombstone.row == row && cell_x(tombstone.column) * scale > x) {
            return true;
        }
    }
    return false;
}

void Game::explode(int row, int column) {
    _events.explosion = true;
    for (auto& tombstone : _tombstones) {
        if (tombstone.active() && detail::absolute(tombstone.row - row) <= 1 &&
            detail::absolute(cell_x(tombstone.column) - cell_x(column)) <= 37) {
            damage(tombstone, 100);
        }
    }
    for (auto& zombie : _zombies) {
        if (zombie.active && detail::absolute(zombie.row - row) <= 1 &&
            detail::absolute(zombie.x - cell_x(column) * scale) <= 37 * scale) {
            damage(zombie, 100, DamageType::blast);
        }
    }
}

void Game::update_plants() {
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            auto& plant = _plants[cell_index(row, column)];
            if (!plant.active || plant.wilted) {
                continue;
            }
            if (plant.flash > 0) {
                --plant.flash;
            }
            if (plant.shot_frames > 0) {
                --plant.shot_frames;
            }
            if (plant.exploding()) {
                plant.advance_explosion();
                continue;
            }
            if (plant.type == PlantType::squash) {
                update_squash(plant, row, column);
                continue;
            }
            if (plant.type == PlantType::chomper) {
                update_chomper(plant, row, column);
                continue;
            }
            if (plant.burst_frames > 0 && --plant.burst_frames == 0) {
                if (shoot(row, (cell_x(column) + 8) * scale, false)) {
                    plant.shot_frames = shooter_recoil_frames;
                } else {
                    plant.burst_frames = 1;
                }
            }
            if (plant.timer > 0) {
                --plant.timer;
            }
            if (plant.timer > 0) {
                continue;
            }
            const int x = cell_x(column) * scale;
            switch (plant.type) {
                case PlantType::sunflower:
                    if (spawn_sun(x, (cell_y(row) - 4) * scale, (cell_y(row) + 5) * scale, true)) {
                        plant.timer = spec(plant.type).action_delay;
                    }
                    break;
                case PlantType::peashooter:
                case PlantType::snow_pea:
                case PlantType::repeater:
                    if (lane_has_target(row, x) &&
                        shoot(row, x + 8 * scale, plant.type == PlantType::snow_pea)) {
                        plant.timer = spec(plant.type).action_delay;
                        plant.shot_frames = shooter_recoil_frames;
                        if (plant.type == PlantType::repeater) {
                            plant.burst_frames = 8;
                        }
                    }
                    break;
                case PlantType::cherry:
                    // Keep an explosion sprite alive for 18 frames, then remove it.
                    explode(row, column);
                    plant.health = 0;
                    plant.timer = 18;
                    break;
                case PlantType::potato_mine:
                    for (auto& zombie : _zombies) {
                        if (zombie.active && zombie.vault_frames == 0 && zombie.row == row &&
                            detail::absolute(zombie.x - x) <= 8 * scale) {
                            _events.explosion = true;
                            for (auto& nearby : _zombies) {
                                if (nearby.active && nearby.row == row &&
                                    detail::absolute(nearby.x - x) <= 18 * scale) {
                                    damage(nearby, 100, DamageType::blast);
                                }
                            }
                            for (auto& tombstone : _tombstones) {
                                if (tombstone.active() && tombstone.row == row &&
                                    detail::absolute(cell_x(tombstone.column) * scale - x) <= 18 * scale) {
                                    damage(tombstone, 100);
                                }
                            }
                            plant.health = 0;
                            plant.timer = 18;
                            break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void Game::update_chomper(Plant& plant, int row, int column) {
    if (plant.timer > 0) {
        --plant.timer;
    }
    if (plant.bite_frames > 0) {
        if (--plant.bite_frames == chomper_swallow_frame && plant.bite_target >= 0) {
            auto& target = _zombies[plant.bite_target];
            int distance = target.x - cell_x(column) * scale;
            if (target.active && target.row == row && target.vault_frames == 0 &&
                target.emergence_frames == 0 && distance >= -8 * scale && distance <= 48 * scale) {
                damage(target, 100, DamageType::swallow);
                plant.timer = spec(PlantType::chomper).action_delay;
                _events.chomp = true;
            }
            plant.bite_target = -1;
        }
        return;
    }
    if (plant.timer > 0) {
        return;
    }
    // Reach an enemy biting the plant in the immediately adjacent cell.
    int closest = 40 * scale;
    for (int i = 0; i < zombie_capacity; ++i) {
        const auto& zombie = _zombies[i];
        int distance = zombie.x - cell_x(column) * scale;
        if (zombie.active && zombie.row == row && zombie.vault_frames == 0 && zombie.emergence_frames == 0 &&
            distance >= -5 * scale && distance < closest) {
            closest = distance;
            plant.bite_target = i;
        }
    }
    if (plant.bite_target >= 0) {
        plant.bite_frames = chomper_bite_duration;
    }
}

void Game::update_squash(Plant& plant, int row, int column) {
    if (plant.target_x == 0) {
        int closest = 31 * scale;
        for (const auto& zombie : _zombies) {
            int distance = detail::absolute(zombie.x - cell_x(column) * scale);
            if (zombie.active && zombie.row == row && zombie.vault_frames == 0 && distance < closest) {
                closest = distance;
                plant.target_x = zombie.x;
            }
        }
        if (plant.target_x != 0) {
            plant.timer = 30;
        }
        return;
    }
    if (--plant.timer > 0) {
        return;
    }
    for (auto& zombie : _zombies) {
        if (zombie.active && zombie.row == row && zombie.vault_frames == 0 &&
            detail::absolute(zombie.x - plant.target_x) <= 20 * scale) {
            damage(zombie, 100, DamageType::crush);
        }
    }
    for (auto& stone : _tombstones) {
        if (stone.active() && stone.row == row &&
            detail::absolute(cell_x(stone.column) * scale - plant.target_x) <= 20 * scale) {
            damage(stone, 100);
        }
    }
    _events.explosion = true;
    plant.health = 0;
    plant.timer = 18;
}
} // namespace garden
