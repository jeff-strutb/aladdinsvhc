#!/bin/sh
# builder/release.sh: build Aladdin Builder for macOS, Windows and Linux
# and publish them on the repository's GitHub Releases page as vVERSION.
#
# VERSION: 0.96 unless set. The release is tagged at HEAD as it is when this
# starts, which must already be on GitHub. Running it again at the same
# version replaces the files of that release.
#
# --no-build publishes the zips already in build/builder/dist, which must
# have been built from HEAD (build.sh writes the commit to dist/REV): for
# zips built and checked before the release.
#
# Usage: builder/release.sh [--dry-run] [--no-build]
set -e
cd "$(dirname "$0")/.."
DRY=0 BUILD=1
for a in "$@"; do
    [ "$a" = --dry-run ] && DRY=1
    [ "$a" = --no-build ] && BUILD=0
done

VERSION=${VERSION:-0.96}
TAG=v$VERSION
DIST=build/builder/dist
MAC=$DIST/AladdinBuilder-$VERSION-macOS.zip
WIN=$DIST/AladdinBuilder-$VERSION-Windows.zip
LINUX_X64=$DIST/AladdinBuilder-$VERSION-Linux-x86_64.tar.gz
LINUX_ARM=$DIST/AladdinBuilder-$VERSION-Linux-arm64.tar.gz

if [ -n "$(git status --porcelain -- builder native/aladdin)" ]; then
    echo "release: builder/ or native/aladdin/ has uncommitted changes; commit them first" >&2
    exit 1
fi
git fetch -q origin
REV=$(git rev-parse HEAD)          # the commit checked here is the one tagged, whatever lands meanwhile
if [ $DRY = 0 ] && ! git merge-base --is-ancestor $REV origin/main; then
    echo "release: HEAD is not on origin/main; push it first" >&2
    exit 1
fi

if [ $BUILD = 1 ]; then
    VERSION=$VERSION REV=$REV builder/build.sh
elif [ "$(cat $DIST/REV 2>/dev/null)" != "$REV" ] || [ ! -f $MAC ] || [ ! -f $WIN ] || [ ! -f $LINUX_X64 ] || [ ! -f $LINUX_ARM ]; then
    echo "release: the zips in $DIST are not built from HEAD ($REV); build them first" >&2
    exit 1
fi

NOTES=$(mktemp)
cat > $NOTES <<EOF
Aladdin Builder makes a native build of **Aladdin** (Super Nintendo, 1993) from your own copy of the cartridge: the game as C, decompiled from the cartridge and compiled for your machine, running on nativeshvc, with everything drawn by the GPU. On a Mac it builds the game for the Mac; on Windows, for Windows; on Linux, for Linux.

The game's data is copyrighted, so the builder does not include it: you provide your ROM, the builder checks it by MD5, takes the data the game reads from it (the ROM's code is not used: the C stands for it), has the game make its level select's pictures from that data, and produces a game that runs on its own. The ROM is not needed once the build is done.

### New in 0.96
- **Menus read their own keys and buttons**: Enter or the controller's Start chooses, Escape or the controller's Back goes back, whatever the game's buttons are mapped to; the hints name your real keys, or your controller's own buttons.
- **Remapping**: a button waits for the next key or button only once the press that chose it is let go, says what it is waiting for, and Escape or Back cancels.
- **Widescreen**: the carpet level's obstacles come and hit where the original's do; on levels that scroll by themselves Aladdin can move across the whole wide view; the level's art past its edges no longer shows cut-off props at level starts and ends.
- **Sprites made of several pieces** (such as the lava level's fireballs) no longer show dark lines across them.

### Features
- **Native**: the game's own C for your machine, no emulator; drawn by the GPU (Metal on the Mac, Direct3D 12 on Windows, Vulkan on Linux).
- **Widescreen at any shape**, 4:3 to ultrawide: a wider window shows more of the level, filled past its edges from its own art; play keeps the original's distances.
- **Smooth motion at any refresh rate**: the original's 60 Hz logic, every picture eased between updates.
- **Runahead**: a press shows one update sooner; the game itself is untouched.
- **Save games**: 24 slots, each the exact moment with a picture of it; save from the pause menu, load from there or the title.
- **Level select**: all 19 levels with their pictures; the ones you have beaten can be started.
- **A pause menu** in the game's own lettering (Start or Escape): controller and key remapping, save and load, full screen, quit.
- **Portable**: saves, settings and progress live in a folder beside the game, so a copied game carries them.

The original's password screens and options screen are gone in their place.

### Download
- **macOS** (12 or later, Apple silicon and Intel; the game it builds needs the same): \`AladdinBuilder-$VERSION-macOS.zip\`. Unzip and open Aladdin Builder. It is not notarized: if macOS refuses to open it, Control-click it and choose Open, or allow it in System Settings, Privacy & Security.
- **Windows** (10 or later, 64-bit; the game it builds needs the same, and a graphics driver with Direct3D 12): \`AladdinBuilder-$VERSION-Windows.zip\`. Unzip it and run \`AladdinBuilder.exe\` from the unzipped folder (the \`game\` folder beside it holds the game's code). It is not signed: if SmartScreen stops it, choose More info, then Run anyway.
- **Linux** (x86_64 or arm64; glibc 2.35 or later, which is Ubuntu 22.04, Debian 12, Fedora 36 and anything newer; X11 or XWayland; a graphics driver with Vulkan; GTK 3 for the builder's window): \`AladdinBuilder-$VERSION-Linux-x86_64.tar.gz\` or \`AladdinBuilder-$VERSION-Linux-arm64.tar.gz\`. Extract it and run \`AladdinBuilder\` in the extracted folder (the \`game\` folder beside it holds the game's code). Without a desktop it builds from the terminal: \`./AladdinBuilder --rom YOUR_ROM.sfc --dest FOLDER --build\`. Run the builder, not the program in its \`game\` folder: that is the builder's copy of the game, without its data.

The builder runs the game once, without a window, while it builds (to make the level select's pictures), so the machine that builds needs the graphics driver the game needs.

### What it needs
| Your dump | MD5 |
|---|---|
| The No-Intro ROM of Aladdin (USA), \`.sfc\` (a \`.smc\` with a copier's 512-byte header is accepted) | \`8e00910312b30663768d907114d301c2\` |

The European and Japanese cartridges are not supported.

### What it makes
- Mac: \`Aladdin.app\`.
- Windows: a folder, \`Aladdin\`, with \`Aladdin.exe\` in it.
- Linux: a folder, \`Aladdin\`, with the program \`Aladdin\` in it, and \`Aladdin.desktop\` to copy to \`~/.local/share/applications\` to list the game with your other programs.

### Playing
- A keyboard or a game controller; the pause menu shows and changes what each key and button does.
- Full screen is the default; the pause menu's FULL SCREEN switches to a window and back, as do View, Enter Full Screen (Control-Command-F) on a Mac and Alt+Enter or F11 on Windows and Linux.
- Saves, settings and progress live in the game's own folder: a folder named \`save\` inside the game's folder on Windows and Linux, \`aladdin-save\` beside the app on the Mac (not inside it, so the app's signature stays valid). Where that place cannot be written (a read-only disk, Program Files, a disk image, an app run from a quarantined download) they are kept in the system's folder instead (Application Support on macOS, AppData on Windows, ~/.local/share on Linux). Rebuilding with the builder into the same place updates the game and leaves the saves alone.

Source: [builder/](https://github.com/jeff-strutb/aladdinsvhc/tree/$REV/builder), [native/aladdin/](https://github.com/jeff-strutb/aladdinsvhc/tree/$REV/native/aladdin).
EOF

if [ $DRY = 1 ]; then
    cat $NOTES
    ls -la $MAC $WIN $LINUX_X64 $LINUX_ARM
    rm -f $NOTES
    exit 0
fi
if gh release view $TAG >/dev/null 2>&1; then
    # the same version again: its tag moves to the source these files are from
    git tag -f $TAG $REV >/dev/null
    git push -q -f origin refs/tags/$TAG
    gh release upload $TAG $MAC $WIN $LINUX_X64 $LINUX_ARM --clobber
    gh release edit $TAG --notes-file $NOTES
else
    gh release create $TAG $MAC $WIN $LINUX_X64 $LINUX_ARM --target "$REV" --title "Aladdin Builder $VERSION" --notes-file $NOTES
fi
rm -f $NOTES
gh release view $TAG --json url -q .url
