#ifndef GARDEN_ALMANAC_H
#define GARDEN_ALMANAC_H
#include "game.h"

namespace garden
{
struct AlmanacEntry
{
    const char* name;
    std::array<const char*, 4> description;
};
// Plant entries follow stable asset IDs; the displayed roster follows seed_order.
inline constexpr std::array<AlmanacEntry, plant_count> plant_entries{{
    {"Sunflower", {"She makes sun every nine", "seconds. A few by the", "house will help pay for", "the rest of your plants."}},
    {"Peashooter", {"He fires peas straight", "down his lane. Nothing", "fancy. A row of these", "will get you a long way."}},
    {"Wall-nut", {"He just sits there and", "takes the bites. Put him", "in front of plants that", "need a little more time."}},
    {"Cherry Bomb", {"They don't hang around.", "A short fuse, then they", "take out zombies across", "three nearby lanes."}},
    {"Potato Mine", {"He needs eight seconds", "to get settled. After", "that, the first zombie", "to step on him gets it."}},
    {"Snow Pea", {"His peas slow a zombie's", "walk and bite. Ice won't", "get through a screen", "door, though."}}
}};
inline constexpr std::array<AlmanacEntry, zombie_type_count> zombie_entries{{
    {"Zombie", {"No helmet. No plan.", "He just keeps walking", "toward the house until", "something stops him."}},
    {"Conehead", {"He found a traffic cone", "and wears it as a hat.", "It takes a few more peas", "to knock this one down."}},
    {"Newspaper", {"He was reading that.", "The paper takes the hits", "first. Tear it up and", "he'll come running."}},
    {"Flag Zombie", {"Where he goes, the rest", "follow. Nearby zombies", "move and bite faster", "while he's still around."}},
    {"Buckethead", {"It's a bucket. On his", "head. Somehow that takes", "a lot of peas to break.", "A mine is much quicker."}},
    {"Screen Door", {"He brought his own door.", "It stops peas and ice,", "but a Cherry Bomb goes", "straight past it."}}
}};
inline int plant_display_frame(PlantType type, int clock)
{
    bool pose = clock / 30 % 2;
    switch(type)
    {
    case PlantType::sunflower: return pose ? 12 : 0;
    case PlantType::peashooter: return pose ? 20 : 1;
    case PlantType::walnut: return pose ? 21 : 2;
    case PlantType::cherry: return pose ? 24 : 3;
    case PlantType::potato_mine: return 26 + clock / 20 % 2;
    case PlantType::snow_pea: return 101 + int(pose);
    default: return 0;
    }
}
inline int zombie_display_frame(ZombieType type, int clock)
{
    int pose = clock / 16 % 2;
    switch(type)
    {
    case ZombieType::normal: return pose ? 14 : 4;
    case ZombieType::cone: return pose ? 15 : 5;
    case ZombieType::newspaper: return 29 + pose;
    case ZombieType::flag: return 37 + clock / 8 % 4 * 4 + pose;
    case ZombieType::bucket: return 69 + pose;
    case ZombieType::screen_door: return 85 + pose;
    default: return 4;
    }
}
}
#endif
