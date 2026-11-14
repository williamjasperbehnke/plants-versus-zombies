# Development guide

## Runtime boundaries

| Source | Responsibility |
| --- | --- |
| `include/game.h`, `src/game.cpp` | Deterministic simulation, level definitions, spawn rosters, combat and bounded entity pools |
| `src/main.cpp` | Frame loop, screen-specific input handlers, repeat timing, transitions, event-driven sound and save updates |
| `include/presentation.h`, `src/presentation.cpp` | Butano backgrounds, sprite ownership, animations, HUD, menus and cached text |
| `include/menu_layout.h` | Shared navigation/rendering grid geometry |
| `include/almanac.h` | Catalogue copy and display poses |
| `include/progress.h`, `include/save_store.h` | Portable save encoding/validation and Butano SRAM storage |
| `include/audio_settings.h` | Mixer levels, limits and master/channel gains |
| `include/reward_animation.h` | Reward timing and integer bounce/zoom geometry |
| `include/defeat_audio.h` | Generated loss-cue timing shared by simulation, runtime and emulator checks |
| `include/seed_layout.h`, `include/wave_track.h` | Generated geometry shared by artwork and rendering |

`Game` has no Butano dependency and uses no heap allocation. Positions use integer
subpixels (256 units per pixel). Pools hold 45 plants, 16 zombies, 18 projectiles,
10 suns, five mowers and three tombstones. Pending spawn groups preserve enemies when groups overlap
or the zombie pool is full. `scheduled_zombie()` assigns each planned spawn’s type;
both the preview road roster and actual road spawning use that same schedule.
Tombstone raises are conditional additions. Each standing stone queues one normal
zombie when a flag actually spawns. Queued raises survive pool pressure and count
as pending enemies, but use group zero so they do not alter the road group’s
half-defeated threshold. Destroying a stone prevents future triggers without
canceling an already triggered raise. Raised zombies use a 48-frame emergence
timer; movement and biting wait for it, while damage and status effects remain
active. A separate six-pose atlas (plus matching buffed poses) clips the body at
the ground and reuses the normal zombie palette and sprite slot. Tombstone bodies share the character shadow
palette convention and have their own bounded sprite pool.

The application handles input, ticks the simulation, consumes transient events,
updates persistent progress, renders, and calls `bn::core::update()`, in that order.
Keep that order when refactoring: input can change phase before the tick, and
combat events belong to the just-completed update. Screen-specific input methods
preserve each screen’s button priority. Repeated transitions to level selection
share `show_level_select()`, including view and navigation-repeat resets. Audio
settings are retained when the view resets.

`Presentation::change_screen()` owns screen resource transitions. Gameplay’s lawn
is a separate resource from menu backgrounds. Pause clears entity sprites; reward
and loss overlays retain the lawn and entities. Scouting uses a HUD-free panorama,
with independently scrolling text. Its returning lawn pixels align with gameplay.
Victory enters the won phase immediately, then finishes active explosions and
moving mowers before advancing the reward clock. Both play and victory use
`Plant::advance_explosion()` for the same blast lifetime. The finishing period does
not update live plant timers, projectiles, sun production or input actions. Render
those finishing effects until the reward clock starts; freeze the remaining lawn
under the reward overlay. The victory jingle follows the reward clock rather than
the phase transition.

Rendering caches should affect drawing cost only, never simulation timing.

The gameplay sprite budget is checked at compile time against the GBA’s 128 OAM
entries. The build also raises Butano’s sort-layer limit to 64 for lane depth,
HUD and overlays. Keep new effects within these limits; host simulation tests
alone cannot catch sprite/palette allocation failures.

## Assets

Edit the authoring sources, then run `python3 tools/generate_assets.py`:

| Source | Output |
| --- | --- |
| `tools/pixel_art.py` | Plant/zombie poses, icons, shared character details and scenery helpers |
| `tools/menu_art.py` | Graveyard scenery, signs and tombstones |
| `tools/generate_assets.py` | Indexed BMPs/JSON metadata, backgrounds, atlases, HUD, effects and generated headers |
| `tools/generate_music.py` | Original tracker music |
| `tools/audio_config.py` | Shared generated output gain |
| `tools/audio_sources/` | Imported defeat audio and per-source credits |
| `tools/generate_previews.py` | Published screenshots and reward animation from emulator captures |

Do not edit generated layout headers to change artwork geometry. Keep stable
`PlantType`/`ZombieType` IDs: assets, save counters, catalogue entries and runtime
frames depend on their ordering. Display/unlock order follows `seed_order`, which
is separate from plant asset IDs.

Character authoring marks ground shadows with `SHADOW_PIXEL`. Atlas generation
packs that mask into palette index 15 while preserving body colors in other
slots. Red-hit and frost palettes preserve the shadow color. This allows body and
shadow colors to differ without allocating another sprite per actor. Review both
native-size art and in-ROM rendering after palette or atlas changes; a correct
BMP does not establish correct frame routing or sprite ordering.

Imported audio is kept locally so regeneration needs no network access. Preserve
per-file licensing in `credits.json`. `defeat_audio.h` derives cue and input-unlock
timing from the actual clip durations. Do not hand-adjust its generated constants.

## Progress compatibility

`save_format` in `progress.h` names the existing byte layout. Records are explicitly
encoded little endian; they are not a dump of a C++ struct.

| Byte offset | Current 48-byte record |
| ---: | --- |
| 0 | `PVZ2` magic |
| 4 | Version 2 |
| 5 | Unlocked level count |
| 6 | Last started level index |
| 8 | Completed-level bit mask, 32 bits |
| 12 | Sequence number, 32 bits |
| 20 | Six lifetime kill counters, 32 bits each, in stable zombie-type order |
| 44 | Checksum over the preceding bytes |

Unassigned bytes are zeroed during encoding. Normal records start at SRAM offset
256; debug records start at 512. Each region has two alternating records. Loading
validates both and selects the newest valid sequence with wraparound handling.
Writes occur only when progress changes. Kill counters saturate at `UINT32_MAX`.

Legacy 24-byte records use `SVS1`, version 1, and a checksum at byte 20. Their pairs
start at 0 (normal) and 128 (debug). Migration reads them only if neither current
record is valid and writes the new region without destroying the old pair. Legacy
kill counts start at zero. Change format versions and migration deliberately if
adding persistent fields; do not silently reinterpret existing saves.

## Validation

Use the commands in the [project README](../README.md#verification-and-development).
The host suite verifies deterministic rules and all ten campaign levels. Emulator
checks cover input/navigation, locks, progress/reload/backup recovery, Almanac
animation, preview HUD/timing and return alignment, rewards, damage palettes,
freezing, defeat audio, retry and save isolation/migration.

Run both ROM builds and all four smoke modes after changes that cross application,
rendering or save boundaries. Run the matching host tests for simulation changes.
Normal and debug builds, and screenshot-producing smoke checks, run sequentially.
Do not run the retired `campaign` smoke mode or `TEST_LEVEL` build commands from
older documentation; use the normal/debug/loss/legacy checks and debug selector.

Screenshots are nearest-neighbor enlargements of fresh PPM captures in `build/`.
The publication script checks each capture against the normal or debug ROM that
produces it and validates inputs before replacing images. See the
[gallery](README.md) for the current published selection.
