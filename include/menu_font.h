#ifndef GARDEN_MENU_FONT_H
#define GARDEN_MENU_FONT_H

#include "bn_sprite_font.h"
#include "bn_sprite_palette_item.h"
#include "bn_color.h"
#include "bn_sprite_items_menu_font.h"
#include <array>

namespace garden
{
// Native background glyphs: 3x5 pixels, with one pixel between characters.
inline constexpr auto menu_font_widths = [] {
    std::array<int8_t, 95> widths{};
    for(auto& width : widths) { width = 4; }
    return widths;
}();
// Dark ink for the light almanac pages; the normal menus retain their cream lettering.
alignas(int) inline constexpr auto almanac_ink_colors = [] {
    std::array<bn::color, 16> colors{};
    colors[8] = bn::color(3, 6, 5);
    return colors;
}();
inline const bn::sprite_palette_item almanac_ink(
    bn::span<const bn::color>(almanac_ink_colors.data(), almanac_ink_colors.size()), bn::bpp_mode::BPP_4);
alignas(int) inline constexpr auto menu_error_colors = [] {
    std::array<bn::color, 16> colors{};
    colors[8] = bn::color(31, 9, 6);
    return colors;
}();
inline const bn::sprite_palette_item menu_error_ink(
    bn::span<const bn::color>(menu_error_colors.data(), menu_error_colors.size()), bn::bpp_mode::BPP_4);
alignas(int) inline constexpr auto menu_gold_colors = [] {
    std::array<bn::color, 16> colors{};
    colors[8] = bn::color(31, 26, 10);
    return colors;
}();
inline const bn::sprite_palette_item menu_gold_ink(
    bn::span<const bn::color>(menu_gold_colors.data(), menu_gold_colors.size()), bn::bpp_mode::BPP_4);
inline constexpr bn::sprite_font menu_font(
    bn::sprite_items::menu_font, bn::utf8_characters_map_ref(),
    bn::span<const int8_t>(menu_font_widths.data(), menu_font_widths.size()));
}
#endif
