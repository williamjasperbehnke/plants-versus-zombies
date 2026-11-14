#ifndef GARDEN_GAME_H
#define GARDEN_GAME_H
#include "reward_animation.h"
#include "defeat_audio.h"
#include "seed_layout.h"

#include <array>
#include "progress.h"
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
inline constexpr int sky_sun_interval = 600;
inline constexpr int frost_duration = 180;
inline constexpr int visible_seed_count = seed_layout::slots;

struct LevelSpec
{
    const char* name;
    int waves;
    int wave_base;
    int spawn_delay;
    int cone_every;
    int newspaper_every;
    int newspaper_first_wave;
    bool bucketheads = false;
    bool screen_doors = false;
    constexpr int normal_size(int wave) const { return wave > 0 ? wave + 2 : 0; }
    constexpr int wave_size(int wave) const
    {
        int size = wave > 0 ? wave_base + wave * 2 : 0;
        return wave == waves ? size + (size + 1) / 2 : size;
    }
};
inline constexpr std::array<LevelSpec, level_count> levels{{
    {"Backyard 1-1", 3, 3, 96, 0, 0, 0},
    {"Backyard 1-2", 3, 3, 96, 4, 0, 0},
    {"Backyard 1-3", 3, 5, 96, 3, 0, 0, true, false},
    {"Backyard 1-4", 5, 3, 96, 3, 5, 3, true, false},
    {"Backyard 1-5", 5, 3, 96, 3, 5, 2, true, true},
    {"Backyard 1-6", 5, 4, 96, 3, 5, 1, true, true},
    {"Backyard 1-7", 5, 4, 96, 3, 4, 1, true, true},
    {"Backyard 1-8", 5, 5, 96, 3, 4, 1, true, true},
    {"Backyard 1-9", 5, 5, 96, 3, 4, 1, true, true},
    {"Backyard 1-10", 5, 6, 96, 3, 4, 1, true, true}
}};

constexpr int cell_x(int column) { return lawn_left + column * cell_width + cell_width / 2; }
constexpr int cell_y(int row) { return lawn_top + row * cell_height + cell_height / 2; }
constexpr int cell_index(int row, int column) { return row * columns + column; }

enum class PlantType : uint8_t { sunflower, peashooter, walnut, cherry, potato_mine, snow_pea, count };
inline constexpr int plant_count = static_cast<int>(PlantType::count);
inline constexpr std::array<PlantType, plant_count> seed_order{{
    PlantType::sunflower, PlantType::peashooter, PlantType::walnut,
    PlantType::potato_mine, PlantType::snow_pea, PlantType::cherry
}};
constexpr int seed_slot(PlantType type)
{
    for(int i = 0; i < plant_count; ++i) { if(seed_order[i] == type) { return i; } }
    return plant_count;
}
enum class Phase : uint8_t { title, playing, paused, won, lost, level_select, preview, almanac };
enum class PlantResult : uint8_t { planted, occupied, insufficient_sun, cooling_down, inactive };

struct PlantSpec { int cost; int health; int recharge; int action_delay; };
inline constexpr std::array<PlantSpec, plant_count> plant_specs{{
    {50, 5, 240, 540}, {100, 5, 180, 90}, {50, 45, 600, 0}, {150, 1, 1200, 65},
    {25, 3, 900, 480}, {175, 5, 450, 90}
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
enum class ZombieType : uint8_t { normal, cone, newspaper, flag, bucket, screen_door };
inline constexpr int zombie_type_count = zombie_kind_count;
constexpr bool appears_in_level(const LevelSpec& level, ZombieType type)
{
    switch(type)
    {
    case ZombieType::normal:
    case ZombieType::flag: return true;
    case ZombieType::cone: return level.cone_every > 0;
    case ZombieType::newspaper: return level.newspaper_every > 0 && level.newspaper_first_wave <= level.waves;
    case ZombieType::bucket: return level.bucketheads;
    case ZombieType::screen_door: return level.screen_doors;
    default: return false;
    }
}
// Assign types to planned group slots, independent of when overlapping groups release them.
constexpr ZombieType scheduled_zombie(const LevelSpec& level, int wave, bool horde, int slot)
{
    if(! horde) { return ZombieType::normal; }
    if(slot == 0) { return ZombieType::flag; }
    int ordinal = level.normal_size(wave) + slot;
    for(int previous = 1; previous < wave; ++previous)
    { ordinal += level.normal_size(previous) + level.wave_size(previous); }
    if(level.screen_doors && ordinal % 7 == 6) { return ZombieType::screen_door; }
    if(level.bucketheads && ordinal % 6 == 4) { return ZombieType::bucket; }
    if(level.cone_every && ordinal % level.cone_every == 0) { return ZombieType::cone; }
    if(level.newspaper_every && wave >= level.newspaper_first_wave && ordinal % level.newspaper_every == 0)
    { return ZombieType::newspaper; }
    return ZombieType::normal;
}
inline constexpr auto level_rosters = [] {
    std::array<std::array<int, zombie_type_count>, level_count> rosters{};
    for(int level = 0; level < level_count; ++level)
    {
        const auto& definition = levels[level];
        for(int wave = 1; wave <= definition.waves; ++wave)
        {
            rosters[level][static_cast<int>(ZombieType::normal)] += definition.normal_size(wave);
            for(int slot = 0; slot < definition.wave_size(wave); ++slot)
            { ++rosters[level][static_cast<int>(scheduled_zombie(definition, wave, true, slot))]; }
        }
    }
    return rosters;
}();
inline constexpr int preview_pan_frames = 60;
inline constexpr int preview_ready_frame = 75;
enum class DamageType : uint8_t { projectile, frost, blast, crush };
struct ZombieSpec { int health; int speed; int shield_health = 0; };
inline constexpr std::array<ZombieSpec, 6> zombie_specs{{{6, 32}, {12, 28}, {6, 96, 16}, {6, 36}, {6, 26, 30}, {6, 28, 18}}};
constexpr const ZombieSpec& spec(ZombieType type) { return zombie_specs[static_cast<int>(type)]; }

struct Zombie
{
    bool active = false;
    ZombieType type = ZombieType::normal;
    int row = 0;
    int x = 0;
    int health = 0;
    int bite_timer = 0;
    int flash = 0;
    bool eating = false;
    int shield_health = 0;
    int group = 0;
    int frost_frames = 0;
};
inline int movement_speed(const Zombie& zombie)
{
    int speed = zombie.type == ZombieType::newspaper && zombie.shield_health > 0 ?
                spec(ZombieType::normal).speed : spec(zombie.type).speed;
    return zombie.frost_frames > 0 ? speed / 2 : speed;
}

inline void advance_zombie_status(Zombie& zombie)
{
    if(zombie.frost_frames > 0 && --zombie.frost_frames == 0)
    { zombie.bite_timer = (zombie.bite_timer + 1) / 2; }
}

inline bool take_zombie_damage(Zombie& zombie, int amount, DamageType type)
{
    bool frost_blocked = zombie.type == ZombieType::screen_door && zombie.shield_health > 0;
    bool bypass = type == DamageType::crush ||
                  (zombie.type == ZombieType::screen_door && type == DamageType::blast);
    if(zombie.shield_health > 0 && ! bypass)
    {
        int absorbed = amount < zombie.shield_health ? amount : zombie.shield_health;
        zombie.shield_health -= absorbed;
        amount -= absorbed;
    }
    zombie.health -= amount;
    if(type == DamageType::frost && ! frost_blocked && zombie.health > 0)
    {
        if(zombie.frost_frames == 0) { zombie.bite_timer *= 2; }
        zombie.frost_frames = frost_duration;
    }
    return zombie.health <= 0;
}

struct Shot { bool active = false; int row = 0; int x = 0; bool frozen = false; };
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
struct Mower { bool available = true; bool moving = false; int x = 21 * scale; };

// Transient cues from one simulation update; the audio/UI decide how to use them.
struct Events
{
    bool shot = false;
    bool hit = false;
    bool bite = false;
    bool explosion = false;
    bool mower = false;
    bool wave_started = false;
    bool horde_started = false;
    bool final_wave_approaching = false;
    bool wave_cleared = false;
};

class Game
{
public:
    bool start(int level = -1);
    bool prepare_level();
    void begin_level();
    void open_almanac();
    void close_almanac();
    int preview_frames() const { return _preview_frames; }
    bool preview_returning() const { return _preview_returning; }
    bool preview_ready() const { return ! _preview_returning && _preview_frames >= preview_ready_frame; }
    void load_progress(const Progress& progress);
    const Progress& progress() const { return _progress; }
    void open_level_select();
    void return_to_title();
    void select_level(int delta);
    void tick();
    void toggle_pause();
    PlantResult plant(int row, int column, PlantType type);
    bool shovel(int row, int column);
    int collect_sun();

    Phase phase() const { return _phase; }
    int sun() const { return _sun; }
    int wave() const { return _wave; }
    int level() const { return _level; }
    int unlocked_level() const { return _progress.unlocked - 1; }
    bool level_unlocked(int level) const { return level >= 0 && level < _progress.unlocked; }
    bool level_completed(int level) const { return level >= 0 && level < level_count && (_progress.completed & (uint32_t(1) << level)) != 0; }
    int world() const { return _level / levels_per_world; }
    const WorldSpec& world_spec() const { return worlds[world()]; }
    const LevelSpec& level_spec() const { return levels[_level]; }
    bool flag_buffed(const Zombie& zombie) const;
    int zombie_speed(const Zombie& zombie) const;
    int bite_delay(const Zombie& zombie) const
    { return (flag_buffed(zombie) ? 35 : 40) * (zombie.frost_frames > 0 ? 2 : 1); }
    int total_waves() const { return level_spec().waves; }
    const auto& zombie_roster() const { return level_rosters[_level]; }
    int available_plants() const { return _level + 2 < plant_count ? _level + 2 : plant_count; }
    bool plant_available(PlantType type) const { return seed_slot(type) < available_plants(); }
    PlantType unlocked_plant() const { return _unlocked_plant; }
    int reward_frames() const { return _reward_frames; }
    bool reward_ready() const { return _reward_frames >= reward_ready_frame; }
    int loss_frames() const { return _loss_frames; }
    bool loss_ready() const { return _loss_frames >= defeat_audio::ready_frame; }
    int breached_zombie() const { return _breached_zombie; }
    bool has_next_level() const { return _level + 1 < level_count; }
    int elapsed() const { return _frame; }
    int kills() const { return _kills; }
    uint32_t lifetime_kills(ZombieType type) const { return _progress.zombie_kills[static_cast<int>(type)]; }
    bool horde_active() const { return _horde && (_remaining > 0 || active_group_zombies() > 0); }
    int current_group() const { return _group; }
    bool horde_remaining(int wave) const;
    bool horde_entered(int wave) const { return wave > 0 && wave <= total_waves() && _hordes_entered[wave - 1]; }
    bool final_wave_approaching() const { return _wave == total_waves() && ! horde_entered(_wave); }
    int next_wave_frames() const { return _wave == 0 ? _preparation : 0; }
    int enemies_left() const;
    // Each normal stretch spans 100 units; its horde holds the indicator at the flag.
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
    struct PendingGroup
    {
        int remaining = 0;
        int timer = 0;
        int group = 0;
        int wave = 0;
        bool horde = false;
    };
    bool spawn_zombie(int wave, bool horde, int remaining, int group);
    void retain_group_spawns();
    bool pending_spawns() const;
    int spawn_delay(int wave, bool horde) const;
    bool spawn_sun(int x, int y, int target_y);
    bool shoot(int row, int x, bool frozen);
    void damage(Zombie& zombie, int amount, DamageType type);
    void explode(int row, int column);
    bool lane_has_target(int row, int x) const;
    bool any_zombies() const;
    int active_group_zombies() const;
    uint32_t random();

    Phase _phase = Phase::title;
    int _level = 0;
    Progress _progress;
    int _sun = 175;
    int _frame = 0;
    int _wave = 0;
    int _remaining = 0;
    int _spawn_timer = 0;
    int _preparation = 900;
    int _sky_timer = 180;
    int _kills = 0;
    int _spawn_number = 0;
    int _group = 0;
    std::array<bool, wave_count> _hordes_entered{};
    std::array<PendingGroup, wave_count * 2> _pending_groups{};
    bool _horde = false;
    PlantType _unlocked_plant = PlantType::count;
    int _preview_frames = 0;
    bool _preview_returning = false;
    int _reward_frames = 0;
    int _loss_frames = 0;
    int _breached_zombie = -1;
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
