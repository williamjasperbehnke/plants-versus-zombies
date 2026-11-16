#include "game.h"
#include "almanac.h"
#include "menu_layout.h"
#include "wave_track.h"
#include "audio_settings.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace garden;

namespace
{
void almanac_page_navigation()
{
    for(int count : {plant_count, zombie_type_count})
    {
        for(int entry = 0; entry < count; ++entry)
        {
            for(auto direction : {std::array<int, 2>{-1, 0}, {1, 0}, {0, -1}, {0, 1}})
            {
                int next = menu_layout::almanac_move(entry, count, direction[0], direction[1]);
                assert(next >= 0 && next < count && next / 6 == entry / 6);
            }
        }
    }
    assert(menu_layout::almanac_move(5, plant_count, 1, 0) == 0);
    assert(menu_layout::almanac_move(0, plant_count, -1, 0) == 5);
    assert(menu_layout::almanac_move(6, plant_count, 0, 1) == 9);
    assert(menu_layout::almanac_move(7, plant_count, 0, 1) == 7);
    assert(menu_layout::almanac_move(6, zombie_type_count, 0, -1) == 6);
    std::puts("PASS Almanac D-pad stays on its current page, including incomplete final pages");
}

void scouting_and_almanac()
{
    Game game;
    const auto progress = game.progress();
    game.open_almanac();
    assert(game.phase() == Phase::almanac);
    for(int i = 0; i < 100; ++i) { game.tick(); }
    assert(game.elapsed() == 0 && game.progress() == progress);
    game.close_almanac();
    assert(game.phase() == Phase::title);
    game.open_level_select();
    game.select_level(1);
    assert(! game.prepare_level() && game.phase() == Phase::level_select);
    game.select_level(-1);
    assert(game.prepare_level() && game.phase() == Phase::preview);
    game.begin_level();
    assert(! game.preview_returning());
    assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::inactive);
    for(int i = 0; i < preview_ready_frame; ++i) { game.tick(); }
    assert(game.preview_ready() && game.elapsed() == 0 && game.sun() == 175);
    game.open_almanac();
    game.tick();
    assert(game.preview_frames() == preview_ready_frame && game.phase() == Phase::preview);
    game.close_almanac();
    assert(game.phase() == Phase::preview && game.preview_ready());
    game.choose_seeds();
    assert(game.phase() == Phase::preview && game.preview_returning());
    game.open_almanac();
    assert(game.phase() == Phase::preview);
    for(int i = 0; i < preview_pan_frames - 1; ++i) { game.tick(); }
    assert(game.phase() == Phase::preview && game.elapsed() == 0);
    game.tick();
    assert(game.phase() == Phase::playing && game.elapsed() == 0 && game.next_wave_frames() == 900);
    game.open_almanac();
    assert(game.phase() == Phase::playing);
    game.tick();
    assert(game.elapsed() == 1);
    for(int i = 0; i < zombie_type_count; ++i)
    {
        auto type = static_cast<ZombieType>(i);
        assert(appears_in_level(levels[0], type) == (type == ZombieType::normal || type == ZombieType::flag));
        assert(appears_in_level(levels[9], type));
        assert(zombie_entries[i].name[0] && zombie_display_frame(type, 0) >= 4);
        for(const auto* line : zombie_entries[i].description) { assert(std::strlen(line) <= 24); }
    }
    for(auto type : seed_order)
    {
        const auto& entry = plant_entries[static_cast<int>(type)];
        assert(entry.name[0]);
        for(const auto* line : entry.description) { assert(std::strlen(line) <= 24); }
    }
    game.open_level_select();
    game.open_almanac();
    game.close_almanac();
    assert(game.phase() == Phase::level_select);
    std::puts("PASS level-specific scouting, preview timing, no early simulation, title-only almanac and catalogue");
}

void advance(Game& game, int frames);

void new_plants_and_seed_bank()
{
    Game game;
    game.load_progress(Progress::all_unlocked());
    assert(game.start(9));
    assert(game.prepare_level());
    advance(game, preview_ready_frame);
    game.choose_seeds();
    assert(game.phase() == Phase::seed_select && game.required_seeds() == 6);
    assert(! game.toggle_seed(PlantType::repeater)); // Full bank cannot silently replace a choice.
    assert(game.toggle_seed(PlantType::cherry));
    assert(game.available_plants() == 5);
    game.begin_level(); assert(game.phase() == Phase::seed_select);
    assert(game.toggle_seed(PlantType::repeater));
    assert(game.toggle_seed(PlantType::potato_mine));
    assert(game.toggle_seed(PlantType::squash));
    auto progress = game.progress(); advance(game, 300);
    assert(game.elapsed() == 0 && game.sun() == 175 && game.progress() == progress);
    game.cancel_seed_selection(); assert(game.preview_ready());
    game.choose_seeds(); game.begin_level(); advance(game, preview_pan_frames);
    assert(game.phase() == Phase::playing && game.available_plants() == 6);
    assert(! game.plant_available(PlantType::cherry) && ! game.plant_available(PlantType::potato_mine));
    assert(game.seed_at(game.bank_slot(PlantType::repeater)) == PlantType::repeater);
    assert(game.plant(1, 1, PlantType::cherry) == PlantResult::inactive);
    // Combat fixtures use real plant placement and fixed actor pools without waiting through a whole level.
    auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
    zombies[0] = {true, ZombieType::bucket, 2, cell_x(5) * scale, 6};
    zombies[0].shield_health = 30;
    assert(game.plant(2, 4, PlantType::squash) == PlantResult::planted);
    game.tick();
    assert(game.plants()[cell_index(2, 4)].committed());
    int squash_timer = game.plants()[cell_index(2, 4)].timer;
    game.toggle_pause(); advance(game, 100);
    assert(game.plants()[cell_index(2, 4)].timer == squash_timer);
    game.toggle_pause(); advance(game, 30);
    assert(! zombies[0].active && game.lifetime_kills(ZombieType::bucket) == 1);
    assert(game.plants()[cell_index(2, 4)].exploding());
    advance(game, 18); assert(! game.plants()[cell_index(2, 4)].active);
    for(int i = 0; i < 8; ++i) { advance(game, 200); game.collect_sun(); }
    assert(game.plant(1, 0, PlantType::repeater) == PlantResult::planted);
    zombies[0] = {true, ZombieType::normal, 1, 230 * scale, 6};
    assert(game.plants()[cell_index(1, 0)].shot_frames == 0);
    advance(game, 90); assert(game.events().shot && game.plants()[cell_index(1, 0)].shot_frames == 6);
    advance(game, 7); assert(! game.events().shot);
    game.tick(); assert(game.events().shot && game.plants()[cell_index(1, 0)].shot_frames == 6);
    int peas = 0; for(const auto& shot : game.shots()) { peas += shot.active; }
    assert(peas == 2);
    advance(game, 82); assert(game.events().shot);
    assert(game.start(0) && game.available_plants() == 1 && ! game.plant_available(PlantType::repeater));
    std::puts("PASS six-packet selection, limited early roster, frozen setup, Squash armor crush/pause/cleanup and Repeater paired shots");
}

void wallnut_vault()
{
    Game game; game.load_progress(Progress::all_unlocked()); assert(game.start(5));
    assert(game.plant(2, 4, PlantType::walnut) == PlantResult::planted);
    auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
    zombies[0] = {true, ZombieType::vaulter, 2, (cell_x(4) + 25) * scale, 8};
    game.tick(); assert(zombies[0].vault_frames == 36 && zombies[0].pole_used && ! zombies[0].eating);
    int origin = zombies[0].x;
    game.toggle_pause(); advance(game, 100); assert(zombies[0].x == origin && zombies[0].vault_frames == 36);
    game.toggle_pause(); advance(game, 36);
    assert(zombies[0].vault_frames == 0 && zombies[0].x == (cell_x(4) - 22) * scale);
    assert(game.plants()[cell_index(2, 4)].health == spec(PlantType::walnut).health);
    assert(game.zombie_speed(zombies[0]) == 28);
    advance(game, 600);
    assert(game.plant(2, 2, PlantType::walnut) == PlantResult::planted);
    zombies[0].x = (cell_x(2) + 14) * scale;
    game.tick(); assert(zombies[0].eating && zombies[0].vault_frames == 0);
    zombies[1] = {true, ZombieType::vaulter, 3, (cell_x(2) + 14) * scale, 8};
    assert(game.plant(3, 2, PlantType::sunflower) == PlantResult::planted);
    game.tick(); assert(! zombies[1].eating && zombies[1].pole_used && zombies[1].vault_frames == 36);
    assert(game.start(5));
    assert(game.plant(2, 4, PlantType::walnut) == PlantResult::planted);
    zombies[0] = {true, ZombieType::vaulter, 2, (cell_x(4) + 25) * scale, 8};
    zombies[0].frost_frames = 180;
    game.tick(); advance(game, 36); assert(zombies[0].vault_frames == 18);
    advance(game, 36); assert(zombies[0].vault_frames == 0 && game.zombie_speed(zombies[0]) == 14);
    std::puts("PASS one Wall-nut vault, unchanged nut health, slower landing, first ordinary-plant vault, frost and pause");
}

void vaults_first_plant()
{
    for(auto type : {PlantType::sunflower, PlantType::peashooter, PlantType::walnut,
                     PlantType::potato_mine, PlantType::snow_pea, PlantType::cherry,
                     PlantType::repeater, PlantType::chomper})
    {
        Game game; game.load_progress(Progress::all_unlocked()); assert(game.start(9));
        auto& plants = const_cast<std::array<Plant, rows * columns>&>(game.plants());
        plants[cell_index(2, 4)] = {true, type, spec(type).health, type == PlantType::chomper ? 0 : spec(type).action_delay};
        plants[cell_index(2, 2)] = {true, PlantType::sunflower, 5, 540};
        auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
        zombies[0] = {true, ZombieType::vaulter, 2, (cell_x(4) + 25) * scale, 8};
        game.tick(); assert(zombies[0].vault_frames == 36 && zombies[0].pole_used && ! zombies[0].eating);
        assert(zombies[0].vault_target == (cell_x(4) - 22) * scale);
        assert(plants[cell_index(2, 4)].health == spec(type).health);
        // Remove a lit bomb before checking the next contact; the vault itself must not disarm it.
        plants[cell_index(2, 4)].active = false;
        advance(game, 36);
        zombies[0].x = (cell_x(2) + 14) * scale;
        game.tick(); assert(zombies[0].eating && zombies[0].vault_frames == 0);
    }
    std::puts("PASS vaulters leap the first ordinary plant once, then bite later plants; Tall-nut exception covered separately");
}

void tutorial_lanes_and_counters()
{
    Game game; game.load_progress(Progress::all_unlocked());
    for(int level = 0; level < 3; ++level)
    {
        assert(game.start(level));
        assert(game.level_spec().lanes == (level == 0 ? 1 : (level == 1 ? 3 : 5)));
        assert(game.unlocked_plants() == level + 1);
        assert(game.plant_unlocked(PlantType::sunflower) == (level >= 1));
        assert(game.plant_unlocked(PlantType::walnut) == (level >= 2));
        for(int row = 0; row < rows; ++row)
        {
            assert(game.mowers()[row].available == game.row_active(row));
            if(! game.row_active(row))
            { assert(game.plant(row, 0, PlantType::peashooter) == PlantResult::inactive); }
        }
        for(int frame = 0; frame < 2000; ++frame)
        {
            game.tick();
            for(const auto& zombie : game.zombies()) { if(zombie.active) { assert(game.row_active(zombie.row)); } }
            for(const auto& sun : game.suns())
            {
                if(sun.active) { assert(sun.target_y >= cell_y(game.first_row()) * scale && sun.target_y <= cell_y(game.last_row()) * scale); }
            }
        }
    }
    struct Introduction { int level; PlantType counter; ZombieType enemy; };
    for(auto entry : {Introduction{2, PlantType::walnut, ZombieType::cone},
                      Introduction{3, PlantType::potato_mine, ZombieType::bucket},
                      Introduction{4, PlantType::snow_pea, ZombieType::newspaper},
                      Introduction{5, PlantType::cherry, ZombieType::screen_door},
                      Introduction{6, PlantType::squash, ZombieType::football},
                      Introduction{7, PlantType::tallnut, ZombieType::vaulter}})
    {
        assert(game.start(entry.level));
        assert(game.plant_unlocked(entry.counter));
        assert(level_rosters[entry.level][static_cast<int>(entry.enemy)] > 0);
        for(int prior = 0; prior < entry.level; ++prior)
        { assert(level_rosters[prior][static_cast<int>(entry.enemy)] == 0); }
        assert(seed_order[entry.level] == entry.counter); // Awarded by the immediately preceding clear.
    }
    assert(game.start(7) && game.prepare_level()); advance(game, preview_ready_frame); game.choose_seeds();
    assert(game.toggle_seed(PlantType::cherry) && game.toggle_seed(PlantType::tallnut));
    game.begin_level(); advance(game, preview_pan_frames);
    assert(game.plant(2, 4, PlantType::tallnut) == PlantResult::planted);
    auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
    zombies[0] = {true, ZombieType::vaulter, 2, (cell_x(4) + 14) * scale, spec(ZombieType::vaulter).health};
    advance(game, 160);
    assert(zombies[0].active && zombies[0].eating && ! zombies[0].pole_used && zombies[0].vault_frames == 0);
    const auto& nut = game.plants()[cell_index(2, 4)];
    assert(spec(PlantType::tallnut).health == spec(PlantType::walnut).health * 2);
    assert(nut.active && nut.health == spec(PlantType::tallnut).health - 4);
    assert(zombies[0].x == (cell_x(4) + 14) * scale);
    std::puts("PASS one/three/five active lanes, restricted planting/sun/mowers/spawns, counter-before-enemy progression and Tall-nut stopping fresh pole vaulters");
}

void chomper_combat()
{
    Game game; game.load_progress(Progress::all_unlocked());
    assert(game.start(8) && ! game.plant_unlocked(PlantType::chomper));
    assert(game.start(9) && game.plant_unlocked(PlantType::chomper));
    assert(game.prepare_level()); advance(game, preview_ready_frame); game.choose_seeds();
    assert(game.toggle_seed(PlantType::cherry) && game.toggle_seed(PlantType::chomper));
    game.begin_level(); advance(game, preview_pan_frames);
    assert(game.plant(2, 3, PlantType::chomper) == PlantResult::planted && game.sun() == 25);
    const auto& plant = game.plants()[cell_index(2, 3)];
    assert(plant.timer == 0); // New Chompers are ready; the delay is only for digestion.
    auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
    zombies[0] = {true, ZombieType::football, 2, (cell_x(3) + 20) * scale, 10}; zombies[0].shield_health = 40;
    game.tick(); assert(plant.bite_frames == 36 && plant.timer == 0);
    advance(game, 23); assert(zombies[0].active && ! game.events().chomp);
    game.tick(); assert(game.events().chomp && ! zombies[0].active && plant.timer == 900);
    assert(game.lifetime_kills(ZombieType::football) == 1 && plant.bite_frames == 12);
    for(const auto& effect : game.zombie_effects()) { assert(! effect.active); } // Swallowed targets stay in the mouth instead of dropping a corpse/helmet.
    game.toggle_pause(); advance(game, 120); assert(plant.timer == 900 && plant.bite_frames == 12);
    game.toggle_pause();
    zombies[0] = {true, ZombieType::bucket, 2, (cell_x(3) + 22) * scale, 6}; zombies[0].shield_health = 30;
    advance(game, 20); assert(zombies[0].active && zombies[0].shield_health == 30 && plant.timer == 880 && plant.bite_frames == 0);
    zombies[0].active = false;
    advance(game, 879); assert(plant.timer == 1 && plant.bite_frames == 0);
    zombies[0] = {true, ZombieType::bucket, 2, (cell_x(3) + 20) * scale, 6}; zombies[0].shield_health = 30;
    game.tick(); assert(plant.bite_frames == 36);
    advance(game, 24); assert(! zombies[0].active && game.lifetime_kills(ZombieType::bucket) == 1);
    auto& fixture = const_cast<Plant&>(plant); fixture.timer = 0; fixture.bite_frames = 0;
    zombies[0] = {true, ZombieType::vaulter, 2, (cell_x(3) + 20) * scale, 8};
    zombies[0].vault_frames = 36; zombies[0].vault_origin = zombies[0].vault_target = zombies[0].x;
    game.tick(); assert(plant.bite_frames == 0 && zombies[0].active);
    std::puts("PASS Chomper late unlock, immediate readiness, windup, armored swallow/kill credit, digestion, pause and airborne immunity");
}

void chomper_reaches_over_wallnut()
{
    Game game; game.load_progress(Progress::all_unlocked());
    assert(game.start(9) && game.prepare_level());
    advance(game, preview_ready_frame); game.choose_seeds();
    assert(game.toggle_seed(PlantType::cherry) && game.toggle_seed(PlantType::chomper));
    game.begin_level(); advance(game, preview_pan_frames);
    advance(game, 240); assert(game.collect_sun() == 25);
    assert(game.plant(2, 4, PlantType::walnut) == PlantResult::planted);
    auto& zombies = const_cast<std::array<Zombie, zombie_capacity>&>(game.zombies());
    zombies[0] = {true, ZombieType::bucket, 2, (cell_x(4) + 15) * scale, 6};
    zombies[0].shield_health = 30;
    game.tick(); assert(zombies[0].eating);
    assert(game.plant(2, 3, PlantType::chomper) == PlantResult::planted);
    const auto& chomper = game.plants()[cell_index(2, 3)];
    const auto& nut = game.plants()[cell_index(2, 4)];
    game.tick(); assert(chomper.bite_frames == 36);
    advance(game, 24);
    assert(! zombies[0].active && nut.active && chomper.timer == 900);
    assert(game.lifetime_kills(ZombieType::bucket) == 1 && game.events().chomp);
    // A defender in the next row and enemies two cells ahead are out of reach.
    auto& fixture = const_cast<Plant&>(chomper); fixture.timer = 0; fixture.bite_frames = 0;
    zombies[0] = {true, ZombieType::normal, 1, (cell_x(4) + 15) * scale, 6};
    zombies[1] = {true, ZombieType::normal, 2, (cell_x(5) + 15) * scale, 6};
    game.tick(); assert(chomper.bite_frames == 0);
    std::puts("PASS Chomper swallows an armored zombie already eating the next-cell Wall-nut, while respecting lane/range limits");
}

void automatic_small_seed_banks()
{
    Game game; game.load_progress(Progress::all_unlocked());
    for(int level = 0; level < level_count; ++level)
    {
        assert(game.start(level) && game.prepare_level());
        game.choose_seeds(); assert(! game.preview_returning()); // The outgoing camera cannot be skipped.
        advance(game, preview_ready_frame);
        game.choose_seeds();
        if(game.unlocked_plants() <= visible_seed_count)
        {
            assert(game.phase() == Phase::preview && game.preview_returning());
            assert(game.available_plants() == game.unlocked_plants());
            for(int i = 0; i < game.unlocked_plants(); ++i)
            { assert(game.seed_at(i) == seed_order[i] && game.plant_available(seed_order[i])); }
            assert(! game.toggle_seed(seed_order[0]));
            advance(game, preview_pan_frames);
            assert(game.phase() == Phase::playing && game.elapsed() == 0 && game.next_wave_frames() == 900);
        }
        else
        {
            assert(game.phase() == Phase::seed_select && ! game.preview_returning());
            for(int count = visible_seed_count - 1; count >= 0; --count)
            {
                assert(game.toggle_seed(seed_order[count]));
                assert(game.available_plants() == count);
                game.begin_level(); assert(game.phase() == Phase::seed_select);
                advance(game, 5); assert(game.elapsed() == 0 && game.sun() == 175);
            }
            for(int count = 0; count < visible_seed_count; ++count) { assert(game.toggle_seed(seed_order[count])); }
            game.begin_level(); assert(game.phase() == Phase::preview && game.preview_returning());
        }
    }
    std::puts("PASS automatic one-to-six packet banks, selection only for larger rosters and rejection of every incomplete bank");
}

void reward_animation()
{
    int previous_x = reward_pose(0).x;
    for(int frame = 0; frame <= reward_ready_frame; ++frame)
    {
        auto pose = reward_pose(frame);
        assert(pose.x <= previous_x && pose.x >= 120);
        assert(pose.height >= 0 && pose.height <= 35);
        assert(pose.scale >= 50 && pose.scale <= 100);
        previous_x = pose.x;
    }
    for(int contact : {0, 48, 84, 112, 132})
    { assert(reward_pose(contact).height == 0 && reward_pose(contact).y == 112); }
    assert(reward_pose(24).height > reward_pose(66).height);
    assert(reward_pose(66).height > reward_pose(98).height);
    assert(reward_pose(98).height > reward_pose(122).height);
    assert(reward_pose(149).y - reward_pose(150).y <= 1);
    assert(reward_pose(reward_reveal_frame).scale == 100);
    std::puts("PASS reward travel, diminishing bounces, landing contacts, and reveal timing");
}

void wave_track_alignment()
{
    for(int total : {3, 5})
    {
        int previous = 56;
        for(int progress = 0; progress <= total * 100; ++progress)
        {
            int filled = 0;
            for(int wave = 0; wave < total; ++wave)
            { filled += wave_track::fill(wave, progress, total); }
            int head = wave_track::position(progress, total);
            assert(head == 56 + filled && head >= previous && head <= 224);
            if(progress % 100 == 0)
            { assert(head == wave_track::boundary(progress / 100, total)); }
            previous = head;
        }
    }
    std::puts("PASS three- and five-wave tracks align at every pixel and flag pole");
}

void advance(Game& game, int frames)
{
    for(int i = 0; i < frames; ++i) { game.tick(); }
}

void road_entry()
{
    Game game;
    assert(game.start());
    advance(game, 899);
    for(const auto& zombie : game.zombies()) { assert(! zombie.active); }
    game.tick();
    const auto& zombie = game.zombies()[0];
    assert(zombie.active && zombie.x == road_spawn_x * scale - spec(ZombieType::normal).speed);
    assert(zombie.x / scale - 14 >= 240); // Even its reaching hand is beyond the screen.
    game.toggle_pause();
    int x = zombie.x;
    advance(game, 20);
    assert(zombie.x == x);
    game.toggle_pause();
    for(int frame = 0; frame < 40; ++frame)
    {
        int before = zombie.x;
        game.tick();
        assert(zombie.x == before - spec(ZombieType::normal).speed);
    }
    assert(zombie.x / scale - 14 < 240); // It crosses the edge through ordinary walking.
    std::puts("PASS road zombies spawn fully offscreen, pause correctly, and walk into view without teleporting");
}

void flag_rallies()
{
    Game game;
    game.load_progress(Progress::all_unlocked());
    assert(game.start(5));
    for(const auto& mower : game.mowers()) { assert(mower.available); }
    bool entering_flag_seen = false;
    for(int frame = 0; ! game.events().horde_started && frame < 6000; ++frame)
    {
        assert(! game.horde_entered(1));
        for(const auto& zombie : game.zombies())
        {
            if(zombie.active)
            {
                assert(zombie.type == ZombieType::normal || (zombie.type == ZombieType::flag && zombie.x >= flag_entry_x * scale));
                entering_flag_seen |= zombie.type == ZombieType::flag;
            }
        }
        game.tick();
    }
    assert(entering_flag_seen && game.events().horde_started && game.horde_entered(1) && game.wave_progress() == 100);
    for(const auto& zombie : game.zombies())
    { if(zombie.active && zombie.type == ZombieType::flag) { assert(zombie.x < flag_entry_x * scale); } }
    advance(game, 480);
    int group = 0;
    int buffed = 0;
    for(const auto& zombie : game.zombies())
    {
        if(! zombie.active) { continue; }
        ++group;
        if(zombie.type == ZombieType::flag)
        { assert(! game.flag_buffed(zombie)); continue; }
        if(game.flag_buffed(zombie))
        {
            assert(game.zombie_speed(zombie) == movement_speed(zombie) * 9 / 8);
            assert(game.bite_delay(zombie) == 35);
            ++buffed;
        }
    }
    assert(group >= 5 && buffed >= 3);
    Zombie far = game.zombies()[1];
    far.x -= 120 * scale;
    assert(! game.flag_buffed(far));
    assert(game.plant(0, 5, PlantType::cherry) == PlantResult::planted);
    advance(game, 65);
    assert(! game.zombies()[0].active);
    int survivors = 0;
    for(const auto& zombie : game.zombies())
    {
        if(! zombie.active) { continue; }
        ++survivors;
        assert(! game.flag_buffed(zombie));
        assert(game.zombie_speed(zombie) == movement_speed(zombie));
        assert(game.bite_delay(zombie) == 40);
    }
    assert(survivors > 0);
    assert(game.start(2));
    for(int row = 0; row < rows; ++row)
    { assert(game.mowers()[row].available); }
    assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::planted);
    std::puts("PASS flag escorts, speed/bite aura, distance limit, leader death, all five lanes restored");
}

void armor_damage()
{
    auto enemy = [](ZombieType type) {
        Zombie zombie;
        zombie.active = true;
        zombie.type = type;
        zombie.health = spec(type).health;
        zombie.shield_health = spec(type).shield_health;
        return zombie;
    };
    auto paper = enemy(ZombieType::newspaper);
    assert(paper.health == spec(ZombieType::normal).health && movement_speed(paper) == 32);
    for(int hit = 0; hit < 16; ++hit)
    {
        assert(! take_zombie_damage(paper, 1, DamageType::projectile));
        assert(paper.health == 6);
    }
    assert(paper.shield_health == 0 && movement_speed(paper) == 96);
    assert(take_zombie_damage(paper, 6, DamageType::projectile));
    auto bucket = enemy(ZombieType::bucket);
    assert(bucket.shield_health + bucket.health == 36);
    assert(! take_zombie_damage(bucket, 30, DamageType::projectile));
    assert(bucket.shield_health == 0 && bucket.health == 6);
    auto door = enemy(ZombieType::screen_door);
    for(int hit = 0; hit < 18; ++hit)
    { assert(! take_zombie_damage(door, 1, DamageType::projectile)); assert(door.health == 6); }
    assert(door.shield_health == 0 && take_zombie_damage(door, 6, DamageType::projectile));
    door = enemy(ZombieType::screen_door);
    assert(! take_zombie_damage(door, 5, DamageType::blast));
    assert(door.health == 1 && door.shield_health == 18);
    assert(take_zombie_damage(door, 1, DamageType::blast));
    for(auto type : {ZombieType::newspaper, ZombieType::bucket, ZombieType::screen_door})
    {
        auto zombie = enemy(type);
        assert(take_zombie_damage(zombie, 100, DamageType::crush));
        zombie = enemy(type);
        assert(take_zombie_damage(zombie, 100, DamageType::blast));
    }
    std::puts("PASS newspaper durability and full body health, bucket armor, door blocking, blasts and mowers");
}

void frost_effects()
{
    Zombie zombie;
    zombie.active = true;
    zombie.health = 6;
    zombie.bite_timer = 20;
    assert(! take_zombie_damage(zombie, 1, DamageType::frost));
    assert(zombie.frost_frames == frost_duration && movement_speed(zombie) == 16);
    assert(zombie.bite_timer == 40);
    for(int frame = 0; frame < 90; ++frame) { advance_zombie_status(zombie); }
    assert(! take_zombie_damage(zombie, 1, DamageType::frost));
    assert(zombie.frost_frames == frost_duration && zombie.bite_timer == 40);
    for(int frame = 0; frame < frost_duration; ++frame) { advance_zombie_status(zombie); }
    assert(zombie.frost_frames == 0 && movement_speed(zombie) == 32 && zombie.bite_timer == 20);
    for(auto type : {ZombieType::bucket, ZombieType::newspaper, ZombieType::screen_door})
    {
        Zombie armored;
        armored.active = true;
        armored.type = type;
        armored.health = spec(type).health;
        armored.shield_health = spec(type).shield_health;
        assert(! take_zombie_damage(armored, 1, DamageType::frost));
        assert(armored.health == 6 && armored.shield_health == spec(type).shield_health - 1);
        assert((armored.frost_frames > 0) == (type != ZombieType::screen_door));
        if(type == ZombieType::screen_door)
        {
            for(int hit = 1; hit < spec(type).shield_health; ++hit)
            { assert(! take_zombie_damage(armored, 1, DamageType::frost)); }
            assert(armored.shield_health == 0 && armored.frost_frames == 0);
            assert(! take_zombie_damage(armored, 1, DamageType::frost));
            assert(armored.health == 5 && armored.frost_frames == frost_duration);
        }
    }
    Game game;
    assert(game.start());
    assert(game.plant(1, 2, PlantType::snow_pea) == PlantResult::inactive);
    game.load_progress(Progress::all_unlocked());
    assert(game.start(4) && game.available_plants() == 5);
    assert(game.plant(1, 2, PlantType::snow_pea) == PlantResult::planted && game.sun() == 0);
    int chilled = -1;
    for(int frame = 0; frame < 1600 && chilled < 0; ++frame)
    {
        game.tick();
        for(int i = 0; i < zombie_capacity; ++i)
        { if(game.zombies()[i].active && game.zombies()[i].frost_frames > 0) { chilled = i; break; } }
    }
    assert(chilled >= 0);
    assert(game.zombie_speed(game.zombies()[chilled]) == 16 && game.bite_delay(game.zombies()[chilled]) == 80);
    int timer = game.zombies()[chilled].frost_frames, x = game.zombies()[chilled].x;
    game.toggle_pause(); advance(game, 120);
    assert(game.zombies()[chilled].frost_frames == timer && game.zombies()[chilled].x == x);
    game.toggle_pause(); game.tick();
    assert(game.zombies()[chilled].frost_frames == timer - 1 && x - game.zombies()[chilled].x == 16);
    std::puts("PASS snow pea cost/unlock, icy hits, half movement/bites, refresh/expiry, pause, armor and door immunity");
}

void sun_cadence()
{
    Game game;
    auto fixture = Progress::all_unlocked(); fixture.selected = 2; game.load_progress(fixture);
    game.start();
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::planted);
    auto tokens = [&]() { int count = 0; for(const auto& sun : game.suns()) { count += sun.active; } return count; };
    advance(game, 179);
    assert(tokens() == 0);
    game.tick();
    assert(tokens() == 1);
    advance(game, 359);
    assert(tokens() == 1 && game.plants()[cell_index(2, 0)].timer == 1);
    game.tick();
    assert(tokens() == 2);
    advance(game, 239);
    assert(tokens() == 2);
    game.tick();
    assert(tokens() == 3);
    std::puts("PASS sky sun every ten seconds, sunflower sun every nine seconds");
}

void economy_and_pause()
{
    Game game;
    auto fixture = Progress::all_unlocked(); fixture.selected = 2; game.load_progress(fixture);
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::inactive);
    game.start();
    assert(game.sun() == 175);
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::planted);
    assert(game.sun() == 125);
    assert(game.plant(2, 0, PlantType::peashooter) == PlantResult::occupied);
    assert(game.plant(2, 1, PlantType::sunflower) == PlantResult::cooling_down);
    assert(game.plant(-1, 0, PlantType::walnut) == PlantResult::inactive);
    assert(game.plant(0, columns, PlantType::walnut) == PlantResult::inactive);
    assert(game.plant(0, 1, PlantType::cherry) == PlantResult::inactive);
    game.toggle_pause();
    advance(game, 1000);
    assert(game.elapsed() == 0);
    assert(game.next_wave_frames() == 900);
    assert(game.cooldown(PlantType::sunflower) == 240);
    assert(game.collect_sun() == 0);
    game.toggle_pause();
    advance(game, 240);
    assert(game.cooldown(PlantType::sunflower) == 0);
    assert(game.collect_sun() == 25);
    assert(game.collect_sun() == 0);
    assert(game.sun() == 150);
    advance(game, 360);
    assert(game.collect_sun() == 25); // The sunflower produces; the next sky token arrives at frame 780.
    assert(game.shovel(2, 0));
    assert(! game.shovel(2, 0));
    assert(game.sun() == 175); // Shoveling gives no refund.
    game.start();
    assert(game.elapsed() == 0 && game.sun() == 175 && game.wave() == 0);
    for(const auto& plant : game.plants()) { assert(! plant.active); }
    std::puts("PASS economy, cooldowns, sun production, pause, shovel, restart");
}

void combat()
{
    Game bomb;
    bomb.load_progress(Progress::all_unlocked());
    bomb.start(5);
    advance(bomb, 900);
    assert(bomb.wave() == 1);
    assert(bomb.zombies()[0].active && bomb.zombies()[0].row == 1);
    assert(bomb.plant(1, 8, PlantType::cherry) == PlantResult::planted);
    advance(bomb, 65);
    assert(bomb.events().explosion && bomb.events().hit);
    assert(bomb.wave_progress() == 100 / ((bomb.level_spec().normal_size(1) + 1) / 2));
    assert(! bomb.zombies()[0].active);
    assert(bomb.kills() == 1);
    assert(bomb.plants()[cell_index(1, 8)].active);
    advance(bomb, 18);
    assert(! bomb.events().explosion);
    assert(! bomb.plants()[cell_index(1, 8)].active);

    Game wall;
    wall.load_progress(Progress::all_unlocked());
    wall.start(2);
    assert(wall.plant(1, 8, PlantType::walnut) == PlantResult::planted);
    advance(wall, 1400);
    const auto& nut = wall.plants()[cell_index(1, 8)];
    assert(nut.active && nut.health < spec(PlantType::walnut).health);
    assert(wall.zombies()[0].x > 230 * scale); // The nut holds it at contact.
    std::puts("PASS bomb fuse, blast damage, removal, zombie biting, wall blocking");
}

void all_collected_suns_fly_to_bank()
{
    Game game;
    auto fixture = Progress::all_unlocked(); fixture.selected = 2; game.load_progress(fixture);
    game.start();
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::planted);
    advance(game, 600);
    auto origins = game.suns();
    int tokens = 0;
    for(const auto& sun : origins) { tokens += sun.active; }
    assert(tokens == 2); // A sunflower token and the less frequent sky token.
    assert(game.collect_sun() == tokens * 25);
    assert(game.sun() == 175);
    assert(game.collect_sun() == 0); // Repeated collection cannot restart a flight or pay twice.
    game.tick();
    for(int i = 0; i < sun_capacity; ++i)
    {
        if(! origins[i].active) { continue; }
        const auto& sun = game.suns()[i];
        assert(sun.active && sun.collecting);
        assert(sun.x < origins[i].x && sun.x >= sun_bank_x * scale);
        assert(sun.y < origins[i].y && sun.y >= sun_bank_y * scale);
    }
    auto paused = game.suns();
    game.toggle_pause();
    advance(game, 100);
    for(int i = 0; i < sun_capacity; ++i)
    {
        assert(game.suns()[i].x == paused[i].x && game.suns()[i].y == paused[i].y);
        assert(game.suns()[i].flight_frame == paused[i].flight_frame);
    }
    game.toggle_pause();
    advance(game, sun_collection_frames - 1);
    for(int i = 0; i < sun_capacity; ++i)
    {
        if(! origins[i].active) { continue; }
        assert(! game.suns()[i].active);
        assert(game.suns()[i].x == sun_bank_x * scale && game.suns()[i].y == sun_bank_y * scale);
    }
    assert(game.sun() == 175 && game.collect_sun() == 0);

    game.start();
    advance(game, 839); // The first sky token is one frame away from expiring.
    assert(game.collect_sun() == 50);
    advance(game, 2);
    assert(game.suns()[0].active && game.suns()[0].collecting);
    game.start();
    for(const auto& sun : game.suns()) { assert(! sun.active && ! sun.collecting); }
    std::puts("PASS all suns fly independently, credit once, finish at bank, pause, expiry, restart");
}

void unattended_loss()
{
    Game game;
    game.start();
    int frames = 0;
    while(game.phase() == Phase::playing && frames++ < 30000) { game.tick(); }
    assert(game.phase() == Phase::lost);
    assert(game.kills() >= game.level_spec().lanes);
    for(const auto& mower : game.mowers()) { assert(! mower.available); }
    int elapsed = game.elapsed();
    assert(game.breached_zombie() >= 0 && ! game.loss_ready());
    advance(game, 100);
    assert(game.elapsed() == elapsed && game.loss_frames() == 100 && ! game.loss_ready());
    advance(game, defeat_audio::ready_frame - 100);
    assert(game.loss_ready() && game.elapsed() == elapsed);
    assert(game.start() && game.loss_frames() == 0 && game.breached_zombie() == -1);
    std::puts("PASS one-use mowers, unattended loss, terminal state freezes");
}

void defend(Game& game, int strategy = 0)
{
    game.collect_sun();
    // Economy-first alternatives deliberately use the initial mowers as a buffer.
    // Every attempt still starts with the normal resources and public player actions.
    if(strategy >= 3 && game.elapsed() < (strategy == 3 ? 1800 : 2700))
    {
        for(int column = 0; column < 2; ++column)
        { for(int row = 0; row < rows; ++row)
          { if(! game.plants()[cell_index(row, column)].active) { game.plant(row, column, PlantType::sunflower); } } }
        return;
    }
    if(game.plant_available(PlantType::squash) && ! game.cooldown(PlantType::squash))
    {
        for(const auto& zombie : game.zombies())
        {
            if(! zombie.active || zombie.x > 150 * scale || zombie.vault_frames > 0) { continue; }
            int column = (zombie.x / scale - lawn_left) / cell_width;
            if(column < 0) { continue; }
            if(game.plant(zombie.row, column, PlantType::squash) == PlantResult::planted) { break; }
        }
    }
    // Save for a Cherry Bomb when a rally presents a worthwhile blast target.
    if(game.plant_available(PlantType::cherry) && ! game.cooldown(PlantType::cherry))
    {
        int best = 0, best_row = 0, best_column = 0;
        for(int row = 0; row < rows; ++row)
        {
            for(int column = 0; column < columns; ++column)
            {
                if(game.plants()[cell_index(row, column)].active || game.tombstone_at(row, column)) { continue; }
                int count = 0;
                for(const auto& zombie : game.zombies())
                {
                    int dy = zombie.row - row;
                    int dx = zombie.x / scale - cell_x(column);
                    if(zombie.active && dy >= -1 && dy <= 1 && dx >= -37 && dx <= 37)
                    {
                        count += zombie.x < 130 * scale ? 3 : 1;
                        if(zombie.shield_health > 0 || zombie.type == ZombieType::flag) { ++count; }
                    }
                }
                if(count > best) { best = count; best_row = row; best_column = column; }
            }
        }
        if(best >= 3 && game.plant(best_row, best_column, PlantType::cherry) == PlantResult::insufficient_sun)
        { return; }
    }
    const int wall_column = strategy == 1 && game.plant_available(PlantType::cherry) ? 6 : 5;
    auto wall_type = game.plant_available(PlantType::tallnut) ? PlantType::tallnut : PlantType::walnut;
    if(game.plant_available(wall_type))
    {
        for(const auto& zombie : game.zombies())
        {
            if(zombie.active && zombie.x < (cell_x(wall_column) + 35) * scale && zombie.x >= (cell_x(wall_column) - 5) * scale &&
               game.plants()[cell_index(zombie.row, 2)].active &&
               ! game.plants()[cell_index(zombie.row, wall_column)].active)
            { game.plant(zombie.row, wall_column, wall_type); }
        }
    }
    // A simple player strategy, using only the same public actions as the controller.
    // Cover the lane with the nearest approaching enemy before expanding the economy.
    for(const auto& zombie : game.zombies())
    {
        if(! zombie.active) { continue; }
        int row = zombie.row;
        if(! game.plants()[cell_index(row, 2)].active)
        { game.plant(row, 2, PlantType::peashooter); }
    }
    for(int row = 0; row < rows; ++row)
    {
        if(! game.plants()[cell_index(row, 0)].active)
        { game.plant(row, 0, PlantType::sunflower); }
    }
    // Graves are stationary targets: cover those lanes before their first horde.
    for(const auto& stone : game.tombstones())
    {
        if(stone.active() && ! game.plants()[cell_index(stone.row, 2)].active)
        { game.plant(stone.row, 2, PlantType::peashooter); }
    }
    if(game.wave() == 0) { return; }
    // Later yards need a second economy column to replace the extra defenses.
    if(game.level_spec().tombstones > 0)
    {
        for(int row = 0; row < rows; ++row)
        {
            if(! game.plants()[cell_index(row, 1)].active)
            { game.plant(row, 1, PlantType::sunflower); }
        }
    }
    if(game.plant_available(PlantType::snow_pea))
    {
        for(int row = 0; row < rows; ++row)
        {
            if(! game.plants()[cell_index(row, 1)].active)
            { game.plant(row, 1, PlantType::snow_pea); }
        }
    }
    for(int column = 2; column <= (strategy == 1 && game.plant_available(PlantType::cherry) ? 5 : 4); ++column)
    {
        for(int row = 0; row < rows; ++row)
        {
            if(! game.plants()[cell_index(row, column)].active)
            {
                auto type = ((strategy == 2 || strategy == 6) && column == 4 && game.plant_available(PlantType::snow_pea)) ? PlantType::snow_pea :
                            (strategy >= 5 && game.plant_available(PlantType::repeater) ? PlantType::repeater : PlantType::peashooter);
                game.plant(row, column, type);
            }
        }
    }
}


void unlock_next(Game& game)
{
    while(game.phase() == Phase::playing) { defend(game); game.tick(); }
    assert(game.phase() == Phase::won);
    game.open_level_select();
    assert(game.start());
}

void mine_and_newspaper()
{
    Game game;
    game.start();
    assert(game.plant(1, 7, PlantType::potato_mine) == PlantResult::inactive);
    assert(! game.start(1));
    unlock_next(game);
    assert(game.available_plants() == 2);
    game.load_progress(Progress::all_unlocked());
    assert(game.start(3));
    assert(game.plant(1, 7, PlantType::potato_mine) == PlantResult::planted);
    assert(game.sun() == 150);
    advance(game, 479);
    assert(game.plants()[cell_index(1, 7)].timer == 1);
    game.toggle_pause();
    advance(game, 100);
    assert(game.plants()[cell_index(1, 7)].timer == 1);
    game.toggle_pause();
    game.tick();
    assert(game.plants()[cell_index(1, 7)].timer == 0);
    int detonations = 0;
    for(int frame = 0; frame < 1000; ++frame)
    {
        game.tick();
        if(game.events().explosion)
        {
            ++detonations;
            assert(game.kills() == 1 && ! game.zombies()[0].active);
            for(const auto& zombie : game.zombies())
            {
                if(zombie.active)
                {
                    assert(zombie.row != 1);
                    assert(zombie.health == spec(zombie.type).health);
                }
            }
            assert(game.plants()[cell_index(1, 7)].health == 0);
            game.toggle_pause();
            advance(game, 60);
            assert(game.plants()[cell_index(1, 7)].timer == 18);
            game.toggle_pause();
        }
    }
    assert(detonations == 1 && ! game.plants()[cell_index(1, 7)].active);
    assert(game.plant(1, 7, PlantType::potato_mine) == PlantResult::planted);
    assert(game.plant(2, 7, PlantType::potato_mine) == PlantResult::cooling_down);
    game.start();
    advance(game, 900);
    assert(game.plant(1, 8, PlantType::potato_mine) == PlantResult::planted);
    for(int frame = 0; frame < 360; ++frame) { game.tick(); assert(! game.events().explosion); }
    assert(! game.plants()[cell_index(1, 8)].active); // Eaten before arming.
    game.load_progress(Progress::all_unlocked());
    assert(game.start(6));
    assert(game.level() == 6);
    for(int frame = 0; ! game.events().horde_started && frame < 6000; ++frame) { game.tick(); }
    assert(game.events().horde_started);
    advance(game, 480);
    bool newspaper_seen = false;
    for(int i = 0; i < zombie_capacity; ++i)
    {
        if(game.zombies()[i].active && game.zombies()[i].type == ZombieType::newspaper)
        {
            newspaper_seen = true;
            int x = game.zombies()[i].x;
            assert(game.zombies()[i].health == spec(ZombieType::normal).health);
            game.tick();
            assert(game.zombies()[i].shield_health == 16);
            assert(x - game.zombies()[i].x == game.zombie_speed(game.zombies()[i]));
            assert(game.plant(game.zombies()[i].row, 0, PlantType::peashooter) == PlantResult::planted);
            bool paper_hit = false;
            for(int tick = 0; tick < 900 && game.zombies()[i].active; ++tick)
            {
                game.tick();
                if(! game.zombies()[i].active) { break; }
                assert(game.zombies()[i].health == spec(ZombieType::normal).health);
                paper_hit |= game.zombies()[i].shield_health < 16;
                if(paper_hit) { break; }
            }
            assert(paper_hit && game.zombies()[i].shield_health > 0);
            break;
        }
    }
    assert(newspaper_seen && spec(ZombieType::newspaper).health == spec(ZombieType::normal).health);
    std::puts("PASS mine unlock, price, arming, pause, cooldown, detonation, lane scope, vulnerability, newspaper shield and rage speed");
}

void tombstone_rules()
{
    Game game;
    game.load_progress(Progress::all_unlocked());
    for(int level = 0; level < level_count; ++level)
    {
        assert(game.start(level));
        int stones = 0;
        for(const auto& stone : game.tombstones())
        {
            if(! stone.active()) { continue; }
            ++stones;
            assert(stone.health == tombstone_health && stone.pending == 0 && stone.column >= 6);
            assert(game.tombstone_at(stone.row, stone.column));
            assert(game.plant(stone.row, stone.column, PlantType::peashooter) == PlantResult::occupied);
            assert(! game.shovel(stone.row, stone.column) && stone.health == tombstone_health);
        }
        assert(stones == game.level_spec().tombstones);
        assert((stones > 0) == (level >= 5));
        assert(game.sun() == 175 && game.cooldown(PlantType::peashooter) == 0);
    }
    // Both pea types acquire a stone as a target even with no zombies present.
    for(auto shooter : {PlantType::peashooter, PlantType::snow_pea})
    {
        assert(game.start(9));
        const auto& stone = game.tombstones()[0];
        assert(game.plant(stone.row, stone.column - 1, shooter) == PlantResult::planted);
        int hit_count = 0;
        while(stone.health == tombstone_health && game.elapsed() < 150) { game.tick(); }
        assert(stone.health == tombstone_health - 1 && stone.flash > 0);
        ++hit_count;
        game.toggle_pause();
        advance(game, 120);
        assert(stone.health == tombstone_health - 1 && stone.flash == 6);
        game.toggle_pause();
        for(int tick = 0; stone.active() && tick < 800; ++tick)
        {
            int previous = stone.health;
            game.tick();
            if(stone.health < previous) { ++hit_count; assert(game.events().hit); }
        }
        assert(! stone.active() && hit_count == tombstone_health && game.kills() == 0);
        assert(! game.tombstone_at(stone.row, stone.column));
        if(shooter == PlantType::peashooter)
        { assert(game.plant(stone.row, stone.column, PlantType::sunflower) == PlantResult::planted); }
    }
    // A stone shields a zombie behind it, including from the freezing effect.
    assert(game.start(9));
    advance(game, 900);
    assert(game.zombies()[0].row == 1);
    assert(game.plant(1, 6, PlantType::snow_pea) == PlantResult::planted);
    advance(game, 100);
    assert(game.tombstones()[2].health == tombstone_health - 1);
    assert(game.zombies()[0].health == spec(ZombieType::normal).health && game.zombies()[0].frost_frames == 0);
    // Once a zombie walks in front, it takes the hit instead of the stone behind it.
    assert(game.start(9));
    advance(game, 900);
    while(game.zombies()[0].x > 196 * scale) { game.tick(); }
    assert(game.plant(1, 5, PlantType::snow_pea) == PlantResult::planted);
    advance(game, 100);
    assert(game.zombies()[0].health == spec(ZombieType::normal).health - 1 && game.zombies()[0].frost_frames > 0);
    assert(game.tombstones()[2].health == tombstone_health);
    assert(game.start(9));
    assert(game.plant(0, 5, PlantType::cherry) == PlantResult::planted);
    advance(game, spec(PlantType::cherry).action_delay);
    assert(! game.tombstones()[0].active() && game.events().explosion);
    assert(game.tombstones()[1].health == tombstone_health); // Outside the blast.
    // Ordinary waves do not raise graves. The actual flag entry triggers each living stone once.
    assert(game.start(9));
    int triggered = 0, spawned = 0;
    for(int frame = 0; frame < 6000 && triggered == 0; ++frame)
    {
        int standing = 0, pending_before = 0;
        for(const auto& stone : game.tombstones())
        { standing += stone.active(); pending_before += stone.pending; }
        game.tick();
        int pending = 0;
        for(const auto& stone : game.tombstones()) { pending += stone.pending; }
        int raised = game.events().tombstone_spawns;
        assert(pending - pending_before + raised == (game.events().horde_started ? standing : 0));
        if(game.events().horde_started) { triggered += standing; }
        spawned += raised;
    }
    int queued = 0;
    for(const auto& stone : game.tombstones()) { queued += stone.pending; }
    assert(triggered > 0 && spawned + queued == triggered);
    game.tick(); // Entry queues raises; they are released by the next spawn update.
    spawned += game.events().tombstone_spawns;
    assert(spawned == triggered);
    int grave_zombies = 0;
    for(const auto& zombie : game.zombies())
    {
        if(! zombie.active || zombie.group != 0) { continue; }
        ++grave_zombies;
        assert(zombie.type == ZombieType::normal && zombie.health == spec(ZombieType::normal).health);
        assert(zombie.emergence_frames == emergence_duration - 1 && game.zombie_speed(zombie) == 0 && ! zombie.eating);
        bool at_grave = false;
        for(const auto& stone : game.tombstones())
        { at_grave |= stone.active() && stone.row == zombie.row &&
                      cell_x(stone.column) * scale - zombie.x < scale; }
        assert(at_grave);
    }
    assert(grave_zombies == spawned);
    auto graves = game.tombstones();
    auto zombies = game.zombies();
    game.toggle_pause();
    advance(game, 100);
    for(int i = 0; i < tombstone_capacity; ++i)
    {
        assert(game.tombstones()[i].pending == graves[i].pending);
        assert(game.tombstones()[i].health == graves[i].health);
    }
    for(int i = 0; i < zombie_capacity; ++i)
    {
        assert(game.zombies()[i].x == zombies[i].x);
        assert(game.zombies()[i].emergence_frames == zombies[i].emergence_frames);
    }
    game.toggle_pause();
    advance(game, emergence_duration - 1);
    for(int i = 0; i < zombie_capacity; ++i)
    {
        if(! zombies[i].active || zombies[i].group != 0) { continue; }
        assert(game.zombies()[i].active && game.zombies()[i].emergence_frames == 0);
        assert(game.zombies()[i].x == zombies[i].x && ! game.zombies()[i].eating);
    }
    game.tick();
    for(int i = 0; i < zombie_capacity; ++i)
    {
        if(zombies[i].active && zombies[i].group == 0) { assert(game.zombies()[i].x < zombies[i].x); }
    }
    Zombie rising{true, ZombieType::normal, 0, 0, 6};
    rising.emergence_frames = emergence_duration;
    assert(! take_zombie_damage(rising, 1, DamageType::frost) && rising.health == 5 && rising.frost_frames > 0);
    assert(movement_speed(rising) == 0);
    assert(take_zombie_damage(rising, 100, DamageType::blast));
    assert(game.start(9));
    for(const auto& stone : game.tombstones()) { assert(stone.health == tombstone_health && stone.pending == 0); }
    assert(game.plant(0, 5, PlantType::cherry) == PlantResult::planted);
    advance(game, spec(PlantType::cherry).action_delay);
    assert(! game.tombstones()[0].active());
    int standing_at_flag = 0;
    for(int frame = 0; ! game.events().horde_started && frame < 6000; ++frame)
    {
        standing_at_flag = 0;
        for(const auto& stone : game.tombstones()) { standing_at_flag += stone.active(); }
        game.tick();
    }
    int pending = 0;
    for(const auto& stone : game.tombstones()) { pending += stone.pending; }
    assert(game.events().horde_started && game.events().tombstone_spawns + pending == standing_at_flag);
    assert(game.tombstones()[0].pending == 0 && ! game.tombstones()[0].active());
    std::puts("PASS tombstone layouts, occupied cells, pea/frost damage, cracks/flash, pause, blast, horde-only spawns and restart");
}

void football_and_crumbling()
{
    assert(spec(ZombieType::football).health + spec(ZombieType::football).shield_health == 50);
    Zombie athlete{true, ZombieType::football, 0, 0, 10};
    athlete.shield_health = 40;
    assert(movement_speed(athlete) > spec(ZombieType::normal).speed * 2);
    assert(! take_zombie_damage(athlete, 1, DamageType::frost));
    assert(athlete.health == 10 && athlete.shield_health == 39 && movement_speed(athlete) == 44);
    for(int hit = 0; hit < 39; ++hit) { assert(! take_zombie_damage(athlete, 1, DamageType::projectile)); }
    assert(athlete.health == 10 && athlete.shield_health == 0);
    assert(take_zombie_damage(athlete, 100, DamageType::blast));
    for(int level = 0; level < level_count; ++level)
    { assert((level_rosters[level][static_cast<int>(ZombieType::football)] > 0) == (level == 6 || level >= 8)); }
    Game game; game.load_progress(Progress::all_unlocked()); assert(game.start(9));
    assert(game.plant(0, 5, PlantType::cherry) == PlantResult::planted);
    advance(game, 65);
    assert(! game.tombstones()[0].active() && game.tombstones()[0].crumble_frames == 30);
    assert(! game.tombstone_at(0, 6));
    game.toggle_pause(); advance(game, 60);
    assert(game.tombstones()[0].crumble_frames == 30);
    game.toggle_pause(); advance(game, 30);
    assert(game.tombstones()[0].crumble_frames == 0);
    assert(game.start(9));
    assert(game.tombstones()[0].active() && game.tombstones()[0].crumble_frames == 0);
    // Existing version-two records have zero in the reserved football counter word.
    V2SaveRecord old{}; put_save_word(old, 0, 0x325A5650); old[4] = 2; old[5] = 3; old[6] = 1;
    for(int i = 0; i < 6; ++i) { put_save_word(old, 20 + i * 4, uint32_t(i + 1) * 10); }
    put_save_word(old, 44, save_checksum(old)); Progress loaded;
    put_save_word(old, 16, 123); put_save_word(old, 44, save_checksum(old));
    assert(decode_v2_progress(old, loaded) && loaded.zombie_kills[6] == 123 && loaded.zombie_kills[7] == 0);
    for(int i = 0; i < 6; ++i) { assert(loaded.zombie_kills[i] == uint32_t(i + 1) * 10); }
    Mower mower; mower.available = false; mower.moving = true;
    int steps = 0;
    while(mower.moving) { int x = mower.x; mower.advance(); assert(mower.x == x + 3 * scale); ++steps; }
    assert(steps == 78 && mower.x > 252 * scale);
    std::puts("PASS football armor/charge/frost/blast/rosters, reserved-word save compatibility, crumbling/pause/restart and full mower travel");
}

void explosion_lifetimes()
{
    for(auto type : {PlantType::cherry, PlantType::potato_mine})
    {
        Plant effect{true, type, 0, 18, 0};
        int stages[3]{};
        for(int tick = 0; tick < 18; ++tick)
        {
            assert(effect.exploding());
            ++stages[(18 - effect.timer) / 6];
            effect.advance_explosion();
        }
        assert(! effect.active && ! effect.exploding());
        for(int duration : stages) { assert(duration == 6); }
        Plant untriggered{true, type, 1, 18, 0};
        untriggered.advance_explosion();
        assert(untriggered.active && untriggered.timer == 18);
    }
    std::puts("PASS shared explosion completion keeps every blast frame and leaves untriggered plants alone");
}

void zombie_damage_effects()
{
    Game game;
    auto fixture = Progress::all_unlocked(); fixture.selected = 2; game.load_progress(fixture);
    assert(game.start());
    assert(game.plant(1, 0, PlantType::peashooter) == PlantResult::planted);
    int arm_drops = 0;
    bool paused = false, dead = false;
    for(int frame = 0; frame < 1800 && ! dead; ++frame)
    {
        game.tick();
        for(const auto& effect : game.zombie_effects())
        {
            if(effect.active && effect.type == ZombieEffectType::arm && effect.remaining == effect_duration(effect.type))
            {
                ++arm_drops;
                assert(game.zombies()[0].active && game.zombies()[0].arm_lost && game.zombies()[0].health == 3);
                if(! paused)
                {
                    auto effects = game.zombie_effects();
                    int x = game.zombies()[0].x;
                    game.toggle_pause(); advance(game, 100);
                    for(int i = 0; i < zombie_effect_capacity; ++i)
                    { assert(game.zombie_effects()[i].remaining == effects[i].remaining); }
                    assert(game.zombies()[0].x == x);
                    game.toggle_pause(); paused = true;
                }
            }
            if(effect.active && effect.type == ZombieEffectType::death)
            {
                assert(effect.remaining == effect_duration(effect.type));
                assert(! game.zombies()[0].active && game.kills() == 1 && game.lifetime_kills(ZombieType::normal) == 1);
                dead = true;
            }
        }
    }
    assert(dead && paused && arm_drops == 1);
    advance(game, effect_duration(ZombieEffectType::death));
    for(const auto& effect : game.zombie_effects()) { assert(! effect.active); }
    assert(game.start());
    for(const auto& effect : game.zombie_effects()) { assert(! effect.active); }
    std::puts("PASS one arm drop at half body health, missing-arm state, pause, immediate kill credit, corpse lifetime and restart");
}

int finish_victory(Game& game)
{
    int effects = 0;
    int ticks = 0;
    const int elapsed = game.elapsed(), sun = game.sun();
    const auto progress = game.progress();
    while(game.victory_settling())
    {
        assert(++ticks <= 160 && game.phase() == Phase::won && ! game.reward_ready());
        auto plants = game.plants();
        auto mowers = game.mowers();
        auto shots = game.shots();
        auto particles = game.zombie_effects();
        assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::inactive);
        game.tick();
        assert(game.elapsed() == elapsed && game.sun() == sun && game.progress() == progress);
        assert(game.reward_frames() == 0);
        for(int i = 0; i < zombie_effect_capacity; ++i)
        {
            if(particles[i].active)
            {
                effects |= 4;
                assert(game.zombie_effects()[i].remaining == particles[i].remaining - 1);
            }
        }
        for(int i = 0; i < rows * columns; ++i)
        {
            const auto& before = plants[i];
            const auto& after = game.plants()[i];
            if(before.active && before.health == 0 &&
               (before.type == PlantType::cherry || before.type == PlantType::potato_mine))
            {
                effects |= 1;
                assert(after.timer == before.timer - 1 && after.active == (after.timer > 0));
            }
            else { assert(after.active == before.active && after.timer == before.timer); }
        }
        for(int i = 0; i < rows; ++i)
        {
            const auto& before = mowers[i];
            const auto& after = game.mowers()[i];
            if(before.moving)
            {
                effects |= 2;
                assert(after.x == before.x + 3 * scale);
                assert(after.moving == (after.x <= 252 * scale));
            }
            else { assert(after.x == before.x && ! after.moving); }
        }
        for(int i = 0; i < shot_capacity; ++i) { assert(game.shots()[i].x == shots[i].x); }
    }
    return effects;
}

void playable_victory()
{
    Game game;
    assert(! game.start(1));
    game.select_level(1);
    assert(game.level() == 0);
    assert(game.start());
    bool queued_overlap_seen = false;
    int finishing_effects = 0;
    std::array<bool, static_cast<int>(ZombieEffectType::count)> effects_seen{};
    std::array<uint32_t, zombie_kind_count> campaign_kills{};
    for(int level = 0; level < level_count; ++level)
    {
        assert(game.level() == level);
        const Game level_start = game;
        for(int strategy = 0; strategy < 11; ++strategy)
        {
            game = level_start;
            if(strategy >= 5 && game.unlocked_plants() == plant_count)
            {
                assert(game.prepare_level()); advance(game, preview_ready_frame); game.choose_seeds();
                assert(game.toggle_seed(PlantType::potato_mine) && game.toggle_seed(strategy == 7 || strategy == 10 ? PlantType::squash : (strategy == 9 ? PlantType::tallnut : PlantType::repeater)));
                if(strategy >= 9) { assert(game.toggle_seed(PlantType::walnut) && game.toggle_seed(strategy == 9 ? PlantType::repeater : PlantType::tallnut)); }
                if(strategy == 8) { assert(game.toggle_seed(PlantType::peashooter) && game.toggle_seed(PlantType::squash)); }
                game.begin_level(); advance(game, preview_pan_frames);
            }
            int frames = 0;
            int previous_progress = 0;
            int started = 0;
            int hordes = 0;
            int final_warnings = 0;
            int cleared = 0;
            int grave_spawns = 0;
            bool cone_seen = false;
            bool newspaper_seen = false;
            bool bucket_seen = false;
            bool door_seen = false;
            bool overlap_seen = false;
            while(game.phase() == Phase::playing && frames++ < 60000)
            {
                auto previous_zombies = game.zombies();
                int previous_group = game.current_group();
                int previous_size = game.horde_active() ? game.level_spec().wave_size(game.wave()) : game.level_spec().normal_size(game.wave());
                defend(game, strategy);
                game.tick();
                for(const auto& effect : game.zombie_effects())
                { if(effect.active) { effects_seen[static_cast<int>(effect.type)] = true; } }
                for(const auto& zombie : game.zombies())
                { if(zombie.active && zombie.arm_lost) { assert(zombie.health <= arm_loss_health(zombie.type)); } }
                for(int i = 0; i < zombie_capacity; ++i)
                {
                    const auto& zombie = game.zombies()[i];
                    bool new_spawn = zombie.active && (! previous_zombies[i].active || previous_zombies[i].group != zombie.group);
                    queued_overlap_seen |= new_spawn && zombie.group < game.current_group();
                }
                if(previous_group > 0 && game.current_group() != previous_group)
                {
                    int survivors = 0;
                    for(const auto& zombie : game.zombies())
                    { survivors += zombie.active && zombie.group == previous_group; }
                    assert(survivors <= previous_size / 2);
                    overlap_seen |= survivors > 0;
                    assert(game.next_wave_frames() == 0);
                }
                assert(game.sun() >= 0);
                int progress = game.wave_progress();
                assert(progress >= previous_progress && progress <= game.total_waves() * 100);
                previous_progress = progress;
                started += game.events().wave_started;
                hordes += game.events().horde_started;
                final_warnings += game.events().final_wave_approaching;
                if(game.events().final_wave_approaching)
                {
                    assert(game.final_wave_approaching());
                    assert(! game.horde_active() && game.next_wave_frames() == 0);
                    int base = game.level_spec().wave_base + game.total_waves() * 2;
                    assert(game.level_spec().wave_size(game.wave()) >= base * 3 / 2);
                }
                if(game.events().horde_started)
                {
                    assert(game.wave_progress() == game.wave() * 100);
                    assert(game.horde_entered(game.wave()));
                    bool flag = false;
                    for(const auto& z : game.zombies())
                    { flag |= z.active && z.type == ZombieType::flag && z.x >= 239 * scale; }
                    assert(flag);
                }
                cleared += game.events().wave_cleared;
                grave_spawns += game.events().tombstone_spawns;
                if(game.events().wave_cleared && game.phase() == Phase::playing)
                { assert(game.next_wave_frames() == 0); }
                for(const auto& zombie : game.zombies())
                {
                    if(! zombie.active) { continue; }
                    if(! game.horde_active() && zombie.group == game.current_group()) { assert(zombie.type == ZombieType::normal); }
                    cone_seen |= zombie.type == ZombieType::cone;
                    newspaper_seen |= zombie.type == ZombieType::newspaper;
                    bucket_seen |= zombie.type == ZombieType::bucket;
                    door_seen |= zombie.type == ZombieType::screen_door;
                    if(zombie.type == ZombieType::newspaper) { assert(game.wave() >= game.level_spec().newspaper_first_wave); }
                }
            }
            int expected_kills = 0;
            for(int wave = 1; wave <= game.total_waves(); ++wave)
            { expected_kills += game.level_spec().normal_size(wave) + game.level_spec().wave_size(wave); }
            expected_kills += grave_spawns;
            std::printf("Defense: %s phase=%d waves=%d kills=%d time=%ds\n",
                        game.level_spec().name, int(game.phase()), game.wave(), game.kills(), frames / 60);
            if(game.phase() != Phase::won && strategy < 10) { continue; }
            assert(game.phase() == Phase::won);
            uint32_t recorded = 0;
            for(int i = 0; i < zombie_kind_count; ++i)
            {
                auto total = game.lifetime_kills(static_cast<ZombieType>(i));
                assert(total >= campaign_kills[i]);
                assert(total - campaign_kills[i] == uint32_t(game.zombie_roster()[i] + (i == 0 ? grave_spawns : 0)));
                recorded += total - campaign_kills[i];
                campaign_kills[i] = total;
            }
            assert(recorded == uint32_t(game.kills()));
            assert(game.wave() == game.total_waves() && game.kills() == expected_kills);
            assert(game.enemies_left() == 0 && game.wave_progress() == game.total_waves() * 100);
            assert(started == game.total_waves() && hordes == game.total_waves() && cleared == game.total_waves());
            assert(overlap_seen && final_warnings == 1);
            assert(cone_seen == appears_in_level(game.level_spec(), ZombieType::cone) && newspaper_seen == appears_in_level(game.level_spec(), ZombieType::newspaper));
            assert(bucket_seen == game.level_spec().bucketheads && door_seen == game.level_spec().screen_doors);
            assert(game.available_plants() == (level < 5 ? level + 1 : visible_seed_count));
            for(int slot = 0; slot < plant_count; ++slot)
            {
                assert(seed_slot(seed_order[slot]) == slot);
                if(game.plant_available(seed_order[slot])) { assert(game.plant_unlocked(seed_order[slot])); }
                for(int other = slot + 1; other < game.available_plants(); ++other)
                { if(slot < game.available_plants()) { assert(game.seed_at(slot) != game.seed_at(other)); } }
            }
            assert(game.unlocked_plant() == (level + 1 < plant_count ? seed_order[level + 1] : PlantType::count));
            finishing_effects |= finish_victory(game);
            { assert(! game.reward_ready()); advance(game, reward_ready_frame); assert(game.reward_ready()); }
            if(level + 1 < level_count) { game.open_level_select(); assert(game.start()); }
            break;
        }
    }
    for(auto total : campaign_kills) { assert(total > 0); }
    assert(queued_overlap_seen); // Advancing a group never discards its remaining queued zombies.
    std::printf("Victory finishing effects: %d\n", finishing_effects);
    for(auto kind : {ZombieEffectType::death, ZombieEffectType::newspaper_death, ZombieEffectType::arm,
                     ZombieEffectType::cone, ZombieEffectType::newspaper, ZombieEffectType::bucket, ZombieEffectType::door,
                     ZombieEffectType::flag, ZombieEffectType::flag_death})
    { assert(effects_seen[static_cast<int>(kind)]); }
    assert(finishing_effects & 4); // Last-kill corpses finish before reward timing starts.
    assert(game.progress().completed == (uint32_t(1) << level_count) - 1);
    game.return_to_title();
    assert(game.unlocked_level() == 9 && game.unlocked_plants() == plant_count);
    game.open_level_select();
    game.select_level(-level_count);
    assert(game.level() == 0 && game.start());
    assert(game.unlocked_level() == 9 && game.available_plants() == 1);
    assert(game.plant(0, 0, PlantType::walnut) == PlantResult::inactive);
    while(game.phase() == Phase::playing) { defend(game); game.tick(); }
    assert(game.phase() == Phase::won && game.unlocked_plant() == PlantType::count);
    assert(game.lifetime_kills(ZombieType::flag) == campaign_kills[3] + 3);
    assert(game.lifetime_kills(ZombieType::normal) == campaign_kills[0] + uint32_t(level_rosters[0][0]));
    assert(! game.reward_ready());
    finish_victory(game);
    advance(game, reward_ready_frame);
    assert(game.reward_ready());
    std::puts("PASS full campaign, level unlocks, mine unlock, replay, and session retention");
}

void wave_feedback_and_active_pause()
{
    Game game;
    game.start();
    assert(game.next_wave_frames() == 900 && game.enemies_left() == 0);
    assert(game.wave_progress() == 0);
    advance(game, 899);
    assert(game.next_wave_frames() == 1);
    game.tick();
    assert(game.wave() == 1 && game.enemies_left() == 3 && game.next_wave_frames() == 0);
    assert(game.events().wave_started && game.wave_progress() == 0);
    int x = game.zombies()[0].x;
    int elapsed = game.elapsed();
    game.toggle_pause();
    advance(game, 1000);
    assert(! game.events().wave_started && ! game.events().shot && ! game.events().hit);
    assert(game.zombies()[0].x == x && game.elapsed() == elapsed);
    assert(game.plant(0, 0, PlantType::peashooter) == PlantResult::inactive);
    game.toggle_pause();
    game.tick();
    assert(game.zombies()[0].x < x);
    std::puts("PASS preparation countdown, enemies remaining, pause during active combat");
}

void chewing_tracks_damage()
{
    Game game;
    game.load_progress(Progress::all_unlocked());
    game.start(2);
    assert(game.plant(1, 8, PlantType::walnut) == PlantResult::planted);
    advance(game, 900);
    assert(! game.events().bite);
    int bites = 0;
    int previous_bite = -1;
    for(int frame = 0; frame < 500; ++frame)
    {
        int health = game.plants()[cell_index(1, 8)].health;
        game.tick();
        bool damaged = game.plants()[cell_index(1, 8)].health < health;
        assert(game.events().bite == damaged);
        if(damaged || previous_bite >= 0)
        {
            bool eating = false;
            for(const auto& zombie : game.zombies())
            { eating |= zombie.active && zombie.eating; }
            assert(eating);
        }
        if(damaged)
        {
            if(previous_bite >= 0) { assert(frame - previous_bite == 40); }
            previous_bite = frame;
            ++bites;
        }
    }
    assert(bites > 5);
    game.toggle_pause();
    advance(game, 100);
    assert(! game.events().bite);
    game.toggle_pause();
    assert(game.shovel(1, 8));
    for(int frame = 0; frame < 100; ++frame)
    {
        game.tick();
        assert(! game.events().bite);
        for(const auto& zombie : game.zombies()) { assert(! zombie.eating); }
    }
    std::puts("PASS chewing cues match plant damage, bite cadence, pause, and target removal");
}
}

int main()
{
    Progress fresh;
    assert(fresh.valid() && fresh.unlocked == 1);
    for(int unlocked = 1; unlocked <= level_count; ++unlocked)
    {
        Progress progress{(uint32_t(1) << (unlocked - 1)) - 1, unlocked, unlocked - 1};
        for(int i = 0; i < zombie_kind_count; ++i) { progress.zombie_kills[i] = uint32_t(i + 1) * 1234567u; }
        progress.zombie_kills[5] = UINT32_MAX;
        auto record = encode_progress(progress, 123);
        Progress loaded;
        assert(decode_progress(record, loaded) && loaded == progress);
        for(int byte = 0; byte < int(record.size()); ++byte)
        {
            auto corrupt = record;
            corrupt[byte] ^= 1;
            assert(! decode_progress(corrupt, loaded));
        }
        Game game;
        game.load_progress(progress);
        game.open_level_select();
        game.select_level(level_count);
        assert(game.level() == 9);
        if(unlocked < level_count) { assert(! game.start()); }
        game.select_level(-level_count);
        assert(game.start() && game.level() == 0);
        assert(game.progress().unlocked == unlocked);
    }
    LegacySaveRecord legacy{};
    put_save_word(legacy, 0, 0x31535653);
    legacy[4] = 1; legacy[5] = 4; legacy[6] = 2;
    put_save_word(legacy, 8, 7);
    put_save_word(legacy, 12, 99);
    put_save_word(legacy, 20, save_checksum(legacy));
    Progress migrated;
    assert(decode_legacy_progress(legacy, migrated));
    assert(migrated.unlocked == 4 && migrated.selected == 2 && migrated.completed == 7);
    for(auto count : migrated.zombie_kills) { assert(count == 0); }
    Progress restored;
    assert(decode_progress(encode_progress(migrated, 100), restored) && restored == migrated);
    Game saturated;
    Progress maximum;
    maximum.zombie_kills[0] = UINT32_MAX;
    saturated.load_progress(maximum);
    saturated.start();
    while(saturated.phase() == Phase::playing) { saturated.tick(); }
    assert(saturated.kills() > 0 && saturated.lifetime_kills(ZombieType::normal) == UINT32_MAX);
    std::puts("PASS kill counter round-trips, legacy save conversion, saturation, and original zombie types");
    Progress invalid{0, 11, 0};
    Progress loaded;
    assert(! decode_progress(encode_progress(invalid, 1), loaded));
    std::puts("PASS versioned progress encoding, corrupt-save rejection, locked selection, restored unlocks");
    AudioSettings audio;
    assert(audio.gain(AudioChannel::music) == 100);
    assert(! audio.adjust(AudioChannel::effects, 1));
    assert(audio.adjust(AudioChannel::master, -5));
    assert(audio.gain(AudioChannel::music) == 50);
    assert(audio.gain(AudioChannel::effects) == 50);
    assert(audio.adjust(AudioChannel::music, -20));
    assert(audio.gain(AudioChannel::music) == 0);
    assert(audio.gain(AudioChannel::effects) == 50);
    assert(! audio.adjust(AudioChannel::music, -1));
    assert(audio.adjust(AudioChannel::master, -20));
    assert(audio.gain(AudioChannel::effects) == 0);
    assert(audio.adjust(AudioChannel::music, 30));
    assert(audio.levels[1] == 10 && audio.gain(AudioChannel::music) == 0);
    almanac_page_navigation();
    automatic_small_seed_banks();
    vaults_first_plant();
    chomper_combat();
    chomper_reaches_over_wallnut();
    tutorial_lanes_and_counters();
    new_plants_and_seed_bank();
    wallnut_vault();
    scouting_and_almanac();
    reward_animation();
    wave_track_alignment();
    road_entry();
    flag_rallies();
    armor_damage();
    frost_effects();
    sun_cadence();
    economy_and_pause();
    combat();
    all_collected_suns_fly_to_bank();
    unattended_loss();
    wave_feedback_and_active_pause();
    chewing_tracks_damage();
    mine_and_newspaper();
    tombstone_rules();
    football_and_crumbling();
    explosion_lifetimes();
    zombie_damage_effects();
    playable_victory();
}
