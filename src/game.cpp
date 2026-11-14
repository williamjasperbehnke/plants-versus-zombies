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
    return true;
}

bool Game::prepare_level()
{
    if(! start()) { return false; }
    _phase = Phase::preview;
    return true;
}

void Game::begin_level()
{
    if(_phase != Phase::preview || ! preview_ready()) { return; }
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
    if(_phase != Phase::playing || ! valid_cell(row, column) ||
       ! plant_available(type))
    {
        return PlantResult::inactive;
    }
    auto& target = _plants[cell_index(row, column)];
    if(target.active) { return PlantResult::occupied; }
    if(cooldown(type)) { return PlantResult::cooling_down; }
    const auto& definition = spec(type);
    if(_sun < definition.cost) { return PlantResult::insufficient_sun; }
    target = {true, type, definition.health, definition.action_delay, 0};
    _sun -= definition.cost;
    _cooldowns[static_cast<int>(type)] = definition.recharge;
    return PlantResult::planted;
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
    { if(_reward_frames < reward_ready_frame) { ++_reward_frames; } return; }
    if(_phase == Phase::lost)
    { if(_loss_frames < defeat_audio::ready_frame) { ++_loss_frames; } return; }
    if(_phase != Phase::playing) { return; }
    ++_frame;
    for(auto& zombie : _zombies) { if(zombie.active) { advance_zombie_status(zombie); } }
    for(auto& cooldown : _cooldowns) { if(cooldown > 0) { --cooldown; } }
    update_suns();
    update_waves();
    if(_phase != Phase::playing) { return; }
    update_plants();
    update_shots();
    update_mowers();
    update_zombies();
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
            if(! level_completed(_level) && available_plants() < plant_count)
            { _unlocked_plant = seed_order[available_plants()]; }
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
}

bool Game::spawn_zombie(int wave, bool horde, int remaining, int group)
{
    auto* zombie = free_slot(_zombies);
    if(! zombie) { return false; }
    const int row = (_spawn_number * 3 + wave) % rows;
    const auto& definition = level_spec();
    int size = horde ? definition.wave_size(wave) : definition.normal_size(wave);
    ZombieType type = scheduled_zombie(definition, wave, horde, size - remaining);
    *zombie = {true, type, row, 240 * scale, spec(type).health, 40, 0};
    zombie->shield_health = spec(type).shield_health;
    zombie->group = group;
    if(type == ZombieType::flag)
    {
        _hordes_entered[wave - 1] = true;
        _events.horde_started = true;
    }
    ++_spawn_number;
    return true;
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
                  cell_y(int(random() % rows)) * scale);
        _sky_timer = sky_sun_interval;
    }
}

bool Game::lane_has_target(int row, int x) const
{
    for(const auto& zombie : _zombies)
    {
        if(zombie.active && zombie.row == row && zombie.x > x) { return true; }
    }
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

void Game::damage(Zombie& zombie, int amount, DamageType type)
{
    bool killed = take_zombie_damage(zombie, amount, type);
    _events.hit = true;
    zombie.flash = 6;
    if(killed)
    {
        zombie.active = false;
        ++_kills;
        auto& total = _progress.zombie_kills[static_cast<int>(zombie.type)];
        if(total < UINT32_MAX) { ++total; }
    }
}

void Game::explode(int row, int column)
{
    _events.explosion = true;
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
                if(lane_has_target(row, x) && shoot(row, x + 8 * scale, plant.type == PlantType::snow_pea))
                { plant.timer = spec(plant.type).action_delay; }
                break;
            case PlantType::cherry:
                // Keep an explosion sprite alive for 18 frames, then remove it.
                if(plant.health > 0) { explode(row, column); plant.health = 0; plant.timer = 18; }
                else { plant.active = false; }
                break;
            case PlantType::potato_mine:
                if(plant.health == 0) { plant.active = false; break; }
                for(auto& zombie : _zombies)
                {
                    if(zombie.active && zombie.row == row && absolute(zombie.x - x) <= 8 * scale)
                    {
                        _events.explosion = true;
                        for(auto& nearby : _zombies)
                        {
                            if(nearby.active && nearby.row == row && absolute(nearby.x - x) <= 18 * scale)
                            { damage(nearby, 100, DamageType::blast); }
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
            if(zombie.active && zombie.row == shot.row && zombie.x + 7 * scale >= previous_x &&
               zombie.x - 7 * scale <= shot.x && (! closest || zombie.x < closest->x))
            { closest = &zombie; }
        }
        if(closest) { damage(*closest, 1, shot.frozen ? DamageType::frost : DamageType::projectile); shot.active = false; }
        else if(shot.x > 245 * scale) { shot.active = false; }
    }
}

void Game::update_mowers()
{
    for(int row = 0; row < rows; ++row)
    {
        auto& mower = _mowers[row];
        if(! mower.moving) { continue; }
        mower.x += 3 * scale;
        for(auto& zombie : _zombies)
        {
            if(zombie.active && zombie.row == row && zombie.x <= mower.x + 12 * scale)
            { damage(zombie, 100, DamageType::crush); }
        }
        if(mower.x > 252 * scale) { mower.moving = false; }
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
        Plant* target = nullptr;
        for(int column = columns - 1; column >= 0; --column)
        {
            auto& plant = _plants[cell_index(zombie.row, column)];
            int distance = zombie.x - cell_x(column) * scale;
            if(plant.active && plant.type != PlantType::cherry && plant.health > 0 &&
               ! (plant.type == PlantType::potato_mine && plant.timer == 0) &&
               distance >= -5 * scale && distance <= 15 * scale)
            { target = &plant; break; }
        }
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
