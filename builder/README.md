# Aladdin Builder

A native app for macOS, Windows and Linux that builds Aladdin from the
player's own copy of it: the native game (`native/aladdin`) on nativeshvc.
Players download it from the repository's Releases page; nothing here ships
game data. It is Stunt Race FX's builder (`stuntracefx/builder`) with the
game's own parts changed.

1. What it builds: the game for the system the builder runs on (the Mac,
   Windows or Linux), and this page says what it makes.
2. The ROM and the folder to build into: the No-Intro dump of Aladdin (USA),
   `.sfc` (a `.smc` with a copier's 512-byte header is taken without it). The
   builder shows the MD5 it expects (`8e00910312b30663768d907114d301c2`, the
   file whose SHA-1 `native/aladdin/tools/extract_assets.py` checks) and the
   file's own, and builds only from a match.
3. The build: the code ships inside the builder; the data comes from the ROM.
4. The result: the app or the folder, or the reason the build stopped.

The result needs no ROM nor this tree. It carries the cartridge's data as
`cart.dat`, an image of the ROM with each range the game reads
(`native/aladdin/tools/assets.csv`) at its offset and nothing where the ROM's
code was, as `extract_assets.py` makes it (`src/core/aladdin.c`); and the
level select's pictures, `level_pictures.dat`. The pictures are made at build
time by the game's own loader, as `native/aladdin/tools/make_pictures.sh`
makes them (docs/aladdin/native/LEVELSELECT.md): the builder runs the game it
has just staged, without a window, with `ALADDIN_MAKE_PICTURES` set, its data
from the staged `cart.dat` (`PORT_RESDIR`) and its save folder a scratch
folder in the staged build (`PORT_SAVEDIR`, removed after), so the player's
saves are never read or written. The run takes about a second and is stopped
after 300 (`src/core/run.c`, `pictures` in `src/core/build.c`); the file is
checked the way the game's reader checks it (its header, size and FNV-1a sum)
before the build goes on. The machine that builds therefore needs the graphics
driver the game needs. The game, and the builder itself, have the game's own
icon (`native/aladdin/icon/icon.png`).

There is no iPhone build (Jeff, 2026-11-23: Mac, Windows and Linux only):
Aladdin has never been built for iOS and has no touch controls.

**Saves, settings and progress** live in the game's own folder (the game opts
in with the engine's `save_in_game_folder`, `native/aladdin/src/system/main.c`):
a folder `save` inside the game's folder on Windows and Linux, `aladdin-save`
beside the `.app` on the Mac. Where the place cannot be written the engine
falls back to the system's folder. Rebuilding into the same place never
deletes them: on the Mac the app is replaced and the folder beside it is not
touched; on Windows and Linux an earlier build that has a `save` folder is
updated in place (everything but `save` and the marker file replaced,
`rb_clear_except` and `finish` in `src/core/`).

The builder and the game it builds run on macOS 12 or later (Apple silicon and
Intel), Windows 10 or later (64-bit, with a Direct3D 12 graphics driver) and
Linux with glibc 2.35 or later (x86_64 or arm64, X11 or XWayland, a Vulkan
driver; GTK 3 for the builder's window, else it builds from the terminal with
`--rom FILE --dest DIR --build`).

## Layout

| path | what |
|---|---|
| `src/core/` | portable C: MD5, files (`util.c`), running the game (`run.c`), the game's data from the ROM (`aladdin.c`), the build itself (`build.c`) |
| `src/mac/` | the AppKit front end and the builder's Info.plist |
| `src/win/` | the Win32 front end, its manifest and resources |
| `src/linux/` | the GTK 3 front end (GTK opened while it runs, so the builder needs only the C library to start) |
| `linux/` | the Linux build: `build-linux.sh` (run on a Linux machine by `build.sh` over ssh), `toolchain.cmake` and `sysroot.py` (Ubuntu 22.04's C library, so the builds need glibc 2.35 at most) |
| `build.sh` | builds the builders into `build/builder/dist/`: the macOS app, a Windows zip of the exe with the game beside it in `game\`, and a Linux tar.gz per CPU laid out the same way |
| `release.sh` | builds and publishes them as a GitHub release |
| `check_payload.py` | proves the shipped game executables carry none of the cartridge's data |

## Building

```sh
builder/build.sh                 # build/builder/dist/: the macOS and Windows zips, a Linux tar.gz per CPU
builder/release.sh --dry-run     # the release notes and files, nothing published
builder/release.sh               # tag vVERSION at HEAD (already pushed) and publish
```

The payload is the native game built from HEAD (a worktree of it, so
uncommitted game work, and `cart.dat` and `level_pictures.dat`, which are
never committed, cannot ship), as nativeshvc's `nshvc-build` configures it: on
macOS universal (arm64 and x86_64 built apart and joined) drawn with Metal,
with its icon; on Windows drawn with Direct3D 12; on Linux (x86_64 and arm64)
drawn with Vulkan, built on a Linux machine over ssh (`LINUX_HOST`) against
Ubuntu 22.04's C library. The game has no shaders of its own; the engine's are
compiled into it. The builder's own sources are taken from the working tree on
every system (`release.sh` refuses to run with `builder/` uncommitted). The
builder's data table (`cart_table.h`) is written from the game's `assets.csv`
at the same commit, so the two always agree. Building needs nativeshvc's
checkout (`NSHVC`, `../nativeshvc`), Xcode's clang, cmake and ninja,
mingw-w64 for Windows, brew's vulkan-headers for Linux, and the ROM (`ROM`,
default under `reference/aladdin/`; `check_payload.py` reads it to prove its
absence).

Every front end takes arguments for a scripted run:
`--dest DIR --rom FILE --build`. A scripted run quits when the build is done
(exit code 0 on success); the Windows one writes its result to
`builder-result.txt` beside the builder. `RB_SNAPSHOT=DIR` makes the macOS app
write each page it shows to DIR as a PNG.
