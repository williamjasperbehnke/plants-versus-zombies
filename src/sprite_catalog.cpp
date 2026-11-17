#include "sprite_catalog.h"
#include "actor_animation.h"
#include "bn_sprite_items_wilted_plant.h"
#include "bn_regular_bg_items_lawn.h"
#include "bn_regular_bg_items_lawn_1.h"
#include "bn_regular_bg_items_lawn_3.h"
#include "bn_regular_bg_items_yard_road.h"
#include "bn_regular_bg_items_yard_road_1.h"
#include "bn_regular_bg_items_yard_road_3.h"
#include "bn_sprite_items_bucket_zombies.h"
#include "bn_sprite_items_cherries.h"
#include "bn_sprite_items_cherry_reward_packet.h"
#include "bn_sprite_items_cherry_seed_packet.h"
#include "bn_sprite_items_chomper_reward_packet.h"
#include "bn_sprite_items_chomper_seed_packet.h"
#include "bn_sprite_items_chompers.h"
#include "bn_sprite_items_completion_trophy.h"
#include "bn_sprite_items_cone_zombies.h"
#include "bn_sprite_items_emerging_zombies.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_field_items.h"
#include "bn_sprite_items_flag_zombies.h"
#include "bn_sprite_items_football_zombies.h"
#include "bn_sprite_items_newspaper_zombies.h"
#include "bn_sprite_items_normal_zombies.h"
#include "bn_sprite_items_peashooter_reward_packet.h"
#include "bn_sprite_items_peashooter_seed_packet.h"
#include "bn_sprite_items_peashooters.h"
#include "bn_sprite_items_potato_mine_reward_packet.h"
#include "bn_sprite_items_potato_mine_seed_packet.h"
#include "bn_sprite_items_potato_mines.h"
#include "bn_sprite_items_repeater_reward_packet.h"
#include "bn_sprite_items_repeater_seed_packet.h"
#include "bn_sprite_items_repeaters.h"
#include "bn_sprite_items_screen_door_zombies.h"
#include "bn_sprite_items_snow_pea_reward_packet.h"
#include "bn_sprite_items_snow_pea_seed_packet.h"
#include "bn_sprite_items_snow_peashooters.h"
#include "bn_sprite_items_squash.h"
#include "bn_sprite_items_squash_reward_packet.h"
#include "bn_sprite_items_squash_seed_packet.h"
#include "bn_sprite_items_sunflower_reward_packet.h"
#include "bn_sprite_items_sunflower_seed_packet.h"
#include "bn_sprite_items_sunflowers.h"
#include "bn_sprite_items_tallnut_reward_packet.h"
#include "bn_sprite_items_tallnut_seed_packet.h"
#include "bn_sprite_items_tallnuts.h"
#include "bn_sprite_items_vaulting_zombies.h"
#include "bn_sprite_items_wallnuts.h"
#include "bn_sprite_items_walnut_reward_packet.h"
#include "bn_sprite_items_walnut_seed_packet.h"
#include "bn_sprite_items_wounded_cone_zombies.h"
#include "bn_sprite_items_wounded_flag_zombies.h"
#include "bn_sprite_items_wounded_football_zombies.h"
#include "bn_sprite_items_wounded_newspaper_zombies.h"
#include "bn_sprite_items_wounded_normal_zombies.h"
#include "bn_sprite_items_wounded_vaulting_zombies.h"

namespace garden
{
static_assert(bn::sprite_items::wilted_plant.tiles_item().graphics_count() == wilted_plant_poses);
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

}

const bn::sprite_item& SpriteCatalog::seed(PlantType type)
{
    return *seed_assets[static_cast<int>(type)].packet;
}

const bn::sprite_item& SpriteCatalog::reward(PlantType type)
{
    return type == PlantType::count ? bn::sprite_items::completion_trophy :
                                     *seed_assets[static_cast<int>(type)].reward;
}

const bn::regular_bg_item& SpriteCatalog::lawn(int lanes)
{
    if(lanes == 1) { return bn::regular_bg_items::lawn_1; }
    if(lanes == 3) { return bn::regular_bg_items::lawn_3; }
    return bn::regular_bg_items::lawn;
}
const bn::regular_bg_item& SpriteCatalog::road(int lanes)
{
    if(lanes == 1) { return bn::regular_bg_items::yard_road_1; }
    if(lanes == 3) { return bn::regular_bg_items::yard_road_3; }
    return bn::regular_bg_items::yard_road;
}


SpriteFrame SpriteCatalog::actor(int frame, bool wounded)
{
    const bn::sprite_item* item = &bn::sprite_items::field_items;
    int graphics_index = frame - 6;
    if(frame >= wilted_plant_frame && frame < wilted_plant_frame + wilted_plant_poses)
    { item = &bn::sprite_items::wilted_plant; graphics_index = frame - wilted_plant_frame; }
    else if(frame == 9) { item = &bn::sprite_items::explosion; graphics_index = 0; }
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
    return {*item, graphics_index};
}

void draw_actor(bn::optional<bn::sprite_ptr>& sprite, bool active,
                         int x, int y, int frame, int depth, bool wounded)
{
    if(! active) { sprite.reset(); return; }
    const auto frame_item = SpriteCatalog::actor(frame, wounded);
    const auto* item = &frame_item.item;
    int graphics_index = frame_item.graphics_index;
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
}
