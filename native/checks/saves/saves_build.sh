#!/bin/sh
# saves_build.sh OUT: the slot files' check (saves_check.c) from the game's own saves.c and save_crop.c
set -e
top=$(cd "$(dirname "$0")/../../.." && pwd)
s=$top/native/aladdin/src
clang -std=gnu11 -O1 -g -Wall -I "$s" -I "${NSHVC:-$top/../nativeshvc}/include" -o "$1" \
  "$top/native/checks/saves/saves_check.c" "$s/system/saves.c" "$s/system/save_crop.c"
