#!/bin/sh
# Stage the toolchain inside the repo. Nothing is installed on the host.
#
#   ./setup.sh            fetch (pinned commits) and build everything
#   ./setup.sh asar       one component
#
# Sources land in tools/toolchains/src/<name> at a pinned commit; build
# products in tools/toolchains/<name>/. Bumping a pin means re-verifying
# every gate, so pins change deliberately and nowhere else.
set -eu
top="$(cd "$(dirname "$0")" && pwd)"
tc="$top/tools/toolchains"
mkdir -p "$tc/src"
jobs="$(sysctl -n hw.ncpu 2>/dev/null || nproc)"

ASAR_URL=https://github.com/RPGHacker/asar
ASAR_PIN=6a6dbdaed8cb773e583052ce4f66375bd896c3a1      # v1.91
BSNES_URL=https://github.com/bsnes-emu/bsnes
BSNES_PIN=78474e5d39f58872dc21aca18cf9eb583b147fe7     # master after v115

fetch() { # name url pin
    d="$tc/src/$1"
    if [ ! -d "$d/.git" ]; then
        git init -q "$d"
        git -C "$d" remote add origin "$2"
    fi
    if [ "$(git -C "$d" rev-parse -q --verify HEAD 2>/dev/null || true)" != "$3" ]; then
        git -C "$d" fetch -q --depth 1 origin "$3"
        git -C "$d" checkout -q --force "$3"
    fi
    # Local patches live in the repo and are applied on a clean checkout,
    # again whenever the patch set changes.
    pd="$top/tools/toolchains/patches/$1"
    if [ -d "$pd" ]; then
        stamp="$(cat "$pd"/*.patch | shasum | cut -c1-40)"
        if [ "$(cat "$d/.patch-stamp" 2>/dev/null || true)" != "$stamp" ]; then
            git -C "$d" reset -q --hard "$3"
            git -C "$d" clean -qfd
            for p in "$pd"/*.patch; do git -C "$d" apply "$p"; done
            echo "$stamp" > "$d/.patch-stamp"
        fi
    fi
    return 0
}

build_asar() {
    fetch asar "$ASAR_URL" "$ASAR_PIN"
    cmake -S "$tc/src/asar/src/asar" -B "$top/build/toolchains/asar" -DCMAKE_BUILD_TYPE=Release \
        -DASAR_GEN_LIB=OFF -DASAR_GEN_DLL=OFF >/dev/null
    cmake --build "$top/build/toolchains/asar" -j "$jobs" >/dev/null 2>&1
    mkdir -p "$tc/asar"
    cp "$(find "$top/build/toolchains/asar" -type f -name asar -perm -u+x | head -1)" "$tc/asar/asar"
    "$tc/asar/asar" --version | head -1
}

build_bsnes() {
    fetch bsnes "$BSNES_URL" "$BSNES_PIN"
    make -C "$tc/src/bsnes/bsnes" target=libretro binary=library -j "$jobs" >/dev/null 2>&1
    mkdir -p "$tc/bsnes"
    # Built into a new file and moved in: never overwrite a core a run is using.
    cp "$tc/src/bsnes/bsnes/out/bsnes_libretro.dylib" "$tc/bsnes/bsnes_libretro.dylib.new"
    mv "$tc/bsnes/bsnes_libretro.dylib.new" "$tc/bsnes/bsnes_libretro.dylib"
    make -C "$top/tools/aladdin/snesbox" >/dev/null
    echo "bsnes core + box built"
}

what="${1:-all}"
case "$what" in
    asar) build_asar ;;
    bsnes) build_bsnes ;;
    all) build_asar; build_bsnes ;;
    *) echo "unknown component: $what" >&2; exit 2 ;;
esac
python3 "$top/tools/common/install_hooks.py"
