#include "ui/save_menu_view.h"

#include "bn_regular_bg_items_save_delete.h"
#include "bn_regular_bg_items_save_select.h"
#include "bn_string.h"
#include "ui/menu_font.h"

namespace garden {
SaveMenuView::SaveMenuView() : _text(menu_font) {
}

void SaveMenuView::clear() {
    _labels.clear();
    _background.reset();
    _last_state.reset();
}

void SaveMenuView::render(const SaveMenuState& state) {
    if (_last_state && *_last_state == state) {
        return;
    }
    _last_state = state;
    if (!_background) {
        _background = bn::regular_bg_items::save_select.create_bg(8, 48);
        _background->set_priority(1);
    }
    _labels.clear();
    _text.set_left_alignment();
    _text.set_bg_priority(0);
    if (state.confirming_delete) {
        // A full panel replaces the list so confirmation has an unambiguous focus.
        _background->set_item(bn::regular_bg_items::save_delete);
        _text.set_center_alignment();
        _text.set_palette_item(almanac_ink);
        _text.generate(0, -26, bn::string<20>("SAVE ") + bn::to_string<2>(state.slot + 1), _labels);
        bn::string<32> summary = bn::to_string<2>(state.slots[state.slot].cleared);
        summary += "/10 LEVELS CLEARED";
        _text.generate(0, -10, summary, _labels);
        _text.generate(0, 11, "THIS SAVE WILL BE ERASED", _labels);
        for (int i = 0; i < 2; ++i) {
            _text.set_palette_item(state.confirm_delete == bool(i) ? menu_gold_ink : almanac_ink);
            _text.generate(i ? 34 : -34, 30, i ? "DELETE" : "CANCEL", _labels);
        }
        _text.set_palette_item(menu_font.item().palette_item());
        _text.set_center_alignment();
        _text.generate(0, 61, "LEFT/RIGHT: SELECT", _labels);
        _text.generate(0, 71, "A: CONFIRM   B: CANCEL", _labels);
    } else {
        _background->set_item(bn::regular_bg_items::save_select);
        for (int i = 0; i < save_slot_count; ++i) {
            bool selected = i == state.slot;
            const auto& save = state.slots[i];
            int y = -38 + i * 34;
            _text.set_palette_item(selected ? menu_gold_ink : almanac_ink);
            _text.generate(-102, y, bn::string<16>("SAVE ") + bn::to_string<2>(i + 1), _labels);
            _text.set_palette_item(almanac_ink);
            bn::string<32> summary = "EMPTY SLOT";
            if (save.occupied) {
                summary = bn::to_string<2>(save.cleared);
                summary += "/10 CLEARED  1-";
                summary += bn::to_string<2>(save.unlocked);
            }
            _text.generate(-102, y + 11, summary, _labels);
            for (int action = 0; action < (save.occupied ? 2 : 1); ++action) {
                bool highlighted = selected && state.delete_selected == bool(action);
                _text.set_palette_item(highlighted ? menu_gold_ink : almanac_ink);
                const char* label = "NEW GAME";
                if (action) {
                    label = "DELETE";
                } else if (save.occupied) {
                    label = "CONTINUE";
                }
                _text.generate(action ? 68 : 0, y + 5, label, _labels);
            }
        }
    }
    _text.set_palette_item(menu_font.item().palette_item());
    _text.set_left_alignment();
}

} // namespace garden
