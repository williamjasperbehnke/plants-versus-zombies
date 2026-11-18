# Development guide

[Project overview](../../README.md) · [Player guide and animation gallery](../player/README.md)

## Build

Install devkitPro's GBA tools (`devkitARM`, `grit`, `mmutil`, `gbafix`), Python 3,
and [Butano](https://github.com/GValiente/butano). Butano is external to this
repository; the default path is `~/gba_development/butano`.

```sh
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH="$DEVKITARM/bin:/opt/devkitpro/tools/bin:$PATH"
make -j8 BUTANO_ROOT=/absolute/path/to/butano
make -j8 DEBUG_SAVE=1 BUTANO_ROOT=/absolute/path/to/butano
```

The outputs are `plants_vs_zombies.gba` and `plants_vs_zombies_debug.gba`. Run the
builds sequentially because audio conversion shares temporary files. A new debug
save opens all ten levels and uses a separate SRAM region.

Generated assets are checked in. To edit and regenerate them:

```sh
python3 -m pip install Pillow
python3 tools/generate_assets.py
make -j8
make -j8 DEBUG_SAVE=1
```

## Source layout

| Location | Responsibility |
| --- | --- |
| `src/main.cpp`, `include/app/`, `src/app/` | Startup, frame loop, keypad input, screen interaction and view state updates |
| `include/game/game.h`, `src/game/game.cpp` | Simulation state, level lifecycle, economy and update order |
| `src/game/game_waves.cpp` | Spawn schedules, pending groups, hordes and grave spawns |
| `src/game/game_plants.cpp` | Planting, targeting and plant actions |
| `src/game/game_combat.cpp` | Damage, collisions, zombie movement, bites and cosmetic effects |
| `include/game/animation.h`, `src/game/animation.cpp` | Frame clips, actor poses, shared timings and reward motion |
| `include/game/tutorial.h` | Tutorial steps and required actions |
| `include/game/game_internal.h` | Shared private simulation helpers |
| `include/ui/`, `src/ui/` | Screen rendering, sprite resources, fonts and layout |
| `include/audio/`, `src/audio/` | Mixer settings, ordered sound commands and playback |
| `include/save/` | Progress encoding, save-slot journals and SRAM I/O |
| `include/generated/` | Layout and audio constants emitted by the asset generator |

Headers use folder-qualified paths such as `game/game.h`. The Makefile discovers
one level of source subdirectories. Keep source basenames unique: Butano names
object files by basename. Clean the affected build cache after moving files.

## Runtime ownership

`Application` owns the game, controller, audio player, save services and
presentation. Each frame it:

1. Reads keys and runs the controller.
2. Plays queued UI sounds and applies mixer settings.
3. Ticks the simulation and handles its audio events.
4. Saves changed progress and advances view timers.
5. Renders and calls `bn::core::update()`.

`InputController` handles buttons and owns `ViewState` and repeat counters. It
has no Butano dependency. `Game` owns the simulation and uses fixed-capacity pools;
its implementation is split by responsibility, not by entity ownership.

`Presentation` coordinates screens, the HUD, scouting, tutorials and rewards.
`WorldRenderer` owns live actor sprites and status palettes. `AlmanacView`,
`SaveMenuView`, `PauseMenuView` and `SeedSelectionView` own their labels, panels,
sprites and redraw caches. Pause and seed-selection views share
`include/ui/menu_views.h` and `src/ui/menu_views.cpp`.

`SaveSlots` owns portable profile state; `SaveStore` performs SRAM reads and writes.
`AudioCommands` preserves input-side sound order, and `AudioPlayer` owns playback
and combat-sound cooldowns. Resource-owning classes cannot be copied.

## Simulation and rendering

Positions use integer subpixels: 256 units per pixel. The pools hold 45 plants,
16 zombies, 18 shots, 10 suns, five mowers, three tombstones and six cosmetic effects.
The simulation is independent of Butano and heap allocation.

`scheduled_zombie()` supplies both the preview roster and actual road spawns.
Overlapping groups retain their queued enemies when the live pool is full.
Tombstone raises are additional spawns and remain queued until a slot is free.

Damage and death are resolved in the simulation. A defeated zombie stops being a
combat target immediately; a cosmetic effect handles its fall and equipment.
The effect pool replaces the entry nearest expiry when full. Victory waits for
active explosions, moving mowers and finishing effects before the reward clock
advances. Pause stops simulation timers.

An `ActorFrame` contains an `ActorSheet` and a frame index local to that sheet.
`SpriteCatalog` resolves it to a Butano asset and checks frame bounds.
`ActorPose` also carries position, depth and scale. Movement, recoil, vaulting and
Chomper pull calculations belong in the animation module; sprite allocation and
hit/frost palettes belong in the renderer.

`AnimationClip<N>::frame_at()` loops with an optional phase offset;
`frame_once()` holds the last pose. Gameplay, animation and sound share action
and reward timings from `animation.h`.

Keep sprite pools within the GBA's 128-entry OAM limit. The project enables
64 Butano sort layers for lane depth, HUD and overlays. Allocation, palette and
layering changes need emulator checks in addition to host tests.

## Progression and menus

`seed_order` defines plant unlock and display order. `PlantType` and `ZombieType`
IDs identify assets, catalogue entries and saved counters; keep those IDs stable.
A level's roster is independent of the player's selected six-packet bank.

Levels with up to six available plants bring the full roster. Larger rosters
require a complete selection. Scouting and seed selection don't advance combat,
sun production, recharge or preparation time. The tutorial also holds waves until
its required actions are complete.

`TutorialGuide` observes player actions and supplies a `TutorialPrompt` to the
controller and renderer. Gameplay enforces its allowed planting, collection and
shovel actions. The first two levels use one and three active lanes; all pool
capacities remain fixed at five lanes.

Almanac entries store a single paragraph. `WrappedText<24, 32>` wraps it without
allocation, splits long tokens and reports truncation. Details show four lines;
Up/Down scrolls, and L/R changes pages from the grid. Layout and navigation bounds
are shared in `menu_layout.h`.

Redraw caches may skip drawing work, but must not affect simulation timing.
Screen exits must release their resources. Reward and defeat screens keep the
lawn visible; scouting uses the road panorama.

## Assets

| Source | Content |
| --- | --- |
| `tools/pixel_art.py` | Plant and zombie poses, icons and scenery helpers |
| `tools/zombie_effect_art.py` | Wounded sprites, corpses and detached equipment |
| `tools/menu_art.py` | Menu scenery, signs and tombstones |
| `tools/generate_assets.py` | Indexed BMP/JSON export, backgrounds, HUD, audio and generated headers |
| `tools/asset_layout.py` | Graphics categories and paths |
| `tools/generate_music.py`, `tools/audio_config.py` | Music generation and output gain |
| `tools/audio_sources/` | Defeat recordings and credits |
| `tools/generate_previews.py` | ROM screenshot and GIF publication |
| `tools/zombie_previews.py` | Per-type damage/death animation studies |

Graphics are grouped into `plants`, `zombies`, `effects`, `seeds`, `ui` and
`backgrounds` under `graphics/`. Character sheets stack poses vertically. Most
frames are 32×32; Screen Door frames are 64×32. Seed cards use 32×32 frames and
reward packets use 64×64 frames.

Use `asset_path()` when accessing generated artwork from Python. Edit generated
headers through their source templates in `generate_assets.py`. Review artwork at
native size and in the ROM after changing frame layouts or palettes.

Author ground masks with `SHADOW_PIXEL`. Export packs shadows into palette index
15, which status palettes preserve. Body colors use the other slots. Palette
indices are shared contracts between artwork and runtime effects; coordinate any
change with the matching palettes and renderer.

Documentation is organized into `docs/player/`, `docs/development/`, and
`docs/media/`. The publisher places PNGs in `media/screenshots/`, ROM GIFs in
`media/gameplay/`, and sprite studies in `media/studies/`. Manifest paths are
relative to `docs/media/`.

The damage/death studies crop the current indexed sheets and stage equipment,
arm loss and corpse poses in comparison panels. They are labeled as studies in
the gallery and manifest. Study GIFs contain no text; their panel descriptions
belong in the guide or manifest. Gameplay GIFs use emulator frames. Add new study assets
to the publisher so a gallery refresh keeps them and records their sources.

## Save format

Records are explicitly encoded in little endian, not copied from a C++ struct.
`progress.h` and `save_layout.h` define the current 52-byte PVZ3 layout.

| Offset | Field |
| ---: | --- |
| 0 | PVZ3 magic |
| 4 | Version 3 |
| 5 | Unlocked level count |
| 6 | Last started level index |
| 7 | Slot state: occupied or deleted |
| 8 | Completed-level mask, 32 bits |
| 12 | Sequence number, 32 bits |
| 16 | Football kills, 32 bits |
| 20 | Six 32-bit counters: Normal, Conehead, Newspaper, Flag, Buckethead, Screen Door |
| 44 | Pole Vaulter kills, 32 bits |
| 48 | Checksum over bytes 0–47 |

Each profile uses two journal records. Slot bases have a 4096-byte stride: normal
bases are 768, 4864 and 8960; debug bases are 1024, 5120 and 9216. All regions fit
inside 32 KiB SRAM.

Loading selects the newest valid sequence, including wraparound. Creation,
progress changes and deletion dirty only the affected slot. Deletion writes a
newer empty record; recreation continues that journal's sequence. Unsupported
versions and bad checksums are rejected. Kill counters saturate at `UINT32_MAX`.

Only the active profile syncs with `Game`. Returning to the title keeps that
profile available to the Almanac. Empty slots stay empty until selected.

## Checks and gallery publication

Host checks need clang-format 19 and a C++17 compiler with AddressSanitizer and
UndefinedBehaviorSanitizer. Emulator checks also need libmgba headers/libraries;
gallery generation needs Pillow.

```sh
sh tools/test.sh
sh tools/refresh_docs.sh
```

`test.sh` checks style, simulation, campaign clears, controller input and save
journals. `refresh_docs.sh` builds both ROMs, runs those checks, then runs the
normal, debug, defeat and gallery emulator scenarios before publishing images.
The scenarios use fresh emulator SRAM and don't touch player save files.

To customize installation paths:

```sh
BUTANO_ROOT=/absolute/path/to/butano \
MGBA_INCLUDE_DIR=/absolute/path/to/include \
MGBA_LIBRARY_DIR=/absolute/path/to/lib sh tools/refresh_docs.sh
```

`JOBS`, `CC` and `CXX` can also be overridden. To repeat only the capture stages
after building:

```sh
cc -Wall -Wextra -Werror tools/rom_smoke.c \
    -I/usr/local/include -L/usr/local/lib -lmgba -o build/rom_smoke
build/rom_smoke plants_vs_zombies.gba
build/rom_smoke plants_vs_zombies_debug.gba debug
build/rom_smoke plants_vs_zombies.gba loss
build/rom_smoke plants_vs_zombies_debug.gba gallery
python3 tools/generate_previews.py
```

The publisher requires captures newer than their matching ROM. It validates
inputs before replacing images, removes retired PNG/GIF files, and records ROM
hashes, source captures, study sheets and timing in `docs/media/captures.json`.

Emulator tests cover gameplay, saves, rewards, defeat audio, menu transitions,
resource usage and the gallery scenes. Physical hardware performance and perceived
volume still need hardware checks. The deferred reward-freeze investigation is
recorded in [project notes](notes.txt).

## Code style

Use clang-format **19.x**. Install `llvm@19` on macOS or `clang-format-19` from your
package manager. Set `CLANG_FORMAT` to its executable path if needed.

```sh
python3 tools/lint.py        # check; also make lint
python3 tools/lint.py --fix  # format; also make format
```

The check covers C/C++ files in `include`, `src`, `tests` and `tools`, including
generated headers. `.clang-format` specifies four spaces, attached opening braces,
multiline function bodies and a 110-column limit. Multiline array entries and the
closing brace go on separate lines. Prefer `if`/`else` or `switch` to nested ternary
chains. `.editorconfig` sets whitespace defaults; Makefile recipes use tabs.

mGBA feature flags must precede its API headers because they affect structure
layout. The formatter has an explicit include-order rule for `mgba/flags.h`.

## Audio credits

Music and most effects are synthesized. Defeat uses a CC0 record scratch by
filmsndfx and the original Plants vs. Zombies scream. The scream is not CC0;
rights remain with PopCap / EA. See [audio credits](../../tools/audio_sources/credits.json)
for sources, licenses and processing notes.

Audio regeneration is offline. The generator derives defeat cue timing from the
recordings. Output gain is set in `tools/audio_config.py`; mower and explosion
sounds receive an additional boost. The pause sliders adjust playback volume.
