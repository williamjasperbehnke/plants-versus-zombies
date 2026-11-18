#ifndef GARDEN_ALMANAC_VIEW_H
#define GARDEN_ALMANAC_VIEW_H
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "ui/menu_layout.h"
#include "ui/view_state.h"

namespace garden {
// Owns the catalogue grid, animated portrait, statistics, text and scrolling panel.
class AlmanacView {
public:
    AlmanacView();
    AlmanacView(const AlmanacView&) = delete;
    AlmanacView& operator=(const AlmanacView&) = delete;
    void render(const Game& game, const AlmanacState& state);
    void clear();

private:
    bn::sprite_text_generator _text;
    bn::vector<bn::sprite_ptr, 48> _labels;
    bn::vector<bn::sprite_ptr, 32> _entry_text;
    std::array<bn::optional<bn::sprite_ptr>, menu_layout::almanac_page_size> _gallery;
    std::array<bn::optional<bn::sprite_ptr>, 3> _entry_icons;
    bn::optional<bn::regular_bg_ptr> _panel;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<bn::sprite_ptr> _gallery_cursor;
    bn::optional<bn::sprite_ptr> _scrollbar;
    bn::optional<AlmanacState> _last_state;
    int _clock = 0;
};
} // namespace garden
#endif
