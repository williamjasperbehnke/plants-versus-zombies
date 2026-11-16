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
    {"Snow Pea", {"His peas slow a zombie's", "walk and bite. Ice won't", "get through a screen", "door, though."}},
    {"Squash", {"He waits for a zombie", "to get close, then leaps", "on it. One big landing.", "One less problem."}},
    {"Repeater", {"Two peas, same lane.", "He fires a quick pair", "every time. Put a nut", "ahead to buy him time."}},
    {"Tall-nut", {"Too tall to vault over.", "He stops pole vaulters", "and has twice the health", "of his shorter cousin."}},
    {"Chomper", {"One bite, one zombie.", "Helmets go down too.", "He takes fifteen seconds", "to chew, so guard him."}}
}};
inline constexpr std::array<AlmanacEntry, zombie_type_count> zombie_entries{{
    {"Zombie", {"No helmet. No plan.", "He just keeps walking", "toward the house until", "something stops him."}},
    {"Conehead", {"He found a traffic cone", "and wears it as a hat.", "It takes a few more peas", "to knock this one down."}},
    {"Newspaper", {"He was reading that.", "The paper takes the hits", "first. Tear it up and", "he'll come running."}},
    {"Flag Zombie", {"Where he goes, the rest", "follow. Nearby zombies", "move and bite faster", "while he's still around."}},
    {"Buckethead", {"It's a bucket. On his", "head. Somehow that takes", "a lot of peas to break.", "A mine is much quicker."}},
    {"Screen Door", {"He brought his own door.", "It stops peas and ice,", "but a Cherry Bomb goes", "straight past it."}},
    {"Football", {"Helmet on. Head down.", "He charges down the lane", "and takes a lot of hits.", "Bring ice or a mine."}},
    {"Pole Vaulter", {"He jumps the first plant", "that gets in his way.", "Tall-nuts stop his jump.", "Then he has to bite."}}
}};
inline int chomper_idle_pose(int clock)
{
    int beat = clock / 16 % 4;
    return beat == 2 ? 8 : beat % 2;
}
inline int chomper_chew_pose(int clock)
{
    constexpr int poses[] = {5, 9, 6, 10};
    return poses[clock / 10 % 4];
}
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
    case PlantType::squash: return 260 + int(pose);
    case PlantType::repeater: return 270 + int(pose);
    case PlantType::tallnut: return 280 + int(pose);
    case PlantType::chomper: return 400 + chomper_idle_pose(clock);
    default: return 0;
    }
}
inline constexpr int screen_door_first_frame = 500;
inline constexpr int screen_door_armor_frames = 6; // Two walking, four biting poses.
inline constexpr int screen_door_buff_frames = screen_door_armor_frames * 2;

inline int screen_door_bite_pose(int remaining, int interval)
{
    int age = interval - remaining;
    if(age < interval / 4) { return 3; } // Recoil after the bite.
    if(age < interval / 2) { return 0; } // Rest with the door slightly lowered.
    if(age < interval * 3 / 4) { return 1; } // Lean forward.
    return 2; // Open mouth above the fully lowered door.
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
    case ZombieType::screen_door: return screen_door_first_frame + pose;
    case ZombieType::football: return 200 + clock / 6 % 4;
    case ZombieType::vaulter: return 300 + pose;
    default: return 4;
    }
}
}
#endif
