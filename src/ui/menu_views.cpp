#include "ui/menu_views.h"

#include "bn_regular_bg_items_paused.h"
#include "bn_regular_bg_items_seed_select.h"
#include "bn_sprite_items_selection.h"
#include "bn_sprite_items_volume_slider.h"
#include "bn_string.h"
#include "ui/almanac.h"
#include "ui/menu_font.h"
#include "ui/menu_layout.h"
#include "ui/sprite_catalog.h"

namespace garden {
PauseMenuView::PauseMenuView() : _text(menu_font) {
    _text.set_bg_priority(0);
}

void PauseMenuView::open() {
    _background = bn::regular_bg_items::paused.create_bg(8, 48);
    _background->set_priority(1);
}

void PauseMenuView::clear() {
    _labels.clear();
    for (auto& slider : _volume_sliders) {
        slider.reset();
    }
    _background.reset();
    _last_choice = -1;
    _last_audio = -1;
}

void PauseMenuView::render(const ViewState& view) {
    int choice = static_cast<int>(view.pause_choice);
    if (choice == _last_choice && _last_audio == view.audio.key()) {
        return;
    }
    _last_audio = view.audio.key();
    _last_choice = choice;
    _labels.clear();
    _text.set_left_alignment();
    constexpr const char* choices[] = {"Resume", "Restart", "Levels", "Master", "Music", "Effects"};
    constexpr int positions[] = {54, 68, 82, 102, 116, 130};
    for (int i = 0; i < static_cast<int>(PauseChoice::count); ++i) {
        _text.generate(-60, positions[i] - 80, choices[i], _labels);
    }
    _text.generate(-78, positions[choice] - 80, ">", _labels);
    for (int i = 0; i < 3; ++i) {
        if (!_volume_sliders[i]) {
            _volume_sliders[i] = bn::sprite_items::volume_slider.create_sprite(48, positions[i + 3] - 80);
            _volume_sliders[i]->set_bg_priority(0);
        }
        _volume_sliders[i]->set_tiles(bn::sprite_items::volume_slider.tiles_item(), view.audio.levels[i]);
    }
    _text.set_left_alignment();
}

SeedSelectionView::SeedSelectionView() : _text(menu_font) {
    _text.set_bg_priority(0);
}

void SeedSelectionView::open() {
    _panel = bn::regular_bg_items::seed_select.create_bg(8, 48);
    _panel->set_priority(1);
}

void SeedSelectionView::clear() {
    _labels.clear();
    for (auto& packet : _packets) {
        packet.reset();
    }
    for (auto& packet : _bank_packets) {
        packet.reset();
    }
    _panel.reset();
    _last_key = -1;
    _text.set_palette_item(menu_font.item().palette_item());
}

void SeedSelectionView::render(const Game& game, const ViewState& view, bn::sprite_ptr& selection) {
    bool error = view.seed_error_frames > 0;
    int key = view.seed_choice | (error ? 1 << 20 : 0);
    for (int i = 0; i < plant_count; ++i) {
        auto type = seed_order[i];
        bool unlocked = game.plant_unlocked(type);
        bool selected = game.plant_available(type);
        if (selected) {
            key |= 1 << (i + 4);
        }
        int x = menu_layout::seed_x(i);
        int y = menu_layout::seed_y(i);
        if (!unlocked) {
            _packets[i].reset();
            continue;
        }
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame(
            (selected || _bank_packets[i]) ? spec(type).recharge : 0, spec(type).recharge, unlocked
        );
        if (!_packets[i]) {
            _packets[i] = item.create_sprite(x - 120, y - 80, frame);
        }
        _packets[i]->set_item(item, frame);
        _packets[i]->set_bg_priority(0);
    }
    draw_bank(game);
    selection.set_visible(true);
    selection.set_position(
        menu_layout::seed_x(view.seed_choice) - 120, menu_layout::seed_y(view.seed_choice) - 80
    );
    selection.set_tiles(
        bn::sprite_items::selection.tiles_item(), error && view.seed_error_frames / 8 % 2 ? 1 : 0
    );
    if (key == _last_key) {
        return;
    }
    _last_key = key;
    _labels.clear();
    _text.set_center_alignment();
    bn::string<32> count;
    count += bn::to_string<2>(game.available_plants());
    count += "/";
    count += bn::to_string<2>(game.required_seeds());
    _text.generate(95, -71, "SEEDS", _labels);
    _text.generate(95, -59, count, _labels);
    if (error) {
        _text.set_palette_item(menu_error_ink);
        _text.generate(-38, -30, "SELECT 6 SEEDS", _labels);
        _text.set_palette_item(menu_font.item().palette_item());
    } else {
        const char* name = "";
        auto type = seed_order[view.seed_choice];
        if (game.plant_unlocked(type)) {
            name = plant_entries[static_cast<int>(type)].name;
        }
        _text.generate(-38, -30, name, _labels);
    }
    _text.generate(0, 75, "A:PICK START:PLAY B:BACK", _labels);
    _text.set_left_alignment();
}

void SeedSelectionView::draw_bank(const Game& game) {
    auto approach = [](int current, int target) {
        int delta = target - current;
        int step = delta / 4;
        if (!step && delta) {
            step = delta > 0 ? 1 : -1;
        }
        return current + step;
    };
    for (int i = 0; i < plant_count; ++i) {
        auto type = seed_order[i];
        bool selected = game.plant_available(type);
        auto& packet = _bank_packets[i];
        auto& position = _packet_positions[i];
        if (!selected && !packet) {
            continue;
        }
        const auto& item = SpriteCatalog::seed(type);
        int frame = seed_layout::frame(0, spec(type).recharge, true);
        if (!packet) {
            position = {menu_layout::seed_x(i), menu_layout::seed_y(i)};
            packet = item.create_sprite(0, 0, frame);
            packet->set_bg_priority(0);
            packet->set_z_order(-5);
        }
        int x = selected ? menu_layout::bank_x(game.bank_slot(type)) : menu_layout::seed_x(i);
        int y = selected ? menu_layout::bank_y : menu_layout::seed_y(i);
        position = {approach(position.x, x), approach(position.y, y)};
        packet->set_position(position.x - 120, position.y - 80);
        if (!selected && position.x == x && position.y == y) {
            packet.reset();
        }
    }
}

} // namespace garden
