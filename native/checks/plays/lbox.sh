#!/bin/sh
# lbox.sh DIR: builds DIR/lbox, the decomp's snesbox with one more script command, `lhold KEYS [DUMPBYTES]` (lbox.patch): the keys held
# until the game's play-time tick has stepped (one logic frame of the original, which it sometimes spends two video frames on).
# Private copy: tools/aladdin/snesbox is not changed. Run from the repository root with ALADDIN_CPU_RUN set; the core's headers come
# from the main checkout (TOP).
set -eu
dir=$(mkdir -p "$1" && cd "$1" && pwd)
top="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)"
src="$(git rev-parse --show-toplevel)"
rm -rf "$dir/src"; mkdir -p "$dir/src"
cp "$src"/tools/aladdin/snesbox/* "$dir/src/"
(cd "$dir/src" && patch -s snesbox.c < "$src/native/checks/plays/lbox.patch")
"${ALADDIN_CPU_RUN:?set ALADDIN_CPU_RUN to the CPU queue command}" -j 1 -m 1G -- make -s -C "$dir/src" TOP="$top" OUT="$dir/lbox" INC="$src/src/aladdin/include"
