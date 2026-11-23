# Aladdin on nativeshvc: building

## Building the native game

The game is `native/aladdin` (its icon `native/aladdin/icon/icon.png`, the box art, which the build turns into each platform's icon), built against nativeshvc's checkout (API 6.12 or later, for the game's own save folder; tested at `8b5461e` on 6.7 before that; the sound driver needs 6.6 or later) beside this one. Run
from the repository root. The cartridge's data is never committed; `extract_assets.py` checks the ROM's SHA-1
(`bd5fea18ce7456cc0231a39473927d8ea9c6589b`) and writes `native/aladdin/assets/cart.dat`, which the build copies
beside the program:

```bash
python3 native/aladdin/tools/extract_assets.py "reference/aladdin/Aladdin (USA).sfc" native/aladdin/assets
```

`cart.dat` is the cartridge image, 1,310,720 B, with the bytes that are not code at their place and zeros where the
code was: 1,047,228 B read as data by the decomp's traces, 173,223 B no trace has touched (kept, as Stunt Race FX keeps
its unread ranges), 4 B of code the game reads as a table (kind `code-read`), and 90,265 B of code left out (of the
87,793 B the S-CPU ran, 2,353 B the SPC700 ran and 123 B the routine census decodes). The S-CPU code the traces read as
data is 8,972 B in 177 runs; `native/aladdin/tools/code_read.csv` gives each byte's readers and decision (2026-11-19:
4 B kept, read as tables in play by Kind6EStart and Kind2DStart; 8,968 B left out, each with its reason: a block copy
of code to video memory 8,312, read only in unit calls 323, instruction fetches only 243, task calls' inline arguments
132, a checksum 15, code sent to the sound memory 5), and `native/aladdin/tools/code_read/README` how it was made. The
ranges are `native/aladdin/tools/assets.csv` (2,024 ranges), made by
`native/aladdin/tools/make_assets_csv.py` from the decomp's code map (`tools/aladdin/codemap.py`) and census
(`tools/aladdin/census.py`) with the same tests as `tools/aladdin/progress.py`; remake it after new traces with
`python3 native/aladdin/tools/make_assets_csv.py .` (it needs `build/aladdin` traced).

## Packaging and saves (2026-11-23)

The game's saves are in its own folder beside the program (`PAUSE.md`, "Where the game saves"): `aladdin-save` beside `aladdin.app`,
`save` beside `aladdin.exe` or the Linux program. Whoever packages a build ships it without that folder, never with a
developer's, and keeps the player's when an update replaces the build. On iOS they are in the app's container.

## Platforms (2026-11-21)

The engine's own `tools/nshvc-build PLATFORM GAME_DIR` configures each platform (`headless`, `macos`, `ios`, `android`,
`windows` (Direct3D 12), `windows-vulkan`, `linux`); Stunt Race FX's `builder/build.sh` does the same with plain cmake lines. Neither
project has CI, so Aladdin has none: each platform is built the way those two build it. The game has no platform code
(`native/aladdin/CMakeLists.txt` is the same five lines as Stunt Race FX's, less its shaders); the hosts, renderers and
toolchain choices are the engine's. `cart.dat` is made from the ROM, never committed (`native/aladdin/.gitignore`) and
never packaged: a build copies it beside the program, so a build for anyone else is made without it and the player's
own is written from their ROM, as Stunt Race FX's builder does.

| Platform | Built how | Commit | What was run |
|---|---|---|---|
| Linux (x86_64, Vulkan) | cmake + Ninja, `NSHVC_HOST=linux NSHVC_RENDER=vulkan`, fresh build folder | `a92d22da` (engine `8b5461e`) | Built clean. Headless boot (`--headless --updates 3200 --input keys.txt`, Start at 2400): exit 0, `updates 3200 pictures 3200`, on Vulkan through Mesa dzn. Not run in a window on this commit. |
| Windows (x86_64, Direct3D 12) | Cross-built on Jeff's Mac with mingw-w64 (Homebrew), `tools/nshvc-build windows native/aladdin` | `c6803c7` (engine `e64b9e8`) | Built clean (`aladdin.exe`, PE32+ GUI, 2.1 MB, linked static). Run headless on Jeff's Windows 11 machine (AMD RX 6950 XT), 4,000 updates with Start at 2400, at 256 x 224 and 384 x 224: exit 0 and every picture compared with the Mac's (Metal). At each size 3,124 of 4,000 are the same byte for byte; the other 876 are the same updates at both sizes: the title's fade to white (updates 2561 to 2607), where some pixels differ by one step of 255, and the first level from its fade-in on (3169 on), where a few hundred pixels around the sprites differ by up to about 30 steps (update 3500: 421 pixels, rows 84 to 123). The Vulkan build on the same machine gives those same pictures byte for byte, so they come from that GPU's arithmetic and not from either renderer: the Mac's GPU and the AMD one round differently. Which of the two is right was not worked out. Vulkan there also differs in updates 621 to 2430, by 8 and 16 steps in rows 119 to 142; Direct3D 12 does not. **Not run in a window**: nobody was signed in on that machine's desktop. |
| macOS (Metal, arm64) | cmake + Ninja on Jeff's Mac (Xcode 27.2, Metal toolchain), the lines below with `-DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` | `b36b712` (engine `2d4726f`); the view work on engine 6.8 (`21a6c32`) | Built clean (0 warnings). Headless 11,000 updates, exit 0; pictures written; the app opened in a window. The first build stopped at start on the stack check: the tile writer's overrun, fixed in `b36b712`. A headless run of the app's program reads `cart.dat` from `PORT_RESDIR` (set it to the bundle's `Contents/Resources`). |

Windows is Direct3D 12 (Jeff, 2026-11-23: "the windows build should use d3d not vulkan"). From a Mac or Linux machine
with mingw-w64:

```bash
../nativeshvc/tools/nshvc-build windows native/aladdin
```

The program is `native/aladdin/build/windows/aladdin.exe`, with `cart.dat` beside it. The engine's D3D12 shaders are
compiled ahead and kept in its source, so the build needs no `dxc`.

The Vulkan build instead, from a Linux machine (the Vulkan headers are `/usr/include/vulkan` and `vk_video`; they are put
in a folder of their own so the Linux system headers are not pulled in after mingw's):

```bash
mkdir -p build/aladdin/native/vkinc && ln -sfn /usr/include/vulkan build/aladdin/native/vkinc/vulkan && ln -sfn /usr/include/vk_video build/aladdin/native/vkinc/vk_video
cmake -S native/aladdin -B build/aladdin/native/windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT=/home/strutb/projects/strutb/nativeshvc -DNSHVC_HOST=windows -DNSHVC_RENDER=vulkan -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DNSHVC_VULKAN_INCLUDE=$PWD/build/aladdin/native/vkinc && cmake --build build/aladdin/native/windows -j 4
```

Copy `aladdin.exe` and `cart.dat` to a Windows machine and run it (`aladdin.exe --headless --updates 3200 --input keys.txt`
is the boot check; `keys.txt` holds `2400 key=0D` and `2410`; a headless run reads `cart.dat` from `PORT_RESDIR`).

macOS, on a Mac with Xcode, cmake, ninja, `glslangValidator` and `spirv-cross` (the same lines Stunt Race FX uses; add
`-DCMAKE_OSX_ARCHITECTURES=arm64` or `x86_64`, and `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0`, as its builder does for a Mac app):

```bash
cmake -S native/aladdin -B build/aladdin/native/macos -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT=../nativeshvc -DNSHVC_HOST=macos -DNSHVC_RENDER=metal && cmake --build build/aladdin/native/macos
```

The result is `aladdin.app`, with `cart.dat` in its resources. Run the boot check on the executable inside it.

Linux with Vulkan, and the headless build (no host: the null renderer), out of tree under `build/aladdin/native`
(`NSHVC_ROOT` is the nativeshvc checkout; on this machine each build and run went through its shared CPU queue, 4 cores for a build and 2 for a run):

```bash
cmake -S native/aladdin -B build/aladdin/native/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT=/home/strutb/projects/strutb/nativeshvc -DNSHVC_HOST=linux -DNSHVC_RENDER=vulkan && cmake --build build/aladdin/native/linux -j 4
```

```bash
cmake -S native/aladdin -B build/aladdin/native/headless -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT=/home/strutb/projects/strutb/nativeshvc && cmake --build build/aladdin/native/headless -j 4
```

Run headless for 120 updates, writing each picture (`NNNNNN.ppm`, 256 x 224), and in a window for 8 seconds:

```bash
mkdir -p build/aladdin/native/shots-linux && (cd build/aladdin/native/linux && timeout 120 ./aladdin --headless --updates 120 --shots ../shots-linux)
```

```bash
(cd build/aladdin/native/linux && timeout 8 ./aladdin)
```

The game starts at the copyright card and runs the logo, the opening story, the title and, after about 11,000 updates
with no input, the attract demo's level 1. The headless run prints `updates 120 pictures 120` and writes one picture per
update. The headless build's null renderer writes no pictures. The window run is ended by `timeout` (exit 124).
Keys can be replayed from a file (`--input FILE`, lines `UPDATE key=VKHEX`); the developer's starts and plays are in
[CHECK.md](CHECK.md). The checks run for the README: `python3 native/aladdin/tools/clean_check.py native/aladdin/src`,
`python3 native/aladdin/tools/data_check.py "reference/aladdin/Aladdin (USA).sfc"`, and 11,000 updates of each build
(`./aladdin --headless --updates 11000`, exit 0).

**The Vulkan driver on this machine.** Vulkan prints `dzn is not a conformant Vulkan implementation`. This machine is a
Hyper-V guest whose GPU, the host's AMD Radeon RX 6950 XT, is shared through GPU partitioning (`/dev/dxg`, kernel driver
`dxgkrnl`; the guest's PCI device is `Microsoft Basic Render Driver`, its DRM devices `hyperv_drm` and `faux`). The only
Vulkan devices are Mesa's Dozen (Vulkan over Direct3D 12, `/etc/vulkan/icd.d/dzn_icd.x86_64.json`, installed by hand in
`/opt/mesa-dzn`) on that GPU, and llvmpipe on the CPU; the engine takes the first device with a graphics queue that
can show to the window, Dozen. A native driver (RADV, whose ICD is installed) would need the guest to see the AMD GPU
itself: the whole GPU given to the VM (Hyper-V Discrete Device Assignment) or Linux on the machine directly.
