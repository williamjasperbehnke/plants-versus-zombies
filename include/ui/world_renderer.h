#ifndef GARDEN_WORLD_RENDERER_H
#define GARDEN_WORLD_RENDERER_H
#include "bn_optional.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "game/game.h"

namespace garden {
// Owns only live world sprites and their status palettes; no menus or input.
class WorldRenderer {
public:
    WorldRenderer() = default;
    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;
    void render(const Game& game);
    void preview(const Game& game, int pan, int priority);
    void clear();
    void behind_overlay();

private:
    void draw_tombstones(const Game& game, int pan = 0);
    void draw_zombie_effects(const Game& game);
    void apply_zombie_palette(bn::sprite_ptr& sprite, const Zombie& zombie, Phase phase);
    void flash_red(bn::sprite_ptr& sprite);
    std::array<bn::optional<bn::sprite_ptr>, rows * columns> _plants;
    std::array<bn::optional<bn::sprite_ptr>, zombie_capacity> _zombies;
    std::array<bn::optional<bn::sprite_ptr>, zombie_effect_capacity> _zombie_effects;
    std::array<bn::optional<bn::sprite_ptr>, tombstone_capacity> _tombstones;
    std::array<bn::optional<bn::sprite_palette_ptr>, 3> _frost_palettes;
    bn::optional<bn::sprite_palette_ptr> _hit_palette;
    std::array<bn::optional<bn::sprite_ptr>, shot_capacity> _shots;
    std::array<bn::optional<bn::sprite_ptr>, sun_capacity> _suns;
    std::array<bn::optional<bn::sprite_ptr>, rows> _mowers;
};
} // namespace garden
#endif
