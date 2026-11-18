#ifndef GARDEN_MENU_VIEWS_H
#define GARDEN_MENU_VIEWS_H
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "ui/menu_layout.h"
#include "ui/view_state.h"

namespace garden {
// Owns pause labels, mixer sliders, background and redraw state.
class PauseMenuView {
public:
    PauseMenuView();
    PauseMenuView(const PauseMenuView&) = delete;
    PauseMenuView& operator=(const PauseMenuView&) = delete;
    void open();
    void render(const ViewState& view);
    void clear();

private:
    bn::sprite_text_generator _text;
    bn::vector<bn::sprite_ptr, 48> _labels;
    bn::optional<bn::regular_bg_ptr> _background;
    std::array<bn::optional<bn::sprite_ptr>, 3> _volume_sliders;
    int _last_choice = -1;
    int _last_audio = -1;
};

// Owns seed choices, animated bank packets, labels and the panel over the road.
class SeedSelectionView {
public:
    SeedSelectionView();
    SeedSelectionView(const SeedSelectionView&) = delete;
    SeedSelectionView& operator=(const SeedSelectionView&) = delete;
    void open();
    void render(const Game& game, const ViewState& view, bn::sprite_ptr& selection);
    void clear();

private:
    void draw_bank(const Game& game);

    bn::sprite_text_generator _text;
    bn::vector<bn::sprite_ptr, 48> _labels;
    bn::optional<bn::regular_bg_ptr> _panel;
    std::array<bn::optional<bn::sprite_ptr>, plant_count> _packets;
    std::array<bn::optional<bn::sprite_ptr>, plant_count> _bank_packets;
    std::array<menu_layout::Position, plant_count> _packet_positions{};
    int _last_key = -1;
};
} // namespace garden
#endif
