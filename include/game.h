#ifndef GARDEN_GAME_H
#define GARDEN_GAME_H

#include <array>
#include <cstdint>

namespace garden
{
// Simulation coordinates use 1/256 pixel units, independently of the renderer.
inline constexpr int scale = 256;
inline constexpr int rows = 5;
inline constexpr int columns = 9;
inline constexpr int cell_width = 22;
inline constexpr int cell_height = 19;
inline constexpr int lawn_left = 34;
inline constexpr int lawn_top = 58;
inline constexpr int zombie_capacity = 16;
inline constexpr int shot_capacity = 18;
inline constexpr int sun_capacity = 10;
inline constexpr int sun_bank_x = 11;
inline constexpr int sun_bank_y = 32;
inline constexpr int sun_collection_frames = 24;
inline constexpr int wave_count = 5;
constexpr int wave_size(int wave) { return wave > 0 ? 3 + wave * 2 : 0; }

constexpr int cell_x(int column) { return lawn_left + column * cell_width + cell_width / 2; }
constexpr int cell_y(int row) { return lawn_top + row * cell_height + cell_height / 2; }
constexpr int cell_index(int row, int column) { return row * columns + column; }

enum class PlantType : uint8_t { sunflower, peashooter, walnut, cherry, count };
inline constexpr int plant_count = static_cast<int>(PlantType::count);
enum class Phase : uint8_t { title, playing, paused, won, lost };
enum class PlantResult : uint8_t { planted, occupied, insufficient_sun, cooling_down, inactive };

struct PlantSpec { int cost; int health; int recharge; int action_delay; };
inline constexpr std::array<PlantSpec, plant_count> plant_specs{{
    {50, 5, 240, 480}, {100, 5, 180, 90}, {50, 45, 600, 0}, {150, 1, 1200, 65}
}};
constexpr const PlantSpec& spec(PlantType type) { return plant_specs[static_cast<int>(type)]; }

struct Plant
{
    bool active = false;
    PlantType type = PlantType::sunflower;
    int health = 0;
    int timer = 0;
    int flash = 0;
};
struct Zombie
{
    bool active = false;
    bool armored = false;
    int row = 0;
    int x = 0;
    int health = 0;
    int bite_timer = 0;
    int flash = 0;
    bool eating = false;
};
struct Shot { bool active = false; int row = 0; int x = 0; };
struct Sun
{
    bool active = false;
    int x = 0;
    int y = 0;
    int target_y = 0;
    int lifetime = 0;
    bool collecting = false;
    int flight_frame = 0;
    int origin_x = 0;
    int origin_y = 0;
};
struct Mower { bool available = true; bool moving = false; int x = 20 * scale; };

// Transient cues from one simulation update; the audio/UI decide how to use them.
struct Events
{
    bool shot = false;
    bool hit = false;
    bool bite = false;
    bool explosion = false;
    bool mower = false;
    bool wave_started = false;
    bool wave_cleared = false;
};

class Game
{
public:
    void start();
    void tick();
    void toggle_pause();
    PlantResult plant(int row, int column, PlantType type);
    bool shovel(int row, int column);
    int collect_sun();

    Phase phase() const { return _phase; }
    int sun() const { return _sun; }
    int wave() const { return _wave; }
    int elapsed() const { return _frame; }
    int kills() const { return _kills; }
    int wave_warning() const { return _wave_warning; }
    int next_wave_frames() const { return _remaining == 0 && ! any_zombies() ? _intermission : 0; }
    int enemies_left() const;
    // Each wave occupies 100 units. Progress counts defeats, including unspawned enemies.
    int wave_progress() const;
    const Events& events() const { return _events; }
    int cooldown(PlantType type) const { return _cooldowns[static_cast<int>(type)]; }
    const auto& plants() const { return _plants; }
    const auto& zombies() const { return _zombies; }
    const auto& shots() const { return _shots; }
    const auto& suns() const { return _suns; }
    const auto& mowers() const { return _mowers; }

private:
    void update_waves();
    void update_plants();
    void update_shots();
    void update_zombies();
    void update_suns();
    void update_mowers();
    bool spawn_zombie();
    bool spawn_sun(int x, int y, int target_y);
    bool shoot(int row, int x);
    void damage(Zombie& zombie, int amount);
    void explode(int row, int column);
    bool lane_has_target(int row, int x) const;
    bool any_zombies() const;
    uint32_t random();

    Phase _phase = Phase::title;
    int _sun = 175;
    int _frame = 0;
    int _wave = 0;
    int _remaining = 0;
    int _spawn_timer = 0;
    int _intermission = 900;
    int _sky_timer = 180;
    int _wave_warning = 0;
    int _kills = 0;
    int _spawn_number = 0;
    int _reported_clear = 0;
    Events _events;
    uint32_t _random = 0x6d2b79f5;
    std::array<int, plant_count> _cooldowns{};
    std::array<Plant, rows * columns> _plants{};
    std::array<Zombie, zombie_capacity> _zombies{};
    std::array<Shot, shot_capacity> _shots{};
    std::array<Sun, sun_capacity> _suns{};
    std::array<Mower, rows> _mowers{};
};
}
#endif
