#ifndef GARDEN_ALMANAC_H
#define GARDEN_ALMANAC_H
#include "game/game.h"
#include "ui/text_wrap.h"

namespace garden {
struct AlmanacEntry {
    const char* name;
    const char* description;
};

// Plant entries follow stable asset IDs; the displayed roster follows seed_order.
inline constexpr std::array<AlmanacEntry, plant_count> plant_entries{
    {{"Sunflower",
      "Sunflowers turn a quiet patch of lawn into a steady supply of sun. Plant a few near the "
      "house early, then spend their sunshine on the rest of your defense. Each flower produces "
      "sun about every nine seconds."},
     {"Peashooter",
      "A dependable first line of defense. Peashooter sends one pea at a time straight down his lane. Plant "
      "him early and keep something sturdy between his leaves and hungry teeth."},
     {"Wall-nut",
      "Wall-nut has a tough shell and plenty of patience. He holds zombies in place while the plants behind "
      "him do the shooting. His cracked shell shows how much more biting he can take."},
     {"Cherry Bomb",
      "These two share a very short fuse. Once planted, they explode across three nearby "
      "lanes. Save them for a crowd, and remember that they are gone after the blast."},
     {"Potato Mine",
      "Potato Mine spends eight seconds getting ready underground. Once his red light is blinking, the next "
      "zombie that steps close sets him off. Plant ahead of trouble; he needs that head start."},
     {"Snow Pea",
      "Snow Pea fires chilled peas that slow both walking and biting. A slower crowd gives every "
      "plant more time to work. A screen door blocks the chill until the shield is broken."},
     {"Squash",
      "Squash watches for a nearby zombie, leaps up, and lands with his whole weight. He only gets "
      "one jump, but it can flatten several zombies standing together."},
     {"Repeater",
      "Repeater sends two peas down his lane in a quick pair. He costs more sun than a "
      "Peashooter and makes good use of a Wall-nut holding the crowd still."},
     {"Tall-nut",
      "Tall-nut has twice his shorter cousin's health and stands too high for a pole vaulter to "
      "clear. Put him where jumping zombies would otherwise slip past your defense."},
     {"Chomper",
      "Chomper waits with his mouth open and swallows a nearby zombie whole, armor and all. He "
      "can reach over a plant directly in front of him. Give him fifteen seconds to finish "
      "chewing before his next meal."}}
};
inline constexpr std::array<AlmanacEntry, zombie_type_count> zombie_entries{
    {{"Zombie",
      "An ordinary backyard visitor with an extraordinary appetite. He walks toward the house and "
      "bites anything planted in his way. A few well-placed peas will stop him."},
     {"Conehead",
      "A traffic cone makes a surprisingly useful helmet. Break the cone before your peas can "
      "finish the zombie underneath. His orange hat gets battered as it takes damage."},
     {"Newspaper",
      "His newspaper shields him while he shuffles along. Tear it apart and he drops the leisurely pace, "
      "charging toward the house. There is still a full-strength zombie behind that paper."},
     {"Flag Zombie",
      "A flag means a horde is arriving. While its bearer is alive, nearby zombies walk and "
      "bite faster. Bringing him down early helps take the rush out of the crowd."},
     {"Buckethead",
      "He has traded a traffic cone for a heavy metal bucket. It takes a lot of peas to break "
      "this helmet. An explosive, a hungry Chomper, or a patient defense can save the day."},
     {"Screen Door",
      "His screen door catches peas and blocks their icy effect. Keep firing to break it, or use an "
      "explosive to get around the shield. Once the door is gone, he is a regular zombie."},
     {"Football",
      "Football Zombie charges quickly and wears heavy protective gear. Slow him with icy peas "
      "or prepare a mine before he arrives. An unguarded lane will not stay safe for long."},
     {"Pole Vaulter",
      "He carries a pole to vault over the first plant in his lane. After the jump he walks "
      "and bites normally. A Tall-nut is too high to clear and forces him to stop."}}
};
using AlmanacDescription = WrappedText<24, 32>;
inline constexpr int almanac_visible_lines = 4;

inline int almanac_scrollbar_frame(int lines, int scroll) {
    int size = 0;
    if (lines <= 6) {
        size = 3;
    } else if (lines <= 8) {
        size = 2;
    } else if (lines <= 12) {
        size = 1;
    }
    int limit = lines - almanac_visible_lines;
    return size * 25 + (limit > 0 ? scroll * 24 / limit : 0);
}

inline const AlmanacEntry& almanac_entry(bool zombies, int index) {
    return zombies ? zombie_entries[index] : plant_entries[static_cast<int>(seed_order[index])];
}

inline int almanac_scroll_limit(bool zombies, int index) {
    int lines = AlmanacDescription(almanac_entry(zombies, index).description).size();
    return lines > almanac_visible_lines ? lines - almanac_visible_lines : 0;
}
} // namespace garden
#endif
