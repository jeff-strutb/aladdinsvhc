#!/bin/sh
# grid_build.sh OUT: the save slots' screen check (grid_check.c), built from the game's own slot_grid.c, menu_art.c and save_crop.c
set -e
top=$(cd "$(dirname "$0")/../../.." && pwd)
s=$top/native/aladdin/src
clang -std=gnu11 -O2 -g -Wall -I "$s" -I "${NSHVC:-$top/../nativeshvc}/include" -o "$1" \
  "$top/native/checks/pause_check/grid_check.c" "$s/pause/slot_grid.c" "$s/pause/menu_art.c" "$s/pause/pad_art.c" "$s/pause/pad_glyphs.c" \
  "$s/system/save_crop.c" "$s/data/data.c" "$s/data/graphics.c" "$s/input/controls.c" -lm
