#!/bin/sh
# level_build.sh OUT: the level select screen check (level_check.c), built from the game's own level_screen.c, beaten.c, level_pictures.c,
# slot_grid.c and menu_art.c
set -e
top=$(cd "$(dirname "$0")/../../.." && pwd)
s=$top/native/aladdin/src
clang -std=gnu11 -O2 -g -Wall -I "$s" -I "${NSHVC:-$top/../nativeshvc}/include" -o "$1" \
  "$top/native/checks/pause_check/level_check.c" "$s/pause/level_screen.c" "$s/game/beaten.c" "$s/video/level_pictures.c" \
  "$s/pause/slot_grid.c" "$s/pause/menu_art.c" "$s/pause/pad_art.c" "$s/pause/pad_glyphs.c" \
  "$s/data/data.c" "$s/data/graphics.c" "$s/data/level_tables.c" "$s/data/game_tables.c" "$s/input/controls.c" -lm
