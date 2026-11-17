#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude src/game.cpp tests/game_test.cpp -o build/game_test
build/game_test

"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Iinclude src/game.cpp src/input_controller.cpp tests/controller_test.cpp -o build/controller_test
build/controller_test
