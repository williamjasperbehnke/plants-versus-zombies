#include "world_renderer.h"
#include "sprite_catalog.h"
#include "actor_animation.h"
#include "bn_color.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_items_bucket_zombies.h"
#include "bn_sprite_items_damage_flash.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_flag_deaths.h"
#include "bn_sprite_items_football_deaths.h"
#include "bn_sprite_items_newspaper_deaths.h"
#include "bn_sprite_items_newspaper_zombies.h"
#include "bn_sprite_items_normal_zombies.h"
#include "bn_sprite_items_tombstones.h"
#include "bn_sprite_items_vault_deaths.h"
#include "bn_sprite_items_zombie_deaths.h"
#include "bn_sprite_items_zombie_debris.h"

namespace garden
{
void WorldRenderer::flash_red(bn::sprite_ptr& sprite)
{
    if(! _hit_palette) { _hit_palette = bn::sprite_items::damage_flash.palette_item().create_palette(); }
    sprite.set_palette(*_hit_palette);
}

void WorldRenderer::draw_tombstones(const Game& game, int pan)
{
    for(int i = 0; i < tombstone_capacity; ++i)
    {
        const auto& tombstone = game.tombstones()[i];
        auto& sprite = _tombstones[i];
        if(! tombstone.active() && tombstone.crumble_frames == 0) { sprite.reset(); continue; }
        int frame = tombstone.health > tombstone_health * 2 / 3 ? 0 :
                    (tombstone.health > tombstone_health / 3 ? 1 : 2);
        if(tombstone.crumble_frames > 0) { frame = 3 + (30 - tombstone.crumble_frames) / 6; }
        if(! sprite) { sprite = bn::sprite_items::tombstones.create_sprite(0, 0); }
        sprite->set_item(bn::sprite_items::tombstones, frame);
        sprite->set_position(cell_x(tombstone.column) - pan - 120, cell_y(tombstone.row) - 81);
        sprite->set_bg_priority(game.phase() == Phase::preview ? 0 : 2);
        sprite->set_z_order((tombstone.active() ? 2 : 3) - tombstone.row * 4);
        if(tombstone.active() && tombstone.flash > 0 && game.phase() == Phase::playing) { flash_red(*sprite); }
    }
}

void WorldRenderer::draw_zombie_effects(const Game& game)
{
    for(int i = 0; i < zombie_effect_capacity; ++i)
    {
        const auto& effect = game.zombie_effects()[i];
        auto& sprite = _zombie_effects[i];
        if(! effect.active) { sprite.reset(); continue; }
        int duration = effect_duration(effect.type);
        int age = duration - effect.remaining;
        bool corpse = corpse_effect(effect.type);
        int stages = effect_stages(effect.type);
        int debris = effect.type == ZombieEffectType::pole ? 9 : (effect.type == ZombieEffectType::flag ? 6 :
                     (effect.type == ZombieEffectType::football_arm ? 7 :
                      (effect.type == ZombieEffectType::football_helmet ? 8 : static_cast<int>(effect.type) - 2)));
        int frame = (corpse ? (effect.wounded ? stages : 0) : debris * stages) + age * stages / duration;
        const auto& item = corpse ? (effect.type == ZombieEffectType::vault_death ? bn::sprite_items::vault_deaths :
                          (effect.type == ZombieEffectType::football_death ? bn::sprite_items::football_deaths :
                          (effect.type == ZombieEffectType::flag_death ? bn::sprite_items::flag_deaths :
                          (effect.type == ZombieEffectType::newspaper_death ?
                           bn::sprite_items::newspaper_deaths : bn::sprite_items::zombie_deaths)))) :
                          bn::sprite_items::zombie_debris;
        int x = effect.x / scale;
        int y = cell_y(effect.row) - 2;
        if(! corpse)
        {
            bool arm = effect.type == ZombieEffectType::arm || effect.type == ZombieEffectType::newspaper_arm || effect.type == ZombieEffectType::football_arm;
            bool hat = effect.type == ZombieEffectType::cone || effect.type == ZombieEffectType::bucket || effect.type == ZombieEffectType::football_helmet;
            bool flag = effect.type == ZombieEffectType::flag;
            int origin = arm ? 5 : (flag ? -3 : (hat ? -13 : 3));
            int ground = arm ? 10 : (flag ? 4 : 12);
            x += arm ? -9 - age / 9 : (flag ? 3 : (hat ? 0 : -7)) + age / 3;
            y += origin + (ground - origin) * age * age / (duration * duration);
            if(! arm) { y -= 32 * age * (duration - age) / (duration * duration); }
        }
        if(! sprite) { sprite = item.create_sprite(0, 0, frame); }
        sprite->set_item(item, frame);
        if(effect.type == ZombieEffectType::newspaper_arm)
        { sprite->set_palette(bn::sprite_items::newspaper_zombies.palette_item().create_palette()); }
        sprite->set_position(x - 120, y - 80);
        sprite->set_bg_priority(2);
        sprite->set_z_order((corpse ? 1 : -3) - effect.row * 4);
    }
}

void WorldRenderer::render(const Game& game)
{
    draw_tombstones(game);
    draw_zombie_effects(game);
    for(int i = 0; i < rows * columns; ++i)
    {
        const auto& plant = game.plants()[i];
        int idle_pose = (game.elapsed() / 24 + i) % 2;
        int frame = static_cast<int>(plant.type);
        if(plant.type == PlantType::sunflower && idle_pose) { frame = 12; }
        if(plant.type == PlantType::peashooter && idle_pose) { frame = 20; }
        if(plant.type == PlantType::peashooter && plant.timer > spec(plant.type).action_delay - 6) { frame = 13; }
        if(plant.type == PlantType::cherry && idle_pose) { frame = 24; }
        if(plant.type == PlantType::cherry && plant.health == 0) { frame = 9; }
        if(plant.type == PlantType::potato_mine)
        { frame = plant.health == 0 ? 28 : (plant.timer > 0 ? 25 : 26 + game.elapsed() / 20 % 2); }
        if(plant.type == PlantType::walnut && plant.health <= spec(plant.type).health / 3) { frame = 11; }
        else if(plant.type == PlantType::walnut && plant.health <= spec(plant.type).health * 2 / 3) { frame = 10; }
        if(plant.type == PlantType::walnut && idle_pose)
        { frame = frame == 2 ? 21 : frame + 12; }
        if(plant.type == PlantType::tallnut)
        {
            int damage = plant.health <= spec(plant.type).health / 3 ? 2 :
                         (plant.health <= spec(plant.type).health * 2 / 3 ? 1 : 0);
            frame = 280 + damage * 2 + idle_pose;
        }
        bool shooter = plant.type == PlantType::peashooter || plant.type == PlantType::snow_pea || plant.type == PlantType::repeater;
        bool firing = shooter && plant.timer > spec(plant.type).action_delay - 6;
        if(plant.type == PlantType::snow_pea) { frame = firing ? 103 : (idle_pose ? 102 : 101); }
        int repeater_recoil = 0;
        if(plant.type == PlantType::repeater)
        {
            int remaining = plant.shot_frames;
            frame = remaining > 0 ? (remaining >= 5 ? 272 : (remaining >= 3 ? 273 : 275)) :
                    (plant.burst_frames > 0 && plant.burst_frames <= 2 ? 274 : 270 + idle_pose);
            repeater_recoil = remaining >= 5 ? 2 : (remaining >= 3 ? 1 : 0);
        }
        if(plant.type == PlantType::chomper)
        {
            int pose = chomper_idle_pose(game.elapsed() + i * 5);
            if(plant.bite_frames > 0) { pose = plant.bite_frames > 24 ? 2 : (plant.bite_frames > 12 ? 3 : (plant.bite_frames > 6 ? 4 : 5)); }
            else if(plant.timer > 0) { pose = plant.timer < 20 ? 7 : chomper_chew_pose(game.elapsed() + i * 5); }
            frame = 400 + pose;
        }
        int plant_x = cell_x(i % columns);
        int plant_y = cell_y(i / columns) - 1;
        if(plant.type == PlantType::chomper && plant.bite_frames > 0)
        {
            plant_x += plant.bite_frames > 24 ? -1 :
                       (plant.bite_frames > 12 ? (24 - plant.bite_frames) / 2 : plant.bite_frames / 2);
        }
        if(plant.type == PlantType::squash)
        {
            frame = 260 + idle_pose;
            if(plant.committed())
            {
                int age = 30 - plant.timer;
                frame = plant.health == 0 ? 264 : (age < 6 ? 262 : 263);
                if(plant.health == 0) { plant_x = plant.target_x / scale; }
                else if(age >= 6)
                {
                    int time = age - 6;
                    plant_x += (plant.target_x / scale - plant_x) * time / 24;
                    plant_y -= 120 * time * (24 - time) / (24 * 24);
                }
            }
        }
        int recoil = plant.type == PlantType::repeater ? repeater_recoil : (firing ? 1 : 0);
        if(plant.wilted) { frame = wilted_display_frame(game.elapsed(), i); }
        draw_actor(_plants[i], plant.active,
              plant_x - recoil, plant_y, frame, 2 - (i / columns) * 4);
        if(_plants[i] && frame != 9 && plant.flash > 0 && game.phase() == Phase::playing)
        { flash_red(*_plants[i]); }
        if(frame == 9 && _plants[i])
        {
            _plants[i]->set_tiles(bn::sprite_items::explosion.tiles_item(), (18 - plant.timer) / 6);
        }
        if(plant.type == PlantType::chomper && plant.bite_frames > 0 && _plants[i]) { _plants[i]->set_z_order(-24); }
        // Blasts cross lane boundaries, so keep both bomb effects above every plant.
        if((plant.exploding() || plant.committed()) && _plants[i]) { _plants[i]->set_z_order(-28); }
    }
    for(int i = 0; i < zombie_capacity; ++i)
    {
        const auto& zombie = game.zombies()[i];
        bool breached = game.phase() == Phase::lost && i == game.breached_zombie();
        int clock = game.elapsed() + (breached ? game.loss_frames() : 0);
        int slow = zombie.frost_frames > 0 ? 2 : 1;
        int bob = (clock / (14 * slow) + i) % 2;
        int cadence = zombie.type == ZombieType::newspaper && zombie.shield_health == 0 ? 6 : 12;
        int pose = (clock / (cadence * slow) + i) % 2;
        bool chewing = zombie.eating && ! breached;
        if(chewing)
        {
            bob = 0;
            pose = zombie.bite_timer < 12 * slow ? 1 : 0;
        }
        bool cone = zombie.type == ZombieType::cone && zombie.health > 6;
        int frame = chewing ? 16 + pose * 2 : (pose ? 14 : 4);
        if(cone) { ++frame; }
        if(zombie.type == ZombieType::newspaper)
        { frame = (zombie.shield_health > 0 ? 29 : 33) + (chewing ? 2 : 0) + pose; }
        if(zombie.type == ZombieType::flag) { frame = 37 + ((clock / 8 + i) % 4) * 4 + (chewing ? 2 : 0) + pose; }
        if(zombie.shield_health > 0 && zombie.type == ZombieType::bucket)
        {
            int damaged = zombie.shield_health * 2 <= spec(zombie.type).shield_health ? 4 : 0;
            frame = 69 + damaged + (chewing ? 2 : 0) + pose;
        }
        if(zombie.shield_health > 0 && zombie.type == ZombieType::screen_door)
        {
            int damaged = zombie.shield_health * 2 <= spec(zombie.type).shield_health ? screen_door_armor_frames : 0;
            int door_pose = chewing ? 2 + screen_door_bite_pose(zombie.bite_timer, game.bite_delay(zombie)) : pose;
            frame = screen_door_first_frame + damaged + door_pose;
        }
        if(zombie.type == ZombieType::football)
        {
            int armor = zombie.shield_health > 20 ? 0 : (zombie.shield_health > 0 ? 6 : 12);
            frame = 200 + armor + (chewing ? 4 + pose : clock / (6 * slow) % 4);
        }
        if(zombie.type == ZombieType::vaulter)
        { frame = 300 + (zombie.vault_frames > 0 ? 8 + (36 - zombie.vault_frames) / 9 :
                        (zombie.pole_used ? 4 : 0) + (chewing ? 2 : 0) + pose); }
        if(game.flag_buffed(zombie))
        {
            if(frame >= screen_door_first_frame) { frame += screen_door_buff_frames; }
            else if(frame >= 300) { frame += 12; }
            else if(frame >= 200) { frame += 18; }
            else if(frame >= 69) { frame += 8; }
            else if(frame >= 29) { frame += 32; }
            else { frame = frame < 6 ? frame + 49 : frame + 41; }
        }
        if(zombie.emergence_frames > 0)
        {
            int stage = (emergence_duration - zombie.emergence_frames) * emergence_stages / emergence_duration;
            frame = 105 + stage + (game.flag_buffed(zombie) ? emergence_stages : 0);
            bob = 0;
        }
        int draw_x = zombie.x / scale - (breached ? (game.loss_frames() < 90 ? game.loss_frames() : 90) * 24 / 90 : 0);
        int draw_y = cell_y(zombie.row) + bob - 2 - (zombie.vault_frames > 0 ? 88 * zombie.vault_frames * (36 - zombie.vault_frames) / (36 * 36) : 0);
        int pull = 0;
        for(int column = 0; column < columns; ++column)
        {
            const auto& plant = game.plants()[cell_index(zombie.row, column)];
            if(plant.active && plant.type == PlantType::chomper && plant.bite_target == i &&
               plant.bite_frames <= 24 && plant.bite_frames > 12)
            {
                pull = 24 - plant.bite_frames;
                draw_x += (cell_x(column) + 14 - draw_x) * pull / 12;
                draw_y += pull / 3;
                break;
            }
        }
        draw_actor(_zombies[i], zombie.active, draw_x, draw_y, frame,
              pull > 0 ? -23 : (zombie.vault_frames > 0 ? -24 : -zombie.row * 4) - (zombie.type == ZombieType::flag ? 1 : 0), zombie.arm_lost);
        if(_zombies[i])
        {
            if(pull > 0) { _zombies[i]->set_scale(bn::fixed(100 - pull * 5) / 100); }
            else { _zombies[i]->remove_affine_mat(); }
        }
        bool hit = zombie.flash > 0 && game.phase() == Phase::playing;
        if(_zombies[i] && hit) { flash_red(*_zombies[i]); }
        else if(_zombies[i] && zombie.frost_frames > 0)
        {
            int palette = zombie.type == ZombieType::newspaper ? 1 :
                          ((zombie.type == ZombieType::bucket || zombie.type == ZombieType::screen_door) &&
                           zombie.shield_health > 0 ? 2 : 0);
            if(! _frost_palettes[palette])
            {
                const auto& item = palette == 1 ? bn::sprite_items::newspaper_zombies :
                                   (palette == 2 ? bn::sprite_items::bucket_zombies : bn::sprite_items::normal_zombies);
                alignas(int) bn::color colors[16];
                const auto& source = item.palette_item().colors_ref();
                for(int color = 0; color < 16; ++color)
                {
                    colors[color] = bn::color((source[color].red() * 11 + 8 * 9) / 20,
                                              (source[color].green() * 11 + 20 * 9) / 20,
                                              (source[color].blue() * 11 + 31 * 9) / 20);
                }
                colors[15] = source[15]; // Ground shadows have their own color, independent of status effects.
                _frost_palettes[palette] = bn::sprite_palette_ptr::create(
                    bn::sprite_palette_item(bn::span<const bn::color>(colors, 16), bn::bpp_mode::BPP_4));
            }
            _zombies[i]->set_palette(*_frost_palettes[palette]);
        }
    }
    for(int i = 0; i < shot_capacity; ++i)
    {
        const auto& shot = game.shots()[i];
        draw_actor(_shots[i], shot.active, shot.x / scale, cell_y(shot.row) - 3, shot.frozen ? 104 : 6, -2 - shot.row * 4);
    }
    for(int i = 0; i < sun_capacity; ++i)
    {
        const auto& sun = game.suns()[i];
        bool visible = sun.active && (sun.collecting || sun.lifetime > 120 || game.elapsed() % 20 < 14);
        draw_actor(_suns[i], visible, sun.x / scale, sun.y / scale, 7, sun.collecting ? -64 : -32);
        if(_suns[i]) { _suns[i]->set_bg_priority(sun.collecting ? 0 : 2); }
    }
    for(int i = 0; i < rows; ++i)
    {
        const auto& mower = game.mowers()[i];
        draw_actor(_mowers[i], game.row_active(i) && (mower.available || mower.moving), mower.x / scale, cell_y(i), 8, -1 - i * 4);
    }
}

void WorldRenderer::clear()
{
    for(auto& sprite : _plants) { sprite.reset(); }
    for(auto& sprite : _zombies) { sprite.reset(); }
    for(auto& sprite : _zombie_effects) { sprite.reset(); }
    for(auto& sprite : _tombstones) { sprite.reset(); }
    for(auto& palette : _frost_palettes) { palette.reset(); }
    _hit_palette.reset();
    for(auto& sprite : _shots) { sprite.reset(); }
    for(auto& sprite : _suns) { sprite.reset(); }
    for(auto& sprite : _mowers) { sprite.reset(); }
}

void WorldRenderer::preview(const Game& game, int pan, int priority)
{
    draw_tombstones(game, pan);
    for(int i = 0; i < rows * columns; ++i)
    {
        const auto& plant = game.plants()[i];
        int frame = plant.wilted ? wilted_display_frame(0, i) : plant_display_frame(plant.type, 0);
        draw_actor(_plants[i], plant.active, cell_x(i % columns) - pan, cell_y(i / columns) - 1, frame,
                   2 - (i / columns) * 4);
        if(_plants[i]) { _plants[i]->set_bg_priority(priority); }
    }
    for(int row = 0; row < rows; ++row)
    {
        draw_actor(_mowers[row], game.row_active(row), 21 - pan, cell_y(row), 8);
        if(_mowers[row]) { _mowers[row]->set_bg_priority(priority); }
    }
}

void WorldRenderer::behind_overlay()
{
    auto behind_panel = [](auto& pool)
    {
        for(auto& sprite : pool) { if(sprite) { sprite->set_bg_priority(2); } }
    };
    behind_panel(_plants);
    behind_panel(_tombstones);
    behind_panel(_zombies);
    behind_panel(_zombie_effects);
    behind_panel(_shots);
    behind_panel(_suns);
    behind_panel(_mowers);
}
}
