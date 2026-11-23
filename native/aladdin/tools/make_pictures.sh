#!/bin/sh
# make_pictures.sh GAME_BINARY RESOURCE_DIR OUT_DIR: makes level_pictures.dat from the cartridge (cart.dat) with the game's own
# loader, once, as part of a build (docs/aladdin/native/LEVELSELECT.md). Run the built headless game; OUT_DIR is the game's
# assets folder (native/aladdin/assets), next to cart.dat, which the build packages. Never committed.
set -e
bin=$1 res=$2 out=$3
[ -x "$bin" ] && [ -f "$res/cart.dat" ] && [ -d "$out" ] || { echo "usage: make_pictures.sh GAME_BINARY RESOURCE_DIR(with cart.dat) OUT_DIR" >&2; exit 2; }
PORT_RESDIR=$res ALADDIN_MAKE_PICTURES=$out/level_pictures.dat perl -e 'alarm 300; exec @ARGV' "$bin" --headless --updates 3
