#include "game.h"
#include "wave_track.h"
#include "audio_settings.h"
#include <cassert>
#include <cstdio>

using namespace garden;

namespace
{
void wave_track_alignment()
{
    constexpr std::array<int, 6> expected_flags{{56, 89, 123, 156, 190, 224}};
    for(int wave = 0; wave <= wave_count; ++wave)
    { assert(wave_track::position(wave * 100) == expected_flags[wave]); }
    int previous = wave_track::boundaries.front();
    for(int progress = 0; progress <= wave_count * 100; ++progress)
    {
        int filled = 0;
        for(int wave = 0; wave < wave_count; ++wave)
        { filled += wave_track::fill(wave, progress); }
        int head = wave_track::position(progress);
        assert(head == wave_track::boundaries.front() + filled);
        assert(head >= previous && head <= wave_track::boundaries.back());
        previous = head;
    }
    std::puts("PASS wave fill frontier and head align at every pixel and all five flag poles");
}

void advance(Game& game, int frames)
{
    for(int i = 0; i < frames; ++i) { game.tick(); }
}

void economy_and_pause()
{
    Game game;
    assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::inactive);
    game.start();
    assert(game.sun() == 175);
    assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::planted);
    assert(game.sun() == 125);
    assert(game.plant(0, 0, PlantType::peashooter) == PlantResult::occupied);
    assert(game.plant(1, 0, PlantType::sunflower) == PlantResult::cooling_down);
    assert(game.plant(-1, 0, PlantType::walnut) == PlantResult::inactive);
    assert(game.plant(0, columns, PlantType::walnut) == PlantResult::inactive);
    assert(game.plant(0, 1, PlantType::cherry) == PlantResult::insufficient_sun);
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
    assert(game.collect_sun() == 50); // One sunflower sun and the next sky sun.
    assert(game.shovel(0, 0));
    assert(! game.shovel(0, 0));
    assert(game.sun() == 200); // Shoveling gives no refund.
    game.start();
    assert(game.elapsed() == 0 && game.sun() == 175 && game.wave() == 0);
    for(const auto& plant : game.plants()) { assert(! plant.active); }
    std::puts("PASS economy, cooldowns, sun production, pause, shovel, restart");
}

void combat()
{
    Game bomb;
    bomb.start();
    advance(bomb, 900);
    assert(bomb.wave() == 1);
    assert(bomb.zombies()[0].active && bomb.zombies()[0].row == 1);
    assert(bomb.plant(1, 8, PlantType::cherry) == PlantResult::planted);
    advance(bomb, 65);
    assert(bomb.events().explosion && bomb.events().hit);
    assert(bomb.wave_progress() == 20); // One defeat out of five; queued enemies still count.
    assert(! bomb.zombies()[0].active);
    assert(bomb.kills() == 1);
    assert(bomb.plants()[cell_index(1, 8)].active);
    advance(bomb, 18);
    assert(! bomb.events().explosion);
    assert(! bomb.plants()[cell_index(1, 8)].active);

    Game wall;
    wall.start();
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
    game.start();
    assert(game.plant(0, 0, PlantType::sunflower) == PlantResult::planted);
    advance(game, 600);
    auto origins = game.suns();
    int tokens = 0;
    for(const auto& sun : origins) { tokens += sun.active; }
    assert(tokens == 3); // A sunflower token and two sky tokens at different positions.
    assert(game.collect_sun() == tokens * 25);
    assert(game.sun() == 200);
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
    assert(game.sun() == 200 && game.collect_sun() == 0);

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
    assert(game.kills() >= rows);
    for(const auto& mower : game.mowers()) { assert(! mower.available); }
    int elapsed = game.elapsed();
    advance(game, 100);
    assert(game.elapsed() == elapsed);
    std::puts("PASS one-use mowers, unattended loss, terminal state freezes");
}

void defend(Game& game)
{
    game.collect_sun();
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
    for(int column = 2; column <= 4; ++column)
    {
        for(int row = 0; row < rows; ++row)
        {
            if(! game.plants()[cell_index(row, column)].active)
            { game.plant(row, column, PlantType::peashooter); }
        }
    }
}

void playable_victory()
{
    Game game;
    game.start();
    int frames = 0;
    int previous_progress = 0;
    int started_waves = 0;
    int cleared_waves = 0;
    while(game.phase() == Phase::playing && frames++ < 60000)
    {
        defend(game);
        game.tick();
        assert(game.sun() >= 0);
        int progress = game.wave_progress();
        assert(progress >= previous_progress && progress <= wave_count * 100);
        previous_progress = progress;
        started_waves += game.events().wave_started;
        cleared_waves += game.events().wave_cleared;
    }
    std::printf("Defense run: phase=%d waves=%d kills=%d time=%ds\n",
                int(game.phase()), game.wave(), game.kills(), frames / 60);
    assert(game.phase() == Phase::won);
    assert(game.wave() == wave_count);
    assert(game.kills() == 45);
    assert(game.enemies_left() == 0);
    assert(game.wave_progress() == wave_count * 100);
    assert(started_waves == wave_count && cleared_waves == wave_count);
    for(const auto& zombie : game.zombies()) { assert(! zombie.active); }
    std::puts("PASS full five-wave victory through public player actions");
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
    assert(game.wave() == 1 && game.enemies_left() == 5 && game.next_wave_frames() == 0);
    assert(game.events().wave_started && game.wave_progress() == 0);
    int x = game.zombies()[0].x;
    int warning = game.wave_warning();
    int elapsed = game.elapsed();
    game.toggle_pause();
    advance(game, 1000);
    assert(! game.events().wave_started && ! game.events().shot && ! game.events().hit);
    assert(game.zombies()[0].x == x && game.wave_warning() == warning && game.elapsed() == elapsed);
    assert(game.plant(0, 0, PlantType::peashooter) == PlantResult::inactive);
    game.toggle_pause();
    game.tick();
    assert(game.zombies()[0].x < x);
    std::puts("PASS preparation countdown, enemies remaining, pause during active combat");
}

void chewing_tracks_damage()
{
    Game game;
    game.start();
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
    AudioSettings audio;
    assert(audio.gain(AudioChannel::music) == 50);
    assert(! audio.adjust(AudioChannel::effects, 1));
    assert(audio.adjust(AudioChannel::master, -5));
    assert(audio.gain(AudioChannel::music) == 25);
    assert(audio.gain(AudioChannel::effects) == 50);
    assert(audio.adjust(AudioChannel::music, -20));
    assert(audio.gain(AudioChannel::music) == 0);
    assert(audio.gain(AudioChannel::effects) == 50);
    assert(! audio.adjust(AudioChannel::music, -1));
    assert(audio.adjust(AudioChannel::master, -20));
    assert(audio.gain(AudioChannel::effects) == 0);
    assert(audio.adjust(AudioChannel::music, 30));
    assert(audio.levels[1] == 10 && audio.gain(AudioChannel::music) == 0);
    wave_track_alignment();
    economy_and_pause();
    combat();
    all_collected_suns_fly_to_bank();
    unattended_loss();
    wave_feedback_and_active_pause();
    chewing_tracks_damage();
    playable_victory();
}
