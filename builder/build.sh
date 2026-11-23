#!/bin/sh
# builder/build.sh: build Aladdin Builder for macOS, Windows and Linux.
#
#   build/builder/dist/AladdinBuilder-VERSION-macOS.zip    Aladdin Builder.app, universal:
#                                                          builds Aladdin for the Mac
#   build/builder/dist/AladdinBuilder-VERSION-Windows.zip  Aladdin Builder\AladdinBuilder.exe
#                                                          and game\ (the payload), x64
#   build/builder/dist/AladdinBuilder-VERSION-Linux-x86_64.tar.gz  Aladdin Builder/AladdinBuilder
#   build/builder/dist/AladdinBuilder-VERSION-Linux-arm64.tar.gz   and game/ (the payload), per CPU
#
# Each carries the game (native/aladdin on nativeshvc) built with none of the
# cartridge's data in it: the game reads cart.dat, which the builder writes
# from the player's ROM, and level_pictures.dat, which the builder has the
# game make from cart.dat; check_payload.py proves the executables carry no
# data before anything is packaged. The payload is built from HEAD, or REV
# (a worktree of it at build/builder/tree, so cart.dat and level_pictures.dat,
# which are never committed, cannot be in it), never from the working tree,
# so what is released is what is committed. Its icon, and the builder's, is
# the game's own (native/aladdin/icon/icon.png, made into each system's form
# by nativeshvc's nshvc-icons).
#
# VERSION: 0.1 unless set.
# Needs nativeshvc's checkout (NSHVC, ../nativeshvc), the ROM (ROM, default
# reference/aladdin/: check_payload.py reads it to prove its absence), Xcode's
# clang, cmake and ninja, mingw-w64 (x86_64-w64-mingw32-*) for Windows, and
# the Vulkan headers (brew's vulkan-headers) for Linux. The game has no
# shaders of its own; the engine's are built into it. Linux is built on a
# Linux machine over ssh (LINUX_HOST, default strutb@192.168.1.25, in
# ~/aladdin-linux there): builder/linux/build-linux.sh says what it needs on it.
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
VERSION=${VERSION:-0.1}
NSHVC=$(cd "${NSHVC:-../nativeshvc}" && pwd)
B=$ROOT/build/builder
DIST=$B/dist
TREE=$B/tree
ROM=${ROM:-"$ROOT/reference/aladdin/Aladdin (USA).sfc"}
[ -f "$ROM" ] || { echo "build.sh: no ROM at $ROM (set ROM)" >&2; exit 1; }
echo "Aladdin Builder $VERSION"

# ---- the source: HEAD, as committed ----------------------------------------------------------
REV=${REV:-$(git rev-parse HEAD)}   # (release.sh: the commit it tags)
if [ -d "$TREE" ]; then
    git worktree remove --force "$TREE" 2>/dev/null || rm -rf "$TREE"
    git worktree prune
fi
git worktree add -q --detach "$TREE" "$REV"
G=$TREE/native/aladdin
echo "source: $REV"
if [ -n "$(ls -A "$G/assets" | grep -v '^\.gitkeep$')" ]; then
    echo "build.sh: the game's assets folder holds files at $REV; the payload must carry no data" >&2
    exit 1
fi
rm -rf $B/payload-macos $B/payload-windows $B/payload-linux-* $B/gen $B/game-* $B/linux
mkdir -p $DIST $B/payload-macos $B/payload-windows $B/win $B/gen $B/linux

# the game for one system, as nativeshvc's nshvc-build configures it
game() {    # DIR CMAKE-ARGS...
    name=$1 d=$B/$1; shift
    if ! { cmake -S "$G" -B "$d" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$NSHVC" "$@" &&
           cmake --build "$d"; } > "$d.log" 2>&1; then
        grep -E "error|Error" "$d.log" | head -20
        echo "build.sh: the game did not build ($d.log)" >&2
        exit 1
    fi
    echo "built $name"
}

# ---- the payload: the Mac (universal) ---------------------------------------------------------
for A in arm64 x86_64; do
    game game-macos-$A -DNSHVC_HOST=macos -DNSHVC_RENDER=metal -DCMAKE_OSX_ARCHITECTURES=$A -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
done
MACAPP=$B/game-macos-arm64/aladdin.app/Contents
lipo -create $MACAPP/MacOS/aladdin $B/game-macos-x86_64/aladdin.app/Contents/MacOS/aladdin \
    -output $B/payload-macos/aladdin
strip -S $B/payload-macos/aladdin
cp $MACAPP/Resources/AppIcon.icns $B/payload-macos/AppIcon.icns

# ---- the payload: Windows ---------------------------------------------------------------------
game game-windows -DNSHVC_HOST=windows -DNSHVC_RENDER=d3d12 -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
x86_64-w64-mingw32-strip -o $B/payload-windows/aladdin.exe $B/game-windows/aladdin.exe

python3 builder/check_payload.py "$ROM" "$G/tools/assets.csv" $B/payload-macos/aladdin $B/payload-windows/aladdin.exe

# ---- the builder's data table: the game's own list of what it reads ---------------------------
python3 - "$G/tools/assets.csv" > $B/gen/cart_table.h <<'EOF'
import csv, sys
rows = [(int(r['start'], 16), int(r['end'], 16)) for r in csv.DictReader(open(sys.argv[1]))]
print('/* cart_table.h, written by builder/build.sh from native/aladdin/tools/assets.csv:')
print('   each range [start, end) of the ROM the game reads */')
print('#define ALADDIN_CART_RANGES %d' % len(rows))
print('static const uint32_t k_aladdin_cart[ALADDIN_CART_RANGES][2] = {')
for s, e in rows:
    print('    { 0x%06X, 0x%06X },' % (s, e))
print('};')
EOF

# ---- the payload and builder: Linux, x86_64 and arm64 -----------------------------------------
# built on a Linux machine against Ubuntu 22.04's C library (builder/linux/), from the same commit
LINUX_HOST=${LINUX_HOST:-strutb@192.168.1.25}
LW=aladdin-linux
ssh -o BatchMode=yes $LINUX_HOST "mkdir -p $LW/src/native $LW/nshvc $LW/vk $LW/gen"
rsync -a --delete --exclude build --exclude assets "$G" $LINUX_HOST:$LW/src/native/
rsync -a --delete --exclude build "$ROOT/builder" $LINUX_HOST:$LW/src/   # as the Mac and Windows builders (release.sh requires it committed)
rsync -a --delete "$NSHVC/src" "$NSHVC/include" "$NSHVC/cmake" "$NSHVC/tools" "$NSHVC/CMakeLists.txt" $LINUX_HOST:$LW/nshvc/
rsync -aL --delete "$(brew --prefix)/include/vulkan" "$(brew --prefix)/include/vk_video" $LINUX_HOST:$LW/vk/
rsync -a $B/gen/cart_table.h $LINUX_HOST:$LW/gen/
ssh -o BatchMode=yes $LINUX_HOST "mkdir -p $LW/src/native/aladdin/assets && sh $LW/src/builder/linux/build-linux.sh $LW $VERSION" \
    > $B/linux/build.log 2>&1 || { tail -30 $B/linux/build.log; echo "build.sh: the Linux build failed ($B/linux/build.log)" >&2; exit 1; }
grep -E "^built|needs glibc" $B/linux/build.log
for A in amd64 arm64; do
    rsync -a $LINUX_HOST:$LW/out-$A/ $B/payload-linux-$A/
done
python3 builder/check_payload.py "$ROM" "$G/tools/assets.csv" $B/payload-linux-amd64/aladdin \
    $B/payload-linux-arm64/aladdin
rm -f $DIST/AladdinBuilder-*-Linux-*.tar.gz
for A in amd64 arm64; do
    [ $A = amd64 ] && CPU=x86_64 || CPU=arm64
    L="$B/linux/$CPU/Aladdin Builder"
    mkdir -p "$L/game"
    cp $B/payload-linux-$A/AladdinBuilder "$L/"
    cp $B/payload-linux-$A/aladdin $B/payload-linux-$A/icon.png $B/payload-linux-$A/icon.argb "$L/game/"
    chmod 755 "$L/AladdinBuilder" "$L/game/aladdin"
    COPYFILE_DISABLE=1 tar -C "$B/linux/$CPU" --no-mac-metadata --uid 0 --gid 0 --uname root --gname root -czf "$DIST/AladdinBuilder-$VERSION-Linux-$CPU.tar.gz" \
        "Aladdin Builder"
done

# ---- macOS: Aladdin Builder.app ---------------------------------------------------------------
APP="$B/mac/Aladdin Builder.app"
rm -rf "$B/mac"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/payload"
clang -arch arm64 -arch x86_64 -mmacosx-version-min=12.0 -O2 -Wall -Wextra -fobjc-arc -DRB_VERSION="\"$VERSION\"" \
    -Ibuilder/src/core -I$B/gen builder/src/core/*.c builder/src/mac/main.m -framework Cocoa \
    -o "$APP/Contents/MacOS/AladdinBuilder"
sed "s/@VERSION@/$VERSION/g" builder/src/mac/Info.plist > "$APP/Contents/Info.plist"
printf 'APPL????' > "$APP/Contents/PkgInfo"
cp $B/payload-macos/AppIcon.icns "$APP/Contents/Resources/AppIcon.icns"
cp -R $B/payload-macos/ "$APP/Contents/Resources/payload/"
codesign --force --sign - "$APP/Contents/Resources/payload/aladdin"
codesign --force --sign - "$APP"
codesign --verify --deep --strict "$APP"
rm -f $DIST/AladdinBuilder-*-macOS.zip
ditto -c -k --keepParent "$APP" $DIST/AladdinBuilder-$VERSION-macOS.zip

# ---- Windows: AladdinBuilder.exe --------------------------------------------------------------
MAJOR=${VERSION%%.*}
MINOR=$(echo "${VERSION#*.}" | sed 's/^0*\([0-9]\)/\1/')
cat > $B/win/payload.h <<EOF
#define RB_VER_MAJOR $MAJOR
#define RB_VER_MINOR $MINOR
#define RB_VERSION_STR "$VERSION"
EOF
cp $B/game-windows/aladdin-icon/icon.ico $B/win/builder.ico
x86_64-w64-mingw32-windres -I$B/win -Ibuilder/src/win builder/src/win/builder.rc -O coff -o $B/win/builder.res.o
rm -rf $DIST/AladdinBuilder-*-Windows.* "$B/win/Aladdin Builder"
mkdir -p "$B/win/Aladdin Builder/game"
OBJS=
for s in builder/src/core/*.c builder/src/win/main.c; do
    o=$B/win/$(basename $s .c).o
    x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DRB_VERSION="\"$VERSION\"" -Ibuilder/src/core -I$B/gen \
        -c $s -o $o
    OBJS="$OBJS $o"
done
x86_64-w64-mingw32-gcc -municode -mwindows -static -s -o "$B/win/Aladdin Builder/AladdinBuilder.exe" \
    $OBJS $B/win/builder.res.o -lcomctl32 -lole32 -lshell32 -luuid
# the game beside it as a plain file: the builder copies it, it carries no
# executable inside itself
cp $B/payload-windows/aladdin.exe "$B/win/Aladdin Builder/game/"
(cd $B/win && zip -qrX "$DIST/AladdinBuilder-$VERSION-Windows.zip" "Aladdin Builder")

git worktree remove --force "$TREE"
echo $REV > $DIST/REV          # the source these zips are from (release.sh --no-build)
ls -la $DIST
