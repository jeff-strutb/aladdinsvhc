# Aladdin on nativeshvc: the check against the original

The native game is checked against the original only to find bugs in the game's own logic. One of the decomp's input
scripts is played on the original ROM in bsnes and on the native game, headless, with the same buttons for the same
number of frames, and nativeshvc's `tools/nshvc-check` compares the two sounds sample by sample. bsnes is used only
by this check, as a reference on the test bench. The native game has no emulation in it.

## Commands

Run from the repository root, with `ALADDIN_CPU_RUN` set to the machine's CPU queue. Build the headless game first (`docs/aladdin/native/BUILD.md`). Then the full check
for one script (a name in `tools/aladdin/scripts`, or a path to a `.box` file):

```bash
native/aladdin/tools/check.sh attract
```

The original alone, with no native game, which also measures how long the script is and how much sound it makes:

```bash
native/aladdin/tools/check.sh --oracle-only attract
```

Turning one script into the engine's `inputs.txt` (one line per change: the update number, then each pad's buttons
in hex), and listing which scripts can be replayed:

```bash
python3 native/aladdin/tools/check_inputs.py tools/aladdin/scripts/level1_walk.box --out inputs.txt
```

```bash
python3 native/aladdin/tools/check_inputs.py --classify
```

`check.sh` writes `build/aladdin/native/check/NAME/`: `inputs.txt`, `script.txt` (the oracle's script, made by
`nshvc-check`'s own conversion), `keys.txt` (the game's input), `original.raw` and `game.raw` (16-bit stereo at the
DSP's 32,040 Hz), `result.txt`. Every compile and run goes through the machine's CPU and memory queue, the command
`ALADDIN_CPU_RUN` names (as for `tools/aladdin/runner.py`; `check.sh` will not start without it), each with a
timeout: by default 60 s plus a second per 50 frames for the original (it ran `level1_walk`, 4,704 frames,
in 24 s here), and twice that for the full check. Its settings are environment variables:

| Variable | Default | |
|---|---|---|
| `NSHVC_ROOT` | `/home/strutb/projects/strutb/nativeshvc` | engine checkout: `tools/nshvc-check`, `tests/oracle/oracle.c` |
| `CORE` | the main checkout's `tools/toolchains/bsnes/bsnes_libretro.so` | the project's core, as `setup.sh bsnes` builds it |
| `ROM` | the main checkout's `reference/aladdin/Aladdin (USA).sfc` | its SHA-1 is checked |
| `GAME` | `build/aladdin/native/headless/aladdin` | the native game |
| `KEYS` | `nshvc-check`'s own (B=5A, Y=41, SELECT=10, START=0D, UP=26, DOWN=28, LEFT=25, RIGHT=27, A=58, X=53, L=51, R=57) | the key the game's controls give each button; set it when the game's controls are written |
| `FRAMES` | the whole script | play only this many frames |
| `LIMIT` | 60 + frames / 50 | seconds for the original's run |

**The oracle and the core.** The oracle is compiled from `tests/oracle/oracle.c` into
`build/aladdin/native/check/bin/oracle`, and nothing is written into the nativeshvc checkout. The project's own
bsnes core serves as the oracle's core: it has every libretro function the oracle loads, and the DSP sample hook the
oracle records from (patch `0011-snesbox-dsp-samples.patch`). That hook is named `snesbox_dsp_sample`, and the oracle
looks for `sfxbox_dsp_sample`, Stunt Race FX's name for the same patch. Without the hook the oracle records the
core's 48 kHz resampled output, which can never match the engine's 32,040 Hz sound. So `check.sh` copies the core
into `bin/` and builds `bin/libcheckcore.so` from `native/aladdin/tools/check_core_shim.c`: a library linked against
that copy that gives the core's hook and its `snesbox_memory` under the oracle's names. The oracle loads it as its
`--core`. The core itself is not changed or rebuilt. The copy is refreshed whenever the main checkout's core
differs, so a rebuild of the shared core never lands in the middle of a run.

## Recordings of the original

`native/checks/record.sh` records the original on input-only scripts, for the stage 1 replay, the sound driver's check
(stage 3) and the state comparison (stage 5). Recordings find bugs; they are never a target (PLAN.md decision 3).
Run from the repository root with `ALADDIN_CPU_RUN` set:

```bash
native/checks/record.sh attract level1_walk
```

```bash
JOBS=4 native/checks/record.sh --all
```

Each recording is `build/aladdin/native/recordings/NAME/` (from the ROM, never committed). Its files and their formats
are in `native/checks/record/record.c`'s header:

- `state.bin`: the machine (work RAM, VRAM, the sprite table (OAM), the palette (CGRAM), APU RAM) at each frame's end
  and at each picture's line 0, each as changes from the one before. The picture's own mark is what the replay draws
  from: the core returns from a frame early in the vertical blank, before that blank's uploads.
- `ppu.bin`: every write that reaches a PPU register, by the CPU, DMA or HDMA, with its update, line and dot. With
  `state.bin` this gives the per-line picture state.
- `pictures.txt`: the update and DSP sample count at each picture's line 0.
- `sound.raw`: the DSP's samples at 32,040 Hz, as `check.sh` compares.
- `apu.bin`: the APU RAM after power-on, then every SPC700 write (so every DSP register write, through `$F2`/`$F3`)
  with its DSP sample time. The voice events (key-ons, pitch, volumes) are read from these.
- `ports.bin`: every S-CPU or DMA write to an APU port, with its update and DSP sample time: the port commands.
- `frames.txt`: the DSP samples made by each frame's end.
- `shots/F.ppm`: bsnes's picture at the end of frame F, every `SHOT_EVERY` frames (default 60) and at the frames in
  `SHOTS`.
- `voices.bin`, with `VOICES=1`: each voice's state at every sample (160 bytes a sample).

**The core.** The project's core has no hook for PPU register writes. `native/checks/core/build.sh` builds a private
copy into `build/aladdin/native/checks/`: the main checkout's patched source (`tools/toolchains/src/bsnes`) copied with
its objects, `native/checks/core/0001-ppu-writes.patch` applied, and what it touches rebuilt. The patch adds two hooks,
`snesbox_ppu_write` and `snesbox_ppu_picture`, defined in the PPU's own file. The project's core, its source and the
gates are not changed. `record.sh` runs the build each time; when nothing changed, it only relinks the
library. The first build recompiled 10 of the 36 objects, not only the PPU's. The main checkout's source had its patch
set applied again at 2026-11-18 10:05 (its `.patch-stamp` and every patched file carry that time), and its `obj/` is
from 2026-11-17, so every object that includes a patched file was out of date. The private core exports the same
symbols as the installed project core plus the two hooks. On `attract` its sound is byte for byte the oracle's sound
from the installed core (4,798,023 samples).

**Measured, `attract`** (9,000 frames): 47 s, 231 MB (`apu.bin` 81 MB, `ppu.bin` 78 MB, `state.bin` 35 MB, `sound.raw`
19 MB), 150 pictures. 6,536,848 PPU writes, most of them in lines 224-255 (the sprite table and VRAM by DMA in the
vertical blank); 113,751 port writes; 9,000 picture starts, picture N starting in update N (every line of
`pictures.txt`). Within an update the vertical blank's writes (lines 225 on) come first, then the picture's lines 0-224.

**All 30 input-only scripts** (`JOBS=4 record.sh --all`, 2026-11-19): 30 recorded, none failed; 244,559 frames; 6.3 GB.
The BG modes they use, from `ppu.bin`: `attract` modes 0 and 1 only; the other 29 also mode 7 (the title path); the
two act 7 plays (`rand_pw7_1`, `rand_pw7_2`) also mode 2. None writes `$2133` (no interlace, hires or overscan) or
mosaic.

## The replay

`native/checks/replay` (ported from Stunt Race FX's, which reads the same recording format) is a small program on the
engine that makes, from one recording, the calls a converted game would: the tiles and palette from VRAM and CGRAM, the
sprite table, each layer's map and scroll, windows, colour math, and each mid-picture register write as a line change
(`nshvc_line_from`). It tests how the hardware's picture state maps to the engine, and the engine, with no game logic.
`native/checks/compare.py` puts each of the recording's bsnes pictures beside the engine's picture of the same frame.

```bash
native/checks/replay.sh attract
```

```bash
native/checks/replay.sh attract --plant
```

It writes `build/aladdin/native/replays/NAME[-plant]/`: `shots/` (the engine's pictures that have a bsnes picture),
`beside/F.ppm` (bsnes left, the engine right, the differing pixels in white below), `compare.txt`. `--plant` plants a
change the comparison must find: BG1's horizontal scroll one pixel off.

**bsnes's colours.** The core's libretro front end draws every colour through a gamma of 1.5
(`target-libretro/program.cpp`, `updateVideoPalette`, hard-coded). So bsnes's pictures are not the console's colours:
palette colour (2, 1, 10) shows as (0, 0, 8). The engine draws the palette's colours. `compare.py` puts each engine
colour through the same function before comparing; `--raw` compares without it. Found on the first run: 141 of 150
pictures differed with no gamma, every difference a dark channel brighter in the engine (bsnes (0, 0, 8) against the
engine's (2, 1, 10)). The recording's palette at that picture held the engine's colours and not bsnes's; the function
then gives bsnes's exactly. The same gamma is in `check.sh`'s oracle pictures, which are looked at, not compared.

**Run, `attract`** (2026-11-19, the Linux Vulkan build headless on Dozen):

| Run | Pictures compared | The same | Differ | Pixels |
|---|---:|---:|---:|---:|
| `replay.sh attract` | 150 | 150 | 0 | 0 |
| `replay.sh attract --plant` | 150 | 15 | 135 | 597,860 |

The 15 the plant leaves the same, each from the recording: BG1 off in 6 (frames 60-360, `$212C` = $04), forced blank
in 2 (420, 7680), brightness 0 in 5 (720, 780, 3540, 3720, 6300), and at 3600 and 3660 BG1's whole map is one tile
whose pixels are all 0 (transparent; the picture is 52 and 32 sprites).

**Mode 7 and offset-per-tile** (added 2026-11-20). Mode 7: the matrix, centre and scroll through the console's shared
write-twice latch (`$211B`-`$2120` and the Mode 7 side of `$210D`/`$210E`), the plane from VRAM (the low byte of words
0-`$3FFF` the 128 x 128 map, the high byte the tiles' texels), and each of these as a line change. Offset-per-tile:
BG3's map words given raw as the offset map (`nshvc_map_write_snes`), and BG1 and BG2 given it whenever any line of the
picture is in mode 2, 4 or 6. Act 7 shows its HUD in mode 1 for lines 0-43 and switches to mode 2 at line 44, and the
engine has no line change for the offsets. Fixed on the way, found by act 7: the replay's maps are two 64-row regions
(1,024 rows). A vertical scroll or offset past 512 (act 7's offsets wrap below 0: `$1FF`, `$1FE`, ...) ran into
region 1 instead of back to the top, so region 1 now starts as a copy of region 0.

The replay gives the engine the console's own limits (`nshvc_limits`: 32 sprites and 34 sprite tiles a line, the
wrap, and engine 6.7's `blank_hides_sprites`), as it replays the console's state.

| Run (2026-11-20, engine 6.7 uncommitted, see below) | Pictures | The same | Differ | Pixels |
|---|---:|---:|---:|---:|
| `attract` (on 6.6, before the limits) | 150 | 150 | 0 | 0 |
| `cov_title_idle` (title, Mode 7) | 198 | 196 | 2 | 5,038 |
| `rand_pw7_1` (act 7, mode 2) | 145 | 145 | 0 | 0 |

The 2 title pictures, 5160 and 5220 (195 and 4,843 pixels): Mode 7 drawn smaller than its texels (the matrix's a and d
`$08D0` and `$0150`); the engine area-filters the plane there by its design (api.md section 3.4) and bsnes point-samples.
Not a bug.

What the runs before found, and how each was settled:

- **Engine: offset-per-tile on mode 1 lines** (act 7, 25 pictures from 7140 on, rows 8-42, columns 48-87). BG3's map
  row 1 holds five HUD words with bit 13 set (`$2C60`), and the engine took them as BG1's offsets on the HUD's mode 1
  lines (`compose.c` `opt_on` did not look at the line's mode); the console has no offsets in mode 1. Predicted from the
  map before the run, then seen. Fixed in the engine (6.7).
- **Engine: sprites on the first line after forced blank** (title 3720 and 4500, 12 and 9 pixels on picture row 8). The
  console finds a line's sprites during the line above and none while blanked (bsnes `sfc/ppu/object.cpp` lines 37 and
  101). Engine 6.7 has it as a limit, `nshvc_limits.blank_hides_sprites`, off by default as the API's limits are
  (Jeff's choice); the replay turns it on.
- **Replay: the sprite tile limit** (act 7 8340 and 8400, about 170 and 5 pixels at the snake's edge). Rows 164-170
  need 39-40 sprite tiles and rows 173-174 need 35, past the console's 34 a line, exactly the rows that differed; bsnes
  drops the excess and the replay set no limits. Settled by the limits above.
- **Replay: offsets from a mode switched partway** and **maps that did not wrap at 512 rows** (act 7): fixed in the
  replay (1bf75c8).

**The engine change** (Jeff, 2026-11-19: "The bug should be fixed wherever owns it"), left uncommitted in the nativeshvc
checkout for the session that commits it: `compose.c` `opt_on` requires mode 2, 4 or 6; `picture.c` `build` gives a row
below a line blanked anywhere no sprites when `blank_hides_sprites` is set; `nshvc_limits.blank_hides_sprites` at the
struct's end; two scenes in `tests/scenes/scenes.py`. API 6.7: `docs/contract.md`'s rule "A **minor** release may only add
calls, settings, or fields at the end of a struct that a zeroed struct leaves off" for the new field, the offset fix
being a patch-level fix ("fixes where the engine differs from what `docs/api.md` describes": api.md gives offsets for
modes 2, 4 and 6) inside it. Its 118 scenes on Linux Vulkan (dzn), compared through the core's gamma (the scenes'
byte comparison cannot pass with this project's core, which draws through a gamma of 1.5): on 6.6 112 match and 4 KNOWN
of the first 116, and the two new scenes differ by 19 and 23,980 pixels; on 6.7 114 match and the same 4 KNOWN, no other
picture changed.

**Paused** (the master, 2026-11-20, until the attract demo runs on the native game). A run of the remaining
recordings on engine 6.7 was stopped after 3: `attract` 150 of 150 the same (with the limits set);
`cov_debug_code` 75 of 76; `cov_title_demo_skip` finished, its result not kept; the 4th stopped partway.

**Open:** `cov_debug_code`, 1 picture of 76 differs, 6,402 pixels: not examined.

**Not covered yet:** 25 recordings have not been replayed at all, and `cov_title_demo_skip` needs its run again. The
pictures every 60 frames are a sample: a difference in the frames between is not seen.

## What a result means

- `FAIL sound: the first difference is at sample S of N`, exit 1: the first sample where the two sounds differ. At
  32,040 Hz, sample S is about frame S / 533.1 of the original (60.0988 frames a second). Look there for the bug:
  in the game's logic, or in when the game starts a sound. `game.raw` and `original.raw` are kept for looking at.
- `ok   sound: N samples`, exit 0: every compared sample is the same. Check that N is the script's length, about
  frames × 533. `nshvc-check` compares only as many samples as the shorter file has, so a game that writes no sound
  at all reports `ok` with 0 samples.
- `--sound-offset N` (pass it to `nshvc-check` by hand) skips N samples of the game's sound, or of the original's
  when negative, for a game that reaches the same music sooner or later than the original.

The engine's guide (`docs/conversion-guide.md` in nativeshvc, section 7) says a converted game's frame timing
differs from the ROM's: its C does not model the original's slow frames. So a correct conversion plays the same
music, but not necessarily aligned sample for sample across a whole script. What the check gives is the first point
where they part, the place to start looking.

### Shown so far, on the stage 0 build (a blank screen and no sound)

| Run | Frames | Original's samples | First sound | Result |
|---|---|---|---|---|
| `--oracle-only attract` | 9,000 | 4,798,023 (4,492,693 not silent) | sample 197,619 | ran |
| `--oracle-only level1_walk` | 4,704 | 2,507,770 (2,112,509 not silent) | sample 197,619 | ran, 24 s |
| `attract`, full check | 9,000 | 4,798,023 | | `FAIL` at sample 197,619, exit 1 |
| `level1_walk`, full check | 4,704 | 2,507,770 | | `FAIL` at sample 197,619, exit 1 |

The expected results: 9,000 frames at 60.0988 a second is 4.80 M samples at 32,040 Hz (48 kHz would be 7.19 M),
and the silent game first differs where the original first makes a sound.

## What the check does not cover

- **Pictures.** They are not compared. The engine draws natively at the window's size, and an emulator's picture
  is not its standard. The game's `--shots DIR` writes each picture to look at.
- **Timing.** Frame and sample alignment are not a standard here (see above). A difference caused only by when
  something happens is not by itself a bug in the game's logic.
- **Anything a script reaches by pokes, state recipes with pokes, unit calls or copier RAM.** Only 30 of the 378
  scripts replay by input. The rest set up the game's memory directly, which a native game cannot take.

## A developer's start at a screen (not the game's)

Some screens are far into a play (the Genie, the continue screen, the level end, the story card, the ending). The environment can start the game at one, with the state it reads set. Off unless asked for; read once at the boot task's first step (`system/dev_start.c`):

| Variable | |
|---|---|
| `ALADDIN_DEV_SCREEN` | `genie`, `continue`, `level-end`, `story-card`, `ending-story`, `ending` (stages 4-$0B, 60 items), `credits` (the single stage with the credits' text, 10 items) |
| `ALADDIN_DEV_PART`, `ALADDIN_DEV_STAGE` | the progress (`level-end` and `story-card` read the part; 6 is the last, which goes on to the ending's story) |
| `ALADDIN_DEV_ITEMS` | the items counted (the ending's stages need 50 or more) |
| `ALADDIN_DEV_CHEAT=1` | the cheat set from the start, for the title's level select (the options screen's cheat code is gone with the screen: DEPARTURES.md section 7). Every play by keys is run with it set |

```bash
cd build/aladdin/native/linux && ALADDIN_DEV_SCREEN=genie ./aladdin --headless --updates 2500 --shots ../shots-genie
```

Each screen waits for a button where the game does (the Genie for one to stop the wheel, the level end for a button after its pauses, the continue screen for the rope). Without the variables nothing changes.

The level select by keys: with `ALADDIN_DEV_CHEAT=1` set, at the title hold R and press Up (part) and Right (stage), then Game Start (`native/checks/plays/select_input.py PART STAGE` writes the keys). Before 2026-11-23 the cheat was entered on pad 2 at the options screen (L, R, Start, Select, X, Y, A, B, one press each within 32 frames); the screen is gone, and `native/checks/plays/level_select_input.py` writes that old prefix, kept for `convert_prefix.py` and for the original ROM.

## The scripts that replay by input

`check_inputs.py --classify` sorts all 378 scripts in `tools/aladdin/scripts`. A script replays by input when it
uses only `wait`, `hold`, `press` and `repeat` (its `shot`, `dump` and `save` lines are left out). It may also start
with `load` of a state whose recipe in `tools/aladdin/states` is itself input only. That recipe is replayed from
power-on up to its `save`, the way the oracle replays a load. The input-only states are `title` and `level1`
(`01_title.box`) and `pw1` to `pw7` (`02_passwords.box`, each password entered on the password screen). The
`stage_*` states (`03_stages.box`) poke the act and stage, so they are not input only.

| | Scripts |
|---|---|
| Replayable by input | **30**: 3 from power-on with no state, 27 through an input-only state |
| Not: pokes | 267 |
| Not: `set` (unit calls) | 44 |
| Not: `call` | 28 |
| Not: copier RAM (`cartram`) | 5 |
| Not: a `load` after frames have run | 2 |
| Not: `spccall` | 2 |

Each blocked script is counted by the first blocker found. Password K selects act K, stage 0, from the table at
`$80:FE36` (`01 02 03 04 05 06 07`, read from the ROM). `level1` is GAME START from the title: act 0, stage 0. The
act and stage are the game's `$035C` and `$035D`. By area, with each script's length in frames:

| Area | Scripts |
|---|---|
| Power-on: logo, intro story, title, attract demo | `attract` 9,000; `rand_menus1` 6,035; `rand_menus2` 6,080 (random menu play) |
| Title menu, options, password screen (from `title`) | `cov_title_idle` 11,884; `cov_title_idle_long` 20,884 (the demo twice); `cov_title_demo_skip` 4,908; `cov_title_up` 2,962; `options` 3,934; `options_all` 12,798; `cov_debug_code` 4,600 (the options screen's pad-2 code); `password` 4,070 |
| Act 0, stage 0 (from `level1`) | `level1_walk` 4,704; `rand_level1_1` 7,507; `rand_level1_2` 7,493; `rand_level1_3` 7,548; `rand_level1_4` 7,561 |
| Act 1, stage 0 (`pw1`) | `rand_pw1_1` 8,752; `rand_pw1_2` 8,792 |
| Act 2, stage 0 (`pw2`) | `rand_pw2_1` 8,760; `rand_pw2_2` 8,774 |
| Act 3, stage 0 (`pw3`) | `rand_pw3_1` 8,800; `rand_pw3_2` 8,740 |
| Act 4, stage 0 (`pw4`) | `rand_pw4_1` 8,745; `rand_pw4_2` 8,705 |
| Act 5, stage 0 (`pw5`) | `rand_pw5_1` 8,759; `rand_pw5_2` 8,778 |
| Act 6, stage 0 (`pw6`) | `rand_pw6_1` 8,772; `rand_pw6_2` 8,756 |
| Act 7, stage 0 (`pw7`) | `rand_pw7_1` 8,747; `rand_pw7_2` 8,711 |

The `rand_*` level scripts are random play from the level's start (`randplay.py`). How far each one gets into its
level has not been looked at. Not covered by any input-only script: act 8, every stage after stage 0, the Genie
bonus screen (unless a random play happens to reach it, which has not been checked) and the ending. Reaching those
by input needs new scripts: played from a password, and written into `tools/aladdin/scripts` like any other.

## Checking every actor kind against the original (the method of kinds D, 2026-11-21)

Aim: for each registered kind (`native/aladdin/src/actor/kinds_*.c`), a recording of the ORIGINAL with its work RAM dumped after every frame, and
`aladdin_actor_check` (native/checks/actors) comparing the native kind with it. Results go in `build/aladdin/writers/NATIVE-ACTOR-CHECK.txt` (main checkout).

**Tools** (all in `native/checks/`; the scripts in `coverage/` have the main checkout's path `M=` and the writer's build dir `build/aladdin/wip-kinds-d` hard-coded: edit them first):
- `play_dump.py OUT.box FIRST LAST PLAY.box...` input-only plays from power-on (passwords: `tools/aladdin/states/02_passwords.box` + `tools/aladdin/scripts/rand_pwN_1.box`; the recipe is a prefix, see `coverage/` and the segment trick below).
- `stage_dump.py OUT.box FIRST LAST 03_stages.box stage_aX_sY PLAY.box` the same from a stage's recipe (`repeat`/`poke` expanded; stage states give levels 4-18 with their kinds).
- `coverage/scriptdump.py OUT.box SCRIPT.box` any decomp script (`tools/aladdin/scripts/cov_*.box`) with `dump fNNNNN` after every frame; `call`/`set` commands are not supported (reported).
- `actor_check_kinds.sh DIR FIRST LAST KIND[,KIND...]` one check run, output in `$OUT/kind_KK.txt`: per kind "N fields differ, the first at frame F". Run it with `ALADDIN_CPU_RUN` set to the CPU queue command, `BUILD` = the dir of `aladdin_actor_check` (cmake -S native/checks/actors, NSHVC_ROOT) and an ABSOLUTE `OUT`.
- `coverage/sect.py SCRIPT KINDS` checks a replayed script SECTION by section; `coverage/covrun.sh`, `covall.sh` run scripts in a queue; `windows.py` splits a play at record-free frames (death restarts); `levelst.py`/`alls.sh`/`long.sh` find a level's load frame and record a stage play; `plc.py LEVEL KIND..` lists a level's placements.
- Recording: `build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script X.box --out DIR` through the CPU queue (`cpu-run -j 1 -m 2G`) (each dump is 128 KB: a 3,500-frame window is 450 MB; delete when done unless asked to keep).

**Pitfalls (each cost a day):**
1. The first dump must precede the records' creation (level loaded, no record live), or the records already running in it are only FOLLOWED, not compared. After a death restart the records are freed by the level code: start a new window at the last record-free frame (windows.py).
2. A script's `load` restarts the whole world: check each section between loads separately (sect.py); differences "at the load frame" are not faults.
3. List a kind WITH the kinds it depends on: $7D with $7C (BackdropMover sets $1A2C first), $A6/$35 with $68/$6D, $63 with $A2, $97 with $53, $68 with $00 (the player's mid-frame x; his dump is the frame's end). Alone, the player/world fields come from the end-of-frame dump (check artifacts (b): his x/y/$1428 after the actors moved them).
4. A script that pokes a LIVE record (its fields, state, or its kind) cannot be followed by the native record: differences after the poke are not faults. Pokes into a free record that the check sees as made new are fine.
5. The original lags: a frame whose loop was cut mid-way ($032F/$0330 nonzero in the dump, the tick $147C not stepped) shifts records after the cut by one frame; level $05's course maker stalls the game 85 frames (DEPARTURES.md). Not faults.
6. Only the first 40 differing fields are printed: run ONE kind at a time (or list companions) for the first lines, and read the dumps themselves (record = $08DB + $50 n; +$00 state, +$06 kind, +$03-$05 resume, own bytes from +$40) to see what the original did.
7. Native own bytes that name a record hold its INDEX (original: its address $08DB + $50 n), and kind $97's +$4A the free-list place as an index (original: byte offset): the check compares those by meaning.
8. Never run the game or the check without `--headless --updates N` through the CPU queue; an unknown flag runs the game uncapped.
9. To find what the original executed in a frame: save states a few frames before (`save NAME` in the script), then `snesbox --trace` over the frames and compare `edges.csv` against a normal window (control transfers by instruction address).

**The all-kinds run (kinds D, 2026-11-21): what was added and learnt** (results: the "EVERY KIND" section of `NATIVE-ACTOR-CHECK.txt`)
- `coverage/sect.py SCRIPT KINDS` (env `LEVELSPLIT=1`: also sections at each level-number change and where records first appear after none; KINDS `+KK,KK` = checked together; frames beyond a recording's first need no 1-based numbering),
  `covone.sh`, `agg2.py WORKDIR` (per kind: sections exact / after a restart / after a poke / unexplained, from the live ranges), `table.py` (the table lines), `mass.py` (frames where 3 or more records are freed at once),
  `classify.py` (what the original did at each unexplained first frame: the loop's cut, a restart = play-time tick reset to 0, records appearing/vanishing), `playkinds.py` (every kind of a recording one by one, also with the player's turn),
  `tracewhy.sh SCRIPT FRAME` (snesbox --trace: the control transfers new in that frame; 81:825F LevelRamSetup = a restart), `longrec.sh`/`longcheck.sh` (snesbox stops a run at 9,999 frames: a script is run to frame SPLIT without dumps, then from its saved state with dumps).
- An "exact" result for a kind that is not live is vacuous (nothing is compared of it): check the kind's live frames in the recording (agg2 does).
- The original's loop is often CUT by the NMI ($032F nonzero, the records left): at the level's first loop (a start that takes longer than a frame), in a course maker. That dump has the records up to the one it was in; the next dump has the rest of
  the loop and the tail, and no new loop; a cut that stays at the same record for several dumps is a stall (nothing runs between). The check models this (check.c), and a record cut in the middle of its turn shows half done in the original at that one frame.
- A kind alone sees the dump for everything else: a kind whose creator or helper is not listed differs ($5B needs $55, $63 needs $A2, $97 with $53); list them together. A kind that reads the player ($68) is exact only with kind 00 listed.
- Kinds $21 $2E $40: +$16 is not compared (a probe list their start sets that no routine reads for them).

**Finding a recording for a kind:** plays first (stage states 4-18; the random play of 3,000 frames dies early and meets only what is near its start), then the decomp's coverage scripts (`tools/aladdin/scripts/cov_kind*`, `cov_*`: names, comments and `t3.py --qualify ADDR` show which reach a routine), whose pokes put a record of the kind in front of the original; where a script has no frame-by-frame form (unit `call`) or pokes a live record, say so in the table.

## Plays by keys, start to a level's exit (the developer's player)

`native/checks/plays/pPART-STAGE.txt` are `--input` files: the level select by keys, then the keys the developer's player (`system/dev_bot.c`)
found. **The prefix (2026-11-23):** every play is run with `ALADDIN_DEV_CHEAT=1` and begins with `select_input.py`'s keys: Start at 2400, Down at 2700 and Up at 2715 (the moves keep the title's idle countdown from handing over to the demo, as the old prefix's visit to the options screen did), R held with Up and Right from 3700, Start at 3900; the level plays from update 3934 as it did, and the player's keys (from 4100) are unchanged. The plays were converted by `convert_prefix.py OLD_DIR NEW_DIR` (it refuses a file whose old prefix is not exactly `level_select_input.py`'s for the part and stage it selects), and proved by `prove_convert.sh` on the build before the menu changed: each original and its converted play, run to its last key + 600 with the developer traces (the player's line, every live actor with its own bytes, the video fields), have the same trace from update 3900 on. `same.sh`, `behind.sh`, `make.sh`, `run.sh` and `fill_shots.sh` set `ALADDIN_DEV_CHEAT=1` for runs with an input. Replay: `native/checks/plays/run.sh FILE UPDATES [PICTURES_DIR [EVERY [FROM]]]`. Make one: `native/checks/plays/make.sh PART STAGE LAST`
with `ALADDIN_DEV_BOT_GE=10` (the archive search: it keeps for each place the shortest keys that got there and tries random keys from the
places, until the level changes), `ALADDIN_DEV_BOT_SEG=6`, `_GE_ROUNDS`, `_GE_LEN`, `_GE_HURT`, `_GE_HEALTH`, `_GE_CELL`, `_GE_SHORT`, `_GE_STALL`
(the depth-first helper), `_GOAL=boss`, or waypoints (`_WAY`, `native/checks/plays/ways.txt`). `ALADDIN_DEV_MAP`/`_PLACED` list a level's map and
actor placements. Linux only (the copies are forked); run it under the CPU queue (`cpu-run -j 11`). The table of what each play reaches is in
`build/aladdin/writers/NATIVE-FAULTS.txt` ("PLAYS START TO EXIT").

## Replaying a native play on the original, update by update (2026-11-21)

A play (`native/checks/plays/pPART-STAGE.txt`, keys by native update) is run on the original and on the native game, and `plays/compare.py` gives the first update where the
player (x, y, health) or the actor list (kind, x, y) differs. The two games do not keep step by video frame: the original often spends two video frames on one logic frame
(its play-time tick $147C/$147D does not step in the second), and a level's load takes it longer. So:

- `plays/lbox.sh DIR` builds `DIR/lbox`: the decomp's snesbox plus two script commands (`lbox.patch`; `tools/aladdin/snesbox` is not changed). `lstart BYTES` runs the original until
  the level's first logic frame is done (tick 1), however long the load took. `lhold KEYS BYTES` holds the keys until the tick has stepped once, one LOGIC frame, dumping the first
  BYTES of work RAM after every video frame (`dNNNNNN`, NNNNNN = the video frame).
- `LH_FROM=3936 LH_DUMP=8192 python3 plays/to_box.py PLAY.txt OUT.box LAST 1 3900` makes that script (the cheat poked at update 3180 and the level select as before; the converted plays' Down and Up at 2700 and 2715 are keys to the original's title menu, which has the same three lines, then `lstart`, then one `lhold` per
  native update from LH_FROM on). Without LH_FROM it makes the older one-video-frame-an-update script (then START_EARLIER must be calibrated by hand, and a lag frame shifts every
  later key by a frame: a difference that is the harness's, not the game's).
- Native: `ALADDIN_DEV_ACTORS=1 ALADDIN_DEV_TRACE=1 ./aladdin --headless --updates N --input PLAY.txt > nat.txt` (through the CPU queue). Then
  `python3 plays/compare.py ORIGDIR nat.txt 3936 LAST 2` (the 2: the tick at update 3936): it maps native update u to the first original frame whose tick reads u - 3934, so lag frames do not count.
- A death or restart ends the comparison: the native load and the original's differ in length.
- Found with it (2026-11-21): the original's four-frame freeze of the player after his health reaches 0 in p3-0 is three logic frames and one lag frame (SongPlay's upload to the
  sound processor in that frame), the same three as the native game's: no difference in the game. p0-1: the native game never counted `g.sprite_flicker` ($032D), see NATIVE-FAULTS.txt.

`ALADDIN_DEV_RECORD=FILE` (system/main.c): whatever keys and controller buttons are played, in the window or by `--input`, are written as an `--input` file (a line at each update where they change); play a level by hand with it set, then `native/checks/plays/run.sh FILE UPDATES` replays it. Checked: replaying `p0-2.txt` with it set writes the same keys at the same updates.

`ALADDIN_DEV_LEVEL=PART-STAGE` (system/dev_level.c; level 3 is `0-3`): the game's own level select is played by keys from power-on, the keys `select_input.py PART STAGE` writes, with the level select's cheat set at boot as `ALADDIN_DEV_CHEAT=1` sets it (the title menu has no options screen to type the code in), then the person's keys take over (after update 3903; the level plays from about 3934). Set with `ALADDIN_DEV_RECORD`, the recording replays from power-on with `--input` and `ALADDIN_DEV_CHEAT=1`. Checked 2026-11-23, after the options screen's removal: `0-3` headless and the `select_input.py 0 3` file with `ALADDIN_DEV_CHEAT=1` give the same trace line and actor list at every update to 4600 (9,413 lines, level 3 from update 4000); the recording's replay was not rerun.

## The two last fights: the flow after the last hit, by patched health (2026-11-21)

Jeff's decision: the bosses of level 16 (Jafar's first form, kind $98) and level 17 (the snake, kind $73) are NOT beaten in full by keys (three sessions
could not). They are proven as flow: a real stomp by keys on the boss, with the boss's health set by the developer's patch before it so that this one
stomp is the last. **These two fights are not won in full; each was proven by patched health and one real stomp.** The patch is
`ALADDIN_DEV_OWN=U:KIND:INDEX:VALUE` (`system/dev_bot.c`, `DevDump`): at update U every live record of that kind (hex) gets its own byte INDEX set to VALUE
(the record's own bytes, layout-independent; `ALADDIN_DEV_PATCH`, the raw state patch, is the other way).

One command each, from the repository root with `ALADDIN_CPU_RUN` set (the arguments after the level are `plays/run.sh`'s: updates, a pictures directory,
the interval, the first picture):

```bash
native/checks/plays/boss-flow.sh 16
native/checks/plays/boss-flow.sh 17
```

| Level | Play and patch | Seen (native game, headless, pictures from the Linux build) |
|---|---|---|
| 16 (5-2), kind $98 | `boss16-2hits.txt`; at update 8000 kind $98's own byte 1 (hits left, 6 as it starts) set to 1 | the play's first stomp, update 8379: the record goes to state $8C with 0 hits left; hit pose 8382-8387, then Jafar dissolves (pictures 13 down to 0 until 8427) and the record ends about 8436; the camera is freed (xmin 4512, xmax 32672 at 8420); the play's right key walks Aladdin to the exit (4841,168), `level_exiting` 8540; the stage-clear screen (level $24, "STAGE 6 CLEAR", pass word, "CONGRATULATIONS!") from 8700; level 17 loads at 9660 ("FINAL BATTLE"). Run to 10500, exit 0 |
| 17 (5-3), kind $73 | `p5-3.txt`; at update 5500 kind $73's own byte 1 (health, 7 as it starts) set to 0 | the play's stomp, update 5577: health 0 to -1 (beaten; `g.chain_head_struck` up), the head flashes 31 frames, the chain falls 5610-5720, the finale (the head sliding across, the fade) until the ending's story (level $39) loads at 8740; the story's six scenes (Jafar's wish, Genie, "JUST CALL ME AL." held 12,288 frames by the ROM's table), the credits (level $1C) from 26560, "The End" held to the end of the run (40000). Exit 0 |

The ending was reached from level 17's real end with no developer start after the patch. At "The End" the game does nothing more (its text task waits a frame at a time
for good; run to 40000, no screen change). Faults found are in `NATIVE-FAULTS.txt` (PLAYER SCENE CUED).

## The whole map of a level as pictures (developer tool)

`ALADDIN_DEV_MAPDUMP=DIR ALADDIN_DEV_MAPDUMP_LEVEL=N` (headless, `--updates 3`) loads level N as `LevelLoad` does, writes
`LL-field`, `LL-field-grid`, `LL-back`, `LL-both`, `LL-start`, `LL-backmap` (`.ppm`) and `LL-info.txt` into DIR (LL the level in hex)
and ends the run (`native/aladdin/src/system/dev_mapdump.c`). The pictures are the map through the metatiles, the tile sets and the
palette as the load leaves them, one pixel a game pixel, colour 0 flat magenta; `sips -s format png` converts them. With
`ALADDIN_DEV_MAPDUMP_HOLD=1` the run goes on with the loaded level held on the screen, so the engine's own picture of it can be
written (`--shots`) and compared with `LL-start`. It changes nothing while the variable is unset.

## The title menu without the options screen (2026-11-23, DEPARTURES.md section 7)

Builds: the tree before the change (`HEAD` of main, with the options screen) and the tree after, each headless and as a Metal build (pictures). The plays are the converted ones (above), run with `ALADDIN_DEV_CHEAT=1` on both sides.

- Conversion proof (`prove_convert.sh`, headless build before the change): all 35 plays (19 `p*`, `boss16-2hits`, 15 `end-*` with their `ALADDIN_DEV_AT`) have the same trace, from update 3900 on, as the original, update by update (the player's line, every live actor with its own bytes, the video fields): none desynced.
- `native/checks/view/same.sh` old against new, 256x224: 21 runs, 0 differ. The trace holds no text-layer pictures, so the menu's new words do not show in it.
- `native/checks/between/behind.sh` old against new (both on engine 6.10, so `BEHIND_SHIFT=0` and `ALADDIN_DEV_WHOLE_PIXELS=1` for both): every run differs in pictures that show the title menu, and in no other: the 20 plays at updates 2625 to 3925 (53 pictures of every 25th, the menu from its first frame to Start's fade; p5-2 three more, 25400 to 25450, the title after its ending shows the menu again), the boot run at 9125 to 9650 (22 pictures, the menu's one stay). 0 differ outside the menu.
- Pictures of the menu with the mark on each line, and the press on SAVED GAMES and LEVEL SELECT (A at 3000 and Start at 3040 on LEVEL SELECT; A at 3260 and Start at 3300 on SAVED GAMES): the pictures every 2nd update from 2600 to 3418 and the task trace (`ALADDIN_TASK_TRACE=1`) are the same, byte for byte, as a run of the same keys without the presses (410 pictures; 22 trace lines, the last the menu's window effect at 2622); on the build before the change the same press at 3040 starts the fade (a task at 3040) and the options screen.


## The view following the window (developer variables and checks; VIEW.md 4.8)

The view follows the window at an update's start, in a level, on a still screen and with the pause menu open. Headless there is no window, so the changes are made by the developer's variables (all read at an update's start):

| Variable | What it does |
|---|---|
| `ALADDIN_DEV_VIEW_AT=UPDATE:WxH[,UPDATE:WxH...]` | at each update named the view asked for becomes WxH (the original's height, at least 288 across, as `ALADDIN_VIEW` is clamped), taken at once with no wait. Up to 512 entries. With `ALADDIN_VIEW=WxH` the run starts at that view (a level started at 256 x 224 is not the check: start at 288x224); alone it starts at the headless 256 x 224 |
| `ALADDIN_DEV_WINDOW_AT=UPDATE:WxH[,...]` | at each update named the window's pixels the rule reads become WxH (1920x1080 is 16:9), so the view follows by the rule and waits as a window's does: a change that needs the level maps' margins made again is taken once the target has held 6 updates. `ALADDIN_VIEW_WINDOW=WxH` is the window before the first |
| `ALADDIN_DEV_VIEW=1` | prints each view chosen and, for each change in a level, the maps made again and the time it took (`view followed the window at update N: M level maps made again, X ms`) |
| `ALADDIN_DEV_NO_SPRITES=1` | the picture without sprites: the level's layers and the status line alone |
| `ALADDIN_DEV_SLOTS=1` | prints each palette slot a kind takes (`SLOT u=N kind=KK slot=S of C`) |

`native/checks/view/change.sh APP OUTDIR PLAY LAST FROM TO UPDATE [K]` (APP a macOS build, the pictures need Metal; PLAY a file of `native/checks/plays` or `-` for the boot; `CHANGE_ENV="NAME=VALUE ..."` adds environment to the three runs, e.g. `ALADDIN_DEV_SCREEN=genie` or `ALADDIN_DEV_NO_SPRITES=1`): the play is run to LAST three times with the trace and a picture every update from UPDATE on: *chg* starts at FROM wide and changes to TO at UPDATE, *same* stays at FROM, *fresh* starts at TO. `change_report.py` then prints the traces of chg against same and against fresh (the player's line, the video line, the actors), and for K pictures from UPDATE + 1 the width, how many are identical to fresh's, for each that differs the pixels and the box they are in and whether the two games differ in state at that update (a difference with no state difference is listed UNEXPLAINED), and the added columns that are all black (levels only: a still screen's are black by design). A picture `uN` is the one drawn after update N - 1.

`native/checks/view/shape_load.sh BIN A_PLAY U LEN B_PLAY V A_VIEW B_VIEW` (a save made at one shape and loaded at another: `saves/exact.sh` with `EXACT_A_ENV`/`EXACT_B_ENV` `ALADDIN_VIEW=...`, then the player's, video and actor lines of the two compared; the same shape must be the same). `native/checks/view/level_select_open.txt` is an `--input` file that opens the title's LEVEL SELECT (Start at 2400, Down twice, Z at 2760), for `change.sh` on a screen that is not a level (pass its absolute path as PLAY).

Results, 2026-11-23: VIEW.md 4.8 "As built" and 4.9.

## The beaten levels' record, the level pictures and the level start (2026-11-23, LEVELSELECT.md)

Stage 1 of the level select (the screen is not built, LEVELSELECT.md section 6). macOS headless builds, `build/aladdin/native/base` (71ea8d0) and `new`; each run under `perl -e 'alarm N'` (this Mac has no `timeout`).

- `native/checks/view/same.sh` base against new, 256x224: 21 runs, 0 differ.
- Beaten record, each `p*.txt` play to its last key + 600, `ALADDIN_DEV_CHEAT=1`, an empty `PORT_SAVEDIR`: the file holds only the play's level for p0-0 to p5-1 and p6-0 (`ALADDIN_DEV_BEATEN=1` prints it). p5-2 and p5-3 reach no exit by keys (Jeff's decision above); with `boss-flow.sh`'s patches (`8000:98:1:1` on `boss16-2hits.txt`, `5500:73:1:0` on `p5-3.txt`) each marks its own level ($10, $11) and no other. p1-1 then p0-2 on one folder with a run between: `05`, then `02 05`. Which call fired in each: `LevelExitCheck` for $00 $06 $07 $0A $0D $0E $0F $12 (and the others by their plays), `walk_out_fade` $03, `land_fade` $04, level 5's end scene $05, the chain's fade $11 (LEVELSELECT.md section 2).
- Bad files (`levels` cut to 5 bytes, empty, one byte longer, format 2, size byte 7, a wrong check byte, a bit above the 24, level $13's bit with a right check): `beaten:` with nothing. A right file gives its levels.
- Start by `LevelSelectStart` (`ALADDIN_DEV_START_LEVEL=N` with Start at 2400 only) against the original's select (`plays/select_input.py PART STAGE`, R held), both `ALADDIN_DEV_CHEAT=1`, traces with `ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1`, 600 updates from the level's first update: levels $00, $04, $07, $0A, $0B, $0C, $0E, $10, $11, $12: 0 differ in each (the original's first update 3933, the new one's 2656). No level is a Mode 7 stage. Without the cheat byte: level $11 starts with `health=3 lives=3`.
- Level pictures: `ALADDIN_DEV_MAPDUMP` for the 19 levels writes the same `LL-*` files as before the picture code moved to `video/level_picture.c`; `LL-picture.ppm` (`LevelPicture`, 96 x 72) differs in 0 of 6,912 pixels from the same crop of `LL-start.ppm`, for each. Not run: the crop against the engine's own picture (`ALADDIN_DEV_MAPDUMP_HOLD` with `--shots`), which needs a renderer; this run was the null headless build.

## Save slots (2026-11-23, SAVES.md)

- `native/checks/saves/saves_build.sh OUT` then `OUT DIR`: the slot files with a fake snapshot (system/saves.c): written, read back, loaded; refused whole with a wrong layout mark, a byte too many, cut short, a bit changed in the snapshot or in the crop, a bad magic; a failed write (the temporary's name taken) leaves the old slot. 14 lines, all ok.
- `native/checks/pause_check/grid_build.sh OUT` then `OUT BASE.ppm W H game|title PAGE SEL ASK USED OUT.ppm [CROP.ppm]`: the slots' screen over a game picture (pictures in `docs/aladdin/native/shots/saves-*.png`); the crop equals the picture's own pixels (0 of 6,912 differ).
- A developer's picture (`ALADDIN_DEV_SHOTS`) is also taken while the pause menu is open; the headless renderer draws no overlays, so the screens are composed by these tools as `pause_check menu` composes the pause menu.
- `native/checks/saves/exact.sh BIN A_PLAY U LEN B_PLAY V [fault]` (header of the script; `EXACT_A_ENV`, `EXACT_UI`, `EXACT_MENU_SAVE`): a save at update U of one run loaded at update V of another (or from the title through SAVED GAMES by keys, `title_load_keys.txt`); the traces from the load on are the same, update for update; results in SAVES.md (7 runs: 6 same, the planted fault caught).

## Runahead (developer variables and checks; FRAMERATE.md section 9)

`ALADDIN_RUNAHEAD=N`: the ahead steps a tick takes after the real one. Default 1 in a window, 0 (off) in a headless run; 0 turns it
off anywhere. Only 0 and 1 are built: a larger N is taken as 1, with a note on stderr. It is also off, whatever N says, when one of the
developer's tools that drive or log the steps themselves is set (`ALADDIN_DEV_BOT`, `ALADDIN_DEV_TRIGGERS`, `ALADDIN_TASK_TRACE`,
`ALADDIN_DEV_MAPDUMP`, `ALADDIN_DEV_CELDUMP`). While the pause menu is open there are no ahead steps.
`ALADDIN_DEV_STATE_HASH=1`: a line `hash u=N H` on stderr after each real update, H the hash of the real world (g, the video parts
`between_keep` registered, the statics `RUNAHEAD_KEEP` registered, the level maps' words); `=2` adds each part's own hash, to find which
part differs. `ALADDIN_DEV_RUNAHEAD_REPORT=1`: at the end, how many ticks ran ahead, how many ended early, and the time of the
ticks (mean, the real step alone, median, 99th percentile, worst, how many over 16.7 ms).

- `native/checks/runahead/state.sh HEADLESS_BIN`: the real world is the same with and without ahead steps. The boot and every play of
  `native/checks/plays` run on one build with `ALADDIN_RUNAHEAD=0` and with `=1`, both with the state hash and the developer's trace
  (`view/same.sh` with `SAME_BASE_ENV`, `SAME_NEW_ENV`); the two logs must be the same line for line.
- `native/checks/runahead/future.sh MACOS_BIN INPUT FROM TO BETWEEN`: the ahead picture is the true future when the pad does not change
  (Metal build, a picture every update, drawn at between BETWEEN).
- `native/checks/runahead/latency.sh MACOS_BIN "4100 key=27\n4200" [BETWEEN]`: the updates from a key press to the first change in the
  player's state and in the picture, with and without runahead, on level 0's start.

## The snapshot (developer variables and checks; FRAMERATE.md section 10)

`ALADDIN_DEV_SNAP_SAVE=FILE@U`: at the start of update U the snapshot (the world after update U - 1) is written to FILE and its size and time
printed. `ALADDIN_DEV_SNAP_LOAD=FILE@L`: update L loads FILE and runs no step. `ALADDIN_DEV_KEPT_DUMP=U:FILE`: at update U the kept video parts' used
bytes to FILE (with `ALADDIN_DEV_STATE_HASH=2` this finds which part differs).

- `native/checks/snapshot/cross.sh MACOS_BIN PLAY S K [BASE_PLAY L]`: the snapshot across two processes, or with `SNAP_SAME=1` in one. Process A plays
  PLAY and saves at S; a fresh process B plays BASE_PLAY (or the boot and the title) to update L, loads there and is given PLAY's keys from S on. K
  updates of the trace and the state hash, and the picture of each update from the third, must be the same. `SNAP_SAVE_ENV` and `SNAP_LOAD_ENV` add
  environment (the runahead, a view, a planted omission) to A and to B.
- `native/checks/snapshot/pointers.sh`: the pointer members of every type the game state holds are the ones the snapshot writes as codes.
- `python3 native/aladdin/tools/gen_code_table.py --check`: `actor/code_table.c` names every function a row of the kinds tables or an `ActorRunAs` names.
- `native/checks/saves/exact.sh` with `EXACT_RUNAHEAD=1`: the six cases and the planted fault as with runahead off (SAVES.md, "With runahead on"). `native/checks/saves/crop_check.sh MACBIN [U]` (Metal build, headless): the crop a pause-menu save keeps against the picture of the saved state, runahead off and on: 0 of 6,912 pixels differ in both.

## The level select screen (2026-11-23, LEVELSELECT.md section 9)

- Pictures made at build time (`native/aladdin/tools/make_pictures.sh`, 0.7 s, 525,412 B). `native/checks/pause_check/level_build.sh OUT` (with `NSHVC` set to the engine's checkout) builds `level_check`: `level_check BASE.ppm W H PAGE SEL OUT.ppm [PICS_PREFIX]` with `LEVEL_FILE` (a `levels` file) and `DATA_TEST_DIR` (`cart.dat`, `level_pictures.dat`). The screens: `shots/levelselect-none.png`, `-some.png`, `-page2.png`, `-last.png` (every level shown, unbeaten at 25%).
- 9 levels: each picture the screen reads equals the map dump's `LL-picture.ppm` (0 of 6,912 differ), and on the panel at one pixel (704 x 564 window) 0 of 6,912 differ at its cell.
- Choosing: all 19 entries, from the title by keys with all 19 beaten, no cheat byte: each reaches its level (health above 0) at update 2839 to 2914.
- `same.sh` base 71ea8d0 against the merged tree: 21 runs, 0 differ.
- Unbeaten cells cannot be chosen: empty save folder, Start, Down, Down, B, B, B: no level plays to update 3500; with level $00 beaten the same keys play it from 2839.
