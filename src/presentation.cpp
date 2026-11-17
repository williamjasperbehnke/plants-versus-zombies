#include "bn_regular_bg_items_reward_panel.h"
#include "sprite_catalog.h"
#include "presentation.h"
#include "almanac.h"
#include "bn_regular_bg_items_tutorial_header.h"
#include "bn_sprite_items_lesson_target.h"
#include "bn_sprite_items_almanac_scrollbar.h"
#include "actor_animation.h"
#include "menu_layout.h"
#include "bn_string.h"
#include "wave_track.h"
#include "bn_color.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_double_size_mode.h"
#include "common_variable_8x8_sprite_font.h"
#include "menu_font.h"
#include "bn_regular_bg_items_almanac.h"
#include "bn_regular_bg_items_almanac_entry.h"
#include "bn_regular_bg_items_lawn.h"
#include "bn_regular_bg_items_level_select.h"
#include "bn_regular_bg_items_loss_message.h"
#include "bn_regular_bg_items_loss_prompt.h"
#include "bn_regular_bg_items_paused.h"
#include "bn_regular_bg_items_seed_select.h"
#include "bn_regular_bg_items_title.h"
#include "bn_regular_bg_items_save_select.h"
#include "bn_regular_bg_items_save_delete.h"
#include "bn_sprite_items_almanac_cell.h"
#include "bn_sprite_items_almanac_stats.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_hud_icons.h"
#include "bn_sprite_items_level_cursor.h"
#include "bn_sprite_items_level_tiles.h"
#include "bn_sprite_items_phase_clock.h"
#include "bn_sprite_items_reward_shadow.h"
#include "bn_sprite_items_scouting_heading.h"
#include "bn_sprite_items_selection.h"
#include "bn_sprite_items_title_options.h"
#include "bn_sprite_items_title_plants.h"
#include "bn_sprite_items_volume_slider.h"
#include "bn_sprite_items_wave_bar.h"
#include "bn_sprite_items_wave_bar_short.h"

namespace garden
{
static_assert(wave_track::boundaries.size() == wave_count + 1);
static_assert(bn::sprite_items::lesson_target.tiles_item().graphics_count() == rows * columns + 2);

// Gameplay only generates the sun counter; menu text does not share these live entity pools.
constexpr int visible_packets = plant_count < visible_seed_count ? plant_count : visible_seed_count;
static_assert(rows * columns + zombie_capacity + shot_capacity + sun_capacity + rows +
              visible_packets + wave_count + tombstone_capacity + zombie_effect_capacity + 2 + 3 + 2 + 4 <= 128, "Sprite pools exceed GBA OAM capacity");

Presentation::Presentation() :
    _background(bn::regular_bg_items::lawn.create_bg(8, 48)),
    _menu(bn::regular_bg_items::title.create_bg(8, 48)),
    _text(menu_font),
    _sun_text(common::variable_8x8_sprite_font),
    _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
    _selection(bn::sprite_items::selection.create_sprite(0, 0))
{
    _background.set_priority(3);
    _background.set_visible(false);
    _menu->set_priority(1);
    _text.set_bg_priority(0);
    _sun_text.set_bg_priority(0);
    _cursor.set_bg_priority(0);
    _cursor.set_z_order(-10);
    _selection.set_bg_priority(0);
    _selection.set_z_order(-11);
    _cursor.set_visible(false);
    _selection.set_visible(false);
}

void Presentation::change_screen(const Game& game)
{
    Phase phase = game.phase();
    bool same_road = (phase == Phase::preview || phase == Phase::seed_select) &&
                     (_screen == Phase::preview || _screen == Phase::seed_select);
    _screen = phase;
    _tutorial_panel.reset();
    _lesson_target.reset();
    _lesson_text.clear();
    _lesson_step = TutorialStep::none;
    _text.set_palette_item(phase == Phase::almanac ? almanac_ink : menu_font.item().palette_item());
    if(phase != Phase::won && phase != Phase::lost) { _hud.clear(); }
    _last_sun = -1;
    _last_pause_choice = -1;
    for(auto& slider : _volume_sliders) { slider.reset(); }
    for(auto& plant : _title_plants) { plant.reset(); }
    _title_options.reset();
    for(auto& sprite : _gallery) { sprite.reset(); }
    _portrait.reset();
    _almanac_panel.reset();
    _almanac_scrollbar.reset();
    _entry_text.clear();
    for(auto& icon : _entry_icons) { icon.reset(); }
    _gallery_cursor.reset();
    for(auto& sprite : _bank_packets) { sprite.reset(); }
    _last_entry = -1;
    for(auto& tile : _level_tiles) { tile.reset(); }
    _level_cursor.reset();
    if(! same_road) { _title_frame = 0; }
    _preview_text.clear();
    for(auto& half : _preview_heading) { half.reset(); }
    _preview_pan = 0;
    _loss_notice_stage = 0;
    _reward_packet.reset();
    _reward_shadow.reset();
    _menu.reset();
    // The gameplay background remains its own resource. Menus use another layer
    // and independent artwork, with no copied lawn, seed bar, or gameplay HUD.
    switch(phase)
    {
    case Phase::save_select: _menu = bn::regular_bg_items::save_select.create_bg(8, 48); break;
    case Phase::title: _menu = bn::regular_bg_items::title.create_bg(8, 48); break;
    case Phase::playing: break;
    case Phase::seed_select:
        _menu = SpriteCatalog::road(game.level_spec().lanes).create_bg(-104, 48);
        _almanac_panel = bn::regular_bg_items::seed_select.create_bg(8, 48);
        _almanac_panel->set_priority(1);
        break;
    case Phase::preview: _menu = SpriteCatalog::road(game.level_spec().lanes).create_bg(136, 48); break;
    case Phase::almanac: _menu = bn::regular_bg_items::almanac.create_bg(8, 48); break;
    case Phase::level_select: _menu = bn::regular_bg_items::level_select.create_bg(8, 48); break;
    case Phase::paused: _menu = bn::regular_bg_items::paused.create_bg(8, 48); break;
    case Phase::won:
    case Phase::lost: break;
    default: break;
    }
    bool playing = phase == Phase::playing;
    bool reward = phase == Phase::won || phase == Phase::lost;
    if(playing) { _background.set_item(SpriteCatalog::lawn(game.level_spec().lanes)); }
    _background.set_visible(playing || reward);
    if(_menu) { _menu->set_priority(phase == Phase::seed_select ? 2 : 1); }
    _cursor.set_visible(playing);
    _selection.set_visible(playing);
    if(! playing && ! reward) { clear_entities(); }
}

void Presentation::clear_entities()
{
    _world.clear();
    for(auto& sprite : _seed_packets) { sprite.reset(); }
    for(auto& sprite : _seed_arrows) { sprite.reset(); }
    for(auto& sprite : _wave_bar) { sprite.reset(); }
    _wave_head.reset();
    _phase_icon.reset();
    _feedback_icon.reset();
}

void Presentation::draw_wave_bar(const Game& game)
{
    // Ease one shared progress value so the fill and zombie head remain aligned.
    int target = game.wave_progress() * 256;
    if(_bar_progress < 0 || target < _bar_progress) { _bar_progress = target; }
    int difference = target - _bar_progress;
    int step = difference / 8;
    if(step < 8) { step = 8; }
    if(step > 512) { step = 512; }
    _bar_progress += difference < step ? difference : step;
    int progress = _bar_progress / 256;
    int total = game.total_waves();
    const auto& item = total == 3 ? bn::sprite_items::wave_bar_short : bn::sprite_items::wave_bar;
    for(int i = 0; i < wave_count; ++i)
    {
        auto& segment = _wave_bar[i];
        if(i >= total) { segment.reset(); continue; }
        int frames_per_state = 168 + total * 2;
        int state = ! game.horde_entered(i + 1) ? 0 : (game.horde_remaining(i + 1) ? 1 : 2);
        int frame = state * frames_per_state + wave_track::frame_offset(i, total);
        if(game.wave() > i) { frame += 1 + wave_track::fill(i, progress, total); }
        if(! segment)
        {
            segment = item.create_sprite(
                281 - wave_track::boundary(i, total) - wave_track::sprite_width / 2 - 120,
                wave_track::sprite_height / 2 - 80, frame);
            segment->set_bg_priority(0);
        }
        segment->set_tiles(item.tiles_item(), frame);
    }
    if(! _wave_head)
    {
        _wave_head = bn::sprite_items::hud_icons.create_sprite(0, wave_track::head_y - 80, 5);
        _wave_head->set_bg_priority(0);
        _wave_head->set_z_order(-2);
    }
    _wave_head->set_x(wave_track::display_position(progress, total) - 120);
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
        constexpr int duration = 900;
        int clock = (duration - remaining) * 16 / duration;
        if(! _phase_icon) { _phase_icon = bn::sprite_items::phase_clock.create_sprite(-105, -68, clock); }
        _phase_icon->set_item(bn::sprite_items::phase_clock, clock);
        _phase_icon->set_bg_priority(0);
    }
    else { _phase_icon.reset(); }
}

void Presentation::draw_hud(const Game& game, const ViewState& view)
{
    // All packets report their readiness, even when another seed is selected.
    for(int i = 0; i < plant_count; ++i)
    {
        if(i >= game.available_plants() || i < _seed_page || i >= _seed_page + visible_seed_count)
        { _seed_packets[i].reset(); continue; }
        auto type = game.seed_at(i);
        int remaining = game.cooldown(type);
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame(remaining, spec(type).recharge, game.sun() >= spec(type).cost);
        auto& packet = _seed_packets[i];
        if(! packet)
        {
            packet = item.create_sprite(
                seed_layout::center(i - _seed_page) - 120, seed_layout::center_y - 80, frame);
            packet->set_bg_priority(0);
        }
        packet->set_tiles(item.tiles_item(), frame);
    }
    for(int i = 0; i < 2; ++i)
    {
        if(game.available_plants() <= visible_seed_count) { _seed_arrows[i].reset(); continue; }
        if(! _seed_arrows[i])
        {
            _seed_arrows[i] = bn::sprite_items::hud_icons.create_sprite(i == 0 ? -76 : 108, -42, 12 + i);
            _seed_arrows[i]->set_bg_priority(0);
        }
    }
    if(game.sun() != _last_sun)
    {
        _last_sun = game.sun();
        _hud.clear();
        _sun_text.generate(-116, -35, bn::to_string<8>(game.sun()), _hud);
    }
    int bounce = view.feedback == Feedback::collected ? view.feedback_frames / 8 : 0;
    for(auto& sprite : _hud) { sprite.set_y(-35 - bounce); }
}

void Presentation::draw_title(const ViewState& view)
{
    constexpr int centers[] = {35, 88};
    for(int i = 0; i < 2; ++i)
    {
        if(! _title_plants[i])
        {
            _title_plants[i] = bn::sprite_items::title_plants.create_sprite(centers[i] - 120, 17);
            _title_plants[i]->set_bg_priority(0);
        }
        int pose = (_title_frame / 24 + i) % 2;
        _title_plants[i]->set_tiles(bn::sprite_items::title_plants.tiles_item(), i * 2 + pose);
    }
    if(! _title_options)
    {
        _title_options = bn::sprite_items::title_options.create_sprite(56, 16);
        _title_options->set_bg_priority(0);
    }
    _title_options->set_tiles(bn::sprite_items::title_options.tiles_item(), static_cast<int>(view.title_choice));
    _title_frame = (_title_frame + 1) % 144;
}

void Presentation::draw_saves(const ViewState& view)
{
    int key = view.save_slot + view.save_delete * 4 + view.delete_confirmation * 8 + view.delete_yes * 16;
    for(const auto& save : view.saves) { key = key * 256 + save.occupied * 121 + save.unlocked * 11 + save.cleared; }
    if(_last_entry == key) { return; }
    _last_entry = key;
    _hud.clear();
    _text.set_left_alignment();
    _text.set_bg_priority(0);
    if(view.delete_confirmation)
    {
        // A full panel replaces the list so confirmation has an unambiguous focus.
        _menu->set_item(bn::regular_bg_items::save_delete);
        _text.set_center_alignment();
        _text.set_palette_item(almanac_ink);
        _text.generate(0, -26, bn::string<20>("SAVE ") + bn::to_string<2>(view.save_slot + 1), _hud);
        bn::string<32> summary = bn::to_string<2>(view.saves[view.save_slot].cleared);
        summary += "/10 LEVELS CLEARED";
        _text.generate(0, -10, summary, _hud);
        _text.generate(0, 11, "THIS SAVE WILL BE ERASED", _hud);
        for(int i = 0; i < 2; ++i)
        {
            _text.set_palette_item(view.delete_yes == bool(i) ? menu_gold_ink : almanac_ink);
            _text.generate(i ? 34 : -34, 30, i ? "DELETE" : "CANCEL", _hud);
        }
        _text.set_palette_item(menu_font.item().palette_item());
        _text.set_center_alignment();
        _text.generate(0, 61, "LEFT/RIGHT: SELECT", _hud);
        _text.generate(0, 71, "A: CONFIRM   B: CANCEL", _hud);
    }
    else
    {
        _menu->set_item(bn::regular_bg_items::save_select);
        for(int i = 0; i < save_slot_count; ++i)
        {
            bool selected = i == view.save_slot;
            const auto& save = view.saves[i];
            int y = -38 + i * 34;
            _text.set_palette_item(selected ? menu_gold_ink : almanac_ink);
            _text.generate(-102, y, bn::string<16>("SAVE ") + bn::to_string<2>(i + 1), _hud);
            _text.set_palette_item(almanac_ink);
            bn::string<32> summary = "EMPTY SLOT";
            if(save.occupied)
            {
                summary = bn::to_string<2>(save.cleared); summary += "/10 CLEARED  1-";
                summary += bn::to_string<2>(save.unlocked);
            }
            _text.generate(-102, y + 11, summary, _hud);
            for(int action = 0; action < (save.occupied ? 2 : 1); ++action)
            {
                _text.set_palette_item(selected && view.save_delete == bool(action) ? menu_gold_ink : almanac_ink);
                _text.generate(action ? 68 : 0, y + 5, action ? "DELETE" : (save.occupied ? "CONTINUE" : "NEW GAME"), _hud);
            }
        }
    }
    _text.set_palette_item(menu_font.item().palette_item());
    _text.set_left_alignment();
}

void Presentation::draw_level_select(const Game& game)
{
    for(int i = 0; i < levels_per_world; ++i)
    {
        int level = game.world_spec().first_level + i;
        int state = ! game.level_unlocked(level) ? 0 : (game.level_completed(level) ? 2 : 1);
        if(! _level_tiles[i])
        {
            _level_tiles[i] = bn::sprite_items::level_tiles.create_sprite(menu_layout::level_x(i), menu_layout::level_y(i));
            _level_tiles[i]->set_bg_priority(0);
        }
        _level_tiles[i]->set_tiles(bn::sprite_items::level_tiles.tiles_item(), state * 10 + i);
    }
    if(! _level_cursor)
    {
        _level_cursor = bn::sprite_items::level_cursor.create_sprite(0, 0);
        _level_cursor->set_bg_priority(0);
        _level_cursor->set_z_order(-10);
    }
    int local = game.level() % levels_per_world;
    _level_cursor->set_position(menu_layout::level_x(local), menu_layout::level_y(local));
    if(_last_pause_choice != game.level())
    {
        _last_pause_choice = game.level();
        _hud.clear();
        _text.set_center_alignment();
        bn::string<48> label(game.level_spec().name);
        if(! game.level_unlocked(game.level())) { label += " - Locked"; }
        _text.generate(0, 54, label, _hud);
        _text.set_left_alignment();
    }
}

void Presentation::draw_preview(const Game& game)
{
    int time = game.preview_frames();
    int remaining = time < preview_pan_frames ? preview_pan_frames - time : 0;
    int pan = 240 - 240 * remaining * remaining / (preview_pan_frames * preview_pan_frames);
    if(game.preview_returning()) { pan = 240 - pan; }
    draw_road(game, pan, true);
}

void Presentation::draw_road(const Game& game, int pan, bool labels)
{
    _menu->set_x(136 - pan);
    _world.preview(game, pan, labels ? 0 : 2);
    // Staggered, fixed road positions keep small rosters near the yard and avoid parade rows.
    constexpr std::array<PacketPosition, zombie_type_count> crowd{{
        {300, 94}, {318, 136}, {349, 104}, {370, 131},
        {398, 91}, {419, 138}, {447, 103}, {463, 137}
    }};
    std::array<PacketPosition, zombie_type_count> positions{};
    int slot_position = 0;
    for(int i = 0; i < zombie_type_count; ++i)
    {
        if(! game.zombie_roster()[i]) { continue; }
        positions[i] = crowd[slot_position++];
    }
    int slot = 0;
    for(int i = 0; i < zombie_type_count; ++i)
    {
        if(! game.zombie_roster()[i]) { continue; }
        int x = positions[i].x - pan;
        int y = positions[i].y;
        draw_actor(_gallery[slot], true, x, y,
              zombie_display_frame(static_cast<ZombieType>(i), _title_frame + slot * 11), 160 - y);
        _gallery[slot]->set_bg_priority(labels ? 0 : 2);
        ++slot;
    }
    ++_title_frame;
    if(! labels) { return; }
    for(int half = 0; half < 2; ++half)
    {
        auto& sprite = _preview_heading[half];
        if(! sprite)
        {
            sprite = bn::sprite_items::scouting_heading.create_sprite(0, -48, half);
            sprite->set_bg_priority(0);
        }
        sprite->set_x(328 + half * 64 - pan - 120);
    }
    if(_preview_text.empty())
    {
        _text.set_center_alignment();
        _text.generate(240, -70, game.level_spec().name, _preview_text);
        _text.generate(240, -36, game.unlocked_plants() <= visible_seed_count ?
                       "A/START: BEGIN   B: BACK" : "A/START: SEEDS   B: BACK", _preview_text);
        for(int i = 0; i < zombie_type_count; ++i)
        {
            int count = game.zombie_roster()[i];
            if(! count) { continue; }
            bn::string<16> label("x"); label += bn::to_string<4>(count);
            _text.generate(positions[i].x - 120, positions[i].y + 17 - 80, label, _preview_text);
        }
        _text.set_left_alignment();
    }
    for(auto& label : _preview_text) { label.set_x(label.x() + _preview_pan - pan); }
    _preview_pan = pan;

}

void Presentation::draw_seed_select(const Game& game, const ViewState& view)
{
    draw_road(game, 240, false);
    bool error = view.seed_error_frames > 0;
    int key = view.seed_choice | (error ? 1 << 20 : 0);
    for(int i = 0; i < plant_count; ++i)
    {
        auto type = seed_order[i];
        bool unlocked = game.plant_unlocked(type);
        bool selected = game.plant_available(type);
        if(selected) { key |= 1 << (i + 4); }
        int x = menu_layout::seed_x(i);
        int y = menu_layout::seed_y(i);
        if(! unlocked) { _seed_packets[i].reset(); continue; }
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame((selected || _bank_packets[i]) ? spec(type).recharge : 0, spec(type).recharge, unlocked);
        if(! _seed_packets[i]) { _seed_packets[i] = item.create_sprite(x - 120, y - 80, frame); }
        _seed_packets[i]->set_item(item, frame);
        _seed_packets[i]->set_bg_priority(0);

    }
    draw_seed_bank(game);
    _selection.set_visible(true);
    _selection.set_position(menu_layout::seed_x(view.seed_choice) - 120, menu_layout::seed_y(view.seed_choice) - 80);
    _selection.set_tiles(bn::sprite_items::selection.tiles_item(), error && view.seed_error_frames / 8 % 2 ? 1 : 0);
    if(key == _last_entry) { return; }
    _last_entry = key;
    _hud.clear();
    _text.set_center_alignment();
    bn::string<32> count; count += bn::to_string<2>(game.available_plants()); count += "/"; count += bn::to_string<2>(game.required_seeds());
    _text.generate(95, -71, "SEEDS", _hud);
    _text.generate(95, -59, count, _hud);
    if(error)
    {
        _text.set_palette_item(menu_error_ink);
        _text.generate(-38, -30, "SELECT 6 SEEDS", _hud);
        _text.set_palette_item(menu_font.item().palette_item());
    }
    else
    {
        _text.generate(-38, -30, game.plant_unlocked(seed_order[view.seed_choice]) ?
                       plant_entries[static_cast<int>(seed_order[view.seed_choice])].name : "", _hud);
    }
    _text.generate(0, 75, "A:PICK START:PLAY B:BACK", _hud);
    _text.set_left_alignment();
}

void Presentation::draw_seed_bank(const Game& game)
{
    auto approach = [](int current, int target)
    {
        int delta = target - current;
        int step = delta / 4;
        if(! step && delta) { step = delta > 0 ? 1 : -1; }
        return current + step;
    };
    for(int i = 0; i < plant_count; ++i)
    {
        auto type = seed_order[i];
        bool selected = game.plant_available(type);
        auto& packet = _bank_packets[i];
        auto& position = _packet_positions[i];
        if(! selected && ! packet) { continue; }
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame(0, spec(type).recharge, true);
        if(! packet)
        {
            position = {menu_layout::seed_x(i), menu_layout::seed_y(i)};
            packet = item.create_sprite(0, 0, frame);
            packet->set_bg_priority(0);
            packet->set_z_order(-5);
        }
        int x = selected ? menu_layout::bank_x(game.bank_slot(type)) : menu_layout::seed_x(i);
        int y = selected ? menu_layout::bank_y : menu_layout::seed_y(i);
        position = {approach(position.x, x), approach(position.y, y)};
        packet->set_position(position.x - 120, position.y - 80);
        if(! selected && position.x == x && position.y == y) { packet.reset(); }
    }
}

void Presentation::draw_almanac(const Game& game, const ViewState& view)
{
    int index = view.almanac_entry;
    int count = view.almanac_zombies ? zombie_type_count : plant_count;
    int page = index / 6;
    int local = index % 6;
    int key = index + (view.almanac_zombies ? plant_count : 0) + (view.almanac_details ? 64 : 0) + view.almanac_scroll * 128;
    _menu->set_priority(3);
    auto draw_entry = [this, &view](auto& sprite, int entry, int x, int y, int clock, int portrait_scale = 1)
    {
        int frame = view.almanac_zombies ? zombie_display_frame(static_cast<ZombieType>(entry), clock) :
                                          plant_display_frame(seed_order[entry], clock);
        // The door's visible bounds center at x=28 in a padded 64px frame.
        if(view.almanac_zombies && entry == static_cast<int>(ZombieType::screen_door)) { x += 4 * portrait_scale; }
        draw_actor(sprite, true, x, y, frame);
    };
    for(int i = 0; i < 6; ++i)
    {
        int entry = page * 6 + i;
        if(entry >= count) { _gallery[i].reset(); continue; }
        draw_entry(_gallery[i], entry, 54 + i % menu_layout::almanac_columns * 66, 62 + i / menu_layout::almanac_columns * 46,
                   (view.almanac_zombies ? _title_frame / 2 : _title_frame) + i * 7);
        _gallery[i]->set_bg_priority(2);
    }
    if(! _gallery_cursor)
    {
        _gallery_cursor = bn::sprite_items::almanac_cell.create_sprite(0, 0);
        _gallery_cursor->set_bg_priority(2);
        _gallery_cursor->set_z_order(-10);
    }
    _gallery_cursor->set_position(54 + local % menu_layout::almanac_columns * 66 - 120, 64 + local / menu_layout::almanac_columns * 46 - 80);
    if(view.almanac_details)
    {
        if(! _almanac_panel)
        {
            _almanac_panel = bn::regular_bg_items::almanac_entry.create_bg(8, 48);
            _almanac_panel->set_priority(1);
        }
        draw_entry(_portrait, index, 64 + menu_layout::almanac_detail_shift, 80, view.almanac_zombies ? _title_frame / 2 : _title_frame, 2);
        _portrait->set_bg_priority(0);
        _portrait->set_scale(2);
        _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
    }
    else
    {
        _almanac_panel.reset();
        _almanac_scrollbar.reset();
        for(auto& icon : _entry_icons) { icon.reset(); }
        _portrait.reset();
        _entry_text.clear();
    }
    ++_title_frame;
    if(_last_entry == key) { return; }
    _last_entry = key;
    _hud.clear();
    _entry_text.clear();
    _text.set_center_alignment();
    _text.set_bg_priority(2);
    _text.generate(0, -42, view.almanac_zombies ? (page ? "ZOMBIES 2/2" : "ZOMBIES 1/2") : (page ? "PLANTS 2/2" : "PLANTS 1/2"), _hud);
    for(int i = 0; i < 6; ++i)
    {
        int entry_index = page * 6 + i;
        if(entry_index >= count) { continue; }
        const auto& entry = view.almanac_zombies ? zombie_entries[entry_index] : plant_entries[static_cast<int>(seed_order[entry_index])];
        _text.generate(54 + i % menu_layout::almanac_columns * 66 - 120, i / menu_layout::almanac_columns * 46, entry.name, _hud);
    }
    _text.set_bg_priority(0);
    if(view.almanac_details)
    {
        const auto& entry = view.almanac_zombies ? zombie_entries[index] : plant_entries[static_cast<int>(seed_order[index])];
        _text.generate(menu_layout::almanac_detail_shift, -46, entry.name, _entry_text);
        auto stat = [this](int slot, int frame, int x, int y, const auto& value)
        {
            x += menu_layout::almanac_detail_shift;
            _entry_icons[slot] = bn::sprite_items::almanac_stats.create_sprite(x - 120, y - 80, frame);
            _entry_icons[slot]->set_bg_priority(0);
            _text.set_left_alignment();
            _text.generate(x + 12 - 120, y - 80, value, _entry_text);
            _text.set_center_alignment();
        };
        for(auto& icon : _entry_icons) { icon.reset(); }
        if(view.almanac_zombies)
        {
            const auto& definition = spec(static_cast<ZombieType>(index));
            stat(0, 1, 78, 120, bn::to_string<4>(definition.health + definition.shield_health));
            stat(1, 3, 143, 120, bn::to_string<12>(game.lifetime_kills(static_cast<ZombieType>(index))));
        }
        else
        {
            const auto& definition = spec(seed_order[index]);
            stat(0, 0, 50, 120, bn::to_string<4>(definition.cost));
            stat(1, 1, 107, 120, bn::to_string<4>(definition.health));
            int frames = definition.recharge;
            bn::string<16> recharge = bn::to_string<4>(frames / 60);
            if(frames % 60) { recharge += ".5"; }
            recharge += "s";
            stat(2, 2, 164, 120, recharge);
        }
        AlmanacDescription description(entry.description);
        int visible = description.size() < almanac_visible_lines ? description.size() : almanac_visible_lines;
        _text.set_left_alignment();
        for(int i = 0; i < visible; ++i)
        { _text.generate(-9 + menu_layout::almanac_detail_shift, -5 - (visible - 1) * 5 + i * 10, description[view.almanac_scroll + i], _entry_text); }
        int limit = almanac_scroll_limit(view.almanac_zombies, index);
        if(limit)
        {
            _almanac_scrollbar = bn::sprite_items::almanac_scrollbar.create_sprite(89, -5, almanac_scrollbar_frame(description.size(), view.almanac_scroll));
            _almanac_scrollbar->set_bg_priority(0);
        }
        else { _almanac_scrollbar.reset(); }

    }
    _text.set_left_alignment();
}

void Presentation::draw_menu(const Game& game, const ViewState& view)
{
    if(game.phase() == Phase::save_select) { draw_saves(view); return; }
    if(game.phase() == Phase::title) { draw_title(view); return; }
    if(game.phase() == Phase::preview) { draw_preview(game); return; }
    if(game.phase() == Phase::seed_select) { draw_seed_select(game, view); return; }
    if(game.phase() == Phase::almanac) { draw_almanac(game, view); return; }
    if(game.phase() == Phase::level_select) { draw_level_select(game); return; }
    if(game.phase() == Phase::won) { draw_reward(game); return; }
    if(game.phase() == Phase::lost) { draw_loss(game); return; }
    int choice = static_cast<int>(view.pause_choice);
    if(_last_sun != -1 && choice == _last_pause_choice && _last_audio == view.audio.key()) { return; }
    _last_audio = view.audio.key();
    _last_sun = game.sun();
    _last_pause_choice = choice;
    _hud.clear();
    _text.set_center_alignment();
    if(game.phase() == Phase::paused)
    {
        _text.set_left_alignment();
        constexpr const char* choices[] = {"Resume", "Restart", "Levels", "Master", "Music", "Effects"};
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
    _text.set_left_alignment();
}

void Presentation::put_entities_behind_overlay()
{
    auto behind_panel = [](auto& pool)
    {
        for(auto& sprite : pool) { if(sprite) { sprite->set_bg_priority(2); } }
    };
    _world.behind_overlay();
    behind_panel(_seed_packets);
    behind_panel(_seed_arrows);
    behind_panel(_wave_bar);
    for(auto* sprite : {&_wave_head, &_phase_icon, &_feedback_icon})
    { if(*sprite) { (*sprite)->set_bg_priority(2); } }
    for(auto& sprite : _hud) { sprite.set_bg_priority(2); }
}

void Presentation::draw_loss(const Game& game)
{
    _world.render(game);
    put_entities_behind_overlay();
    int frame = game.loss_frames();
    int stage = frame < 30 ? 0 : (game.loss_ready() ? 2 : 1);
    if(stage != _loss_notice_stage)
    {
        _loss_notice_stage = stage;
        const auto& item = stage == 2 ? bn::regular_bg_items::loss_prompt : bn::regular_bg_items::loss_message;
        _menu = item.create_bg(8, 48);
        _menu->set_priority(1);
    }
    if(_menu)
    {
        int drop = frame < 42 ? (frame - 42) * 2 : (frame < 48 ? 2 : 0);
        _menu->set_y(48 + drop);
    }
}

void Presentation::draw_reward(const Game& game)
{
    int frame = game.reward_frames();
    if(frame == 0) { _world.render(game); }
    if(game.victory_settling()) { return; }
    int type = static_cast<int>(game.unlocked_plant());
    if(! _reward_packet)
    {
        _reward_packet = SpriteCatalog::reward(game.unlocked_plant()).create_sprite(0, 100);
        _reward_packet->set_bg_priority(0);
        _reward_packet->set_z_order(-20);
    }
    constexpr int lift = 8;
    const auto pose = reward_pose(frame, lift + (game.unlocked_plant() == PlantType::count ? 8 : 0));
    if(frame < reward_reveal_frame)
    {
        if(! _reward_shadow)
        {
            _reward_shadow = bn::sprite_items::reward_shadow.create_sprite(0, 46);
            _reward_shadow->set_bg_priority(0);
            _reward_shadow->set_z_order(1);
        }
        _reward_shadow->set_position(pose.x - 120, 46);
        _reward_shadow->set_scale(bn::fixed(100 - pose.height) / 100);
    }
    else if(! _menu)
    {
        put_entities_behind_overlay();
        _reward_shadow.reset();
        _menu = bn::regular_bg_items::reward_panel.create_bg(8, 48);
        _menu->set_priority(1);
    }
    _reward_packet->set_position(pose.x - 120, pose.y - 80);
    _reward_packet->set_scale(bn::fixed(pose.scale + pose.squash * 2) / 50,
                              bn::fixed(pose.scale - pose.squash * 2) / 50);
    if(! game.reward_ready() || _last_sun != -1) { return; }
    _last_sun = game.sun();
    constexpr const char* descriptions[] = {"Makes more sun", "Fires peas", "Blocks zombies", "Blasts nearby foes", "Arms, then bursts", "Slows zombies", "Crushes nearby foes", "Fires two peas", "Stops pole vaulters", "Swallows whole zombies"};
    _text.set_center_alignment();
    // Move the information with the unlock sign, retaining its internal text margins.
    _text.generate(0, 58 - lift, (game.unlocked_plant() == PlantType::count ? game.level_spec().name : plant_entries[type].name), _hud);
    _text.generate(0, 70 - lift, (game.unlocked_plant() == PlantType::count ? "Garden defended!" : descriptions[type]), _hud);
    _text.set_left_alignment();
}

void Presentation::draw_tutorial(const Game& game, const ViewState& view)
{
    auto step = view.tutorial.step;
    bool active = step != TutorialStep::none;
    for(auto& segment : _wave_bar) { if(segment) { segment->set_visible(! active); } }
    if(_wave_head) { _wave_head->set_visible(! active); }
    if(_phase_icon) { _phase_icon->set_visible(! active); }
    if(! active)
    {
        _tutorial_panel.reset(); _lesson_target.reset(); _lesson_text.clear();
        _lesson_step = step;
        return;
    }
    if(! _tutorial_panel)
    {
        _tutorial_panel = bn::regular_bg_items::tutorial_header.create_bg(8, 48);
        _tutorial_panel->set_priority(1);
        _lesson_target = bn::sprite_items::lesson_target.create_sprite(0, 0);
        _lesson_target->set_bg_priority(0);
        _lesson_target->set_z_order(-9);
    }
    if(step != _lesson_step)
    {
        _lesson_step = step;
        _lesson_text.clear();
        constexpr const char* titles[] = {"", "PLANT A PEASHOOTER", "COLLECT FALLING SUN", "CLEAR THE OLD PLANTS", "PLANT A SUNFLOWER", "SUNFLOWERS MAKE MORE SUN"};
        constexpr const char* controls[] = {"", "A: PLANT IN THE HIGHLIGHTED CELL", "B: COLLECT THE GLOWING SUN", "SELECT + A: CLEAR HIGHLIGHTED CELL", "R: SUNFLOWER  A: HIGHLIGHTED CELL", "B: COLLECT YOUR FLOWER SUN"};
        _text.set_center_alignment();
        _text.generate(0, -73, titles[static_cast<int>(step)], _lesson_text);
        _text.generate(0, -63, controls[static_cast<int>(step)], _lesson_text);
        _text.set_left_alignment();
    }
    bool collecting = step == TutorialStep::collect_sun || step == TutorialStep::sunflower_sun;
    int x = cell_x(view.tutorial.column), y = cell_y(view.tutorial.row);
    bool visible = ! collecting && game.elapsed() % 60 < 40;
    if(collecting)
    {
        // Mark the collectible itself, never the sun counter in the HUD.
        for(const auto& sun : game.suns())
        {
            if(sun.active && ! sun.collecting && sun.y / scale >= 63 &&
               (step != TutorialStep::sunflower_sun || sun.from_plant))
            { x = sun.x / scale; y = sun.y / scale; visible = true; break; }
        }
    }
    _lesson_target->set_bg_priority(collecting ? 0 : 2);
    _lesson_target->set_z_order(collecting ? -9 : 3 - view.tutorial.row * 4);
    _lesson_target->set_visible(visible);
    _lesson_target->set_position(x - 120, y - 80);
    _lesson_target->set_tiles(bn::sprite_items::lesson_target.tiles_item(),
                               collecting ? rows * columns + game.elapsed() / 24 % 2 :
                                            cell_index(view.tutorial.row, view.tutorial.column));
}

void Presentation::update(const Game& game, const ViewState& view)
{
    if(game.phase() != _screen) { change_screen(game); }
    if(game.phase() != Phase::playing) { draw_menu(game, view); return; }
    int page = _seed_page < 0 ? 0 : _seed_page;
    int selected = game.bank_slot(view.cursor.seed);
    if(selected < page) { page = selected; }
    else if(selected >= page + visible_seed_count) { page = selected - visible_seed_count + 1; }
    if(page != _seed_page)
    {
        _seed_page = page;
        for(auto& packet : _seed_packets) { packet.reset(); }
    }
    const auto& cursor = view.cursor;
    bool on_feedback_tile = cell_x(cursor.column) == view.feedback_x && cell_y(cursor.row) == view.feedback_y;
    bool rejected = on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::occupied;
    int shake = rejected ? (view.feedback_frames % 4 < 2 ? -1 : 1) : 0;
    _cursor.set_position(cell_x(cursor.column) - 120 + shake, cell_y(cursor.row) - 80);
    bool blocked = game.plants()[cell_index(cursor.row, cursor.column)].active ||
                   game.tombstone_at(cursor.row, cursor.column) ||
                   game.sun() < spec(cursor.seed).cost || game.cooldown(cursor.seed) > 0;
    int tint = view.shoveling ? 2 : (blocked ? 1 : 0);
    if(on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::planted) { tint = 3; }
    _cursor.set_tiles(bn::sprite_items::cursor.tiles_item(), tint);
    _selection.set_position(seed_layout::center(game.bank_slot(cursor.seed) - _seed_page) - 120, seed_layout::center_y - 80);
    _world.render(game);
    draw_wave_bar(game);
    draw_feedback(game, view);
    draw_hud(game, view);
    draw_tutorial(game, view);
}
}
