#ifndef GARDEN_MENU_LAYOUT_H
#define GARDEN_MENU_LAYOUT_H

namespace garden::menu_layout
{
// Sprite coordinates are relative to the center of the 240 x 160 screen.
inline constexpr int level_columns = 5;
inline constexpr int almanac_columns = 3;
constexpr int level_x(int index) { return -76 + index % level_columns * 38; }
constexpr int level_y(int index) { return -12 + index / level_columns * 40; }
}
#endif
