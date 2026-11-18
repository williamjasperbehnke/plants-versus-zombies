#include "game/game.h"

#include "game/game_internal.h"

namespace garden {
bool Game::start(int level) {
    if (level < 0) {
        level = _level;
    }
    if (!level_unlocked(level)) {
        return false;
    }
    auto progress = _progress;
    progress.selected = level;
    *this = Game();
    _level = level;
    _progress = progress;
    _phase = Phase::playing;
    _seed_count = required_seeds();
    for (int i = 0; i < _seed_count; ++i) {
        _seed_bank[i] = seed_order[i];
    }
    for (int row = 0; row < rows; ++row) {
        _mowers[row].available = row_active(row);
    }
    if (level == 1) {
        for (const auto& cell : tutorial_weeds) {
            auto& plant = _plants[cell_index(cell.row, cell.column)];
            plant.active = true;
            plant.type = PlantType::peashooter;
            plant.health = 1;
            plant.wilted = true;
        }
    }
    // Keep the house side open; later levels add stones in distinct lanes.
    constexpr int positions[tombstone_capacity][2] = {{0, 6}, {3, 7}, {1, 7}};
    for (int i = 0; i < level_spec().tombstones; ++i) {
        _tombstones[i] = {positions[i][0], positions[i][1], tombstone_health};
    }
    return true;
}

bool Game::prepare_level() {
    if (!start()) {
        return false;
    }
    _phase = Phase::preview;
    return true;
}

void Game::choose_seeds() {
    if (_phase != Phase::preview || !preview_ready()) {
        return;
    }
    _phase = Phase::seed_select;
    if (unlocked_plants() <= visible_seed_count) {
        begin_level();
    }
}

void Game::cancel_seed_selection() {
    if (_phase == Phase::seed_select) {
        _phase = Phase::preview;
    }
}

bool Game::toggle_seed(PlantType type) {
    if (_phase != Phase::seed_select || !plant_unlocked(type)) {
        return false;
    }
    int slot = bank_slot(type);
    if (slot < _seed_count) {
        for (int i = slot; i + 1 < _seed_count; ++i) {
            _seed_bank[i] = _seed_bank[i + 1];
        }
        --_seed_count;
    } else {
        if (_seed_count == required_seeds()) {
            return false;
        }
        _seed_bank[_seed_count++] = type;
    }
    return true;
}

void Game::begin_level() {
    if (_phase != Phase::seed_select || _seed_count != required_seeds()) {
        return;
    }
    _phase = Phase::preview;
    _preview_returning = true;
    _preview_frames = 0;
}

void Game::open_save_select() {
    if (_phase == Phase::title) {
        _phase = Phase::save_select;
    }
}

void Game::open_almanac() {
    if (_phase == Phase::title) {
        _phase = Phase::almanac;
    }
}

void Game::close_almanac() {
    if (_phase == Phase::almanac) {
        _phase = Phase::title;
    }
}

void Game::load_progress(const Progress& progress) {
    if (!progress.valid()) {
        return;
    }
    _progress = progress;
    _level = progress.selected;
}

void Game::return_to_title() {
    auto progress = _progress;
    *this = Game();
    load_progress(progress);
}

void Game::open_level_select() {
    int selected = _level;
    if (_phase == Phase::won && has_next_level()) {
        ++selected;
    }
    return_to_title();
    _level = selected;
    _phase = Phase::level_select;
}

void Game::select_level(int delta) {
    if (_phase != Phase::level_select) {
        return;
    }
    _level += delta;
    if (_level < 0) {
        _level = 0;
    } else if (_level >= level_count) {
        _level = level_count - 1;
    }
}

void Game::toggle_pause() {
    if (_phase == Phase::playing) {
        _phase = Phase::paused;
    } else if (_phase == Phase::paused) {
        _phase = Phase::playing;
    }
}

int Game::collect_sun(bool plants_only) {
    if (_phase != Phase::playing) {
        return 0;
    }
    int collected = 0;
    for (auto& sun : _suns) {
        if (sun.active && !sun.collecting && (!plants_only || sun.from_plant)) {
            sun.collecting = true;
            sun.flight_frame = 0;
            sun.origin_x = sun.x;
            sun.origin_y = sun.y;
            collected += 25;
        }
    }
    _sun += collected;
    if (_sun > 9999) {
        _sun = 9999;
    }
    return collected;
}

void Game::hasten_sun(int frames, bool from_plant) {
    if (frames <= 0) {
        return;
    }
    if (!from_plant) {
        if (_sky_timer > frames) {
            _sky_timer = frames;
        }
        return;
    }
    for (auto& plant : _plants) {
        if (plant.active && !plant.wilted && plant.type == PlantType::sunflower && plant.timer > frames) {
            plant.timer = frames;
        }
    }
}

void Game::tick(bool hold_waves, bool hold_sky_sun) {
    _events = {};
    if (_phase == Phase::preview) {
        if (_preview_frames < preview_ready_frame) {
            ++_preview_frames;
        }
        if (_preview_returning && _preview_frames >= preview_pan_frames) {
            _phase = Phase::playing;
        }
        return;
    }
    if (_phase == Phase::won) {
        if (victory_settling()) {
            // Finish effects already in flight without advancing the economy or combat.
            for (int index = 0; index < rows * columns; ++index) {
                auto& plant = _plants[index];
                if (plant.active && plant.type == PlantType::chomper && plant.bite_frames > 0) {
                    update_chomper(plant, index / columns, index % columns);
                } else if (plant.committed() && plant.health > 0) {
                    update_squash(plant, index / columns, index % columns);
                } else {
                    plant.advance_explosion();
                }
            }
            update_mowers();
            update_zombie_effects();
            update_tombstones();
        } else if (_reward_frames < reward_ready_frame) {
            ++_reward_frames;
        }
        return;
    }
    if (_phase == Phase::lost) {
        update_zombie_effects();
        update_tombstones();
        if (_loss_frames < defeat_audio::ready_frame) {
            ++_loss_frames;
        }
        return;
    }
    if (_phase != Phase::playing) {
        return;
    }
    ++_frame;
    update_zombie_effects();
    for (auto& zombie : _zombies) {
        if (zombie.active) {
            advance_zombie_status(zombie);
        }
    }
    for (auto& cooldown : _cooldowns) {
        if (cooldown > 0) {
            --cooldown;
        }
    }
    update_tombstones();
    update_suns(hold_sky_sun);
    if (!hold_waves) {
        update_waves();
    }
    if (_phase != Phase::playing) {
        return;
    }
    update_plants();
    update_shots();
    update_mowers();
    update_zombies();
}

bool Game::victory_settling() const {
    if (_phase != Phase::won) {
        return false;
    }
    for (const auto& plant : _plants) {
        if (plant.exploding() || plant.committed() || (plant.active && plant.bite_frames > 0)) {
            return true;
        }
    }
    for (const auto& mower : _mowers) {
        if (mower.moving) {
            return true;
        }
    }
    for (const auto& effect : _zombie_effects) {
        if (effect.active) {
            return true;
        }
    }
    for (const auto& stone : _tombstones) {
        if (stone.crumble_frames > 0) {
            return true;
        }
    }
    return false;
}

uint32_t Game::random() {
    _random ^= _random << 13;
    _random ^= _random >> 17;
    _random ^= _random << 5;
    return _random;
}

bool Game::spawn_sun(int x, int y, int target_y, bool from_plant) {
    auto* sun = detail::free_slot(_suns);
    if (!sun) {
        return false;
    }
    *sun = {true, x, y, target_y, sun_lifetime};
    sun->from_plant = from_plant;
    return true;
}

void Game::update_suns(bool hold_sky_sun) {
    for (auto& sun : _suns) {
        if (!sun.active) {
            continue;
        }
        if (sun.collecting) {
            ++sun.flight_frame;
            int remaining = sun_collection_frames - sun.flight_frame;
            constexpr int duration_squared = sun_collection_frames * sun_collection_frames;
            int progress = duration_squared - remaining * remaining;
            sun.x = sun.origin_x + (sun_bank_x * scale - sun.origin_x) * progress / duration_squared;
            sun.y = sun.origin_y + (sun_bank_y * scale - sun.origin_y) * progress / duration_squared;
            if (remaining == 0) {
                sun.active = false;
                sun.collecting = false;
            }
            continue;
        }
        if (sun.y < sun.target_y) {
            sun.y += 128;
        }
        if (--sun.lifetime <= 0) {
            sun.active = false;
        }
    }
    if (!hold_sky_sun && --_sky_timer <= 0) {
        spawn_sun(
            (45 + int(random() % 174)) * scale,
            lawn_top * scale,
            cell_y(first_row() + int(random() % level_spec().lanes)) * scale
        );
        _sky_timer = sky_sun_interval;
    }
}
} // namespace garden
