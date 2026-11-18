#ifndef GARDEN_SPRITE_CATALOG_H
#define GARDEN_SPRITE_CATALOG_H
#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "game/animation.h"
#include "game/game.h"

namespace garden {
struct SpriteFrame {
    const bn::sprite_item& item;
    int graphics_index;
};

// Central resource routing; renderers do not need to know atlas offsets.
class SpriteCatalog {
public:
    static SpriteFrame actor(ActorFrame frame, bool wounded = false);
    static const bn::sprite_item& seed(PlantType type);
    static const bn::sprite_item& reward(PlantType type);
    static const bn::regular_bg_item& lawn(int lanes);
    static const bn::regular_bg_item& road(int lanes);
};

// Shared placement operation for world actors, scouting and Almanac portraits.
void draw_actor(
    bn::optional<bn::sprite_ptr>& sprite, bool active, int x, int y, ActorFrame frame, int depth = 0,
    bool wounded = false
);
} // namespace garden
#endif
