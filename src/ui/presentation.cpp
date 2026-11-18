#include "ui/presentation.h"

#include "bn_regular_bg_items_almanac.h"
#include "bn_regular_bg_items_lawn.h"
#include "bn_regular_bg_items_level_select.h"
#include "bn_regular_bg_items_loss_message.h"
#include "bn_regular_bg_items_loss_prompt.h"
#include "bn_regular_bg_items_reward_panel.h"
#include "bn_regular_bg_items_title.h"
#include "bn_regular_bg_items_tutorial_header.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_hud_icons.h"
#include "bn_sprite_items_lesson_target.h"
#include "bn_sprite_items_level_cursor.h"
#include "bn_sprite_items_level_tiles.h"
#include "bn_sprite_items_phase_clock.h"
#include "bn_sprite_items_reward_shadow.h"
#include "bn_sprite_items_scouting_heading.h"
#include "bn_sprite_items_selection.h"
#include "bn_sprite_items_title_options.h"
#include "bn_sprite_items_title_plants.h"
#include "bn_sprite_items_wave_bar.h"
#include "bn_sprite_items_wave_bar_short.h"
#include "bn_string.h"
#include "common_variable_8x8_sprite_font.h"
#include "game/animation.h"
#include "generated/wave_track.h"
#include "ui/almanac.h"
#include "ui/menu_font.h"
#include "ui/menu_layout.h"
#include "ui/sprite_catalog.h"

namespace garden {
static_assert(wave_track::boundaries.size() == wave_count + 1);
static_assert(bn::sprite_items::lesson_target.tiles_item().graphics_count() == rows * columns + 2);

// Gameplay only generates the sun counter; menu text does not share these live entity pools.
constexpr int visible_packets = plant_count < visible_seed_count ? plant_count : visible_seed_count;
static_assert(
    rows * columns + zombie_capacity + shot_capacity + sun_capacity + rows + visible_packets + wave_count +
            tombstone_capacity + zombie_effect_capacity + 2 + 3 + 2 + 4 <=
        128,
    "Sprite pools exceed GBA OAM capacity"
);

Presentation::Presentation()
    : _background(bn::regular_bg_items::lawn.create_bg(8, 48)),
      _menu(bn::regular_bg_items::title.create_bg(8, 48)), _text(menu_font),
      _sun_text(common::variable_8x8_sprite_font), _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
      _selection(bn::sprite_items::selection.create_sprite(0, 0)) {
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

void Presentation::change_screen(const Game& game) {
    Phase phase = game.phase();
    bool same_road = (phase == Phase::preview || phase == Phase::seed_select) &&
                     (_screen == Phase::preview || _screen == Phase::seed_select);
    _screen = phase;
    _tutorial_panel.reset();
    _lesson_target.reset();
    _lesson_text.clear();
    _lesson_step = TutorialStep::none;
    _text.set_palette_item(menu_font.item().palette_item());
    if (phase != Phase::won && phase != Phase::lost) {
        _hud.clear();
    }
    _last_sun = -1;
    _last_level = -1;
    _reward_caption_drawn = false;
    _pause_menu.clear();
    for (auto& plant : _title_plants) {
        plant.reset();
    }
    _title_options.reset();
    for (auto& sprite : _road_zombies) {
        sprite.reset();
    }
    _almanac.clear();
    _save_menu.clear();
    _seed_selection.clear();
    for (auto& tile : _level_tiles) {
        tile.reset();
    }
    _level_cursor.reset();
    if (!same_road) {
        _menu_frame = 0;
    }
    _preview_text.clear();
    for (auto& half : _preview_heading) {
        half.reset();
    }
    _preview_pan = 0;
    _loss_notice_stage = 0;
    _reward_packet.reset();
    _reward_shadow.reset();
    _menu.reset();
    // The gameplay background remains its own resource. Menus use another layer
    // and independent artwork, with no copied lawn, seed bar, or gameplay HUD.
    switch (phase) {
        case Phase::save_select:
            break;
        case Phase::title:
            _menu = bn::regular_bg_items::title.create_bg(8, 48);
            break;
        case Phase::playing:
            break;
        case Phase::seed_select:
            _menu = SpriteCatalog::road(game.level_spec().lanes).create_bg(-104, 48);
            _seed_selection.open();
            break;
        case Phase::preview:
            _menu = SpriteCatalog::road(game.level_spec().lanes).create_bg(136, 48);
            break;
        case Phase::almanac:
            _menu = bn::regular_bg_items::almanac.create_bg(8, 48);
            break;
        case Phase::level_select:
            _menu = bn::regular_bg_items::level_select.create_bg(8, 48);
            break;
        case Phase::paused:
            _pause_menu.open();
            break;
        case Phase::won:
        case Phase::lost:
            break;
        default:
            break;
    }
    bool playing = phase == Phase::playing;
    bool reward = phase == Phase::won || phase == Phase::lost;
    if (playing) {
        _background.set_item(SpriteCatalog::lawn(game.level_spec().lanes));
    }
    _background.set_visible(playing || reward);
    if (_menu) {
        _menu->set_priority(phase == Phase::seed_select ? 2 : 1);
    }
    _cursor.set_visible(playing);
    _selection.set_visible(playing);
    if (!playing && !reward) {
        clear_entities();
    }
}

void Presentation::clear_entities() {
    _world.clear();
    for (auto& sprite : _seed_packets) {
        sprite.reset();
    }
    for (auto& sprite : _seed_arrows) {
        sprite.reset();
    }
    for (auto& sprite : _wave_bar) {
        sprite.reset();
    }
    _wave_head.reset();
    _phase_icon.reset();
    _feedback_icon.reset();
}

void Presentation::draw_wave_bar(const Game& game) {
    // Ease one shared progress value so the fill and zombie head remain aligned.
    int target = game.wave_progress() * 256;
    if (_bar_progress < 0 || target < _bar_progress) {
        _bar_progress = target;
    }
    int difference = target - _bar_progress;
    int step = difference / 8;
    if (step < 8) {
        step = 8;
    }
    if (step > 512) {
        step = 512;
    }
    _bar_progress += difference < step ? difference : step;
    int progress = _bar_progress / 256;
    int total = game.total_waves();
    const auto& item = total == 3 ? bn::sprite_items::wave_bar_short : bn::sprite_items::wave_bar;
    for (int i = 0; i < wave_count; ++i) {
        auto& segment = _wave_bar[i];
        if (i >= total) {
            segment.reset();
            continue;
        }
        int frames_per_state = 168 + total * 2;
        int state = 0;
        if (game.horde_entered(i + 1)) {
            state = game.horde_remaining(i + 1) ? 1 : 2;
        }
        int frame = state * frames_per_state + wave_track::frame_offset(i, total);
        if (game.wave() > i) {
            frame += 1 + wave_track::fill(i, progress, total);
        }
        if (!segment) {
            segment = item.create_sprite(
                281 - wave_track::boundary(i, total) - wave_track::sprite_width / 2 - 120,
                wave_track::sprite_height / 2 - 80,
                frame
            );
            segment->set_bg_priority(0);
        }
        segment->set_tiles(item.tiles_item(), frame);
    }
    if (!_wave_head) {
        _wave_head = bn::sprite_items::hud_icons.create_sprite(0, wave_track::head_y - 80, 5);
        _wave_head->set_bg_priority(0);
        _wave_head->set_z_order(-2);
    }
    _wave_head->set_x(wave_track::display_position(progress, total) - 120);
}

void Presentation::draw_feedback(const Game& game, const ViewState& view) {
    int frame = -1;
    int x = view.feedback_x;
    int y = view.feedback_y - 6;
    if (view.shoveling) {
        frame = 2;
        x = cell_x(view.cursor.column);
        y = cell_y(view.cursor.row) - 6;
    } else if (view.feedback_frames > 0) {
        switch (view.feedback) {
            case Feedback::planted:
                frame = 1;
                y -= (30 - view.feedback_frames) / 5;
                break;
            case Feedback::removed:
                frame = 2;
                break;
            case Feedback::occupied:
                frame = 0;
                break;
            case Feedback::cooling_down: {
                int recharge = spec(view.feedback_seed).recharge;
                int stage = (recharge - game.cooldown(view.feedback_seed)) * 3 / recharge;
                if (stage > 2) {
                    stage = 2;
                }
                frame = stage == 0 ? 3 : 9 + stage;
                break;
            }
            case Feedback::insufficient_sun:
                // The packet border pulses red; the bank's sun icon stays still.
                break;
            case Feedback::collected:
                break; // Each sun animates in its own entity slot.
            default:
                break;
        }
    }
    if (frame < 0) {
        _feedback_icon.reset();
    } else {
        if (!_feedback_icon) {
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
    if (rejected && view.feedback_frames % 8 < 4) {
        selection_frame = view.feedback == Feedback::cooling_down ? 2 : 1;
    }
    _selection.set_tiles(bn::sprite_items::selection.tiles_item(), selection_frame);

    int remaining = game.next_wave_frames();
    if (remaining > 0) {
        constexpr int duration = 900;
        int clock = (duration - remaining) * 16 / duration;
        if (!_phase_icon) {
            _phase_icon = bn::sprite_items::phase_clock.create_sprite(-105, -68, clock);
        }
        _phase_icon->set_item(bn::sprite_items::phase_clock, clock);
        _phase_icon->set_bg_priority(0);
    } else {
        _phase_icon.reset();
    }
}

void Presentation::draw_hud(const Game& game, const ViewState& view) {
    // All packets report their readiness, even when another seed is selected.
    for (int i = 0; i < plant_count; ++i) {
        if (i >= game.available_plants() || i < _seed_page || i >= _seed_page + visible_seed_count) {
            _seed_packets[i].reset();
            continue;
        }
        auto type = game.seed_at(i);
        int remaining = game.cooldown(type);
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame(remaining, spec(type).recharge, game.sun() >= spec(type).cost);
        auto& packet = _seed_packets[i];
        if (!packet) {
            packet = item.create_sprite(
                seed_layout::center(i - _seed_page) - 120, seed_layout::center_y - 80, frame
            );
            packet->set_bg_priority(0);
        }
        packet->set_tiles(item.tiles_item(), frame);
    }
    for (int i = 0; i < 2; ++i) {
        if (game.available_plants() <= visible_seed_count) {
            _seed_arrows[i].reset();
            continue;
        }
        if (!_seed_arrows[i]) {
            _seed_arrows[i] = bn::sprite_items::hud_icons.create_sprite(i == 0 ? -76 : 108, -42, 12 + i);
            _seed_arrows[i]->set_bg_priority(0);
        }
    }
    if (game.sun() != _last_sun) {
        _last_sun = game.sun();
        _hud.clear();
        _sun_text.generate(-116, -35, bn::to_string<8>(game.sun()), _hud);
    }
    int bounce = view.feedback == Feedback::collected ? view.feedback_frames / 8 : 0;
    for (auto& sprite : _hud) {
        sprite.set_y(-35 - bounce);
    }
}

void Presentation::draw_title(const ViewState& view) {
    constexpr int centers[] = {35, 88};
    for (int i = 0; i < 2; ++i) {
        if (!_title_plants[i]) {
            _title_plants[i] = bn::sprite_items::title_plants.create_sprite(centers[i] - 120, 17);
            _title_plants[i]->set_bg_priority(0);
        }
        int pose = (_menu_frame / 24 + i) % 2;
        _title_plants[i]->set_tiles(bn::sprite_items::title_plants.tiles_item(), i * 2 + pose);
    }
    if (!_title_options) {
        _title_options = bn::sprite_items::title_options.create_sprite(56, 16);
        _title_options->set_bg_priority(0);
    }
    _title_options->set_tiles(
        bn::sprite_items::title_options.tiles_item(), static_cast<int>(view.title_choice)
    );
    _menu_frame = (_menu_frame + 1) % 144;
}

void Presentation::draw_level_select(const Game& game) {
    for (int i = 0; i < levels_per_world; ++i) {
        int level = game.world_spec().first_level + i;
        int state = 0;
        if (game.level_unlocked(level)) {
            state = game.level_completed(level) ? 2 : 1;
        }
        if (!_level_tiles[i]) {
            _level_tiles[i] =
                bn::sprite_items::level_tiles.create_sprite(menu_layout::level_x(i), menu_layout::level_y(i));
            _level_tiles[i]->set_bg_priority(0);
        }
        _level_tiles[i]->set_tiles(bn::sprite_items::level_tiles.tiles_item(), state * 10 + i);
    }
    if (!_level_cursor) {
        _level_cursor = bn::sprite_items::level_cursor.create_sprite(0, 0);
        _level_cursor->set_bg_priority(0);
        _level_cursor->set_z_order(-10);
    }
    int local = game.level() % levels_per_world;
    _level_cursor->set_position(menu_layout::level_x(local), menu_layout::level_y(local));
    if (_last_level != game.level()) {
        _last_level = game.level();
        _hud.clear();
        _text.set_center_alignment();
        bn::string<48> label(game.level_spec().name);
        if (!game.level_unlocked(game.level())) {
            label += " - Locked";
        }
        _text.generate(0, 54, label, _hud);
        _text.set_left_alignment();
    }
}

void Presentation::draw_preview(const Game& game) {
    int time = game.preview_frames();
    int remaining = time < preview_pan_frames ? preview_pan_frames - time : 0;
    int pan = 240 - 240 * remaining * remaining / (preview_pan_frames * preview_pan_frames);
    if (game.preview_returning()) {
        pan = 240 - pan;
    }
    draw_road(game, pan, true);
}

void Presentation::draw_road(const Game& game, int pan, bool labels) {
    _menu->set_x(136 - pan);
    _world.preview(game, pan, labels ? 0 : 2);
    // Staggered, fixed road positions keep small rosters near the yard and avoid parade rows.
    constexpr std::array<menu_layout::Position, zombie_type_count> crowd{
        {{300, 94}, {318, 136}, {349, 104}, {370, 131}, {398, 91}, {419, 138}, {447, 103}, {463, 137}}
    };
    std::array<menu_layout::Position, zombie_type_count> positions{};
    int slot_position = 0;
    for (int i = 0; i < zombie_type_count; ++i) {
        if (!game.zombie_roster()[i]) {
            continue;
        }
        positions[i] = crowd[slot_position++];
    }
    int slot = 0;
    for (int i = 0; i < zombie_type_count; ++i) {
        if (!game.zombie_roster()[i]) {
            continue;
        }
        int x = positions[i].x - pan;
        int y = positions[i].y;
        draw_actor(
            _road_zombies[slot],
            true,
            x,
            y,
            zombie_display_frame(static_cast<ZombieType>(i), _menu_frame + slot * 11),
            160 - y
        );
        _road_zombies[slot]->set_bg_priority(labels ? 0 : 2);
        ++slot;
    }
    ++_menu_frame;
    if (!labels) {
        return;
    }
    for (int half = 0; half < 2; ++half) {
        auto& sprite = _preview_heading[half];
        if (!sprite) {
            sprite = bn::sprite_items::scouting_heading.create_sprite(0, -48, half);
            sprite->set_bg_priority(0);
        }
        sprite->set_x(328 + half * 64 - pan - 120);
    }
    if (_preview_text.empty()) {
        _text.set_center_alignment();
        _text.generate(240, -70, game.level_spec().name, _preview_text);
        const char* controls = "A/START: SEEDS   B: BACK";
        if (game.unlocked_plants() <= visible_seed_count) {
            controls = "A/START: BEGIN   B: BACK";
        }
        _text.generate(240, -36, controls, _preview_text);
        for (int i = 0; i < zombie_type_count; ++i) {
            int count = game.zombie_roster()[i];
            if (!count) {
                continue;
            }
            bn::string<16> label("x");
            label += bn::to_string<4>(count);
            _text.generate(positions[i].x - 120, positions[i].y + 17 - 80, label, _preview_text);
        }
        _text.set_left_alignment();
    }
    for (auto& label : _preview_text) {
        label.set_x(label.x() + _preview_pan - pan);
    }
    _preview_pan = pan;
}

void Presentation::draw_menu(const Game& game, const ViewState& view) {
    if (game.phase() == Phase::save_select) {
        _save_menu.render(view.save_menu);
        return;
    }
    if (game.phase() == Phase::title) {
        draw_title(view);
        return;
    }
    if (game.phase() == Phase::preview) {
        draw_preview(game);
        return;
    }
    if (game.phase() == Phase::seed_select) {
        draw_road(game, 240, false);
        _seed_selection.render(game, view, _selection);
        return;
    }
    if (game.phase() == Phase::almanac) {
        _menu->set_priority(3);
        _almanac.render(game, view.almanac);
        return;
    }
    if (game.phase() == Phase::level_select) {
        draw_level_select(game);
        return;
    }
    if (game.phase() == Phase::won) {
        draw_reward(game);
        return;
    }
    if (game.phase() == Phase::lost) {
        draw_loss(game);
        return;
    }
    if (game.phase() == Phase::paused) {
        _pause_menu.render(view);
    }
}

void Presentation::put_entities_behind_overlay() {
    auto behind_panel = [](auto& pool) {
        for (auto& sprite : pool) {
            if (sprite) {
                sprite->set_bg_priority(2);
            }
        }
    };
    _world.behind_overlay();
    behind_panel(_seed_packets);
    behind_panel(_seed_arrows);
    behind_panel(_wave_bar);
    for (auto* sprite : {&_wave_head, &_phase_icon, &_feedback_icon}) {
        if (*sprite) {
            (*sprite)->set_bg_priority(2);
        }
    }
    for (auto& sprite : _hud) {
        sprite.set_bg_priority(2);
    }
}

void Presentation::draw_loss(const Game& game) {
    _world.render(game);
    put_entities_behind_overlay();
    int frame = game.loss_frames();
    int stage = 0;
    if (frame >= 30) {
        stage = game.loss_ready() ? 2 : 1;
    }
    if (stage != _loss_notice_stage) {
        _loss_notice_stage = stage;
        const auto& item =
            stage == 2 ? bn::regular_bg_items::loss_prompt : bn::regular_bg_items::loss_message;
        _menu = item.create_bg(8, 48);
        _menu->set_priority(1);
    }
    if (_menu) {
        int drop = 0;
        if (frame < 42) {
            drop = (frame - 42) * 2;
        } else if (frame < 48) {
            drop = 2;
        }
        _menu->set_y(48 + drop);
    }
}

void Presentation::draw_reward(const Game& game) {
    int frame = game.reward_frames();
    if (frame == 0) {
        _world.render(game);
    }
    if (game.victory_settling()) {
        return;
    }
    int type = static_cast<int>(game.unlocked_plant());
    if (!_reward_packet) {
        _reward_packet = SpriteCatalog::reward(game.unlocked_plant()).create_sprite(0, 100);
        _reward_packet->set_bg_priority(0);
        _reward_packet->set_z_order(-20);
    }
    constexpr int lift = 8;
    const auto pose = reward_pose(frame, lift + (game.unlocked_plant() == PlantType::count ? 8 : 0));
    if (frame < reward_reveal_frame) {
        if (!_reward_shadow) {
            _reward_shadow = bn::sprite_items::reward_shadow.create_sprite(0, 46);
            _reward_shadow->set_bg_priority(0);
            _reward_shadow->set_z_order(1);
        }
        _reward_shadow->set_position(pose.x - 120, 46);
        _reward_shadow->set_scale(bn::fixed(100 - pose.height) / 100);
    } else if (!_menu) {
        put_entities_behind_overlay();
        _reward_shadow.reset();
        _menu = bn::regular_bg_items::reward_panel.create_bg(8, 48);
        _menu->set_priority(1);
    }
    _reward_packet->set_position(pose.x - 120, pose.y - 80);
    _reward_packet->set_scale(
        bn::fixed(pose.scale + pose.squash * 2) / 50, bn::fixed(pose.scale - pose.squash * 2) / 50
    );
    if (!game.reward_ready() || _reward_caption_drawn) {
        return;
    }
    _reward_caption_drawn = true;
    constexpr const char* descriptions[] = {
        "Makes more sun",
        "Fires peas",
        "Blocks zombies",
        "Blasts nearby zombies",
        "Arms, then bursts",
        "Slows zombies",
        "Crushes nearby zombies",
        "Fires two peas",
        "Stops pole vaulters",
        "Swallows whole zombies"
    };
    _text.set_center_alignment();
    // Move the information with the unlock sign, retaining its internal text margins.
    _text.generate(
        0,
        58 - lift,
        (game.unlocked_plant() == PlantType::count ? game.level_spec().name : plant_entries[type].name),
        _hud
    );
    _text.generate(
        0,
        70 - lift,
        (game.unlocked_plant() == PlantType::count ? "Garden defended!" : descriptions[type]),
        _hud
    );
    _text.set_left_alignment();
}

void Presentation::draw_tutorial(const Game& game, const ViewState& view) {
    auto step = view.tutorial.step;
    bool active = step != TutorialStep::none;
    for (auto& segment : _wave_bar) {
        if (segment) {
            segment->set_visible(!active);
        }
    }
    if (_wave_head) {
        _wave_head->set_visible(!active);
    }
    if (_phase_icon) {
        _phase_icon->set_visible(!active);
    }
    if (!active) {
        _tutorial_panel.reset();
        _lesson_target.reset();
        _lesson_text.clear();
        _lesson_step = step;
        return;
    }
    if (!_tutorial_panel) {
        _tutorial_panel = bn::regular_bg_items::tutorial_header.create_bg(8, 48);
        _tutorial_panel->set_priority(1);
        _lesson_target = bn::sprite_items::lesson_target.create_sprite(0, 0);
        _lesson_target->set_bg_priority(0);
        _lesson_target->set_z_order(-9);
    }
    if (step != _lesson_step) {
        _lesson_step = step;
        _lesson_text.clear();
        constexpr const char* titles[] = {
            "",
            "PLANT A PEASHOOTER",
            "COLLECT FALLING SUN",
            "CLEAR THE OLD PLANTS",
            "PLANT A SUNFLOWER",
            "SUNFLOWERS MAKE MORE SUN"
        };
        constexpr const char* controls[] = {
            "",
            "A: PLANT IN THE HIGHLIGHTED CELL",
            "B: COLLECT THE GLOWING SUN",
            "SELECT + A: CLEAR HIGHLIGHTED CELL",
            "R: SUNFLOWER  A: HIGHLIGHTED CELL",
            "B: COLLECT YOUR FLOWER SUN"
        };
        _text.set_center_alignment();
        _text.generate(0, -73, titles[static_cast<int>(step)], _lesson_text);
        _text.generate(0, -63, controls[static_cast<int>(step)], _lesson_text);
        _text.set_left_alignment();
    }
    bool collecting = step == TutorialStep::collect_sun || step == TutorialStep::sunflower_sun;
    int x = cell_x(view.tutorial.column), y = cell_y(view.tutorial.row);
    bool visible = !collecting && game.elapsed() % 60 < 40;
    if (collecting) {
        // Mark the collectible itself, never the sun counter in the HUD.
        for (const auto& sun : game.suns()) {
            if (sun.active && !sun.collecting && sun.y / scale >= 63 &&
                (step != TutorialStep::sunflower_sun || sun.from_plant)) {
                x = sun.x / scale;
                y = sun.y / scale;
                visible = true;
                break;
            }
        }
    }
    _lesson_target->set_bg_priority(collecting ? 0 : 2);
    _lesson_target->set_z_order(collecting ? -9 : 3 - view.tutorial.row * 4);
    _lesson_target->set_visible(visible);
    _lesson_target->set_position(x - 120, y - 80);
    int target_frame;
    if (collecting) {
        target_frame = rows * columns + game.elapsed() / 24 % 2;
    } else {
        target_frame = cell_index(view.tutorial.row, view.tutorial.column);
    }
    _lesson_target->set_tiles(bn::sprite_items::lesson_target.tiles_item(), target_frame);
}

void Presentation::update(const Game& game, const ViewState& view) {
    if (game.phase() != _screen) {
        change_screen(game);
    }
    if (game.phase() != Phase::playing) {
        draw_menu(game, view);
        return;
    }
    int page = _seed_page < 0 ? 0 : _seed_page;
    int selected = game.bank_slot(view.cursor.seed);
    if (selected < page) {
        page = selected;
    } else if (selected >= page + visible_seed_count) {
        page = selected - visible_seed_count + 1;
    }
    if (page != _seed_page) {
        _seed_page = page;
        for (auto& packet : _seed_packets) {
            packet.reset();
        }
    }
    const auto& cursor = view.cursor;
    bool on_feedback_tile = cell_x(cursor.column) == view.feedback_x && cell_y(cursor.row) == view.feedback_y;
    bool rejected = on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::occupied;
    int shake = 0;
    if (rejected) {
        shake = view.feedback_frames % 4 < 2 ? -1 : 1;
    }
    _cursor.set_position(cell_x(cursor.column) - 120 + shake, cell_y(cursor.row) - 80);
    bool blocked = game.plants()[cell_index(cursor.row, cursor.column)].active ||
                   game.tombstone_at(cursor.row, cursor.column) || game.sun() < spec(cursor.seed).cost ||
                   game.cooldown(cursor.seed) > 0;
    int tint = blocked ? 1 : 0;
    if (view.shoveling) {
        tint = 2;
    }
    if (on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::planted) {
        tint = 3;
    }
    _cursor.set_tiles(bn::sprite_items::cursor.tiles_item(), tint);
    _selection.set_position(
        seed_layout::center(game.bank_slot(cursor.seed) - _seed_page) - 120, seed_layout::center_y - 80
    );
    _world.render(game);
    draw_wave_bar(game);
    draw_feedback(game, view);
    draw_hud(game, view);
    draw_tutorial(game, view);
}
} // namespace garden
