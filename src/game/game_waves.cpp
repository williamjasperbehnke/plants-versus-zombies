#include "game/game.h"
#include "game/game_internal.h"

namespace garden {
bool Game::any_zombies() const {
    for (const auto& zombie : _zombies) {
        if (zombie.active) {
            return true;
        }
    }
    return false;
}

int Game::enemies_left() const {
    int count = _remaining;
    for (const auto& pending : _pending_groups) {
        count += pending.remaining;
    }
    for (const auto& tombstone : _tombstones) {
        count += tombstone.pending;
    }
    for (const auto& zombie : _zombies) {
        if (zombie.active) {
            ++count;
        }
    }
    return count;
}

int Game::active_group_zombies() const {
    int count = 0;
    for (const auto& zombie : _zombies) {
        count += zombie.active && zombie.group == _group;
    }
    return count;
}

bool Game::horde_remaining(int wave) const {
    int group = wave * 2;
    if (_group == group && _remaining > 0) {
        return true;
    }
    if (_pending_groups[group - 1].remaining > 0) {
        return true;
    }
    for (const auto& zombie : _zombies) {
        if (zombie.active && zombie.group == group) {
            return true;
        }
    }
    return false;
}

int Game::wave_progress() const {
    if (_wave == 0) {
        return 0;
    }
    if (_horde) {
        return _wave * 100;
    }
    int size = level_spec().normal_size(_wave);
    int threshold = (size + 1) / 2;
    int defeated = size - _remaining - active_group_zombies();
    if (defeated > threshold) {
        defeated = threshold;
    }
    return (_wave - 1) * 100 + defeated * 100 / threshold;
}

void Game::retain_group_spawns() {
    if (_remaining > 0) {
        _pending_groups[_group - 1] = {_remaining, _spawn_timer, _group, _wave, _horde};
    }
}

bool Game::pending_spawns() const {
    for (const auto& pending : _pending_groups) {
        if (pending.remaining > 0) {
            return true;
        }
    }
    for (const auto& tombstone : _tombstones) {
        if (tombstone.pending > 0) {
            return true;
        }
    }
    return false;
}

int Game::spawn_delay(int wave, bool horde) const {
    if (!horde) {
        return 240;
    }
    int delay = level_spec().spawn_delay;
    return wave == total_waves() ? delay * 2 / 3 : delay;
}

void Game::update_waves() {
    bool start_normal = false;
    if (_wave == 0) {
        start_normal = --_preparation <= 0;
    } else {
        int size = _horde ? level_spec().wave_size(_wave) : level_spec().normal_size(_wave);
        int defeated = size - _remaining - active_group_zombies();
        bool half_defeated = defeated >= (size + 1) / 2;
        if (!_horde && half_defeated) {
            retain_group_spawns();
            _horde = true;
            ++_group;
            _remaining = level_spec().wave_size(_wave);
            _spawn_timer = 1;
        } else if (_horde && _wave < total_waves() && half_defeated) {
            retain_group_spawns();
            _events.wave_cleared = true;
            start_normal = true;
        } else if (_horde && _wave == total_waves() && _remaining == 0 && !pending_spawns() &&
                   !any_zombies()) {
            _events.wave_cleared = true;
            if (!level_completed(_level) && unlocked_plants() < plant_count) {
                _unlocked_plant = seed_order[unlocked_plants()];
            }
            _phase = Phase::won;
            _progress.completed |= uint32_t(1) << _level;
            if (has_next_level() && _progress.unlocked <= _level + 1) {
                _progress.unlocked = _level + 2;
            }
            return;
        }
    }
    if (start_normal) {
        ++_wave;
        ++_group;
        _horde = false;
        _remaining = level_spec().normal_size(_wave);
        _events.wave_started = true;
        if (_wave == total_waves()) {
            _events.final_wave_approaching = true;
        }
        _spawn_timer = 1;
    }
    // Keep releasing any unspawned stragglers after their group has advanced.
    for (auto& pending : _pending_groups) {
        if (pending.remaining > 0 && --pending.timer <= 0 &&
            spawn_zombie(pending.wave, pending.horde, pending.remaining, pending.group)) {
            --pending.remaining;
            pending.timer = spawn_delay(pending.wave, pending.horde);
        }
    }
    if (_remaining > 0 && --_spawn_timer <= 0 && spawn_zombie(_wave, _horde, _remaining, _group)) {
        --_remaining;
        _spawn_timer = spawn_delay(_wave, _horde);
    }
    update_tombstone_spawns();
}

bool Game::spawn_zombie(int wave, bool horde, int remaining, int group) {
    const int row =
        first_row() + (_spawn_number * (level_spec().lanes == 3 ? 2 : 3) + wave) % level_spec().lanes;
    const auto& definition = level_spec();
    int size = horde ? definition.wave_size(wave) : definition.normal_size(wave);
    ZombieType type = scheduled_zombie(definition, wave, horde, size - remaining);
    if (!spawn_zombie_at(type, row, road_spawn_x * scale, group)) {
        return false;
    }
    ++_spawn_number;
    return true;
}

void Game::enter_horde(const Zombie& leader) {
    int wave = leader.group / 2;
    if (leader.type != ZombieType::flag || wave <= 0 || wave > total_waves() || _hordes_entered[wave - 1]) {
        return;
    }
    _hordes_entered[wave - 1] = true;
    _events.horde_started = true;
    for (auto& tombstone : _tombstones) {
        if (tombstone.active()) {
            ++tombstone.pending;
        }
    }
}

Zombie* Game::spawn_zombie_at(ZombieType type, int row, int x, int group) {
    auto* zombie = detail::free_slot(_zombies);
    if (!zombie) {
        return nullptr;
    }
    *zombie = {true, type, row, x, spec(type).health, 40, 0};
    zombie->shield_health = spec(type).shield_health;
    zombie->group = group;
    return zombie;
}

void Game::update_tombstone_spawns() {
    for (auto& tombstone : _tombstones) {
        if (tombstone.pending <= 0) {
            continue;
        }
        if (auto* zombie =
                spawn_zombie_at(ZombieType::normal, tombstone.row, cell_x(tombstone.column) * scale, 0)) {
            zombie->emergence_frames = emergence_duration;
            --tombstone.pending;
            ++_events.tombstone_spawns;
        }
    }
}
} // namespace garden
