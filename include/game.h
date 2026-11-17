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
inline constexpr int road_spawn_x = 256;
inline constexpr int flag_entry_x = 253; // Leftmost flag-carrier pixels enter the 240-pixel screen here.
inline constexpr int tombstone_capacity = 3;
inline constexpr int tombstone_health = 6;
inline constexpr int emergence_duration = 48;
inline constexpr int emergence_stages = 6;
inline constexpr int zombie_effect_capacity = 6;
inline constexpr int shot_capacity = 18;
inline constexpr int sun_capacity = 10;
inline constexpr int sun_bank_x = 11;
inline constexpr int sun_bank_y = 32;
inline constexpr int sun_collection_frames = 24;
inline constexpr int wave_count = 5;
inline constexpr int sky_sun_interval = 600;
inline constexpr int sun_lifetime = 360;
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
    int tombstones = 0;
    int football_first_wave = 0;
    int vault_first_wave = 0;
    int lanes = rows;
    int wave_growth = 2;
    // Zero-growth teaching levels keep the ordinary groups to a single zombie.
    constexpr int normal_size(int wave) const { return wave > 0 ? (wave_growth ? wave + wave_growth : 1) : 0; }
    constexpr int wave_size(int wave) const
    {
        int size = wave > 0 ? wave_base + wave * wave_growth : 0;
        return wave == waves ? size + (size + 1) / 2 : size;
    }
};
// A counter is awarded on the preceding clear, before its matching enemy appears.
inline constexpr std::array<LevelSpec, level_count> levels{{
    {"Backyard 1-1", 3, 1, 120, 0, 0, 0, false, false, 0, 0, 0, 1, 0},
    {"Backyard 1-2", 3, 2, 110, 0, 0, 0, false, false, 0, 0, 0, 3},
    {"Backyard 1-3", 3, 3, 100, 4, 0, 0},
    {"Backyard 1-4", 3, 4, 100, 4, 0, 0, true},
    {"Backyard 1-5", 3, 5, 105, 3, 4, 2},
    {"Backyard 1-6", 5, 3, 104, 4, 0, 0, true, true, 1},
    {"Backyard 1-7", 5, 4, 100, 3, 4, 1, false, false, 1, 3},
    {"Backyard 1-8", 5, 5, 96, 4, 0, 0, true, false, 2, 0, 2},
    {"Backyard 1-9", 5, 5, 92, 0, 4, 1, false, true, 2, 3, 1},
    {"Backyard 1-10", 5, 6, 96, 3, 4, 1, true, true, 3, 1, 1}
}};

static_assert([] {
    for(const auto& level : levels)
    { if(level.tombstones < 0 || level.tombstones > tombstone_capacity || (level.lanes != 1 && level.lanes != 3 && level.lanes != rows)) { return false; } }
    return true;
}(), "Level tombstones exceed the fixed pool");

constexpr int cell_x(int column) { return lawn_left + column * cell_width + cell_width / 2; }
constexpr int cell_y(int row) { return lawn_top + row * cell_height + cell_height / 2; }
constexpr int cell_index(int row, int column) { return row * columns + column; }

struct LawnCell { int row; int column; };
// Ordered shovel targets, shared by level setup and tutorial guidance.
inline constexpr std::array<LawnCell, 3> tutorial_weeds{{{2, 2}, {1, 0}, {3, 4}}};

enum class PlantType : uint8_t { sunflower, peashooter, walnut, cherry, potato_mine, snow_pea, squash, repeater, tallnut, chomper, count };
inline constexpr int plant_count = static_cast<int>(PlantType::count);
inline constexpr std::array<PlantType, plant_count> seed_order{{
    PlantType::peashooter, PlantType::sunflower, PlantType::walnut,
    PlantType::potato_mine, PlantType::snow_pea, PlantType::cherry, PlantType::squash, PlantType::tallnut, PlantType::repeater, PlantType::chomper
}};
constexpr int seed_slot(PlantType type)
{
    for(int i = 0; i < plant_count; ++i) { if(seed_order[i] == type) { return i; } }
    return plant_count;
}
enum class Phase : uint8_t { title, playing, paused, won, lost, level_select, preview, almanac, seed_select, save_select };
enum class PlantResult : uint8_t { planted, occupied, insufficient_sun, cooling_down, inactive };

struct PlantSpec { int cost; int health; int recharge; int action_delay; };
inline constexpr std::array<PlantSpec, plant_count> plant_specs{{
    {50, 5, 240, 540}, {100, 5, 180, 90}, {50, 45, 600, 0}, {150, 1, 1200, 65},
    {25, 3, 900, 480}, {175, 5, 450, 90}, {50, 5, 1800, 0}, {200, 5, 180, 90}, {125, 90, 900, 0}, {150, 5, 450, 900}
}};
constexpr const PlantSpec& spec(PlantType type) { return plant_specs[static_cast<int>(type)]; }
static_assert(sun_lifetime < spec(PlantType::sunflower).action_delay);

struct Plant
{
    bool active = false;
    PlantType type = PlantType::sunflower;
    int health = 0;
    int timer = 0;
    int flash = 0;
    int burst_frames = 0;
    int target_x = 0;
    int bite_frames = 0;
    int bite_target = -1;
    int shot_frames = 0;
    bool wilted = false;
    bool committed() const { return active && type == PlantType::squash && target_x != 0; }
    bool exploding() const
    { return active && health == 0 && (type == PlantType::cherry || type == PlantType::potato_mine || type == PlantType::squash); }
    void advance_explosion()
    { if(exploding() && --timer <= 0) { active = false; } }
};
enum class ZombieType : uint8_t { normal, cone, newspaper, flag, bucket, screen_door, football, vaulter };
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
    case ZombieType::football: return level.football_first_wave > 0;
    case ZombieType::vaulter: return level.vault_first_wave > 0;
    default: return false;
    }
}
// Assign types to planned group slots, independent of when overlapping groups release them.
constexpr ZombieType scheduled_zombie(const LevelSpec& level, int wave, bool horde, int slot)
{
    if(! horde) { return ZombieType::normal; }
    if(slot == 0) { return ZombieType::flag; }
    if(slot == 1 && level.football_first_wave > 0 && wave >= level.football_first_wave) { return ZombieType::football; }
    if(slot == 2 && level.vault_first_wave > 0 && wave >= level.vault_first_wave) { return ZombieType::vaulter; }
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
enum class DamageType : uint8_t { projectile, frost, blast, crush, swallow };
struct ZombieSpec { int health; int speed; int shield_health = 0; };
inline constexpr std::array<ZombieSpec, zombie_type_count> zombie_specs{{{6, 32}, {12, 28}, {6, 96, 16}, {6, 36}, {6, 26, 30}, {6, 28, 18}, {10, 88, 40}, {8, 52}}};
constexpr const ZombieSpec& spec(ZombieType type) { return zombie_specs[static_cast<int>(type)]; }
constexpr int arm_loss_health(ZombieType type)
{ return (type == ZombieType::cone ? spec(ZombieType::normal).health : spec(type).health) / 2; }

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
    int emergence_frames = 0;
    bool arm_lost = false;
    bool pole_used = false;
    int vault_frames = 0;
    int vault_origin = 0;
    int vault_target = 0;
};
enum class ZombieEffectType : uint8_t { death, newspaper_death, arm, newspaper_arm, cone, newspaper, bucket, door, flag_death, flag, football_death, football_arm, football_helmet, pole, vault_death, count };
constexpr bool corpse_effect(ZombieEffectType type)
{ return type == ZombieEffectType::death || type == ZombieEffectType::newspaper_death || type == ZombieEffectType::flag_death || type == ZombieEffectType::football_death || type == ZombieEffectType::vault_death; }
constexpr int effect_duration(ZombieEffectType type)
{ return type == ZombieEffectType::flag_death ? 60 : (corpse_effect(type) ? 48 : 36); }
constexpr int effect_stages(ZombieEffectType type)
{ return type == ZombieEffectType::flag_death ? 10 : (corpse_effect(type) ? 8 : 6); }
struct ZombieEffect
{
    bool active = false;
    ZombieEffectType type = ZombieEffectType::death;
    int row = 0;
    int x = 0;
    int remaining = 0;
    bool wounded = false;
};

inline int movement_speed(const Zombie& zombie)
{
    if(zombie.emergence_frames > 0) { return 0; }
    int speed = zombie.type == ZombieType::newspaper && zombie.shield_health > 0 ?
                spec(ZombieType::normal).speed :
                (zombie.type == ZombieType::vaulter && zombie.pole_used ? 28 : spec(zombie.type).speed);
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
    bool bypass = type == DamageType::crush || type == DamageType::swallow ||
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

struct Tombstone
{
    int row = 0;
    int column = 0;
    int health = 0;
    int flash = 0;
    int crumble_frames = 0;
    int pending = 0; // Triggered spawns survive pool pressure, even if the stone is destroyed.
    bool active() const { return health > 0; }
};

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
    bool from_plant = false;
};
struct Mower
{
    bool available = true;
    bool moving = false;
    int x = 21 * scale;
    void advance()
    { if(moving) { x += 3 * scale; if(x > 252 * scale) { moving = false; } } }
};

// Transient cues from one simulation update; the audio/UI decide how to use them.
struct Events
{
    bool shot = false;
    bool chomp = false;
    bool hit = false;
    bool bite = false;
    bool explosion = false;
    bool mower = false;
    bool wave_started = false;
    bool horde_started = false;
    bool final_wave_approaching = false;
    bool wave_cleared = false;
    int tombstone_spawns = 0;
};

class Game
{
public:
    bool start(int level = -1);
    bool prepare_level();
    void begin_level();
    void choose_seeds();
    void cancel_seed_selection();
    bool toggle_seed(PlantType type);
    void open_almanac();
    void close_almanac();
    int preview_frames() const { return _preview_frames; }
    bool preview_returning() const { return _preview_returning; }
    bool preview_ready() const { return ! _preview_returning && _preview_frames >= preview_ready_frame; }
    void load_progress(const Progress& progress);
    const Progress& progress() const { return _progress; }
    void open_level_select();
    void return_to_title();
    void open_save_select();
    void select_level(int delta);
    void tick(bool hold_waves = false, bool hold_sky_sun = false);
    void toggle_pause();
    PlantResult plant(int row, int column, PlantType type);
    bool shovel(int row, int column);
    bool tombstone_at(int row, int column) const;
    int collect_sun(bool plants_only = false);
    void hasten_sun(int frames, bool from_plant);

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
    int unlocked_plants() const { return _level + 1 < plant_count ? _level + 1 : plant_count; }
    int first_row() const { return (rows - level_spec().lanes) / 2; }
    int last_row() const { return first_row() + level_spec().lanes - 1; }
    bool row_active(int row) const { return row >= first_row() && row <= last_row(); }
    int required_seeds() const { return unlocked_plants() < visible_seed_count ? unlocked_plants() : visible_seed_count; }
    int available_plants() const { return _seed_count; }
    bool plant_unlocked(PlantType type) const { return garden::seed_slot(type) < unlocked_plants(); }
    bool plant_available(PlantType type) const { return bank_slot(type) < _seed_count; }
    PlantType seed_at(int slot) const { return _seed_bank[slot]; }
    int bank_slot(PlantType type) const
    { for(int i = 0; i < _seed_count; ++i) { if(_seed_bank[i] == type) { return i; } } return _seed_count; }
    PlantType unlocked_plant() const { return _unlocked_plant; }
    int reward_frames() const { return _reward_frames; }
    bool victory_settling() const;
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
    const auto& tombstones() const { return _tombstones; }
    const auto& plants() const { return _plants; }
    const auto& zombies() const { return _zombies; }
    const auto& zombie_effects() const { return _zombie_effects; }
    const auto& shots() const { return _shots; }
    const auto& suns() const { return _suns; }
    const auto& mowers() const { return _mowers; }

private:
    void update_waves();
    void update_plants();
    void update_squash(Plant& plant, int row, int column);
    void update_chomper(Plant& plant, int row, int column);
    void update_shots();
    void update_zombies();
    void update_suns(bool hold_sky_sun);
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
    Zombie* spawn_zombie_at(ZombieType type, int row, int x, int group);
    void update_tombstone_spawns();
    void update_tombstones();
    void enter_horde(const Zombie& leader);
    void damage(Tombstone& tombstone, int amount);
    void retain_group_spawns();
    bool pending_spawns() const;
    int spawn_delay(int wave, bool horde) const;
    bool spawn_sun(int x, int y, int target_y, bool from_plant = false);
    bool shoot(int row, int x, bool frozen);
    void damage(Zombie& zombie, int amount, DamageType type);
    void spawn_zombie_effect(const Zombie& zombie, ZombieEffectType type);
    void update_zombie_effects();
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
    int _seed_count = 1;
    std::array<PlantType, visible_seed_count> _seed_bank{PlantType::peashooter};
    std::array<int, plant_count> _cooldowns{};
    std::array<Tombstone, tombstone_capacity> _tombstones{};
    std::array<Plant, rows * columns> _plants{};
    std::array<Zombie, zombie_capacity> _zombies{};
    std::array<ZombieEffect, zombie_effect_capacity> _zombie_effects{};
    std::array<Shot, shot_capacity> _shots{};
    std::array<Sun, sun_capacity> _suns{};
    std::array<Mower, rows> _mowers{};
};
}
#endif
