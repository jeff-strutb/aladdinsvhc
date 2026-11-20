#!/bin/sh
# build.sh: the project's bsnes core with native/checks/core's patches, for
# native/checks/record, built privately into build/aladdin/native/checks.
# The project's core (tools/toolchains/bsnes) and its source are not changed:
# the main checkout's patched source (tools/toolchains/src/bsnes, as setup.sh
# leaves it) is copied with its objects, this directory's patches applied to
# the copy, and only what they touch rebuilt. The build goes through the CPU
# and memory queue ALADDIN_CPU_RUN names, with a timeout. Run from the
# repository root; prints the core's path.
set -eu
top="$(git rev-parse --show-toplevel)"
main="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)"
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "build.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }
src="$main/tools/toolchains/src/bsnes"
here="$top/native/checks/core"
dst="$top/build/aladdin/native/checks/core-src"
lib="$top/build/aladdin/native/checks/bsnes_libretro.so"
[ -f "$src/bsnes/out/bsnes_libretro.so" ] || { echo "build.sh: no built core source in $src (./setup.sh bsnes)" >&2; exit 2; }
# The copy is made again whenever the project's patch set or these patches change.
stamp="$(cat "$src/.patch-stamp" 2>/dev/null || true) $(cat "$here"/*.patch | sha1sum | cut -c1-40)"
if [ "$(cat "$dst/.native-stamp" 2>/dev/null || true)" != "$stamp" ]; then
    rm -rf "$dst"
    mkdir -p "$(dirname "$dst")"
    cp -a "$src" "$dst"
    for p in "$here"/*.patch; do patch -s -p1 -d "$dst" < "$p"; done
    echo "$stamp" > "$dst/.native-stamp"
fi
timeout 1800 "$run" -j 4 -m 4G -- sh -c 'make -C "$1" target=libretro binary=library -j "$CPU_RUN_JOBS"' sh "$dst/bsnes" >&2
cp "$dst/bsnes/out/bsnes_libretro.so" "$lib.new"
mv "$lib.new" "$lib"
echo "$lib"
