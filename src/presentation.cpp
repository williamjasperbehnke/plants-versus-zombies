#include "presentation.h"
#include "bn_string.h"
#include "wave_track.h"
#include "bn_sprite_items_actors.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_selection.h"
#include "bn_sprite_items_seed_bars.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_wave_bar.h"
#include "bn_sprite_items_hud_icons.h"
#include "bn_sprite_items_phase_clock.h"
#include "bn_sprite_items_volume_slider.h"
#include "bn_regular_bg_items_title.h"
#include "bn_regular_bg_items_lawn.h"
#include "bn_regular_bg_items_paused.h"
#include "bn_regular_bg_items_victory.h"
#include "bn_regular_bg_items_defeat.h"
#include "common_variable_8x8_sprite_font.h"

namespace garden
{
static_assert(wave_track::boundaries.size() == wave_count + 1);

static_assert(rows * columns + zombie_capacity + shot_capacity + sun_capacity + rows +
              plant_count + wave_count + 2 + 3 + 20 <= 128, "Sprite pools exceed GBA OAM capacity");

Presentation::Presentation() :
    _background(bn::regular_bg_items::lawn.create_bg(8, 48)),
    _menu(bn::regular_bg_items::title.create_bg(8, 48)),
    _text(common::variable_8x8_sprite_font),
    _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
    _selection(bn::sprite_items::selection.create_sprite(0, 0))
{
    _background.set_priority(3);
    _background.set_visible(false);
    _menu->set_priority(1);
    _text.set_bg_priority(0);
    _cursor.set_bg_priority(0);
    _cursor.set_z_order(-10);
    _selection.set_bg_priority(0);
    _cursor.set_visible(false);
    _selection.set_visible(false);
}

void Presentation::change_screen(Phase phase)
{
    _screen = phase;
    _hud.clear();
    _last_sun = -1;
    _last_pause_choice = -1;
    for(auto& slider : _volume_sliders) { slider.reset(); }
    _menu.reset();
    // The gameplay background remains its own resource. Menus use another layer
    // and independent artwork, with no copied lawn, seed bar, or gameplay HUD.
    switch(phase)
    {
    case Phase::title: _menu = bn::regular_bg_items::title.create_bg(8, 48); break;
    case Phase::playing: break;
    case Phase::paused: _menu = bn::regular_bg_items::paused.create_bg(8, 48); break;
    case Phase::won: _menu = bn::regular_bg_items::victory.create_bg(8, 48); break;
    case Phase::lost: _menu = bn::regular_bg_items::defeat.create_bg(8, 48); break;
    default: break;
    }
    bool playing = phase == Phase::playing;
    _background.set_visible(playing);
    if(_menu) { _menu->set_priority(1); }
    _cursor.set_visible(playing);
    _selection.set_visible(playing);
    if(! playing) { clear_entities(); }
}

void Presentation::clear_entities()
{
    for(auto& sprite : _plants) { sprite.reset(); }
    for(auto& sprite : _zombies) { sprite.reset(); }
    for(auto& sprite : _shots) { sprite.reset(); }
    for(auto& sprite : _suns) { sprite.reset(); }
    for(auto& sprite : _mowers) { sprite.reset(); }
    for(auto& sprite : _seed_bars) { sprite.reset(); }
    for(auto& sprite : _wave_bar) { sprite.reset(); }
    _wave_head.reset();
    _phase_icon.reset();
    _feedback_icon.reset();
}

void Presentation::actor(bn::optional<bn::sprite_ptr>& sprite, bool active,
                         int x, int y, int frame, int depth)
{
    if(! active) { sprite.reset(); return; }
    const auto& item = frame == 9 ? bn::sprite_items::explosion : bn::sprite_items::actors;
    const int graphics_index = frame == 9 ? 0 : frame;
    if(! sprite)
    {
        sprite = item.create_sprite(x - 120, y - 80, graphics_index);
        sprite->set_bg_priority(2);
    }
    // Butano shares atlas tiles/palettes between matching sprites.
    sprite->set_item(item, graphics_index);
    sprite->set_position(x - 120, y - 80);
    sprite->set_z_order(depth);
}

void Presentation::draw_entities(const Game& game)
{
    for(int i = 0; i < rows * columns; ++i)
    {
        const auto& plant = game.plants()[i];
        int idle_pose = (game.elapsed() / 24 + i) % 2;
        int frame = static_cast<int>(plant.type);
        if(plant.type == PlantType::sunflower && idle_pose) { frame = 12; }
        if(plant.type == PlantType::peashooter && idle_pose) { frame = 20; }
        if(plant.type == PlantType::peashooter && plant.timer > spec(plant.type).action_delay - 6) { frame = 13; }
        if(plant.type == PlantType::cherry && idle_pose) { frame = 24; }
        if(plant.type == PlantType::cherry && plant.health == 0) { frame = 9; }
        if(plant.type == PlantType::walnut && plant.health <= spec(plant.type).health / 3) { frame = 11; }
        else if(plant.type == PlantType::walnut && plant.health <= spec(plant.type).health * 2 / 3) { frame = 10; }
        if(plant.type == PlantType::walnut && idle_pose)
        { frame = frame == 2 ? 21 : frame + 12; }
        int recoil = plant.type == PlantType::peashooter && plant.timer > spec(plant.type).action_delay - 6 ? 1 : 0;
        actor(_plants[i], plant.active && (plant.flash % 4 < 2),
              cell_x(i % columns) - recoil, cell_y(i / columns) - 1, frame, 2);
        if(frame == 9 && _plants[i])
        {
            _plants[i]->set_tiles(bn::sprite_items::explosion.tiles_item(), (18 - plant.timer) / 6);
            _plants[i]->set_z_order(-3);
        }
    }
    for(int i = 0; i < zombie_capacity; ++i)
    {
        const auto& zombie = game.zombies()[i];
        int bob = (game.elapsed() / 14 + i) % 2;
        int pose = (game.elapsed() / 12 + i) % 2;
        bool chewing = zombie.eating;
        if(chewing)
        {
            bob = 0;
            pose = zombie.bite_timer < 12 ? 1 : 0;
        }
        bool cone = zombie.armored && zombie.health > 6;
        int frame = chewing ? 16 + pose * 2 : (pose ? 14 : 4);
        if(cone) { ++frame; }
        actor(_zombies[i], zombie.active && zombie.flash % 4 < 2,
              zombie.x / scale, cell_y(zombie.row) + bob - 2, frame, 1);
    }
    for(int i = 0; i < shot_capacity; ++i)
    {
        const auto& shot = game.shots()[i];
        actor(_shots[i], shot.active, shot.x / scale, cell_y(shot.row) - 3, 6, 0);
    }
    for(int i = 0; i < sun_capacity; ++i)
    {
        const auto& sun = game.suns()[i];
        bool visible = sun.active && (sun.collecting || sun.lifetime > 120 || game.elapsed() % 20 < 14);
        actor(_suns[i], visible, sun.x / scale, sun.y / scale, 7, sun.collecting ? -11 : -2);
        if(_suns[i]) { _suns[i]->set_bg_priority(sun.collecting ? 0 : 2); }
    }
    for(int i = 0; i < rows; ++i)
    {
        const auto& mower = game.mowers()[i];
        actor(_mowers[i], mower.available || mower.moving, mower.x / scale, cell_y(i), 8, -1);
    }
}

void Presentation::draw_wave_bar(const Game& game)
{
    int progress = game.wave_progress();
    for(int i = 0; i < wave_count; ++i)
    {
        int frame = wave_track::frame_offsets[i] + wave_track::fill(i, progress);
        auto& segment = _wave_bar[i];
        if(! segment)
        {
            segment = bn::sprite_items::wave_bar.create_sprite(
                wave_track::boundaries[i] + wave_track::sprite_width / 2 - 120,
                wave_track::sprite_height / 2 - 80, frame);
            segment->set_bg_priority(0);
        }
        segment->set_tiles(bn::sprite_items::wave_bar.tiles_item(), frame);
    }
    if(! _wave_head)
    {
        _wave_head = bn::sprite_items::hud_icons.create_sprite(0, wave_track::head_y - 80, 5);
        _wave_head->set_bg_priority(0);
        _wave_head->set_z_order(-2);
    }
    _wave_head->set_x(wave_track::position(progress) - 120);
}

void Presentation::draw_feedback(const Game& game, const ViewState& view)
{
    int frame = -1;
    int x = view.feedback_x;
    int y = view.feedback_y - 6;
    if(view.shoveling) { frame = 2; x = cell_x(view.cursor.column); y = cell_y(view.cursor.row) - 6; }
    else if(view.feedback_frames > 0)
    {
        switch(view.feedback)
        {
        case Feedback::planted: frame = 1; y -= (30 - view.feedback_frames) / 5; break;
        case Feedback::removed: frame = 2; break;
        case Feedback::occupied: frame = 0; break;
        case Feedback::cooling_down:
        {
            int recharge = spec(view.feedback_seed).recharge;
            int stage = (recharge - game.cooldown(view.feedback_seed)) * 3 / recharge;
            if(stage > 2) { stage = 2; }
            frame = stage == 0 ? 3 : 9 + stage;
            x = 64 + static_cast<int>(view.feedback_seed) * 42;
            y = 36;
            break;
        }
        case Feedback::insufficient_sun:
            // The packet border pulses red; the bank's sun icon stays still.
            break;
        case Feedback::collected: break; // Each sun animates in its own entity slot.
        default: break;
        }
    }
    if(frame < 0) { _feedback_icon.reset(); }
    else
    {
        if(! _feedback_icon)
        {
            _feedback_icon = bn::sprite_items::hud_icons.create_sprite(x - 120, y - 80, frame);
            _feedback_icon->set_bg_priority(0);
            _feedback_icon->set_z_order(-12);
        }
        _feedback_icon->set_tiles(bn::sprite_items::hud_icons.tiles_item(), frame);
        _feedback_icon->set_position(x - 120, y - 80);
    }
    bool rejected = view.feedback_frames > 0 && view.cursor.seed == view.feedback_seed &&
        (view.feedback == Feedback::insufficient_sun || view.feedback == Feedback::cooling_down);
    int selection_frame = 0;
    if(rejected && view.feedback_frames % 8 < 4)
    { selection_frame = view.feedback == Feedback::cooling_down ? 2 : 1; }
    _selection.set_tiles(bn::sprite_items::selection.tiles_item(), selection_frame);

    int remaining = game.next_wave_frames();
    if(remaining > 0)
    {
        int duration = game.wave() == 0 ? 900 : 300;
        int clock = (duration - remaining) * 16 / duration;
        if(! _phase_icon) { _phase_icon = bn::sprite_items::phase_clock.create_sprite(-105, -68, clock); }
        _phase_icon->set_item(bn::sprite_items::phase_clock, clock);
        _phase_icon->set_bg_priority(0);
    }
    else if(game.wave_warning() > 30)
    {
        int flag = game.wave() == wave_count ? 8 : (game.elapsed() / 10 % 2 ? 6 : 9);
        if(! _phase_icon) { _phase_icon = bn::sprite_items::hud_icons.create_sprite(-105, -68, flag); }
        _phase_icon->set_item(bn::sprite_items::hud_icons, flag);
        _phase_icon->set_bg_priority(0);
    }
    else { _phase_icon.reset(); }
}

void Presentation::draw_hud(const Game& game, const ViewState& view)
{
    // All packets report their readiness, even when another seed is selected.
    for(int i = 0; i < plant_count; ++i)
    {
        auto type = static_cast<PlantType>(i);
        int remaining = game.cooldown(type);
        int frame = (spec(type).recharge - remaining) * 16 / spec(type).recharge;
        if(! remaining && game.sun() < spec(type).cost) { frame = 17; }
        auto& bar = _seed_bars[i];
        if(! bar)
        {
            bar = bn::sprite_items::seed_bars.create_sprite(74 + i * 42 - 120, -31, frame);
            bar->set_bg_priority(0);
        }
        bar->set_tiles(bn::sprite_items::seed_bars.tiles_item(), frame);
    }
    if(game.sun() != _last_sun)
    {
        _last_sun = game.sun();
        _hud.clear();
        _text.generate(-116, -35, bn::to_string<8>(game.sun()), _hud);
    }
    int bounce = view.feedback == Feedback::collected ? view.feedback_frames / 8 : 0;
    for(auto& sprite : _hud) { sprite.set_y(-35 - bounce); }
}

void Presentation::draw_menu(const Game& game, const ViewState& view)
{
    if(game.phase() == Phase::title) { return; }
    int choice = static_cast<int>(view.pause_choice);
    if(_last_sun != -1 && choice == _last_pause_choice && _last_audio == view.audio.key()) { return; }
    _last_audio = view.audio.key();
    _last_sun = game.sun();
    _last_pause_choice = choice;
    _hud.clear();
    _text.set_center_alignment();
    bn::string<64> stats;
    stats += "Wave ";
    stats += bn::to_string<4>(game.wave());
    stats += "/5  -  ";
    stats += bn::to_string<4>(game.kills());
    stats += " defeated";
    if(game.phase() == Phase::paused)
    {
        _text.set_left_alignment();
        constexpr const char* choices[] = {"Resume", "Restart", "Title", "Master", "Music", "Effects"};
        constexpr int positions[] = {54, 68, 82, 102, 116, 130};
        for(int i = 0; i < static_cast<int>(PauseChoice::count); ++i)
        {
            _text.generate(-60, positions[i] - 80, choices[i], _hud);
        }
        _text.generate(-78, positions[choice] - 80, ">", _hud);
        for(int i = 0; i < 3; ++i)
        {
            if(! _volume_sliders[i])
            {
                _volume_sliders[i] = bn::sprite_items::volume_slider.create_sprite(48, positions[i + 3] - 80);
                _volume_sliders[i]->set_bg_priority(0);
            }
            _volume_sliders[i]->set_tiles(bn::sprite_items::volume_slider.tiles_item(), view.audio.levels[i]);
        }
    }
    else
    {
        _text.generate(0, -20, stats, _hud);
        bn::string<32> time;
        int seconds = game.elapsed() / 60;
        time += "Time  ";
        time += bn::to_string<8>(seconds / 60);
        time += ":";
        if(seconds % 60 < 10) { time += "0"; }
        time += bn::to_string<4>(seconds % 60);
        _text.generate(0, 0, time, _hud);
    }
    _text.set_left_alignment();
}

void Presentation::update(const Game& game, const ViewState& view)
{
    if(game.phase() != _screen) { change_screen(game.phase()); }
    if(game.phase() != Phase::playing) { draw_menu(game, view); return; }
    const auto& cursor = view.cursor;
    bool on_feedback_tile = cell_x(cursor.column) == view.feedback_x && cell_y(cursor.row) == view.feedback_y;
    bool rejected = on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::occupied;
    int shake = rejected ? (view.feedback_frames % 4 < 2 ? -1 : 1) : 0;
    _cursor.set_position(cell_x(cursor.column) - 120 + shake, cell_y(cursor.row) - 80);
    bool blocked = game.plants()[cell_index(cursor.row, cursor.column)].active ||
                   game.sun() < spec(cursor.seed).cost || game.cooldown(cursor.seed) > 0;
    int tint = view.shoveling ? 2 : (blocked ? 1 : 0);
    if(on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::planted) { tint = 3; }
    _cursor.set_tiles(bn::sprite_items::cursor.tiles_item(), tint);
    _selection.set_position(74 + static_cast<int>(cursor.seed) * 42 - 120, -42);
    draw_entities(game);
    draw_wave_bar(game);
    draw_feedback(game, view);
    draw_hud(game, view);
}
}
