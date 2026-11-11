# Sprouts vs. Stiffs

A playable GBA lawn defense game inspired by Plants vs. Zombies, built with
[Butano](https://github.com/GValiente/butano). The game uses original pixel art,
synthesized sound effects, and an original looping music track.

Open **[sprouts_vs_stiffs.gba](sprouts_vs_stiffs.gba)** in mGBA or use a compatible
GBA flash cartridge. This is the current ROM filename; the old Garden Guard files
have been removed.

![Current gameplay captured in mGBA](docs/gameplay.png)

## Playing

Defend a five-lane, nine-column lawn through five waves. You start with 175 sun
and approximately 15 seconds to prepare. Build sunflowers, collect sun, and cover
the lanes with peashooters. Each lane has a single-use mower as a last defense.
A zombie reaching the house after that defense is spent ends the run.

| Button | During play |
| --- | --- |
| D-pad | Move the planting cursor; hold to repeat |
| A | Plant the selected seed |
| L / R | Select the previous / next seed packet |
| B | Collect all available sun; hold to collect new tokens as they appear |
| Hold SELECT + A | Shovel the plant under the cursor; no refund |
| START | Pause |

On the title screen, **A or START** begins a run. On results, **A or START**
starts a new run and **B** returns to the title.

| Plant | Cost | Seed recharge | Role |
| --- | --- | --- | --- |
| Sunflower | 50 | ~4 seconds | Produces 25 sun approximately every eight seconds |
| Peashooter | 100 | ~3 seconds | Shoots zombies ahead in its lane |
| Wall-nut | 50 | ~10 seconds | Blocks zombies and absorbs bites |
| Cherry bomb | 150 | ~20 seconds | Explodes after a short fuse, damaging nearby zombies across three lanes |

Timing is measured in simulation updates; seconds above assume approximately
60 updates per second. Normal zombies have six health; cone zombies have twelve.
Waves contain 5, 7, 9, 11, and 13 zombies, with a breather between cleared waves.

## Pause menu and audio

**Up/Down** selects Resume, Restart, Title, Master, Music, or Effects. **A** confirms
a menu action. **Left/Right** adjusts the selected volume in 10% steps; holding a
direction repeats. **B or START** immediately resumes.

Master scales both music and effects. Either channel can be muted independently,
and master at zero silences both. All three sliders default to **100%**. Settings
survive restarts and title visits within the current session; they are not saved
when the emulator or console closes.

The generated audio uses a shared **1.5× output gain** from
[`tools/audio_config.py`](tools/audio_config.py). The sliders attenuate this louder
baseline. The original **Garden at Dusk** track uses three synthesized instruments
and four looping phrases. It continues through menus so music adjustments are
immediately audible. Navigation, planting, collection, shooting, hits, chewing,
explosions, mowers, wave changes, and results have sound effects. Chewing sounds
follow actual plant damage, with a separate playback cooldown.

![Current pause menu and default volume settings](docs/paused.png)

## Graphics and feedback

The title has a centered wooden logo, a rounded Play tombstone with a carved
sprout, and the gameplay sunflower and peashooter sprites enlarged at integer
2× scale. Menus share graveyard scenery with a crescent moon, distant silhouettes,
grass, and moss. Pause actions are unboxed; audio controls share aligned labels
and stone slider handles. The pause tombstone extends below the controls to keep
the instruction text clear. Results show wave progress, defeats, and elapsed time.

![Title, pause, and results captured from the current ROM](docs/menus.png)

During play, feedback uses icons and animation rather than status-text banners:

- Seed recharge bars fill yellow, turn green when ready, and show red when sun
  is insufficient. The selected packet pulses red for an unaffordable placement.
- The cursor is cream for a valid placement, red when blocked, and gold while
  holding SELECT. A check marks a successful planting; a cross marks an occupied
  cell. The shovel appears at the selected cell.
- A borderless hourglass appears over a recharging seed portrait, with three sand
  levels indicating recharge progress.
- Collected suns fly independently to the sun bank over 24 updates. The bank
  credits them immediately; flights pause with the game and cannot credit twice.
- The wave track has five flags, a skull on the final flag, and a zombie head
  drawn from the same face artwork as the enemies. Upcoming flags are red,
  active flags turn gold, and cleared flags turn green; the final skull flag
  stays red until cleared. The head and fill meet each flag pole at its checkpoint.
- Plants have idle poses; peashooters recoil while firing. Wall-nuts show two
  damage stages. Zombies shuffle while walking and hold a steady stance with a
  small jaw movement while eating. Both hands remain visible.

[Seed packets](docs/seed_packets.png) · [Wave indicator](docs/wave_indicator.png) ·
[Plant animation](docs/plant_idle.gif) · [Zombie animation](docs/zombie_eating.gif)

See the **[screenshot and asset gallery](docs/README.md)** for all current previews.
Screenshots come from the ROM; sprite sheets and animation studies are identified
separately in the gallery.

## Build and regenerate assets

Install devkitPro's GBA tools (devkitARM, grit, mmutil, gbafix), Python 3, and
Butano. Butano is an external dependency, not vendored here. The Makefile defaults
to `~/gba_development/butano`; override that path as needed:

```sh
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH="$DEVKITARM/bin:/opt/devkitpro/tools/bin:$PATH"
make -j8 BUTANO_ROOT=/absolute/path/to/butano
mgba sprouts_vs_stiffs.gba
```

Generated BMP, WAV, and MOD assets are included, so Pillow is only needed when
regenerating artwork, audio, or documentation previews:

```sh
python3 -m pip install Pillow
python3 tools/generate_assets.py
make -j8
```

The music composer, pixel art, menu scenery, and audio gain configuration feed
the asset generator. Run it after editing any of those sources, then rebuild.

## Verification and screenshots

Portable C++17 simulation tests run with AddressSanitizer and
UndefinedBehaviorSanitizer:

```sh
sh tools/test.sh
```

Coverage includes economy, cooldowns, sun flights and single-credit collection,
pause/restart, shovel, explosions, walls, bite timing and eating state, mowers,
loss, a complete five-wave victory, wave geometry, and audio settings.

The optional headless mGBA check runs the actual ROM, exercises menus and controls,
verifies visual feedback and wave alignment, and captures screenshots. It also
measures emulator audio to check independent music/effects muting, master muting
and restoration, setting retention through restart/title visits, and an isolated
chewing encounter. With libmgba development headers installed:

```sh
mkdir -p build
cc -Wall -Wextra -Werror tools/rom_smoke.c \
  -I/usr/local/include -L/usr/local/lib -lmgba -o build/rom_smoke
build/rom_smoke
python3 tools/generate_previews.py
```

Adjust the compiler's include/library paths for your libmgba installation. The
smoke check loads `sprouts_vs_stiffs.gba` by default, or accepts a ROM path as its
first argument. It writes raw PPM captures to `build/`; the preview script updates
all documentation images, including close-ups and animated sprite studies, in
`docs/`. Refresh captures after rebuilding before running the preview script.

Build, simulation, and headless emulator checks have been run locally. These
checks do not establish compatibility, performance, or perceived volume on
physical GBA hardware.

## Code layout

| Source | Responsibility |
| --- | --- |
| `include/game.h`, `src/game.cpp` | Deterministic simulation, tuning, economy, waves, collisions, bounded entity pools |
| `include/audio_settings.h` | Volume levels, adjustment limits, and master/channel gains |
| `src/main.cpp` | Input, key repeat, application state, audio playback and mixer settings |
| `include/presentation.h`, `src/presentation.cpp` | Butano rendering, sprite poses, cached text, menu selection, sliders, visual feedback |
| `include/wave_track.h` | Generated geometry shared by wave artwork and rendering |
| `tools/generate_assets.py` | Asset assembly, backgrounds, HUD, and synthesized effects |
| `tools/pixel_art.py` | Characters, seed portraits, lawn, shovel, shared zombie face |
| `tools/menu_art.py` | Graveyard scenery, tombstones, wooden signs |
| `tools/generate_music.py`, `tools/audio_config.py` | Original tracker composition and shared output gain |
| `tools/generate_previews.py` | Documentation captures, close-ups, and animation previews |
| `tests/game_test.cpp`, `tools/test.sh` | Host simulation and audio-setting checks |
| `tools/rom_smoke.c` | Emulator input, visual/audio assertions, and captures |

The simulation has no Butano dependency or heap allocation. Fixed pools hold
45 plants, 16 zombies, 18 shots, 10 suns, and five mowers. Collection flights reuse
the sun pool. A compile-time check bounds the gameplay sprite allocation within
the GBA's 128-sprite limit. Menu sliders exist only on the pause screen, after
entity sprites have been cleared. Characters share a 25-frame, 32×32 sprite atlas
and a 16-color palette; backgrounds use separate palettes. Positions use integer
subpixels and advance once per update at the GBA's native refresh rate.

## Scope and remaining ideas

The MVP has one lawn, four plants, two enemy types, five deterministic waves,
win/loss results, graveyard menus, volume controls, original music, sound effects,
and visual feedback. It has no campaign, persistent save system, touch controls,
or original Plants vs. Zombies assets. Additional plants, zombies, and levels
remain ideas rather than implemented features. See [notes.txt](notes.txt).
