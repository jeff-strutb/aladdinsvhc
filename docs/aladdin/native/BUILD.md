# Aladdin on nativeshvc: building

## Building the native game

The game is `native/aladdin` (its icon `native/aladdin/icon/icon.png`, the box art, which the build turns into each platform's icon), built against nativeshvc's checkout (API 6.5, tested at `8d1745d`) beside this one. Run
from the repository root. The cartridge's data is never committed; `extract_assets.py` checks the ROM's SHA-1
(`bd5fea18ce7456cc0231a39473927d8ea9c6589b`) and writes `native/aladdin/assets/cart.dat`, which the build copies
beside the program:

```bash
python3 native/aladdin/tools/extract_assets.py "reference/aladdin/Aladdin (USA).sfc" native/aladdin/assets
```

`cart.dat` is the cartridge image, 1,310,720 B, with the bytes that are not code at their place and zeros where the
code was: 1,047,228 B read as data by the decomp's traces, 173,223 B no trace has touched (kept, as Stunt Race FX keeps
its unread ranges), and 90,269 B of code left out (87,793 B the S-CPU ran, 2,353 B the SPC700 ran, 123 B the routine
census decodes). The ranges are `native/aladdin/tools/assets.csv` (2,021 ranges), made by
`native/aladdin/tools/make_assets_csv.py` from the decomp's code map (`tools/aladdin/codemap.py`) and census
(`tools/aladdin/census.py`) with the same tests as `tools/aladdin/progress.py`; remake it after new traces with
`python3 native/aladdin/tools/make_assets_csv.py .` (it needs `build/aladdin` traced).

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

At stage 0 the game shows a blank screen: the headless run prints `updates 120 pictures 120` and writes 120 pictures,
every pixel black. The headless build's null renderer writes no pictures. The window run is ended by `timeout` (exit
124).

**The Vulkan driver on this machine.** Vulkan prints `dzn is not a conformant Vulkan implementation`. This machine is a
Hyper-V guest whose GPU, the host's AMD Radeon RX 6950 XT, is shared through GPU partitioning (`/dev/dxg`, kernel driver
`dxgkrnl`; the guest's PCI device is `Microsoft Basic Render Driver`, its DRM devices `hyperv_drm` and `faux`). The only
Vulkan devices are Mesa's Dozen (Vulkan over Direct3D 12, `/etc/vulkan/icd.d/dzn_icd.x86_64.json`, installed by hand in
`/opt/mesa-dzn`) on that GPU, and llvmpipe on the CPU; the engine takes the first device with a graphics queue that
can show to the window, Dozen. A native driver (RADV, whose ICD is installed) would need the guest to see the AMD GPU
itself: the whole GPU given to the VM (Hyper-V Discrete Device Assignment) or Linux on the machine directly.
