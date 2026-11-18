#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/lint.py
mkdir -p build
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude src/game/*.cpp tests/game_test.cpp -o build/game_test
build/game_test

"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude src/game/*.cpp src/app/input_controller.cpp tests/controller_test.cpp -o build/controller_test
build/controller_test

"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude tests/save_slots_test.cpp -o build/save_slots_test
build/save_slots_test
