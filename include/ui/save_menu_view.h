#ifndef GARDEN_SAVE_MENU_VIEW_H
#define GARDEN_SAVE_MENU_VIEW_H
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "ui/view_state.h"

namespace garden {
// Owns save-list/confirmation artwork, text and its redraw cache; never changes saves.
class SaveMenuView {
public:
    SaveMenuView();
    SaveMenuView(const SaveMenuView&) = delete;
    SaveMenuView& operator=(const SaveMenuView&) = delete;
    void render(const SaveMenuState& state);
    void clear();

private:
    bn::sprite_text_generator _text;
    bn::vector<bn::sprite_ptr, 48> _labels;
    bn::optional<bn::regular_bg_ptr> _background;
    bn::optional<SaveMenuState> _last_state;
};
} // namespace garden
#endif
