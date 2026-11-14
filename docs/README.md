# Current screenshots

These images come from the current normal and debug ROMs in mGBA. They use integer
nearest-neighbor scaling. The linked selection below replaces the old three-level
campaign and runner-era gallery.

![Title, pause, and level selection](menus.png)

| View | Captures |
| --- | --- |
| Gameplay | [Lawn and HUD](gameplay.png), [house-side border](lawn_edge.png), [seed packets](seed_packets.png) |
| Menus | [Title](title.png), [pause and audio](paused.png) |
| Progress | [Locked levels](levels_locked.png), [completion and next unlock](levels_progress.png) |
| Scouting | [Backyard 1-1](preview_level_1.png), [Backyard 1-10](preview_level_10.png) |
| Almanac | [Plants](almanac_plants.png), [zombies](almanac_zombies.png), [Sunflower details](almanac_sunflower.png), [Screen Door details](almanac_screen_door.png), [lifetime kills](almanac_lifetime_kills.png) |
| Rewards | [Packet bouncing over the lawn](plant_reward_mid.png), [unlock reveal](plant_reward.png), [bounce animation](plant_reward.gif), [repeat-clear trophy](trophy_reward.png) |
| Tombstones | [Later-level lawn](tombstones_lawn.png), [cracked stone](tombstone_cracked.png), [destroyed stone](tombstone_destroyed.png), [zombie emerging](grave_emergence.png), [emergence animation](grave_emergence.gif) |
| Defeat | [Breach animation](loss_animation.png), [retry prompt](loss_prompt.png) |
| Feedback | [Blast above plants](explosion_above_plants.png), [cooldown hourglass](cooldown_cell.png), [plant damage](plant_red_flash.png), [zombie damage](zombie_damage_flash.png), [Snow Pea combat](snow_pea_gameplay.png) |

## Refreshing

After rebuilding **both** ROMs, run these sequentially from the project root:

```sh
cc -Wall -Wextra -Werror tools/rom_smoke.c \
  -I/usr/local/include -L/usr/local/lib -lmgba -o build/rom_smoke
build/rom_smoke plants_vs_zombies.gba
build/rom_smoke plants_vs_zombies_debug.gba debug
build/rom_smoke plants_vs_zombies.gba loss
build/rom_smoke plants_vs_zombies.gba legacy
python3 tools/generate_previews.py
```

The smoke checks exercise the game using button inputs, verify expected behavior,
and write captures under `build/`. They use fresh emulator SRAM, leaving player
save files untouched. The preview generator requires fresh captures from the
matching ROM and fails rather than publish stale images. Pillow is needed for
publication; libmgba headers/libraries are needed for the emulator checks.

See [build instructions](../README.md#build) and the
[development guide](DEVELOPMENT.md) for code, asset and save-format details.
