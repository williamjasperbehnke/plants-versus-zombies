#!/bin/sh
# Build, verify, capture and publish the current game without touching player saves.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
make -j"${JOBS:-8}"
make -j"${JOBS:-8}" DEBUG_SAVE=1
sh tools/test.sh
"${CC:-cc}" -Wall -Wextra -Werror tools/rom_smoke.c \
    -I"${MGBA_INCLUDE_DIR:-/usr/local/include}" \
    -L"${MGBA_LIBRARY_DIR:-/usr/local/lib}" -lmgba -o build/rom_smoke
build/rom_smoke plants_vs_zombies.gba
build/rom_smoke plants_vs_zombies_debug.gba debug
build/rom_smoke plants_vs_zombies.gba loss
build/rom_smoke plants_vs_zombies_debug.gba gallery
python3 tools/generate_previews.py
