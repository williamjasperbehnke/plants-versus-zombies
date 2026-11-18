#ifndef GARDEN_MENU_LAYOUT_H
#define GARDEN_MENU_LAYOUT_H

namespace garden::menu_layout {
struct Position {
    int x = 0;
    int y = 0;
};

// Sprite coordinates are relative to the center of the 240 x 160 screen.
constexpr int wrap_index(int index, int count) {
    int wrapped = index % count;
    return wrapped < 0 ? wrapped + count : wrapped;
}

inline constexpr int level_columns = 5;
inline constexpr int almanac_columns = 3;
inline constexpr int almanac_detail_shift = -6;
inline constexpr int seed_columns = 5;

constexpr int seed_x(int index) {
    return 22 + index % seed_columns * 30;
}

constexpr int seed_y(int index) {
    return 78 + index / seed_columns * 48;
}

constexpr int bank_x(int slot) {
    return 20 + slot * 31;
}

inline constexpr int bank_y = 20;
inline constexpr int almanac_page_size = almanac_columns * 2;

constexpr int almanac_pages(int entries) {
    return (entries + almanac_page_size - 1) / almanac_page_size;
}

constexpr int almanac_move(int entry, int count, int horizontal, int vertical) {
    int start = entry / almanac_page_size * almanac_page_size;
    int remaining = count - start;
    int size = remaining < almanac_page_size ? remaining : almanac_page_size;
    int local = wrap_index(entry - start + horizontal, size);
    int column = local % almanac_columns;
    int target = local + vertical * almanac_columns;
    if (target < 0) {
        target = column + (size - 1 - column) / almanac_columns * almanac_columns;
    } else if (target >= size) {
        target = column;
    }
    return start + target;
}

constexpr int level_x(int index) {
    return -76 + index % level_columns * 38;
}

constexpr int level_y(int index) {
    return -12 + index / level_columns * 40;
}
} // namespace garden::menu_layout
#endif
