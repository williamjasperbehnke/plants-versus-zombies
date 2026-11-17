#include "input_controller.h"
#include "almanac.h"
#include <cstring>
#include <string>
#include <cassert>
#include <cstdio>

using namespace garden;

namespace
{
void frame(Game& game, InputController& controller, InputFrame input = {})
{
    controller.update(input);
    game.tick(controller.holding_waves(), controller.holding_sky_sun());
    controller.finish_frame();
}

void press(Game& game, InputController& controller, Button button)
{
    frame(game, controller);
    auto mask = button_mask(button);
    frame(game, controller, {mask, mask});
}

void advance(Game& game, InputController& controller, int count)
{
    for(int i = 0; i < count; ++i) { frame(game, controller); }
}

void wrapped_descriptions()
{
    WrappedText<5, 4> words("  one  two\nthree ");
    assert(words.size() == 3 && ! words.truncated());
    assert(std::strcmp(words[0], "one") == 0 && std::strcmp(words[2], "three") == 0);
    WrappedText<5, 4> token("abcdefghijk");
    assert(token.size() == 3 && std::strcmp(token[0], "abcde") == 0 && std::strcmp(token[2], "k") == 0);
    WrappedText<5, 1> clipped("a longer word");
    assert(clipped.truncated() && std::strcmp(clipped[0], "a...") == 0);
    assert((WrappedText<5, 2>(" \t").size() == 0));
    for(int group = 0; group < 2; ++group)
    {
        int count = group ? zombie_type_count : plant_count;
        for(int index = 0; index < count; ++index)
        {
            const auto& entry = almanac_entry(group, index);
            AlmanacDescription lines(entry.description);
            assert(! lines.truncated());
            std::string joined;
            for(int line = 0; line < lines.size(); ++line)
            {
                assert(std::strlen(lines[line]) <= 24);
                if(line) { joined += ' '; }
                joined += lines[line];
            }
            assert(joined == entry.description);
        }
    }
    std::puts("PASS allocation-free word wrapping, long words, whitespace, truncation and complete Almanac copy");
}

void tutorial_progression()
{
    Game game; TutorialGuide guide;
    assert(game.start()); guide.begin(game);
    assert(guide.prompt().step == TutorialStep::plant_pea);
    assert(guide.allows_plant(2, 0, PlantType::peashooter));
    assert(! guide.allows_plant(2, 1, PlantType::peashooter));
    assert(! guide.allows_collect() && ! guide.allows_shovel(2, 0));
    for(int frame = 0; frame < 1200; ++frame) { game.tick(guide.active()); }
    assert(game.wave() == 0 && game.next_wave_frames() == 900);
    assert(game.plant(2, 0, PlantType::peashooter) == PlantResult::planted);
    guide.update(game, false);
    assert(guide.prompt().step == TutorialStep::collect_sun);
    assert(guide.allows_collect() && ! guide.allows_plant(2, 1, PlantType::peashooter));
    guide.update(game, true);
    assert(guide.prompt().step == TutorialStep::none);
    assert(level_rosters[0][0] == 4 && level_rosters[0][3] == 3);
    game.load_progress(Progress::all_unlocked());
    assert(game.start(1)); guide.begin(game);
    assert(guide.prompt().step == TutorialStep::shovel && guide.holds_sky_sun());
    for(int frame = 0; frame < 180; ++frame) { game.tick(); }
    for(const auto& shot : game.shots()) { assert(! shot.active); }
    for(const auto& cell : tutorial_weeds)
    {
        guide.update(game, false);
        assert(guide.prompt().row == cell.row && guide.prompt().column == cell.column);
        assert(guide.allows_shovel(cell.row, cell.column));
        assert(! guide.allows_shovel(cell.row, cell.column + 1));
        assert(! guide.allows_plant(cell.row, cell.column, PlantType::peashooter));
        assert(game.plants()[cell_index(cell.row, cell.column)].wilted);
        assert(game.plant(cell.row, cell.column, PlantType::sunflower) == PlantResult::occupied);
        assert(game.shovel(cell.row, cell.column));
    }
    guide.update(game, false);
    assert(guide.prompt().step == TutorialStep::plant_sunflower);
    assert(guide.prompt().column == 0 && guide.allows_plant(2, 0, PlantType::sunflower));
    assert(! guide.allows_plant(2, 2, PlantType::sunflower));
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::planted);
    assert(! game.plants()[cell_index(2, 0)].wilted);
    guide.update(game, false);
    assert(guide.prompt().step == TutorialStep::sunflower_sun && guide.holds_sky_sun());
    guide.update(game, false);
    assert(guide.prompt().step == TutorialStep::sunflower_sun);
    for(int frame = 0; frame < spec(PlantType::sunflower).action_delay; ++frame) { game.tick(); }
    assert(game.collect_sun(true) == 25);
    assert(game.collect_sun() == 0); // The old sky sun has expired before the flower produces.
    guide.update(game, true);
    assert(guide.prompt().step == TutorialStep::none);
    assert(guide.allows_collect() && guide.allows_plant(1, 5, PlantType::peashooter));
    assert(! guide.holds_sky_sun());
    Progress completed = Progress::all_unlocked(); completed.completed = 3;
    game.load_progress(completed); assert(game.start(0)); guide.begin(game);
    assert(guide.prompt().step == TutorialStep::none);
    std::puts("PASS early tutorial stages, seven-zombie roster, inert shovel props, replacement and replay dismissal");
}

void tutorial_sun_timing()
{
    Game game; assert(game.start());
    for(int i = 0; i < 1000; ++i) { game.tick(true, true); }
    for(const auto& sun : game.suns()) { assert(! sun.active); }
    for(int i = 0; i < 179; ++i) { game.tick(true, false); }
    for(const auto& sun : game.suns()) { assert(! sun.active); }
    game.tick(true, false);
    assert(game.collect_sun() == 25); // Exactly one scheduled drop, with no catch-up burst.
    game.load_progress(Progress::all_unlocked()); assert(game.start(1));
    assert(game.shovel(2, 2));
    assert(game.plant(2, 2, PlantType::sunflower) == PlantResult::planted);
    for(int i = 0; i < 540; ++i) { game.tick(true, true); }
    for(const auto& sun : game.suns()) { if(sun.active) { assert(sun.from_plant); } }
    assert(game.collect_sun(true) == 25 && game.collect_sun() == 0);
    std::puts("PASS tutorial holds sky timer without catch-up drops while Sunflowers still produce sun");
}

void early_sun_and_expiry()
{
    Game game; game.load_progress(Progress::all_unlocked()); assert(game.start(1));
    assert(game.plant(2, 0, PlantType::sunflower) == PlantResult::planted);
    game.hasten_sun(tutorial_sun_delay, true);
    for(int i = 0; i < tutorial_sun_delay - 1; ++i) { game.tick(true, true); }
    assert(game.collect_sun(true) == 0);
    game.tick(true, true);
    int slot = -1;
    for(int i = 0; i < sun_capacity; ++i) { if(game.suns()[i].active) { slot = i; } }
    assert(slot >= 0 && game.suns()[slot].lifetime == sun_lifetime);
    assert(game.plants()[cell_index(2, 0)].timer == spec(PlantType::sunflower).action_delay);
    game.toggle_pause();
    for(int i = 0; i < 100; ++i) { game.tick(); }
    assert(game.suns()[slot].lifetime == sun_lifetime);
    game.toggle_pause();
    for(int i = 0; i < sun_lifetime - 1; ++i) { game.tick(true, true); }
    assert(game.suns()[slot].active && game.suns()[slot].lifetime == 1);
    game.tick(true, true);
    assert(! game.suns()[slot].active && game.collect_sun(true) == 0);
    int until_next = spec(PlantType::sunflower).action_delay - sun_lifetime;
    for(int i = 0; i < until_next - 1; ++i) { game.tick(true, true); }
    assert(game.collect_sun(true) == 0);
    game.tick(true, true);
    assert(game.collect_sun(true) == 25);
    std::puts("PASS accelerated first tutorial sun, normal repeat timing, pause and expiry before the next production");
}

void guided_input()
{
    Game game; InputController controller(game);
    press(game, controller, Button::a);
    press(game, controller, Button::a);
    advance(game, controller, preview_ready_frame);
    press(game, controller, Button::a);
    advance(game, controller, preview_pan_frames);
    assert(game.phase() == Phase::playing && controller.holding_waves());
    advance(game, controller, 1000);
    for(const auto& sun : game.suns()) { assert(! sun.active); }
    assert(controller.holding_sky_sun());
    press(game, controller, Button::right);
    press(game, controller, Button::a);
    assert(! game.plants()[cell_index(2, 1)].active && game.sun() == 175);
    press(game, controller, Button::left);
    press(game, controller, Button::a);
    assert(game.plants()[cell_index(2, 0)].active);
    assert(! controller.holding_sky_sun());
    auto shovel = uint16_t(button_mask(Button::select) | button_mask(Button::a));
    frame(game, controller); frame(game, controller, {shovel, shovel});
    assert(game.plants()[cell_index(2, 0)].active);
    advance(game, controller, tutorial_sun_delay);
    assert(game.wave() == 0 && game.next_wave_frames() == 900);
    bool sun_ready = false;
    for(const auto& sun : game.suns()) { sun_ready |= sun.active && ! sun.collecting; }
    assert(sun_ready);
    press(game, controller, Button::b);
    assert(! controller.holding_waves());
    advance(game, controller, 900);
    assert(game.wave() == 1);
    std::puts("PASS guided input requires the marked cell and sun collection, blocks shoveling, and delays combat");
}

void repeat_timing()
{
    KeyRepeat repeat;
    for(int held = 1; held <= 46; ++held)
    {
        bool expected = held == 1 || (held >= 21 && (held - 21) % 5 == 0);
        assert(repeat.update(true) == expected);
    }
    assert(! repeat.update(false) && repeat.update(true));
    KeyRepeat seeds;
    for(int held = 1; held <= 41; ++held)
    { assert(seeds.update(true, 20, 10) == (held == 1 || held == 30 || held == 40)); }
    std::puts("PASS independent navigation/seed repeats and release reset");
}

void almanac_navigation()
{
    Game game; InputController controller(game);
    press(game, controller, Button::down);
    assert(controller.view().title_choice == TitleChoice::almanac);
    press(game, controller, Button::a);
    assert(game.phase() == Phase::almanac);
    for(int i = 0; i < 6; ++i) { press(game, controller, Button::right); }
    assert(controller.view().almanac_entry == 0 && ! controller.view().almanac_zombies);
    frame(game, controller);
    uint16_t both = button_mask(Button::r) | button_mask(Button::right);
    frame(game, controller, {both, both});
    assert(controller.view().almanac_entry == 7);
    const auto& cues = controller.audio_commands();
    assert(cues.size() == 2 && cues[0] == AudioCommand::navigate && cues[1] == AudioCommand::navigate);
    press(game, controller, Button::a);
    assert(controller.view().almanac_details);
    int entry = controller.view().almanac_entry;
    int limit = almanac_scroll_limit(false, entry);
    for(int i = 0; i < 40; ++i) { press(game, controller, Button::down); }
    assert(controller.view().almanac_scroll == limit && controller.view().almanac_entry == entry);
    press(game, controller, Button::left); press(game, controller, Button::r);
    assert(controller.view().almanac_entry == entry && ! controller.view().almanac_zombies);
    for(int i = 0; i < 40; ++i) { press(game, controller, Button::up); }
    assert(controller.view().almanac_scroll == 0);
    press(game, controller, Button::b);
    assert(! controller.view().almanac_details && game.phase() == Phase::almanac);
    press(game, controller, Button::b);
    assert(game.phase() == Phase::title);
    std::puts("PASS controller title/Almanac navigation, page bounds, detail back and ordered cues");
}

void pause_audio_and_restart()
{
    Game game; assert(game.start(0)); InputController controller(game);
    press(game, controller, Button::start);
    assert(game.phase() == Phase::paused);
    int elapsed = game.elapsed();
    for(int i = 0; i < 3; ++i) { press(game, controller, Button::down); }
    assert(controller.view().pause_choice == PauseChoice::master && game.elapsed() == elapsed);
    press(game, controller, Button::left);
    const auto& cues = controller.audio_commands();
    assert(cues.size() == 2 && cues[0] == AudioCommand::apply_settings && cues[1] == AudioCommand::navigate);
    assert(controller.view().audio.levels[0] == 9);
    press(game, controller, Button::up); press(game, controller, Button::up);
    assert(controller.view().pause_choice == PauseChoice::restart);
    press(game, controller, Button::a);
    assert(game.phase() == Phase::preview && controller.view().audio.levels[0] == 9);
    assert(controller.audio_commands().size() == 1 && controller.audio_commands()[0] == AudioCommand::reset_combat);
    std::puts("PASS pause freezes simulation, mixer command ordering, restart and preserved volume");
}

void seed_selection_and_repeat()
{
    Game game; game.load_progress(Progress::all_unlocked());
    assert(game.start(9) && game.prepare_level()); InputController controller(game);
    advance(game, controller, preview_ready_frame);
    press(game, controller, Button::a); assert(game.phase() == Phase::seed_select);
    press(game, controller, Button::a); assert(game.available_plants() == 5);
    press(game, controller, Button::start);
    assert(game.phase() == Phase::seed_select && controller.view().seed_error_frames == 119);
    assert(controller.audio_commands()[0] == AudioCommand::error && game.elapsed() == 0);
    press(game, controller, Button::a); assert(game.available_plants() == 6 && controller.view().seed_error_frames == 0);
    press(game, controller, Button::start);
    assert(game.preview_returning() && controller.view().cursor.seed == game.seed_at(0));
    advance(game, controller, preview_pan_frames);
    assert(game.phase() == Phase::playing);
    int initial = game.bank_slot(controller.view().cursor.seed);
    for(int held = 1; held <= 40; ++held)
    {
        auto mask = button_mask(Button::r);
        frame(game, controller, {mask, uint16_t(held == 1 ? mask : 0)});
    }
    assert(game.bank_slot(controller.view().cursor.seed) == (initial + 3) % 6);
    std::puts("PASS seed-selection errors, confirmation, return pan and held shoulder cycling");
}

void placement_feedback()
{
    Game game; assert(game.start(0)); InputController controller(game);
    press(game, controller, Button::a);
    assert(game.plants()[cell_index(2, 0)].active && game.sun() == 75);
    assert(controller.view().feedback == Feedback::planted && controller.view().feedback_frames == 29);
    assert(controller.view().feedback_x == cell_x(0) && controller.view().feedback_y == cell_y(2));
    frame(game, controller, {button_mask(Button::start), button_mask(Button::start)});
    assert(game.phase() == Phase::paused && controller.view().feedback_frames == 29);
    advance(game, controller, 20);
    assert(controller.view().feedback_frames == 29);
    press(game, controller, Button::b);
    assert(game.phase() == Phase::playing && controller.view().feedback_frames == 28);
    std::puts("PASS planting feedback lifetime, coordinates and pause/resume timing");
}
}

int main()
{
    wrapped_descriptions();
    tutorial_progression();
    tutorial_sun_timing();
    early_sun_and_expiry();
    guided_input();
    repeat_timing();
    almanac_navigation();
    pause_audio_and_restart();
    seed_selection_and_repeat();
    placement_feedback();
}
