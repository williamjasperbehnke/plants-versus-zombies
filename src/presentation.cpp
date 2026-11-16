#include "bn_sprite_items_scouting_heading.h"
#include "bn_sprite_items_sunflower_seed_packet.h"
#include "bn_sprite_items_sunflower_reward_packet.h"
#include "bn_sprite_items_peashooter_seed_packet.h"
#include "bn_sprite_items_peashooter_reward_packet.h"
#include "bn_sprite_items_walnut_seed_packet.h"
#include "bn_sprite_items_walnut_reward_packet.h"
#include "bn_sprite_items_cherry_seed_packet.h"
#include "bn_sprite_items_cherry_reward_packet.h"
#include "bn_sprite_items_potato_mine_seed_packet.h"
#include "bn_sprite_items_potato_mine_reward_packet.h"
#include "bn_sprite_items_snow_pea_seed_packet.h"
#include "bn_sprite_items_snow_pea_reward_packet.h"
#include "bn_sprite_items_squash_seed_packet.h"
#include "bn_sprite_items_squash_reward_packet.h"
#include "bn_sprite_items_repeater_seed_packet.h"
#include "bn_sprite_items_repeater_reward_packet.h"
#include "bn_sprite_items_tallnut_seed_packet.h"
#include "bn_sprite_items_tallnut_reward_packet.h"
#include "bn_sprite_items_chomper_seed_packet.h"
#include "bn_sprite_items_chomper_reward_packet.h"
#include "bn_sprite_items_chompers.h"
#include "bn_regular_bg_items_lawn_1.h"
#include "bn_regular_bg_items_lawn_3.h"
#include "bn_regular_bg_items_yard_road_1.h"
#include "bn_regular_bg_items_yard_road_3.h"
#include "bn_sprite_items_tallnuts.h"
#include "presentation.h"
#include "almanac.h"
#include "bn_regular_bg_items_seed_select.h"
#include "bn_sprite_items_squash.h"
#include "bn_sprite_items_vault_deaths.h"
#include "bn_sprite_items_repeaters.h"
#include "bn_sprite_items_vaulting_zombies.h"
#include "bn_sprite_items_wounded_vaulting_zombies.h"
#include "bn_sprite_items_tombstones.h"
#include "menu_layout.h"
#include "bn_regular_bg_items_yard_road.h"
#include "bn_regular_bg_items_almanac.h"
#include "bn_sprite_items_title_options.h"
#include "bn_sprite_items_almanac_cell.h"
#include "bn_sprite_items_almanac_stats.h"
#include "bn_regular_bg_items_almanac_entry.h"
#include "bn_string.h"
#include "wave_track.h"
#include "bn_sprite_items_field_items.h"
#include "bn_sprite_items_sunflowers.h"
#include "bn_sprite_items_peashooters.h"
#include "bn_sprite_items_wallnuts.h"
#include "bn_sprite_items_cherries.h"
#include "bn_sprite_items_potato_mines.h"
#include "bn_sprite_items_normal_zombies.h"
#include "bn_sprite_items_cone_zombies.h"
#include "bn_sprite_items_damage_flash.h"
#include "bn_sprite_items_reward_shadow.h"
#include "bn_regular_bg_items_plant_reward.h"
#include "bn_regular_bg_items_trophy_reward.h"
#include "bn_regular_bg_items_campaign_trophy.h"
#include "bn_regular_bg_items_loss_message.h"
#include "bn_regular_bg_items_loss_prompt.h"
#include "bn_sprite_items_completion_trophy.h"
#include "bn_sprite_items_newspaper_zombies.h"
#include "bn_sprite_items_flag_zombies.h"
#include "bn_sprite_items_bucket_zombies.h"
#include "bn_sprite_items_screen_door_zombies.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_selection.h"
#include "bn_sprite_items_snow_peashooters.h"
#include "bn_color.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_emerging_zombies.h"
#include "bn_sprite_items_wounded_normal_zombies.h"
#include "bn_sprite_items_wounded_cone_zombies.h"
#include "bn_sprite_items_wounded_newspaper_zombies.h"
#include "bn_sprite_items_wounded_flag_zombies.h"
#include "bn_sprite_items_zombie_deaths.h"
#include "bn_sprite_items_newspaper_deaths.h"
#include "bn_sprite_items_flag_deaths.h"
#include "bn_sprite_items_football_zombies.h"
#include "bn_sprite_items_wounded_football_zombies.h"
#include "bn_sprite_items_football_deaths.h"
#include "bn_sprite_items_zombie_debris.h"
#include "bn_sprite_items_wave_bar.h"
#include "bn_sprite_items_hud_icons.h"
#include "bn_sprite_items_phase_clock.h"
#include "bn_sprite_items_volume_slider.h"
#include "bn_sprite_items_title_plants.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_items_level_tiles.h"
#include "bn_sprite_items_level_cursor.h"
#include "bn_regular_bg_items_level_select.h"
#include "bn_sprite_items_wave_bar_short.h"
#include "bn_regular_bg_items_title.h"
#include "bn_regular_bg_items_lawn.h"
#include "bn_regular_bg_items_paused.h"
#include "common_variable_8x8_sprite_font.h"
#include "menu_font.h"

namespace garden
{
static_assert(wave_track::boundaries.size() == wave_count + 1);
static_assert(bn::sprite_items::screen_door_zombies.tiles_item().graphics_count() == screen_door_buff_frames * 2);

namespace
{
struct SeedArt
{
    const bn::sprite_item* packet;
    const bn::sprite_item* reward;
};
// Stable PlantType order: each entry has a bank sheet and a reward sheet.
constexpr std::array<SeedArt, plant_count> seed_assets = {{
    {&bn::sprite_items::sunflower_seed_packet, &bn::sprite_items::sunflower_reward_packet},
    {&bn::sprite_items::peashooter_seed_packet, &bn::sprite_items::peashooter_reward_packet},
    {&bn::sprite_items::walnut_seed_packet, &bn::sprite_items::walnut_reward_packet},
    {&bn::sprite_items::cherry_seed_packet, &bn::sprite_items::cherry_reward_packet},
    {&bn::sprite_items::potato_mine_seed_packet, &bn::sprite_items::potato_mine_reward_packet},
    {&bn::sprite_items::snow_pea_seed_packet, &bn::sprite_items::snow_pea_reward_packet},
    {&bn::sprite_items::squash_seed_packet, &bn::sprite_items::squash_reward_packet},
    {&bn::sprite_items::repeater_seed_packet, &bn::sprite_items::repeater_reward_packet},
    {&bn::sprite_items::tallnut_seed_packet, &bn::sprite_items::tallnut_reward_packet},
    {&bn::sprite_items::chomper_seed_packet, &bn::sprite_items::chomper_reward_packet},
}};

const bn::sprite_item& seed_art(PlantType type)
{
    return *seed_assets[static_cast<int>(type)].packet;
}

const bn::sprite_item& reward_art(PlantType type)
{
    return type == PlantType::count ? bn::sprite_items::completion_trophy :
                                     *seed_assets[static_cast<int>(type)].reward;
}

const bn::regular_bg_item& lawn_art(int lanes)
{
    if(lanes == 1) { return bn::regular_bg_items::lawn_1; }
    if(lanes == 3) { return bn::regular_bg_items::lawn_3; }
    return bn::regular_bg_items::lawn;
}
const bn::regular_bg_item& road_art(int lanes)
{
    if(lanes == 1) { return bn::regular_bg_items::yard_road_1; }
    if(lanes == 3) { return bn::regular_bg_items::yard_road_3; }
    return bn::regular_bg_items::yard_road;
}

}

// Gameplay only generates the sun counter; menu text does not share these live entity pools.
constexpr int visible_packets = plant_count < visible_seed_count ? plant_count : visible_seed_count;
static_assert(rows * columns + zombie_capacity + shot_capacity + sun_capacity + rows +
              visible_packets + wave_count + tombstone_capacity + zombie_effect_capacity + 2 + 3 + 2 + 4 <= 128, "Sprite pools exceed GBA OAM capacity");

Presentation::Presentation() :
    _background(bn::regular_bg_items::lawn.create_bg(8, 48)),
    _menu(bn::regular_bg_items::title.create_bg(8, 48)),
    _text(menu_font),
    _sun_text(common::variable_8x8_sprite_font),
    _cursor(bn::sprite_items::cursor.create_sprite(0, 0)),
    _selection(bn::sprite_items::selection.create_sprite(0, 0))
{
    _background.set_priority(3);
    _background.set_visible(false);
    _menu->set_priority(1);
    _text.set_bg_priority(0);
    _sun_text.set_bg_priority(0);
    _cursor.set_bg_priority(0);
    _cursor.set_z_order(-10);
    _selection.set_bg_priority(0);
    _selection.set_z_order(-11);
    _cursor.set_visible(false);
    _selection.set_visible(false);
}

void Presentation::change_screen(const Game& game)
{
    Phase phase = game.phase();
    bool same_road = (phase == Phase::preview || phase == Phase::seed_select) &&
                     (_screen == Phase::preview || _screen == Phase::seed_select);
    _screen = phase;
    _text.set_palette_item(phase == Phase::almanac ? almanac_ink : menu_font.item().palette_item());
    if(phase != Phase::won && phase != Phase::lost) { _hud.clear(); }
    _last_sun = -1;
    _last_pause_choice = -1;
    for(auto& slider : _volume_sliders) { slider.reset(); }
    for(auto& plant : _title_plants) { plant.reset(); }
    _title_options.reset();
    for(auto& sprite : _gallery) { sprite.reset(); }
    _portrait.reset();
    _almanac_panel.reset();
    _entry_text.clear();
    for(auto& icon : _entry_icons) { icon.reset(); }
    _gallery_cursor.reset();
    for(auto& sprite : _bank_packets) { sprite.reset(); }
    _last_entry = -1;
    for(auto& tile : _level_tiles) { tile.reset(); }
    _level_cursor.reset();
    if(! same_road) { _title_frame = 0; }
    _preview_text.clear();
    for(auto& half : _preview_heading) { half.reset(); }
    _preview_pan = 0;
    _loss_notice_stage = 0;
    _reward_packet.reset();
    _reward_shadow.reset();
    _menu.reset();
    // The gameplay background remains its own resource. Menus use another layer
    // and independent artwork, with no copied lawn, seed bar, or gameplay HUD.
    switch(phase)
    {
    case Phase::title: _menu = bn::regular_bg_items::title.create_bg(8, 48); break;
    case Phase::playing: break;
    case Phase::seed_select:
        _menu = road_art(game.level_spec().lanes).create_bg(-104, 48);
        _almanac_panel = bn::regular_bg_items::seed_select.create_bg(8, 48);
        _almanac_panel->set_priority(1);
        break;
    case Phase::preview: _menu = road_art(game.level_spec().lanes).create_bg(136, 48); break;
    case Phase::almanac: _menu = bn::regular_bg_items::almanac.create_bg(8, 48); break;
    case Phase::level_select: _menu = bn::regular_bg_items::level_select.create_bg(8, 48); break;
    case Phase::paused: _menu = bn::regular_bg_items::paused.create_bg(8, 48); break;
    case Phase::won:
    case Phase::lost: break;
    default: break;
    }
    bool playing = phase == Phase::playing;
    bool reward = phase == Phase::won || phase == Phase::lost;
    if(playing) { _background.set_item(lawn_art(game.level_spec().lanes)); }
    _background.set_visible(playing || reward);
    if(_menu) { _menu->set_priority(phase == Phase::seed_select ? 2 : 1); }
    _cursor.set_visible(playing);
    _selection.set_visible(playing);
    if(! playing && ! reward) { clear_entities(); }
}

void Presentation::clear_entities()
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
    for(auto& sprite : _seed_packets) { sprite.reset(); }
    for(auto& sprite : _seed_arrows) { sprite.reset(); }
    for(auto& sprite : _wave_bar) { sprite.reset(); }
    _wave_head.reset();
    _phase_icon.reset();
    _feedback_icon.reset();
}

void Presentation::actor(bn::optional<bn::sprite_ptr>& sprite, bool active,
                         int x, int y, int frame, int depth, bool wounded)
{
    if(! active) { sprite.reset(); return; }
    const bn::sprite_item* item = &bn::sprite_items::field_items;
    int graphics_index = frame - 6;
    if(frame == 9) { item = &bn::sprite_items::explosion; graphics_index = 0; }
    else if(frame >= screen_door_first_frame) { item = &bn::sprite_items::screen_door_zombies; graphics_index = frame - screen_door_first_frame; }
    else if(frame >= 400) { item = &bn::sprite_items::chompers; graphics_index = frame - 400; }
    else if(frame >= 300) { item = &bn::sprite_items::vaulting_zombies; graphics_index = frame - 300; }
    else if(frame >= 280) { item = &bn::sprite_items::tallnuts; graphics_index = frame - 280; }
    else if(frame >= 270) { item = &bn::sprite_items::repeaters; graphics_index = frame - 270; }
    else if(frame >= 260) { item = &bn::sprite_items::squash; graphics_index = frame - 260; }
    else if(frame >= 200) { item = &bn::sprite_items::football_zombies; graphics_index = frame - 200; }
    else if(frame >= 105) { item = &bn::sprite_items::emerging_zombies; graphics_index = frame - 105; }
    else if(frame >= 101) { item = &bn::sprite_items::snow_peashooters; graphics_index = frame - 101; }
    else if(frame >= 69) { item = &bn::sprite_items::bucket_zombies; graphics_index = frame - 69; }
    else if(frame >= 61) { item = &bn::sprite_items::newspaper_zombies; graphics_index = frame - 53; }
    else if(frame >= 53)
    {
        int local = frame - 53;
        item = local % 2 ? &bn::sprite_items::cone_zombies : &bn::sprite_items::normal_zombies;
        graphics_index = 4 + local / 2;
    }
    else if(frame >= 37) { item = &bn::sprite_items::flag_zombies; graphics_index = frame - 37; }
    else if(frame >= 29) { item = &bn::sprite_items::newspaper_zombies; graphics_index = frame - 29; }
    else if(frame == 4 || frame == 5 || (frame >= 14 && frame <= 19))
    {
        item = frame % 2 ? &bn::sprite_items::cone_zombies : &bn::sprite_items::normal_zombies;
        graphics_index = frame < 6 ? 0 : (frame - 12) / 2;
    }
    else if(frame == 0 || frame == 12)
    { item = &bn::sprite_items::sunflowers; graphics_index = frame == 12; }
    else if(frame == 1 || frame == 13 || frame == 20)
    { item = &bn::sprite_items::peashooters; graphics_index = frame == 1 ? 0 : (frame == 13 ? 1 : 2); }
    else if(frame == 2 || frame == 10 || frame == 11 || (frame >= 21 && frame <= 23))
    { item = &bn::sprite_items::wallnuts; graphics_index = frame == 2 ? 0 : (frame < 12 ? frame - 9 : frame - 18); }
    else if(frame == 3 || frame == 24)
    { item = &bn::sprite_items::cherries; graphics_index = frame == 24; }
    else if(frame >= 25 && frame <= 28)
    { item = &bn::sprite_items::potato_mines; graphics_index = frame - 25; }
    if(wounded)
    {
        if(item == &bn::sprite_items::vaulting_zombies) { item = &bn::sprite_items::wounded_vaulting_zombies; }
        else if(item == &bn::sprite_items::football_zombies) { item = &bn::sprite_items::wounded_football_zombies; }
        else if(item == &bn::sprite_items::normal_zombies) { item = &bn::sprite_items::wounded_normal_zombies; }
        else if(item == &bn::sprite_items::cone_zombies) { item = &bn::sprite_items::wounded_cone_zombies; }
        else if(item == &bn::sprite_items::newspaper_zombies) { item = &bn::sprite_items::wounded_newspaper_zombies; }
        else if(item == &bn::sprite_items::flag_zombies) { item = &bn::sprite_items::wounded_flag_zombies; }
    }
    if(! sprite)
    {
        sprite = item->create_sprite(x - 120, y - 80, graphics_index);
        sprite->set_bg_priority(2);
    }
    // Butano shares atlas tiles/palettes between matching sprites.
    sprite->set_item(*item, graphics_index);
    sprite->set_position(x - 120, y - 80);
    sprite->set_z_order(depth);
}

void Presentation::flash_red(bn::sprite_ptr& sprite)
{
    if(! _hit_palette) { _hit_palette = bn::sprite_items::damage_flash.palette_item().create_palette(); }
    sprite.set_palette(*_hit_palette);
}

void Presentation::draw_tombstones(const Game& game, int pan)
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

void Presentation::draw_zombie_effects(const Game& game)
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

void Presentation::draw_entities(const Game& game)
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
        actor(_plants[i], plant.active,
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
        actor(_zombies[i], zombie.active, draw_x, draw_y, frame,
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
        actor(_shots[i], shot.active, shot.x / scale, cell_y(shot.row) - 3, shot.frozen ? 104 : 6, -2 - shot.row * 4);
    }
    for(int i = 0; i < sun_capacity; ++i)
    {
        const auto& sun = game.suns()[i];
        bool visible = sun.active && (sun.collecting || sun.lifetime > 120 || game.elapsed() % 20 < 14);
        actor(_suns[i], visible, sun.x / scale, sun.y / scale, 7, sun.collecting ? -64 : -32);
        if(_suns[i]) { _suns[i]->set_bg_priority(sun.collecting ? 0 : 2); }
    }
    for(int i = 0; i < rows; ++i)
    {
        const auto& mower = game.mowers()[i];
        actor(_mowers[i], game.row_active(i) && (mower.available || mower.moving), mower.x / scale, cell_y(i), 8, -1 - i * 4);
    }
}

void Presentation::draw_wave_bar(const Game& game)
{
    // Ease one shared progress value so the fill and zombie head remain aligned.
    int target = game.wave_progress() * 256;
    if(_bar_progress < 0 || target < _bar_progress) { _bar_progress = target; }
    int difference = target - _bar_progress;
    int step = difference / 8;
    if(step < 8) { step = 8; }
    if(step > 512) { step = 512; }
    _bar_progress += difference < step ? difference : step;
    int progress = _bar_progress / 256;
    int total = game.total_waves();
    const auto& item = total == 3 ? bn::sprite_items::wave_bar_short : bn::sprite_items::wave_bar;
    for(int i = 0; i < wave_count; ++i)
    {
        auto& segment = _wave_bar[i];
        if(i >= total) { segment.reset(); continue; }
        int frames_per_state = 168 + total * 2;
        int state = ! game.horde_entered(i + 1) ? 0 : (game.horde_remaining(i + 1) ? 1 : 2);
        int frame = state * frames_per_state + wave_track::frame_offset(i, total);
        if(game.wave() > i) { frame += 1 + wave_track::fill(i, progress, total); }
        if(! segment)
        {
            segment = item.create_sprite(
                281 - wave_track::boundary(i, total) - wave_track::sprite_width / 2 - 120,
                wave_track::sprite_height / 2 - 80, frame);
            segment->set_bg_priority(0);
        }
        segment->set_tiles(item.tiles_item(), frame);
    }
    if(! _wave_head)
    {
        _wave_head = bn::sprite_items::hud_icons.create_sprite(0, wave_track::head_y - 80, 5);
        _wave_head->set_bg_priority(0);
        _wave_head->set_z_order(-2);
    }
    _wave_head->set_x(wave_track::display_position(progress, total) - 120);
}

void Presentation::draw_feedback(const Game& game, const ViewState& view)
{
    int frame = -1;
    int x = view.feedback_x;
    int y = view.feedback_y - 6;
    if(view.shoveling) { frame = 2; x = cell_x(view.cursor.column); y = cell_y(view.cursor.row) - 6; }
    else if(view.feedback_frames > 0)
    {
        switch(view.feedback)
        {
        case Feedback::planted: frame = 1; y -= (30 - view.feedback_frames) / 5; break;
        case Feedback::removed: frame = 2; break;
        case Feedback::occupied: frame = 0; break;
        case Feedback::cooling_down:
        {
            int recharge = spec(view.feedback_seed).recharge;
            int stage = (recharge - game.cooldown(view.feedback_seed)) * 3 / recharge;
            if(stage > 2) { stage = 2; }
            frame = stage == 0 ? 3 : 9 + stage;
            break;
        }
        case Feedback::insufficient_sun:
            // The packet border pulses red; the bank's sun icon stays still.
            break;
        case Feedback::collected: break; // Each sun animates in its own entity slot.
        default: break;
        }
    }
    if(frame < 0) { _feedback_icon.reset(); }
    else
    {
        if(! _feedback_icon)
        {
            _feedback_icon = bn::sprite_items::hud_icons.create_sprite(x - 120, y - 80, frame);
            _feedback_icon->set_bg_priority(0);
            _feedback_icon->set_z_order(-12);
        }
        _feedback_icon->set_tiles(bn::sprite_items::hud_icons.tiles_item(), frame);
        _feedback_icon->set_position(x - 120, y - 80);
    }
    bool rejected = view.feedback_frames > 0 && view.cursor.seed == view.feedback_seed &&
        (view.feedback == Feedback::insufficient_sun || view.feedback == Feedback::cooling_down);
    int selection_frame = 0;
    if(rejected && view.feedback_frames % 8 < 4)
    { selection_frame = view.feedback == Feedback::cooling_down ? 2 : 1; }
    _selection.set_tiles(bn::sprite_items::selection.tiles_item(), selection_frame);

    int remaining = game.next_wave_frames();
    if(remaining > 0)
    {
        constexpr int duration = 900;
        int clock = (duration - remaining) * 16 / duration;
        if(! _phase_icon) { _phase_icon = bn::sprite_items::phase_clock.create_sprite(-105, -68, clock); }
        _phase_icon->set_item(bn::sprite_items::phase_clock, clock);
        _phase_icon->set_bg_priority(0);
    }
    else { _phase_icon.reset(); }
}

void Presentation::draw_hud(const Game& game, const ViewState& view)
{
    // All packets report their readiness, even when another seed is selected.
    for(int i = 0; i < plant_count; ++i)
    {
        if(i >= game.available_plants() || i < _seed_page || i >= _seed_page + visible_seed_count)
        { _seed_packets[i].reset(); continue; }
        auto type = game.seed_at(i);
        int remaining = game.cooldown(type);
        const auto& item = seed_art(type);
        int frame = seed_layout::frame(remaining, spec(type).recharge, game.sun() >= spec(type).cost);
        auto& packet = _seed_packets[i];
        if(! packet)
        {
            packet = item.create_sprite(
                seed_layout::center(i - _seed_page) - 120, seed_layout::center_y - 80, frame);
            packet->set_bg_priority(0);
        }
        packet->set_tiles(item.tiles_item(), frame);
    }
    for(int i = 0; i < 2; ++i)
    {
        if(game.available_plants() <= visible_seed_count) { _seed_arrows[i].reset(); continue; }
        if(! _seed_arrows[i])
        {
            _seed_arrows[i] = bn::sprite_items::hud_icons.create_sprite(i == 0 ? -76 : 108, -42, 12 + i);
            _seed_arrows[i]->set_bg_priority(0);
        }
    }
    if(game.sun() != _last_sun)
    {
        _last_sun = game.sun();
        _hud.clear();
        _sun_text.generate(-116, -35, bn::to_string<8>(game.sun()), _hud);
    }
    int bounce = view.feedback == Feedback::collected ? view.feedback_frames / 8 : 0;
    for(auto& sprite : _hud) { sprite.set_y(-35 - bounce); }
}

void Presentation::draw_title(const ViewState& view)
{
    constexpr int centers[] = {35, 88};
    for(int i = 0; i < 2; ++i)
    {
        if(! _title_plants[i])
        {
            _title_plants[i] = bn::sprite_items::title_plants.create_sprite(centers[i] - 120, 17);
            _title_plants[i]->set_bg_priority(0);
        }
        int pose = (_title_frame / 24 + i) % 2;
        _title_plants[i]->set_tiles(bn::sprite_items::title_plants.tiles_item(), i * 2 + pose);
    }
    if(! _title_options)
    {
        _title_options = bn::sprite_items::title_options.create_sprite(56, 16);
        _title_options->set_bg_priority(0);
    }
    _title_options->set_tiles(bn::sprite_items::title_options.tiles_item(), static_cast<int>(view.title_choice));
    _title_frame = (_title_frame + 1) % 144;
}

void Presentation::draw_level_select(const Game& game)
{
    for(int i = 0; i < levels_per_world; ++i)
    {
        int level = game.world_spec().first_level + i;
        int state = ! game.level_unlocked(level) ? 0 : (game.level_completed(level) ? 2 : 1);
        if(! _level_tiles[i])
        {
            _level_tiles[i] = bn::sprite_items::level_tiles.create_sprite(menu_layout::level_x(i), menu_layout::level_y(i));
            _level_tiles[i]->set_bg_priority(0);
        }
        _level_tiles[i]->set_tiles(bn::sprite_items::level_tiles.tiles_item(), state * 10 + i);
    }
    if(! _level_cursor)
    {
        _level_cursor = bn::sprite_items::level_cursor.create_sprite(0, 0);
        _level_cursor->set_bg_priority(0);
        _level_cursor->set_z_order(-10);
    }
    int local = game.level() % levels_per_world;
    _level_cursor->set_position(menu_layout::level_x(local), menu_layout::level_y(local));
    if(_last_pause_choice != game.level())
    {
        _last_pause_choice = game.level();
        _hud.clear();
        _text.set_center_alignment();
        bn::string<48> label(game.level_spec().name);
        if(! game.level_unlocked(game.level())) { label += " - Locked"; }
        _text.generate(0, 54, label, _hud);
        _text.set_left_alignment();
    }
}

void Presentation::draw_preview(const Game& game)
{
    int time = game.preview_frames();
    int remaining = time < preview_pan_frames ? preview_pan_frames - time : 0;
    int pan = 240 - 240 * remaining * remaining / (preview_pan_frames * preview_pan_frames);
    if(game.preview_returning()) { pan = 240 - pan; }
    draw_road(game, pan, true);
}

void Presentation::draw_road(const Game& game, int pan, bool labels)
{
    _menu->set_x(136 - pan);
    draw_tombstones(game, pan);
    // Staggered, fixed road positions keep small rosters near the yard and avoid parade rows.
    constexpr std::array<PacketPosition, zombie_type_count> crowd{{
        {300, 94}, {318, 136}, {349, 104}, {370, 131},
        {398, 91}, {419, 138}, {447, 103}, {463, 137}
    }};
    std::array<PacketPosition, zombie_type_count> positions{};
    int slot_position = 0;
    for(int i = 0; i < zombie_type_count; ++i)
    {
        if(! game.zombie_roster()[i]) { continue; }
        positions[i] = crowd[slot_position++];
    }
    for(int row = 0; row < rows; ++row)
    {
        actor(_mowers[row], game.row_active(row), 21 - pan, cell_y(row), 8);
        if(_mowers[row]) { _mowers[row]->set_bg_priority(labels ? 0 : 2); }
    }
    int slot = 0;
    for(int i = 0; i < zombie_type_count; ++i)
    {
        if(! game.zombie_roster()[i]) { continue; }
        int x = positions[i].x - pan;
        int y = positions[i].y;
        actor(_gallery[slot], true, x, y,
              zombie_display_frame(static_cast<ZombieType>(i), _title_frame + slot * 11), 160 - y);
        _gallery[slot]->set_bg_priority(labels ? 0 : 2);
        ++slot;
    }
    ++_title_frame;
    if(! labels) { return; }
    for(int half = 0; half < 2; ++half)
    {
        auto& sprite = _preview_heading[half];
        if(! sprite)
        {
            sprite = bn::sprite_items::scouting_heading.create_sprite(0, -48, half);
            sprite->set_bg_priority(0);
        }
        sprite->set_x(328 + half * 64 - pan - 120);
    }
    if(_preview_text.empty())
    {
        _text.set_center_alignment();
        _text.generate(240, -70, game.level_spec().name, _preview_text);
        _text.generate(240, -36, game.unlocked_plants() <= visible_seed_count ?
                       "A/START: BEGIN   B: BACK" : "A/START: SEEDS   B: BACK", _preview_text);
        for(int i = 0; i < zombie_type_count; ++i)
        {
            int count = game.zombie_roster()[i];
            if(! count) { continue; }
            bn::string<16> label("x"); label += bn::to_string<4>(count);
            _text.generate(positions[i].x - 120, positions[i].y + 17 - 80, label, _preview_text);
        }
        _text.set_left_alignment();
    }
    for(auto& label : _preview_text) { label.set_x(label.x() + _preview_pan - pan); }
    _preview_pan = pan;

}

void Presentation::draw_seed_select(const Game& game, const ViewState& view)
{
    draw_road(game, 240, false);
    bool error = view.seed_error_frames > 0;
    int key = view.seed_choice | (error ? 1 << 20 : 0);
    for(int i = 0; i < plant_count; ++i)
    {
        auto type = seed_order[i];
        bool unlocked = game.plant_unlocked(type);
        bool selected = game.plant_available(type);
        if(selected) { key |= 1 << (i + 4); }
        int x = menu_layout::seed_x(i);
        int y = menu_layout::seed_y(i);
        if(! unlocked) { _seed_packets[i].reset(); continue; }
        const auto& item = seed_art(type);
        int frame = seed_layout::frame((selected || _bank_packets[i]) ? spec(type).recharge : 0, spec(type).recharge, unlocked);
        if(! _seed_packets[i]) { _seed_packets[i] = item.create_sprite(x - 120, y - 80, frame); }
        _seed_packets[i]->set_item(item, frame);
        _seed_packets[i]->set_bg_priority(0);

    }
    draw_seed_bank(game);
    _selection.set_visible(true);
    _selection.set_position(menu_layout::seed_x(view.seed_choice) - 120, menu_layout::seed_y(view.seed_choice) - 80);
    _selection.set_tiles(bn::sprite_items::selection.tiles_item(), error && view.seed_error_frames / 8 % 2 ? 1 : 0);
    if(key == _last_entry) { return; }
    _last_entry = key;
    _hud.clear();
    _text.set_center_alignment();
    bn::string<32> count; count += bn::to_string<2>(game.available_plants()); count += "/"; count += bn::to_string<2>(game.required_seeds());
    _text.generate(95, -71, "SEEDS", _hud);
    _text.generate(95, -59, count, _hud);
    if(error)
    {
        _text.set_palette_item(menu_error_ink);
        _text.generate(-38, -30, "SELECT 6 SEEDS", _hud);
        _text.set_palette_item(menu_font.item().palette_item());
    }
    else
    {
        _text.generate(-38, -30, game.plant_unlocked(seed_order[view.seed_choice]) ?
                       plant_entries[static_cast<int>(seed_order[view.seed_choice])].name : "", _hud);
    }
    _text.generate(0, 75, "A:PICK START:PLAY B:BACK", _hud);
    _text.set_left_alignment();
}

void Presentation::draw_seed_bank(const Game& game)
{
    auto approach = [](int current, int target)
    {
        int delta = target - current;
        int step = delta / 4;
        if(! step && delta) { step = delta > 0 ? 1 : -1; }
        return current + step;
    };
    for(int i = 0; i < plant_count; ++i)
    {
        auto type = seed_order[i];
        bool selected = game.plant_available(type);
        auto& packet = _bank_packets[i];
        auto& position = _packet_positions[i];
        if(! selected && ! packet) { continue; }
        const auto& item = seed_art(type);
        int frame = seed_layout::frame(0, spec(type).recharge, true);
        if(! packet)
        {
            position = {menu_layout::seed_x(i), menu_layout::seed_y(i)};
            packet = item.create_sprite(0, 0, frame);
            packet->set_bg_priority(0);
            packet->set_z_order(-5);
        }
        int x = selected ? menu_layout::bank_x(game.bank_slot(type)) : menu_layout::seed_x(i);
        int y = selected ? menu_layout::bank_y : menu_layout::seed_y(i);
        position = {approach(position.x, x), approach(position.y, y)};
        packet->set_position(position.x - 120, position.y - 80);
        if(! selected && position.x == x && position.y == y) { packet.reset(); }
    }
}

void Presentation::draw_almanac(const Game& game, const ViewState& view)
{
    int index = view.almanac_entry;
    int count = view.almanac_zombies ? zombie_type_count : plant_count;
    int page = index / 6;
    int local = index % 6;
    int key = index + (view.almanac_zombies ? plant_count : 0) + (view.almanac_details ? 64 : 0);
    _menu->set_priority(3);
    auto draw_entry = [this, &view](auto& sprite, int entry, int x, int y, int clock)
    {
        int frame = view.almanac_zombies ? zombie_display_frame(static_cast<ZombieType>(entry), clock) :
                                          plant_display_frame(seed_order[entry], clock);
        actor(sprite, true, x, y, frame);
    };
    for(int i = 0; i < 6; ++i)
    {
        int entry = page * 6 + i;
        if(entry >= count) { _gallery[i].reset(); continue; }
        draw_entry(_gallery[i], entry, 54 + i % menu_layout::almanac_columns * 66, 62 + i / menu_layout::almanac_columns * 46,
                   (view.almanac_zombies ? _title_frame / 2 : _title_frame) + i * 7);
        _gallery[i]->set_bg_priority(2);
    }
    if(! _gallery_cursor)
    {
        _gallery_cursor = bn::sprite_items::almanac_cell.create_sprite(0, 0);
        _gallery_cursor->set_bg_priority(2);
        _gallery_cursor->set_z_order(-10);
    }
    _gallery_cursor->set_position(54 + local % menu_layout::almanac_columns * 66 - 120, 64 + local / menu_layout::almanac_columns * 46 - 80);
    if(view.almanac_details)
    {
        if(! _almanac_panel)
        {
            _almanac_panel = bn::regular_bg_items::almanac_entry.create_bg(8, 48);
            _almanac_panel->set_priority(1);
        }
        draw_entry(_portrait, index, 64, 80, view.almanac_zombies ? _title_frame / 2 : _title_frame);
        _portrait->set_bg_priority(0);
        _portrait->set_scale(2);
        _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
    }
    else
    {
        _almanac_panel.reset();
        for(auto& icon : _entry_icons) { icon.reset(); }
        _portrait.reset();
        _entry_text.clear();
    }
    ++_title_frame;
    if(_last_entry == key) { return; }
    _last_entry = key;
    _hud.clear();
    _entry_text.clear();
    _text.set_center_alignment();
    _text.set_bg_priority(2);
    _text.generate(0, -42, view.almanac_zombies ? (page ? "ZOMBIES 2/2" : "ZOMBIES 1/2") : (page ? "PLANTS 2/2" : "PLANTS 1/2"), _hud);
    for(int i = 0; i < 6; ++i)
    {
        int entry_index = page * 6 + i;
        if(entry_index >= count) { continue; }
        const auto& entry = view.almanac_zombies ? zombie_entries[entry_index] : plant_entries[static_cast<int>(seed_order[entry_index])];
        _text.generate(54 + i % menu_layout::almanac_columns * 66 - 120, i / menu_layout::almanac_columns * 46, entry.name, _hud);
    }
    _text.set_bg_priority(0);
    if(view.almanac_details)
    {
        const auto& entry = view.almanac_zombies ? zombie_entries[index] : plant_entries[static_cast<int>(seed_order[index])];
        _text.generate(0, -46, entry.name, _entry_text);
        auto stat = [this](int slot, int frame, int x, int y, const auto& value)
        {
            _entry_icons[slot] = bn::sprite_items::almanac_stats.create_sprite(x - 120, y - 80, frame);
            _entry_icons[slot]->set_bg_priority(0);
            _text.set_left_alignment();
            _text.generate(x + 12 - 120, y - 80, value, _entry_text);
            _text.set_center_alignment();
        };
        for(auto& icon : _entry_icons) { icon.reset(); }
        if(view.almanac_zombies)
        {
            const auto& definition = spec(static_cast<ZombieType>(index));
            stat(0, 1, 78, 120, bn::to_string<4>(definition.health + definition.shield_health));
            stat(1, 3, 143, 120, bn::to_string<12>(game.lifetime_kills(static_cast<ZombieType>(index))));
        }
        else
        {
            const auto& definition = spec(seed_order[index]);
            stat(0, 0, 50, 120, bn::to_string<4>(definition.cost));
            stat(1, 1, 107, 120, bn::to_string<4>(definition.health));
            int frames = definition.recharge;
            bn::string<16> recharge = bn::to_string<4>(frames / 60);
            if(frames % 60) { recharge += ".5"; }
            recharge += "s";
            stat(2, 2, 164, 120, recharge);
        }
        for(int i = 0; i < static_cast<int>(entry.description.size()); ++i)
        { _text.generate(39, -20 + i * 10, entry.description[i], _entry_text); }

    }
    _text.set_left_alignment();
}

void Presentation::draw_menu(const Game& game, const ViewState& view)
{
    if(game.phase() == Phase::title) { draw_title(view); return; }
    if(game.phase() == Phase::preview) { draw_preview(game); return; }
    if(game.phase() == Phase::seed_select) { draw_seed_select(game, view); return; }
    if(game.phase() == Phase::almanac) { draw_almanac(game, view); return; }
    if(game.phase() == Phase::level_select) { draw_level_select(game); return; }
    if(game.phase() == Phase::won) { draw_reward(game); return; }
    if(game.phase() == Phase::lost) { draw_loss(game); return; }
    int choice = static_cast<int>(view.pause_choice);
    if(_last_sun != -1 && choice == _last_pause_choice && _last_audio == view.audio.key()) { return; }
    _last_audio = view.audio.key();
    _last_sun = game.sun();
    _last_pause_choice = choice;
    _hud.clear();
    _text.set_center_alignment();
    if(game.phase() == Phase::paused)
    {
        _text.set_left_alignment();
        constexpr const char* choices[] = {"Resume", "Restart", "Levels", "Master", "Music", "Effects"};
        constexpr int positions[] = {54, 68, 82, 102, 116, 130};
        for(int i = 0; i < static_cast<int>(PauseChoice::count); ++i)
        {
            _text.generate(-60, positions[i] - 80, choices[i], _hud);
        }
        _text.generate(-78, positions[choice] - 80, ">", _hud);
        for(int i = 0; i < 3; ++i)
        {
            if(! _volume_sliders[i])
            {
                _volume_sliders[i] = bn::sprite_items::volume_slider.create_sprite(48, positions[i + 3] - 80);
                _volume_sliders[i]->set_bg_priority(0);
            }
            _volume_sliders[i]->set_tiles(bn::sprite_items::volume_slider.tiles_item(), view.audio.levels[i]);
        }
    }
    _text.set_left_alignment();
}

void Presentation::put_entities_behind_overlay()
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
    behind_panel(_seed_packets);
    behind_panel(_seed_arrows);
    behind_panel(_wave_bar);
    for(auto* sprite : {&_wave_head, &_phase_icon, &_feedback_icon})
    { if(*sprite) { (*sprite)->set_bg_priority(2); } }
    for(auto& sprite : _hud) { sprite.set_bg_priority(2); }
}

void Presentation::draw_loss(const Game& game)
{
    draw_entities(game);
    put_entities_behind_overlay();
    int frame = game.loss_frames();
    int stage = frame < 30 ? 0 : (game.loss_ready() ? 2 : 1);
    if(stage != _loss_notice_stage)
    {
        _loss_notice_stage = stage;
        const auto& item = stage == 2 ? bn::regular_bg_items::loss_prompt : bn::regular_bg_items::loss_message;
        _menu = item.create_bg(8, 48);
        _menu->set_priority(1);
    }
    if(_menu)
    {
        int drop = frame < 42 ? (frame - 42) * 2 : (frame < 48 ? 2 : 0);
        _menu->set_y(48 + drop);
    }
}

void Presentation::draw_reward(const Game& game)
{
    int frame = game.reward_frames();
    if(frame == 0) { draw_entities(game); }
    if(game.victory_settling()) { return; }
    int type = static_cast<int>(game.unlocked_plant());
    if(! _reward_packet)
    {
        _reward_packet = reward_art(game.unlocked_plant()).create_sprite(0, 100);
        _reward_packet->set_bg_priority(0);
        _reward_packet->set_z_order(-20);
    }
    constexpr int lift = 8;
    const auto pose = reward_pose(frame, lift);
    if(frame < reward_reveal_frame)
    {
        if(! _reward_shadow)
        {
            _reward_shadow = bn::sprite_items::reward_shadow.create_sprite(0, 46);
            _reward_shadow->set_bg_priority(0);
            _reward_shadow->set_z_order(1);
        }
        _reward_shadow->set_position(pose.x - 120, 46);
        _reward_shadow->set_scale(bn::fixed(100 - pose.height) / 100);
    }
    else if(! _menu)
    {
        put_entities_behind_overlay();
        _reward_shadow.reset();
        const auto& panel = game.unlocked_plant() != PlantType::count ? bn::regular_bg_items::plant_reward :
                            (game.has_next_level() ? bn::regular_bg_items::trophy_reward : bn::regular_bg_items::campaign_trophy);
        _menu = panel.create_bg(8, 48);
        _menu->set_priority(1);
    }
    _reward_packet->set_position(pose.x - 120, pose.y - 80);
    _reward_packet->set_scale(bn::fixed(pose.scale + pose.squash * 2) / 50,
                              bn::fixed(pose.scale - pose.squash * 2) / 50);
    if(! game.reward_ready() || _last_sun != -1) { return; }
    _last_sun = game.sun();
    constexpr const char* descriptions[] = {"Makes more sun", "Fires peas", "Blocks zombies", "Blasts nearby foes", "Arms, then bursts", "Slows zombies", "Crushes nearby foes", "Fires two peas", "Stops pole vaulters", "Swallows whole zombies"};
    _text.set_center_alignment();
    // Move the information with the unlock sign, retaining its internal text margins.
    _text.generate(0, 58 - lift, (game.unlocked_plant() == PlantType::count ? game.level_spec().name : plant_entries[type].name), _hud);
    _text.generate(0, 70 - lift, (game.unlocked_plant() == PlantType::count ? "Garden defended!" : descriptions[type]), _hud);
    _text.set_left_alignment();
}

void Presentation::update(const Game& game, const ViewState& view)
{
    if(game.phase() != _screen) { change_screen(game); }
    if(game.phase() != Phase::playing) { draw_menu(game, view); return; }
    int page = _seed_page < 0 ? 0 : _seed_page;
    int selected = game.bank_slot(view.cursor.seed);
    if(selected < page) { page = selected; }
    else if(selected >= page + visible_seed_count) { page = selected - visible_seed_count + 1; }
    if(page != _seed_page)
    {
        _seed_page = page;
        for(auto& packet : _seed_packets) { packet.reset(); }
    }
    const auto& cursor = view.cursor;
    bool on_feedback_tile = cell_x(cursor.column) == view.feedback_x && cell_y(cursor.row) == view.feedback_y;
    bool rejected = on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::occupied;
    int shake = rejected ? (view.feedback_frames % 4 < 2 ? -1 : 1) : 0;
    _cursor.set_position(cell_x(cursor.column) - 120 + shake, cell_y(cursor.row) - 80);
    bool blocked = game.plants()[cell_index(cursor.row, cursor.column)].active ||
                   game.tombstone_at(cursor.row, cursor.column) ||
                   game.sun() < spec(cursor.seed).cost || game.cooldown(cursor.seed) > 0;
    int tint = view.shoveling ? 2 : (blocked ? 1 : 0);
    if(on_feedback_tile && view.feedback_frames > 0 && view.feedback == Feedback::planted) { tint = 3; }
    _cursor.set_tiles(bn::sprite_items::cursor.tiles_item(), tint);
    _selection.set_position(seed_layout::center(game.bank_slot(cursor.seed) - _seed_page) - 120, seed_layout::center_y - 80);
    draw_entities(game);
    draw_wave_bar(game);
    draw_feedback(game, view);
    draw_hud(game, view);
}
}
