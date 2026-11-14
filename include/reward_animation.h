#ifndef GARDEN_REWARD_ANIMATION_H
#define GARDEN_REWARD_ANIMATION_H

namespace garden
{
inline constexpr int reward_reveal_frame = 180;
inline constexpr int reward_ready_frame = 210;
struct RewardPose { int x; int y; int scale; int height; int squash; };

inline RewardPose reward_pose(int frame, int lift = 0)
{
    if(frame < 150)
    {
        constexpr int durations[] = {48, 36, 28, 20};
        constexpr int heights[] = {35, 22, 12, 5};
        int tick = frame;
        int height = 0;
        int squash = 0;
        for(int i = 0; i < 4; ++i)
        {
            if(tick < durations[i])
            {
                int length = durations[i];
                height = 4 * heights[i] * tick * (length - tick) / (length * length);
                squash = tick < 4 ? 4 - tick : 0;
                break;
            }
            tick -= durations[i];
        }
        int travel = frame < 132 ? 132 - frame : 0;
        return {120 + 136 * travel * travel / (132 * 132), 112 - height, 50, height, squash};
    }
    int zoom = frame < reward_reveal_frame ? frame - 150 : 30;
    int remaining = 30 - zoom;
    int eased = 900 - remaining * remaining;
    return {120, 112 - (14 + lift) * eased / 900, 50 + 50 * eased / 900, 0, 0};
}
}
#endif
