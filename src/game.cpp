#include "game.h"

namespace garden
{
namespace
{
template<class Pool> auto* free_slot(Pool& pool)
{
    for(auto& slot : pool)
    {
        if(! slot.active) { return &slot; }
    }
    return static_cast<typename Pool::value_type*>(nullptr);
}
int absolute(int value) { return value < 0 ? -value : value; }
bool shot_reaches(int target_x, int previous_x, int next_x)
{
    constexpr int hit_radius = 7 * scale;
    return target_x + hit_radius >= previous_x && target_x - hit_radius <= next_x;
}
bool valid_cell(int row, int column)
{
    return row >= 0 && row < rows && column >= 0 && column < columns;
}
}

bool Game::start(int level)
{
    if(level < 0) { level = _level; }
    if(! level_unlocked(level)) { return false; }
    auto progress = _progress;
    progress.selected = level;
    *this = Game();
    _level = level;
    _progress = progress;
    _phase = Phase::playing;
    _seed_count = required_seeds();
    for(int i = 0; i < _seed_count; ++i) { _seed_bank[i] = seed_order[i]; }
    for(int row = 0; row < rows; ++row) { _mowers[row].available = row_active(row); }
    // Keep the house side open; later levels add stones in distinct lanes.
    constexpr int positions[tombstone_capacity][2] = {{0, 6}, {3, 7}, {1, 7}};
    for(int i = 0; i < level_spec().tombstones; ++i)
    { _tombstones[i] = {positions[i][0], positions[i][1], tombstone_health}; }
    return true;
}

bool Game::prepare_level()
{
    if(! start()) { return false; }
    _phase = Phase::preview;
    return true;
}

void Game::choose_seeds()
{
    if(_phase != Phase::preview || ! preview_ready()) { return; }
    _phase = Phase::seed_select;
    if(unlocked_plants() <= visible_seed_count) { begin_level(); }
}

void Game::cancel_seed_selection()
{
    if(_phase == Phase::seed_select) { _phase = Phase::preview; }
}

bool Game::toggle_seed(PlantType type)
{
    if(_phase != Phase::seed_select || ! plant_unlocked(type)) { return false; }
    int slot = bank_slot(type);
    if(slot < _seed_count)
    {
        for(int i = slot; i + 1 < _seed_count; ++i) { _seed_bank[i] = _seed_bank[i + 1]; }
        --_seed_count;
    }
    else
    {
        if(_seed_count == required_seeds()) { return false; }
        _seed_bank[_seed_count++] = type;
    }
    return true;
}

void Game::begin_level()
{
    if(_phase != Phase::seed_select || _seed_count != required_seeds()) { return; }
    _phase = Phase::preview;
    _preview_returning = true;
    _preview_frames = 0;
}

void Game::open_almanac()
{
    if(_phase == Phase::title) { _phase = Phase::almanac; }
}

void Game::close_almanac()
{
    if(_phase == Phase::almanac) { _phase = Phase::title; }
}

void Game::load_progress(const Progress& progress)
{
    if(! progress.valid()) { return; }
    _progress = progress;
    _level = progress.selected;
}

void Game::return_to_title()
{
    auto progress = _progress;
    *this = Game();
    load_progress(progress);
}

void Game::open_level_select()
{
    int selected = _level;
    if(_phase == Phase::won && has_next_level()) { ++selected; }
    return_to_title();
    _level = selected;
    _phase = Phase::level_select;
}

void Game::select_level(int delta)
{
    if(_phase != Phase::level_select) { return; }
    int selected = _level + delta;
    _level = selected < 0 ? 0 : (selected >= level_count ? level_count - 1 : selected);
}

void Game::toggle_pause()
{
    if(_phase == Phase::playing) { _phase = Phase::paused; }
    else if(_phase == Phase::paused) { _phase = Phase::playing; }
}

PlantResult Game::plant(int row, int column, PlantType type)
{
    if(_phase != Phase::playing || ! valid_cell(row, column) || ! row_active(row) ||
       ! plant_available(type))
    {
        return PlantResult::inactive;
    }
    auto& target = _plants[cell_index(row, column)];
    if(target.active || tombstone_at(row, column)) { return PlantResult::occupied; }
    if(cooldown(type)) { return PlantResult::cooling_down; }
    const auto& definition = spec(type);
    if(_sun < definition.cost) { return PlantResult::insufficient_sun; }
    target = {true, type, definition.health, type == PlantType::chomper ? 0 : definition.action_delay, 0};
    _sun -= definition.cost;
    _cooldowns[static_cast<int>(type)] = definition.recharge;
    return PlantResult::planted;
}

bool Game::tombstone_at(int row, int column) const
{
    for(const auto& tombstone : _tombstones)
    { if(tombstone.active() && tombstone.row == row && tombstone.column == column) { return true; } }
    return false;
}

bool Game::shovel(int row, int column)
{
    if(_phase != Phase::playing || ! valid_cell(row, column)) { return false; }
    auto& target = _plants[cell_index(row, column)];
    bool removed = target.active;
    target.active = false;
    return removed;
}

int Game::collect_sun()
{
    if(_phase != Phase::playing) { return 0; }
    int collected = 0;
    for(auto& sun : _suns)
    {
        if(sun.active && ! sun.collecting)
        {
            sun.collecting = true;
            sun.flight_frame = 0;
            sun.origin_x = sun.x;
            sun.origin_y = sun.y;
            collected += 25;
        }
    }
    _sun += collected;
    if(_sun > 9999) { _sun = 9999; }
    return collected;
}

void Game::tick()
{
    _events = {};
    if(_phase == Phase::preview)
    {
        if(_preview_frames < preview_ready_frame) { ++_preview_frames; }
        if(_preview_returning && _preview_frames >= preview_pan_frames) { _phase = Phase::playing; }
        return;
    }
    if(_phase == Phase::won)
    {
        if(victory_settling())
        {
            // Finish effects already in flight without advancing the economy or combat.
            for(int index = 0; index < rows * columns; ++index)
            {
                auto& plant = _plants[index];
                if(plant.active && plant.type == PlantType::chomper && plant.bite_frames > 0)
                { update_chomper(plant, index / columns, index % columns); }
                else if(plant.committed() && plant.health > 0)
                { update_squash(plant, index / columns, index % columns); }
                else { plant.advance_explosion(); }
            }
            update_mowers();
            update_zombie_effects();
            update_tombstones();
        }
        else if(_reward_frames < reward_ready_frame) { ++_reward_frames; }
        return;
    }
    if(_phase == Phase::lost)
    { update_zombie_effects(); update_tombstones(); if(_loss_frames < defeat_audio::ready_frame) { ++_loss_frames; } return; }
    if(_phase != Phase::playing) { return; }
    ++_frame;
    update_zombie_effects();
    for(auto& zombie : _zombies) { if(zombie.active) { advance_zombie_status(zombie); } }
    for(auto& cooldown : _cooldowns) { if(cooldown > 0) { --cooldown; } }
    update_tombstones();
    update_suns();
    update_waves();
    if(_phase != Phase::playing) { return; }
    update_plants();
    update_shots();
    update_mowers();
    update_zombies();
}

bool Game::victory_settling() const
{
    if(_phase != Phase::won) { return false; }
    for(const auto& plant : _plants)
    {
        if(plant.exploding() || plant.committed() || (plant.active && plant.bite_frames > 0)) { return true; }
    }
    for(const auto& mower : _mowers) { if(mower.moving) { return true; } }
    for(const auto& effect : _zombie_effects) { if(effect.active) { return true; } }
    for(const auto& stone : _tombstones) { if(stone.crumble_frames > 0) { return true; } }
    return false;
}

uint32_t Game::random()
{
    _random ^= _random << 13;
    _random ^= _random >> 17;
    _random ^= _random << 5;
    return _random;
}

bool Game::any_zombies() const
{
    for(const auto& zombie : _zombies) { if(zombie.active) { return true; } }
    return false;
}

int Game::enemies_left() const
{
    int count = _remaining;
    for(const auto& pending : _pending_groups) { count += pending.remaining; }
    for(const auto& tombstone : _tombstones) { count += tombstone.pending; }
    for(const auto& zombie : _zombies) { if(zombie.active) { ++count; } }
    return count;
}

int Game::active_group_zombies() const
{
    int count = 0;
    for(const auto& zombie : _zombies) { count += zombie.active && zombie.group == _group; }
    return count;
}

bool Game::horde_remaining(int wave) const
{
    int group = wave * 2;
    if(_group == group && _remaining > 0) { return true; }
    if(_pending_groups[group - 1].remaining > 0) { return true; }
    for(const auto& zombie : _zombies)
    { if(zombie.active && zombie.group == group) { return true; } }
    return false;
}

int Game::wave_progress() const
{
    if(_wave == 0) { return 0; }
    if(_horde) { return _wave * 100; }
    int size = level_spec().normal_size(_wave);
    int threshold = (size + 1) / 2;
    int defeated = size - _remaining - active_group_zombies();
    if(defeated > threshold) { defeated = threshold; }
    return (_wave - 1) * 100 + defeated * 100 / threshold;
}

void Game::retain_group_spawns()
{
    if(_remaining > 0)
    { _pending_groups[_group - 1] = {_remaining, _spawn_timer, _group, _wave, _horde}; }
}

bool Game::pending_spawns() const
{
    for(const auto& pending : _pending_groups) { if(pending.remaining > 0) { return true; } }
    for(const auto& tombstone : _tombstones) { if(tombstone.pending > 0) { return true; } }
    return false;
}

int Game::spawn_delay(int wave, bool horde) const
{
    if(! horde) { return 240; }
    int delay = level_spec().spawn_delay;
    return wave == total_waves() ? delay * 2 / 3 : delay;
}

void Game::update_waves()
{
    bool start_normal = false;
    if(_wave == 0)
    {
        start_normal = --_preparation <= 0;
    }
    else
    {
        int size = _horde ? level_spec().wave_size(_wave) : level_spec().normal_size(_wave);
        int defeated = size - _remaining - active_group_zombies();
        bool half_defeated = defeated >= (size + 1) / 2;
        if(! _horde && half_defeated)
        {
            retain_group_spawns();
            _horde = true;
            ++_group;
            _remaining = level_spec().wave_size(_wave);
            _spawn_timer = 1;
        }
        else if(_horde && _wave < total_waves() && half_defeated)
        {
            retain_group_spawns();
            _events.wave_cleared = true;
            start_normal = true;
        }
        else if(_horde && _wave == total_waves() && _remaining == 0 && ! pending_spawns() && ! any_zombies())
        {
            _events.wave_cleared = true;
            if(! level_completed(_level) && unlocked_plants() < plant_count)
            { _unlocked_plant = seed_order[unlocked_plants()]; }
            _phase = Phase::won;
            _progress.completed |= uint32_t(1) << _level;
            if(has_next_level() && _progress.unlocked <= _level + 1) { _progress.unlocked = _level + 2; }
            return;
        }
    }
    if(start_normal)
    {
        ++_wave;
        ++_group;
        _horde = false;
        _remaining = level_spec().normal_size(_wave);
        _events.wave_started = true;
        if(_wave == total_waves())
        {
            _events.final_wave_approaching = true;
        }
        _spawn_timer = 1;
    }
    // Keep releasing any unspawned stragglers after their group has advanced.
    for(auto& pending : _pending_groups)
    {
        if(pending.remaining > 0 && --pending.timer <= 0 &&
           spawn_zombie(pending.wave, pending.horde, pending.remaining, pending.group))
        {
            --pending.remaining;
            pending.timer = spawn_delay(pending.wave, pending.horde);
        }
    }
    if(_remaining > 0 && --_spawn_timer <= 0 && spawn_zombie(_wave, _horde, _remaining, _group))
    {
        --_remaining;
        _spawn_timer = spawn_delay(_wave, _horde);
    }
    update_tombstone_spawns();
}

bool Game::spawn_zombie(int wave, bool horde, int remaining, int group)
{
    const int row = first_row() + (_spawn_number * (level_spec().lanes == 3 ? 2 : 3) + wave) % level_spec().lanes;
    const auto& definition = level_spec();
    int size = horde ? definition.wave_size(wave) : definition.normal_size(wave);
    ZombieType type = scheduled_zombie(definition, wave, horde, size - remaining);
    if(! spawn_zombie_at(type, row, road_spawn_x * scale, group)) { return false; }
    ++_spawn_number;
    return true;
}

void Game::enter_horde(const Zombie& leader)
{
    int wave = leader.group / 2;
    if(leader.type != ZombieType::flag || wave <= 0 || wave > total_waves() || _hordes_entered[wave - 1]) { return; }
    _hordes_entered[wave - 1] = true;
    _events.horde_started = true;
    for(auto& tombstone : _tombstones) { if(tombstone.active()) { ++tombstone.pending; } }
}

Zombie* Game::spawn_zombie_at(ZombieType type, int row, int x, int group)
{
    auto* zombie = free_slot(_zombies);
    if(! zombie) { return nullptr; }
    *zombie = {true, type, row, x, spec(type).health, 40, 0};
    zombie->shield_health = spec(type).shield_health;
    zombie->group = group;
    return zombie;
}

void Game::update_tombstones()
{
    for(auto& stone : _tombstones)
    {
        if(stone.flash > 0) { --stone.flash; }
        if(stone.crumble_frames > 0) { --stone.crumble_frames; }
    }
}

void Game::update_tombstone_spawns()
{
    for(auto& tombstone : _tombstones)
    {
        if(tombstone.pending <= 0) { continue; }
        if(auto* zombie = spawn_zombie_at(ZombieType::normal, tombstone.row, cell_x(tombstone.column) * scale, 0))
        {
            zombie->emergence_frames = emergence_duration;
            --tombstone.pending;
            ++_events.tombstone_spawns;
        }
    }
}

bool Game::spawn_sun(int x, int y, int target_y)
{
    auto* sun = free_slot(_suns);
    if(! sun) { return false; }
    *sun = {true, x, y, target_y, 660};
    return true;
}

void Game::update_suns()
{
    for(auto& sun : _suns)
    {
        if(! sun.active) { continue; }
        if(sun.collecting)
        {
            ++sun.flight_frame;
            int remaining = sun_collection_frames - sun.flight_frame;
            constexpr int duration_squared = sun_collection_frames * sun_collection_frames;
            int progress = duration_squared - remaining * remaining;
            sun.x = sun.origin_x + (sun_bank_x * scale - sun.origin_x) * progress / duration_squared;
            sun.y = sun.origin_y + (sun_bank_y * scale - sun.origin_y) * progress / duration_squared;
            if(remaining == 0) { sun.active = false; sun.collecting = false; }
            continue;
        }
        if(sun.y < sun.target_y) { sun.y += 128; }
        if(--sun.lifetime <= 0) { sun.active = false; }
    }
    if(--_sky_timer <= 0)
    {
        spawn_sun((45 + int(random() % 174)) * scale, lawn_top * scale,
                  cell_y(first_row() + int(random() % level_spec().lanes)) * scale);
        _sky_timer = sky_sun_interval;
    }
}

bool Game::lane_has_target(int row, int x) const
{
    for(const auto& zombie : _zombies)
    {
        if(zombie.active && zombie.row == row && zombie.x > x) { return true; }
    }
    for(const auto& tombstone : _tombstones)
    { if(tombstone.active() && tombstone.row == row && cell_x(tombstone.column) * scale > x) { return true; } }
    return false;
}

bool Game::shoot(int row, int x, bool frozen)
{
    auto* shot = free_slot(_shots);
    if(! shot) { return false; }
    *shot = {true, row, x, frozen};
    _events.shot = true;
    return true;
}

void Game::spawn_zombie_effect(const Zombie& zombie, ZombieEffectType type)
{
    auto* effect = free_slot(_zombie_effects);
    if(! effect)
    {
        // Keep combat independent of the cosmetic pool; replace the effect nearest expiry.
        effect = &_zombie_effects[0];
        for(auto& candidate : _zombie_effects)
        { if(candidate.remaining < effect->remaining) { effect = &candidate; } }
    }
    *effect = {true, type, zombie.row, zombie.x, effect_duration(type), zombie.arm_lost};
}

void Game::update_zombie_effects()
{
    for(auto& effect : _zombie_effects)
    { if(effect.active && --effect.remaining <= 0) { effect.active = false; } }
}

void Game::damage(Zombie& zombie, int amount, DamageType type)
{
    const bool swallowed = type == DamageType::swallow;
    const int old_health = zombie.health;
    const int old_shield = zombie.shield_health;
    bool killed = take_zombie_damage(zombie, amount, type);
    _events.hit = true;
    zombie.flash = 6;
    if(! swallowed && zombie.type == ZombieType::cone && old_health > 6 && (zombie.health <= 6 || killed))
    { spawn_zombie_effect(zombie, ZombieEffectType::cone); }
    if(! swallowed && old_shield > 0 && (zombie.shield_health == 0 || killed))
    {
        switch(zombie.type)
        {
        case ZombieType::newspaper: spawn_zombie_effect(zombie, ZombieEffectType::newspaper); break;
        case ZombieType::bucket: spawn_zombie_effect(zombie, ZombieEffectType::bucket); break;
        case ZombieType::screen_door: spawn_zombie_effect(zombie, ZombieEffectType::door); break;
        case ZombieType::football: spawn_zombie_effect(zombie, ZombieEffectType::football_helmet); break;
        default: break;
        }
    }
    if(killed)
    {
        if(! swallowed && zombie.type == ZombieType::vaulter && ! zombie.pole_used) { spawn_zombie_effect(zombie, ZombieEffectType::pole); }
        if(zombie.type == ZombieType::flag)
        {
            // A mower or edge blast can defeat an entering leader before its first visible step.
            enter_horde(zombie);
            if(! swallowed) { spawn_zombie_effect(zombie, ZombieEffectType::flag); }
        }
        auto death = zombie.type == ZombieType::flag ? ZombieEffectType::flag_death :
                     (zombie.type == ZombieType::newspaper ? ZombieEffectType::newspaper_death :
                      (zombie.type == ZombieType::football ? ZombieEffectType::football_death :
                       (zombie.type == ZombieType::vaulter ? ZombieEffectType::vault_death : ZombieEffectType::death)));
        if(! swallowed) { spawn_zombie_effect(zombie, death); }
        zombie.active = false;
        ++_kills;
        auto& total = _progress.zombie_kills[static_cast<int>(zombie.type)];
        if(total < UINT32_MAX) { ++total; }
    }
    else if(! zombie.arm_lost && zombie.health <= arm_loss_health(zombie.type))
    {
        zombie.arm_lost = true;
        spawn_zombie_effect(zombie, zombie.type == ZombieType::newspaper ?
                           ZombieEffectType::newspaper_arm : ((zombie.type == ZombieType::football || zombie.type == ZombieType::vaulter) ? ZombieEffectType::football_arm : ZombieEffectType::arm));
    }
}

void Game::damage(Tombstone& tombstone, int amount)
{
    tombstone.health -= amount;
    if(tombstone.health <= 0) { tombstone.health = 0; tombstone.crumble_frames = 30; }
    tombstone.flash = 6;
    _events.hit = true;
}

void Game::explode(int row, int column)
{
    _events.explosion = true;
    for(auto& tombstone : _tombstones)
    {
        if(tombstone.active() && absolute(tombstone.row - row) <= 1 &&
           absolute(cell_x(tombstone.column) - cell_x(column)) <= 37)
        { damage(tombstone, 100); }
    }
    for(auto& zombie : _zombies)
    {
        if(zombie.active && absolute(zombie.row - row) <= 1 &&
           absolute(zombie.x - cell_x(column) * scale) <= 37 * scale)
        {
            damage(zombie, 100, DamageType::blast);
        }
    }
}

void Game::update_plants()
{
    for(int row = 0; row < rows; ++row)
    {
        for(int column = 0; column < columns; ++column)
        {
            auto& plant = _plants[cell_index(row, column)];
            if(! plant.active) { continue; }
            if(plant.flash > 0) { --plant.flash; }
            if(plant.shot_frames > 0) { --plant.shot_frames; }
            if(plant.exploding()) { plant.advance_explosion(); continue; }
            if(plant.type == PlantType::squash) { update_squash(plant, row, column); continue; }
            if(plant.type == PlantType::chomper) { update_chomper(plant, row, column); continue; }
            if(plant.burst_frames > 0 && --plant.burst_frames == 0)
            {
                if(shoot(row, (cell_x(column) + 8) * scale, false)) { plant.shot_frames = 6; }
                else { plant.burst_frames = 1; }
            }
            if(plant.timer > 0) { --plant.timer; }
            if(plant.timer > 0) { continue; }
            const int x = cell_x(column) * scale;
            switch(plant.type)
            {
            case PlantType::sunflower:
                if(spawn_sun(x, (cell_y(row) - 4) * scale, (cell_y(row) + 5) * scale))
                { plant.timer = spec(plant.type).action_delay; }
                break;
            case PlantType::peashooter:
            case PlantType::snow_pea:
            case PlantType::repeater:
                if(lane_has_target(row, x) && shoot(row, x + 8 * scale, plant.type == PlantType::snow_pea))
                { plant.timer = spec(plant.type).action_delay; plant.shot_frames = 6; if(plant.type == PlantType::repeater) { plant.burst_frames = 8; } }
                break;
            case PlantType::cherry:
                // Keep an explosion sprite alive for 18 frames, then remove it.
                explode(row, column); plant.health = 0; plant.timer = 18;
                break;
            case PlantType::potato_mine:
                for(auto& zombie : _zombies)
                {
                    if(zombie.active && zombie.vault_frames == 0 && zombie.row == row && absolute(zombie.x - x) <= 8 * scale)
                    {
                        _events.explosion = true;
                        for(auto& nearby : _zombies)
                        {
                            if(nearby.active && nearby.row == row && absolute(nearby.x - x) <= 18 * scale)
                            { damage(nearby, 100, DamageType::blast); }
                        }
                        for(auto& tombstone : _tombstones)
                        {
                            if(tombstone.active() && tombstone.row == row &&
                               absolute(cell_x(tombstone.column) * scale - x) <= 18 * scale)
                            { damage(tombstone, 100); }
                        }
                        plant.health = 0;
                        plant.timer = 18;
                        break;
                    }
                }
                break;
            default: break;
            }
        }
    }
}

void Game::update_chomper(Plant& plant, int row, int column)
{
    if(plant.timer > 0) { --plant.timer; }
    if(plant.bite_frames > 0)
    {
        if(--plant.bite_frames == 12 && plant.bite_target >= 0)
        {
            auto& target = _zombies[plant.bite_target];
            int distance = target.x - cell_x(column) * scale;
            if(target.active && target.row == row && target.vault_frames == 0 && target.emergence_frames == 0 &&
               distance >= -8 * scale && distance <= 48 * scale)
            {
                damage(target, 100, DamageType::swallow);
                plant.timer = spec(PlantType::chomper).action_delay;
                _events.chomp = true;
            }
            plant.bite_target = -1;
        }
        return;
    }
    if(plant.timer > 0) { return; }
    // Reach an enemy biting the plant in the immediately adjacent cell.
    int closest = 40 * scale;
    for(int i = 0; i < zombie_capacity; ++i)
    {
        const auto& zombie = _zombies[i];
        int distance = zombie.x - cell_x(column) * scale;
        if(zombie.active && zombie.row == row && zombie.vault_frames == 0 && zombie.emergence_frames == 0 &&
           distance >= -5 * scale && distance < closest)
        { closest = distance; plant.bite_target = i; }
    }
    if(plant.bite_target >= 0) { plant.bite_frames = 36; }
}

void Game::update_squash(Plant& plant, int row, int column)
{
    if(plant.target_x == 0)
    {
        int closest = 31 * scale;
        for(const auto& zombie : _zombies)
        {
            int distance = absolute(zombie.x - cell_x(column) * scale);
            if(zombie.active && zombie.row == row && zombie.vault_frames == 0 && distance < closest)
            { closest = distance; plant.target_x = zombie.x; }
        }
        if(plant.target_x != 0) { plant.timer = 30; }
        return;
    }
    if(--plant.timer > 0) { return; }
    for(auto& zombie : _zombies)
    {
        if(zombie.active && zombie.row == row && zombie.vault_frames == 0 &&
           absolute(zombie.x - plant.target_x) <= 20 * scale) { damage(zombie, 100, DamageType::crush); }
    }
    for(auto& stone : _tombstones)
    {
        if(stone.active() && stone.row == row && absolute(cell_x(stone.column) * scale - plant.target_x) <= 20 * scale)
        { damage(stone, 100); }
    }
    _events.explosion = true;
    plant.health = 0;
    plant.timer = 18;
}

void Game::update_shots()
{
    for(auto& shot : _shots)
    {
        if(! shot.active) { continue; }
        const int previous_x = shot.x;
        shot.x += 640;
        Zombie* closest = nullptr;
        for(auto& zombie : _zombies)
        {
            if(zombie.active && zombie.vault_frames == 0 && zombie.row == shot.row && shot_reaches(zombie.x, previous_x, shot.x) &&
               (! closest || zombie.x < closest->x))
            { closest = &zombie; }
        }
        Tombstone* obstacle = nullptr;
        for(auto& tombstone : _tombstones)
        {
            int x = cell_x(tombstone.column) * scale;
            if(tombstone.active() && tombstone.row == shot.row && shot_reaches(x, previous_x, shot.x) &&
               (! obstacle || x < cell_x(obstacle->column) * scale))
            { obstacle = &tombstone; }
        }
        if(obstacle && (! closest || cell_x(obstacle->column) * scale <= closest->x))
        { damage(*obstacle, 1); shot.active = false; }
        else if(closest) { damage(*closest, 1, shot.frozen ? DamageType::frost : DamageType::projectile); shot.active = false; }
        else if(shot.x > 245 * scale) { shot.active = false; }
    }
}

void Game::update_mowers()
{
    for(int row = 0; row < rows; ++row)
    {
        auto& mower = _mowers[row];
        if(! mower.moving) { continue; }
        mower.advance();
        for(auto& tombstone : _tombstones)
        {
            if(tombstone.active() && tombstone.row == row && cell_x(tombstone.column) * scale <= mower.x + 12 * scale)
            { damage(tombstone, 100); }
        }
        for(auto& zombie : _zombies)
        {
            if(zombie.active && zombie.row == row && zombie.x <= mower.x + 12 * scale)
            { damage(zombie, 100, DamageType::crush); }
        }
    }
}

bool Game::flag_buffed(const Zombie& zombie) const
{
    if(! zombie.active || zombie.type == ZombieType::flag) { return false; }
    constexpr int radius = 80 * scale;
    for(const auto& leader : _zombies)
    {
        if(! leader.active || leader.type != ZombieType::flag) { continue; }
        int dx = absolute(leader.x - zombie.x);
        int dy = absolute(leader.row - zombie.row) * cell_height * scale;
        if(dx <= radius && dy <= radius && dx * dx + dy * dy <= radius * radius)
        { return true; }
    }
    return false;
}

int Game::zombie_speed(const Zombie& zombie) const
{
    int speed = movement_speed(zombie);
    return flag_buffed(zombie) ? speed * 9 / 8 : speed;
}

void Game::update_zombies()
{
    for(int i = 0; i < zombie_capacity; ++i)
    {
        auto& zombie = _zombies[i];
        if(! zombie.active) { continue; }
        if(zombie.flash > 0) { --zombie.flash; }
        if(zombie.emergence_frames > 0)
        {
            --zombie.emergence_frames;
            zombie.eating = false;
            continue;
        }
        if(zombie.vault_frames > 0)
        {
            if(zombie.frost_frames == 0 || _frame % 2 == 0)
            {
                --zombie.vault_frames;
                zombie.x = zombie.vault_origin + (zombie.vault_target - zombie.vault_origin) * (36 - zombie.vault_frames) / 36;
            }
            zombie.eating = false;
            continue;
        }
        Plant* target = nullptr;
        for(int column = columns - 1; column >= 0; --column)
        {
            auto& plant = _plants[cell_index(zombie.row, column)];
            int distance = zombie.x - cell_x(column) * scale;
            if(zombie.type == ZombieType::vaulter && ! zombie.pole_used && plant.active &&
               plant.health > 0 && ! plant.committed() && plant.type != PlantType::tallnut && distance > 0 && distance <= 26 * scale)
            {
                zombie.pole_used = true;
                zombie.vault_frames = 36;
                zombie.vault_origin = zombie.x;
                zombie.vault_target = (cell_x(column) - 22) * scale;
                spawn_zombie_effect(zombie, ZombieEffectType::pole);
                break;
            }
            if(plant.active && ! plant.committed() && plant.type != PlantType::cherry && plant.health > 0 &&
               ! (plant.type == PlantType::potato_mine && plant.timer == 0) &&
               distance >= -5 * scale && distance <= 15 * scale)
            { target = &plant; break; }
        }
        if(zombie.vault_frames > 0) { zombie.eating = false; continue; }
        zombie.eating = target != nullptr;
        if(target)
        {
            if(--zombie.bite_timer <= 0)
            {
                --target->health;
                _events.bite = true;
                target->flash = 8;
                zombie.bite_timer = bite_delay(zombie);
                if(target->health <= 0) { target->active = false; }
            }
        }
        else
        {
            zombie.x -= zombie_speed(zombie);
            zombie.bite_timer = bite_delay(zombie);
        }
        if(zombie.type == ZombieType::flag && zombie.x < flag_entry_x * scale) { enter_horde(zombie); }
        auto& mower = _mowers[zombie.row];
        if(zombie.x < 29 * scale && mower.available)
        {
            mower.available = false;
            mower.moving = true;
            _events.mower = true;
            damage(zombie, 100, DamageType::crush);
        }
        else if(zombie.x < 8 * scale)
        {
            _breached_zombie = i;
            _phase = Phase::lost;
            return;
        }
    }
}
}
