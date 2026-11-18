#include "ui/world_renderer.h"

#include "bn_color.h"
#include "bn_sprite_items_bucket_zombies.h"
#include "bn_sprite_items_damage_flash.h"
#include "bn_sprite_items_newspaper_zombies.h"
#include "bn_sprite_items_normal_zombies.h"
#include "bn_sprite_items_tombstones.h"
#include "bn_sprite_palette_item.h"
#include "game/animation.h"
#include "ui/sprite_catalog.h"

namespace garden {
void WorldRenderer::flash_red(bn::sprite_ptr& sprite) {
    if (!_hit_palette) {
        _hit_palette = bn::sprite_items::damage_flash.palette_item().create_palette();
    }
    sprite.set_palette(*_hit_palette);
}

void WorldRenderer::draw_tombstones(const Game& game, int pan) {
    for (int i = 0; i < tombstone_capacity; ++i) {
        const auto& tombstone = game.tombstones()[i];
        auto& sprite = _tombstones[i];
        if (!tombstone.active() && tombstone.crumble_frames == 0) {
            sprite.reset();
            continue;
        }
        int frame = damage_stage(tombstone.health, tombstone_health);
        if (tombstone.crumble_frames > 0) {
            frame = 3 + (30 - tombstone.crumble_frames) / 6;
        }
        if (!sprite) {
            sprite = bn::sprite_items::tombstones.create_sprite(0, 0);
        }
        sprite->set_item(bn::sprite_items::tombstones, frame);
        sprite->set_position(cell_x(tombstone.column) - pan - 120, cell_y(tombstone.row) - 81);
        sprite->set_bg_priority(game.phase() == Phase::preview ? 0 : 2);
        sprite->set_z_order((tombstone.active() ? 2 : 3) - tombstone.row * 4);
        if (tombstone.active() && tombstone.flash > 0 && game.phase() == Phase::playing) {
            flash_red(*sprite);
        }
    }
}

void WorldRenderer::draw_zombie_effects(const Game& game) {
    for (int i = 0; i < zombie_effect_capacity; ++i) {
        const auto& effect = game.zombie_effects()[i];
        auto& sprite = _zombie_effects[i];
        if (!effect.active) {
            sprite.reset();
            continue;
        }
        const auto pose = zombie_effect_animation(effect);
        draw_actor(sprite, true, pose.x, pose.y, pose.frame, pose.depth);
        if (effect.type == ZombieEffectType::newspaper_arm) {
            sprite->set_palette(bn::sprite_items::newspaper_zombies.palette_item().create_palette());
        }
    }
}

void WorldRenderer::apply_zombie_palette(bn::sprite_ptr& sprite, const Zombie& zombie, Phase phase) {
    bool hit = zombie.flash > 0 && phase == Phase::playing;
    if (hit) {
        flash_red(sprite);
    } else if (zombie.frost_frames > 0) {
        int palette = 0;
        if (zombie.type == ZombieType::newspaper) {
            palette = 1;
        } else if ((zombie.type == ZombieType::bucket || zombie.type == ZombieType::screen_door) &&
                   zombie.shield_health > 0) {
            palette = 2;
        }
        if (!_frost_palettes[palette]) {
            const bn::sprite_item* item = &bn::sprite_items::normal_zombies;
            if (palette == 1) {
                item = &bn::sprite_items::newspaper_zombies;
            } else if (palette == 2) {
                item = &bn::sprite_items::bucket_zombies;
            }
            alignas(int) bn::color colors[16];
            const auto& source = item->palette_item().colors_ref();
            for (int color = 0; color < 16; ++color) {
                colors[color] = bn::color(
                    (source[color].red() * 11 + 8 * 9) / 20,
                    (source[color].green() * 11 + 20 * 9) / 20,
                    (source[color].blue() * 11 + 31 * 9) / 20
                );
            }
            colors[15] = source[15]; // Ground shadows have their own color, independent of status effects.
            _frost_palettes[palette] = bn::sprite_palette_ptr::create(
                bn::sprite_palette_item(bn::span<const bn::color>(colors, 16), bn::bpp_mode::BPP_4)
            );
        }
        sprite.set_palette(*_frost_palettes[palette]);
    }
}

void WorldRenderer::render(const Game& game) {
    draw_tombstones(game);
    draw_zombie_effects(game);
    for (int i = 0; i < rows * columns; ++i) {
        const auto& plant = game.plants()[i];
        const auto pose = plant_animation(plant, game.elapsed(), i);
        draw_actor(_plants[i], plant.active, pose.x, pose.y, pose.frame, pose.depth);
        if (_plants[i] && pose.frame.sheet != ActorSheet::explosion && plant.flash > 0 &&
            game.phase() == Phase::playing) {
            flash_red(*_plants[i]);
        }
    }
    for (int i = 0; i < zombie_capacity; ++i) {
        const auto& zombie = game.zombies()[i];
        const auto pose = zombie_animation(game, i);
        auto& sprite = _zombies[i];
        draw_actor(sprite, zombie.active, pose.x, pose.y, pose.frame, pose.depth, zombie.arm_lost);
        if (!sprite) {
            continue;
        }
        if (pose.scale_percent < 100) {
            sprite->set_scale(bn::fixed(pose.scale_percent) / 100);
        } else {
            sprite->remove_affine_mat();
        }
        apply_zombie_palette(*sprite, zombie, game.phase());
    }
    for (int i = 0; i < shot_capacity; ++i) {
        const auto& shot = game.shots()[i];
        draw_actor(
            _shots[i],
            shot.active,
            shot.x / scale,
            cell_y(shot.row) - 3,
            ActorFrame{shot.frozen ? ActorSheet::snow_pea : ActorSheet::field_items, shot.frozen ? 3 : 0},
            -2 - shot.row * 4
        );
    }
    for (int i = 0; i < sun_capacity; ++i) {
        const auto& sun = game.suns()[i];
        bool visible = sun.active && (sun.collecting || sun.lifetime > 120 || game.elapsed() % 20 < 14);
        draw_actor(
            _suns[i],
            visible,
            sun.x / scale,
            sun.y / scale,
            {ActorSheet::field_items, 1},
            sun.collecting ? -64 : -32
        );
        if (_suns[i]) {
            _suns[i]->set_bg_priority(sun.collecting ? 0 : 2);
        }
    }
    for (int i = 0; i < rows; ++i) {
        const auto& mower = game.mowers()[i];
        draw_actor(
            _mowers[i],
            game.row_active(i) && (mower.available || mower.moving),
            mower.x / scale,
            cell_y(i),
            {ActorSheet::field_items, 2},
            -1 - i * 4
        );
    }
}

void WorldRenderer::clear() {
    for (auto& sprite : _plants) {
        sprite.reset();
    }
    for (auto& sprite : _zombies) {
        sprite.reset();
    }
    for (auto& sprite : _zombie_effects) {
        sprite.reset();
    }
    for (auto& sprite : _tombstones) {
        sprite.reset();
    }
    for (auto& palette : _frost_palettes) {
        palette.reset();
    }
    _hit_palette.reset();
    for (auto& sprite : _shots) {
        sprite.reset();
    }
    for (auto& sprite : _suns) {
        sprite.reset();
    }
    for (auto& sprite : _mowers) {
        sprite.reset();
    }
}

void WorldRenderer::preview(const Game& game, int pan, int priority) {
    draw_tombstones(game, pan);
    for (int i = 0; i < rows * columns; ++i) {
        const auto& plant = game.plants()[i];
        ActorFrame frame;
        if (plant.wilted) {
            frame = wilted_display_frame(0, i);
        } else {
            frame = plant_display_frame(plant.type, 0);
        }
        draw_actor(
            _plants[i],
            plant.active,
            cell_x(i % columns) - pan,
            cell_y(i / columns) - 1,
            frame,
            2 - (i / columns) * 4
        );
        if (_plants[i]) {
            _plants[i]->set_bg_priority(priority);
        }
    }
    for (int row = 0; row < rows; ++row) {
        draw_actor(_mowers[row], game.row_active(row), 21 - pan, cell_y(row), {ActorSheet::field_items, 2});
        if (_mowers[row]) {
            _mowers[row]->set_bg_priority(priority);
        }
    }
}

void WorldRenderer::behind_overlay() {
    auto behind_panel = [](auto& pool) {
        for (auto& sprite : pool) {
            if (sprite) {
                sprite->set_bg_priority(2);
            }
        }
    };
    behind_panel(_plants);
    behind_panel(_tombstones);
    behind_panel(_zombies);
    behind_panel(_zombie_effects);
    behind_panel(_shots);
    behind_panel(_suns);
    behind_panel(_mowers);
}
} // namespace garden
