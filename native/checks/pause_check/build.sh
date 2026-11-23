#!/bin/sh
# build.sh OUT: the menu's art check, built from the game's own art files and its data readers (cart.dat is read from
# DATA_TEST_DIR when it runs). NSHVC is the engine's checkout (its include folder).
set -e
top=$(cd "$(dirname "$0")/../../.." && pwd)
s=$top/native/aladdin/src
clang -std=gnu11 -O2 -g -Wall -I "$s" -I "${NSHVC:-$top/../nativeshvc}/include" -o "$1" \
  "$top/native/checks/pause_check/pause_check.c" "$s/pause/menu_art.c" "$s/pause/pad_art.c" "$s/pause/pad_glyphs.c" \
  "$s/data/data.c" "$s/data/graphics.c" "$s/input/controls.c" -lm
