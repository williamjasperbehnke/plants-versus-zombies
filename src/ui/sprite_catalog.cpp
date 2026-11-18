#include "ui/sprite_catalog.h"

#include "bn_assert.h"
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
#include "bn_sprite_items_flag_deaths.h"
#include "bn_sprite_items_flag_zombies.h"
#include "bn_sprite_items_football_deaths.h"
#include "bn_sprite_items_football_zombies.h"
#include "bn_sprite_items_newspaper_deaths.h"
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
#include "bn_sprite_items_vault_deaths.h"
#include "bn_sprite_items_vaulting_zombies.h"
#include "bn_sprite_items_wallnuts.h"
#include "bn_sprite_items_walnut_reward_packet.h"
#include "bn_sprite_items_walnut_seed_packet.h"
#include "bn_sprite_items_wilted_plant.h"
#include "bn_sprite_items_wounded_cone_zombies.h"
#include "bn_sprite_items_wounded_flag_zombies.h"
#include "bn_sprite_items_wounded_football_zombies.h"
#include "bn_sprite_items_wounded_newspaper_zombies.h"
#include "bn_sprite_items_wounded_normal_zombies.h"
#include "bn_sprite_items_wounded_vaulting_zombies.h"
#include "bn_sprite_items_zombie_deaths.h"
#include "bn_sprite_items_zombie_debris.h"
#include "game/animation.h"

namespace garden {
static_assert(bn::sprite_items::wilted_plant.tiles_item().graphics_count() == wilted_plant_poses);
static_assert(
    bn::sprite_items::screen_door_zombies.tiles_item().graphics_count() == screen_door_buff_frames * 2
);

namespace {
struct SeedArt {
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

} // namespace

const bn::sprite_item& SpriteCatalog::seed(PlantType type) {
    return *seed_assets[static_cast<int>(type)].packet;
}

const bn::sprite_item& SpriteCatalog::reward(PlantType type) {
    if (type == PlantType::count) {
        return bn::sprite_items::completion_trophy;
    }
    return *seed_assets[static_cast<int>(type)].reward;
}

const bn::regular_bg_item& SpriteCatalog::lawn(int lanes) {
    if (lanes == 1) {
        return bn::regular_bg_items::lawn_1;
    }
    if (lanes == 3) {
        return bn::regular_bg_items::lawn_3;
    }
    return bn::regular_bg_items::lawn;
}

const bn::regular_bg_item& SpriteCatalog::road(int lanes) {
    if (lanes == 1) {
        return bn::regular_bg_items::yard_road_1;
    }
    if (lanes == 3) {
        return bn::regular_bg_items::yard_road_3;
    }
    return bn::regular_bg_items::yard_road;
}

SpriteFrame SpriteCatalog::actor(ActorFrame frame, bool wounded) {
    const bn::sprite_item* item = nullptr;
    switch (frame.sheet) {
        case ActorSheet::sunflower:
            item = &bn::sprite_items::sunflowers;
            break;
        case ActorSheet::peashooter:
            item = &bn::sprite_items::peashooters;
            break;
        case ActorSheet::walnut:
            item = &bn::sprite_items::wallnuts;
            break;
        case ActorSheet::cherry:
            item = &bn::sprite_items::cherries;
            break;
        case ActorSheet::potato_mine:
            item = &bn::sprite_items::potato_mines;
            break;
        case ActorSheet::snow_pea:
            item = &bn::sprite_items::snow_peashooters;
            break;
        case ActorSheet::squash:
            item = &bn::sprite_items::squash;
            break;
        case ActorSheet::repeater:
            item = &bn::sprite_items::repeaters;
            break;
        case ActorSheet::tallnut:
            item = &bn::sprite_items::tallnuts;
            break;
        case ActorSheet::chomper:
            item = &bn::sprite_items::chompers;
            break;
        case ActorSheet::normal_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_normal_zombies;
            } else {
                item = &bn::sprite_items::normal_zombies;
            }
            break;
        case ActorSheet::cone_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_cone_zombies;
            } else {
                item = &bn::sprite_items::cone_zombies;
            }
            break;
        case ActorSheet::newspaper_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_newspaper_zombies;
            } else {
                item = &bn::sprite_items::newspaper_zombies;
            }
            break;
        case ActorSheet::flag_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_flag_zombies;
            } else {
                item = &bn::sprite_items::flag_zombies;
            }
            break;
        case ActorSheet::bucket_zombie:
            item = &bn::sprite_items::bucket_zombies;
            break;
        case ActorSheet::screen_door_zombie:
            item = &bn::sprite_items::screen_door_zombies;
            break;
        case ActorSheet::football_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_football_zombies;
            } else {
                item = &bn::sprite_items::football_zombies;
            }
            break;
        case ActorSheet::vaulter_zombie:
            if (wounded) {
                item = &bn::sprite_items::wounded_vaulting_zombies;
            } else {
                item = &bn::sprite_items::vaulting_zombies;
            }
            break;
        case ActorSheet::wilted_plant:
            item = &bn::sprite_items::wilted_plant;
            break;
        case ActorSheet::explosion:
            item = &bn::sprite_items::explosion;
            break;
        case ActorSheet::emerging_zombie:
            item = &bn::sprite_items::emerging_zombies;
            break;
        case ActorSheet::field_items:
            item = &bn::sprite_items::field_items;
            break;
        case ActorSheet::zombie_death:
            item = &bn::sprite_items::zombie_deaths;
            break;
        case ActorSheet::newspaper_death:
            item = &bn::sprite_items::newspaper_deaths;
            break;
        case ActorSheet::flag_death:
            item = &bn::sprite_items::flag_deaths;
            break;
        case ActorSheet::football_death:
            item = &bn::sprite_items::football_deaths;
            break;
        case ActorSheet::vault_death:
            item = &bn::sprite_items::vault_deaths;
            break;
        case ActorSheet::zombie_debris:
            item = &bn::sprite_items::zombie_debris;
            break;
        default:
            break;
    }
    BN_ASSERT(item, "Unknown actor sheet");
    BN_ASSERT(frame.index >= 0 && frame.index < item->tiles_item().graphics_count(), "Invalid actor pose");
    return {*item, frame.index};
}

void draw_actor(
    bn::optional<bn::sprite_ptr>& sprite, bool active, int x, int y, ActorFrame frame, int depth, bool wounded
) {
    if (!active) {
        sprite.reset();
        return;
    }
    const auto frame_item = SpriteCatalog::actor(frame, wounded);
    const auto* item = &frame_item.item;
    int graphics_index = frame_item.graphics_index;
    if (!sprite) {
        sprite = item->create_sprite(x - 120, y - 80, graphics_index);
        sprite->set_bg_priority(2);
    }
    // Butano shares atlas tiles/palettes between matching sprites.
    sprite->set_item(*item, graphics_index);
    sprite->set_position(x - 120, y - 80);
    sprite->set_z_order(depth);
}
} // namespace garden
