#include "ui/almanac_view.h"

#include "bn_regular_bg_items_almanac_entry.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_items_almanac_cell.h"
#include "bn_sprite_items_almanac_scrollbar.h"
#include "bn_sprite_items_almanac_stats.h"
#include "bn_string.h"
#include "game/animation.h"
#include "ui/almanac.h"
#include "ui/menu_font.h"
#include "ui/sprite_catalog.h"

namespace garden {
AlmanacView::AlmanacView() : _text(menu_font) {
    _text.set_palette_item(almanac_ink);
}

void AlmanacView::clear() {
    _labels.clear();
    for (auto& sprite : _gallery) {
        sprite.reset();
    }
    _portrait.reset();
    _panel.reset();
    _scrollbar.reset();
    _entry_text.clear();
    for (auto& icon : _entry_icons) {
        icon.reset();
    }
    _gallery_cursor.reset();
    _last_state.reset();
    _clock = 0;
}

void AlmanacView::render(const Game& game, const AlmanacState& state) {
    int index = state.entry;
    int count = state.zombies ? zombie_type_count : plant_count;
    int page = index / menu_layout::almanac_page_size;
    int local = index % menu_layout::almanac_page_size;
    auto draw_entry = [&state](auto& sprite, int entry, int x, int y, int clock, int portrait_scale = 1) {
        ActorFrame frame;
        if (state.zombies) {
            frame = zombie_display_frame(static_cast<ZombieType>(entry), clock);
        } else {
            frame = plant_display_frame(seed_order[entry], clock);
        }
        // The door's visible bounds center at x=28 in a padded 64px frame.
        if (state.zombies && entry == static_cast<int>(ZombieType::screen_door)) {
            x += 4 * portrait_scale;
        }
        draw_actor(sprite, true, x, y, frame);
    };
    for (int i = 0; i < menu_layout::almanac_page_size; ++i) {
        int entry = page * menu_layout::almanac_page_size + i;
        if (entry >= count) {
            _gallery[i].reset();
            continue;
        }
        draw_entry(
            _gallery[i],
            entry,
            54 + i % menu_layout::almanac_columns * 66,
            62 + i / menu_layout::almanac_columns * 46,
            (state.zombies ? _clock / 2 : _clock) + i * 7
        );
        _gallery[i]->set_bg_priority(2);
    }
    if (!_gallery_cursor) {
        _gallery_cursor = bn::sprite_items::almanac_cell.create_sprite(0, 0);
        _gallery_cursor->set_bg_priority(2);
        _gallery_cursor->set_z_order(-10);
    }
    _gallery_cursor->set_position(
        54 + local % menu_layout::almanac_columns * 66 - 120,
        64 + local / menu_layout::almanac_columns * 46 - 80
    );
    if (state.details) {
        if (!_panel) {
            _panel = bn::regular_bg_items::almanac_entry.create_bg(8, 48);
            _panel->set_priority(1);
        }
        draw_entry(
            _portrait,
            index,
            64 + menu_layout::almanac_detail_shift,
            80,
            state.zombies ? _clock / 2 : _clock,
            2
        );
        _portrait->set_bg_priority(0);
        _portrait->set_scale(2);
        _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
    } else {
        _panel.reset();
        _scrollbar.reset();
        for (auto& icon : _entry_icons) {
            icon.reset();
        }
        _portrait.reset();
        _entry_text.clear();
    }
    ++_clock;
    if (_last_state && *_last_state == state) {
        return;
    }
    _last_state = state;
    _labels.clear();
    _entry_text.clear();
    _text.set_center_alignment();
    _text.set_bg_priority(2);
    bn::string<20> heading = state.zombies ? "ZOMBIES " : "PLANTS ";
    heading += bn::to_string<2>(page + 1);
    heading += "/";
    heading += bn::to_string<2>(menu_layout::almanac_pages(count));
    _text.generate(0, -42, heading, _labels);
    for (int i = 0; i < menu_layout::almanac_page_size; ++i) {
        int entry_index = page * menu_layout::almanac_page_size + i;
        if (entry_index >= count) {
            continue;
        }
        const auto& entry = almanac_entry(state.zombies, entry_index);
        _text.generate(
            54 + i % menu_layout::almanac_columns * 66 - 120,
            i / menu_layout::almanac_columns * 46,
            entry.name,
            _labels
        );
    }
    _text.set_bg_priority(0);
    if (state.details) {
        const auto& entry = almanac_entry(state.zombies, index);
        _text.generate(menu_layout::almanac_detail_shift, -46, entry.name, _entry_text);
        auto stat = [this](int slot, int frame, int x, int y, const auto& value) {
            x += menu_layout::almanac_detail_shift;
            _entry_icons[slot] = bn::sprite_items::almanac_stats.create_sprite(x - 120, y - 80, frame);
            _entry_icons[slot]->set_bg_priority(0);
            _text.set_left_alignment();
            _text.generate(x + 12 - 120, y - 80, value, _entry_text);
            _text.set_center_alignment();
        };
        for (auto& icon : _entry_icons) {
            icon.reset();
        }
        if (state.zombies) {
            const auto& definition = spec(static_cast<ZombieType>(index));
            stat(0, 1, 78, 120, bn::to_string<4>(definition.health + definition.shield_health));
            stat(1, 3, 143, 120, bn::to_string<12>(game.lifetime_kills(static_cast<ZombieType>(index))));
        } else {
            const auto& definition = spec(seed_order[index]);
            stat(0, 0, 50, 120, bn::to_string<4>(definition.cost));
            stat(1, 1, 107, 120, bn::to_string<4>(definition.health));
            int frames = definition.recharge;
            bn::string<16> recharge = bn::to_string<4>(frames / 60);
            if (frames % 60) {
                recharge += ".5";
            }
            recharge += "s";
            stat(2, 2, 164, 120, recharge);
        }
        AlmanacDescription description(entry.description);
        int visible = description.size() < almanac_visible_lines ? description.size() : almanac_visible_lines;
        _text.set_left_alignment();
        for (int i = 0; i < visible; ++i) {
            _text.generate(
                -9 + menu_layout::almanac_detail_shift,
                -5 - (visible - 1) * 5 + i * 10,
                description[state.scroll + i],
                _entry_text
            );
        }
        int limit = almanac_scroll_limit(state.zombies, index);
        if (limit) {
            _scrollbar = bn::sprite_items::almanac_scrollbar.create_sprite(
                89, -5, almanac_scrollbar_frame(description.size(), state.scroll)
            );
            _scrollbar->set_bg_priority(0);
        } else {
            _scrollbar.reset();
        }
    }
    _text.set_left_alignment();
}

} // namespace garden
