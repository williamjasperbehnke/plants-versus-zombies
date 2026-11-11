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

void Game::start()
{
    *this = Game();
    _phase = Phase::playing;
}

void Game::toggle_pause()
{
    if(_phase == Phase::playing) { _phase = Phase::paused; }
    else if(_phase == Phase::paused) { _phase = Phase::playing; }
}

PlantResult Game::plant(int row, int column, PlantType type)
{
    if(_phase != Phase::playing || ! valid_cell(row, column) || type >= PlantType::count)
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
    if(_phase != Phase::playing) { return; }
    ++_frame;
    if(_wave_warning > 0) { --_wave_warning; }
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
    for(const auto& zombie : _zombies) { if(zombie.active) { ++count; } }
    return count;
}

int Game::wave_progress() const
{
    if(_wave == 0) { return 0; }
    return (_wave - 1) * 100 + (wave_size(_wave) - enemies_left()) * 100 / wave_size(_wave);
}

void Game::update_waves()
{
    if(_remaining == 0 && ! any_zombies())
    {
        if(_wave > _reported_clear)
        {
            _reported_clear = _wave;
            _events.wave_cleared = true;
        }
        if(_wave == wave_count) { _phase = Phase::won; return; }
        if(--_intermission <= 0)
        {
            ++_wave;
            _remaining = wave_size(_wave);
            _events.wave_started = true;
            _spawn_timer = 1;
            _wave_warning = 150;
        }
    }
    if(_remaining > 0 && --_spawn_timer <= 0)
    {
        if(spawn_zombie())
        {
            --_remaining;
            _spawn_timer = 250 - _wave * 24;
            if(_remaining == 0) { _intermission = 300; }
        }
    }
}

bool Game::spawn_zombie()
{
    auto* zombie = free_slot(_zombies);
    if(! zombie) { return false; }
    // Each group of five visits every lane, with a changing offset per wave.
    const int row = (_spawn_number * 3 + _wave) % rows;
    const bool armored = _wave >= 2 && (_spawn_number % 3 == 0);
    *zombie = {true, armored, row, 240 * scale, armored ? 12 : 6, 40, 0};
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
        _sky_timer = 360;
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

bool Game::shoot(int row, int x)
{
    auto* shot = free_slot(_shots);
    if(! shot) { return false; }
    *shot = {true, row, x};
    _events.shot = true;
    return true;
}

void Game::damage(Zombie& zombie, int amount)
{
    zombie.health -= amount;
    _events.hit = true;
    zombie.flash = 6;
    if(zombie.health <= 0) { zombie.active = false; ++_kills; }
}

void Game::explode(int row, int column)
{
    _events.explosion = true;
    for(auto& zombie : _zombies)
    {
        if(zombie.active && absolute(zombie.row - row) <= 1 &&
           absolute(zombie.x - cell_x(column) * scale) <= 37 * scale)
        {
            damage(zombie, 100);
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
                if(lane_has_target(row, x) && shoot(row, x + 8 * scale))
                { plant.timer = spec(plant.type).action_delay; }
                break;
            case PlantType::cherry:
                // Keep an explosion sprite alive for 18 frames, then remove it.
                if(plant.health > 0) { explode(row, column); plant.health = 0; plant.timer = 18; }
                else { plant.active = false; }
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
        if(closest) { damage(*closest, 1); shot.active = false; }
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
            { damage(zombie, 100); }
        }
        if(mower.x > 252 * scale) { mower.moving = false; }
    }
}

void Game::update_zombies()
{
    for(auto& zombie : _zombies)
    {
        if(! zombie.active) { continue; }
        if(zombie.flash > 0) { --zombie.flash; }
        Plant* target = nullptr;
        for(int column = columns - 1; column >= 0; --column)
        {
            auto& plant = _plants[cell_index(zombie.row, column)];
            int distance = zombie.x - cell_x(column) * scale;
            if(plant.active && plant.type != PlantType::cherry && plant.health > 0 &&
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
                zombie.bite_timer = 40;
                if(target->health <= 0) { target->active = false; }
            }
        }
        else
        {
            zombie.x -= zombie.armored ? 28 : 32;
            zombie.bite_timer = 40;
        }
        auto& mower = _mowers[zombie.row];
        if(zombie.x < 29 * scale && mower.available)
        {
            mower.available = false;
            mower.moving = true;
            _events.mower = true;
            damage(zombie, 100);
        }
        else if(zombie.x < 8 * scale) { _phase = Phase::lost; }
    }
}
}
