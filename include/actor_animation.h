#ifndef GARDEN_ACTOR_ANIMATION_H
#define GARDEN_ACTOR_ANIMATION_H
#include "game.h"
namespace garden
{
inline constexpr int wilted_plant_frame = 600;
inline constexpr int wilted_plant_poses = 4;
inline int wilted_display_frame(int clock, int cell)
{
    constexpr int poses[] = {0, 1, 2, 1, 0, 3};
    return wilted_plant_frame + poses[(clock / 30 + cell % 3) % 6];
}
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
