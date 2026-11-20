# Aladdin on the native engine: conversion plan

Status: **stage 0 done, by another session** (`b941e4e`, `d9fcb02`; `docs/aladdin/native/BUILD.md` records its builds and
runs: a blank 256 x 224 screen headless and in a window on Linux). The rest of this plan was written from reading the source.
Engine: nativeshvc API 6.5 (`8d1745d`). Record: `src/aladdin`
at `cffb00c`, 85,916 lines, every code byte certified behaviour exact and byte exact (README). The inventory is
[hardware-touches.md](hardware-touches.md). The model is Stunt Race FX's plan (`stuntracefx/docs/stuntracefx/native/PLAN.md`,
"SRFX" below).

**How this meets the direction, in one line:** a new tree `native/aladdin` holds the game as ordinary typed C that calls
`nshvc_*` from `update` and `draw`, with no register state, bus access, coroutine or imitated hardware between them; `src/aladdin`
and `asm/aladdin` are never edited and stay the certified reference.

## Decisions

| # | Question | Answer |
|---|---|---|
| 1 | Tasks wait up to three calls deep. `docs/aladdin/TASKS.md` said the native game would keep them as coroutines; SRFX made each wait an explicit state. Which? (asked 2026-11-19) | Jeff: explicit states, as SRFX |

## How the conversion is shared between sessions

The rules are SRFX's ("How the conversion is shared between sessions"), for this tree:

- **The bar** is section 8. No bridge, no `cpu.`, `abi_`, `spc.`, flag helpers, `goto` over translated assembly, carried
  registers or SNES addresses in the game's C, under any name. Dead routines are listed with their proof, not converted.
- **The source.** Read `src/aladdin` and `asm/aladdin` where the C is unclear; write what it means in `native/aladdin/src`.
  Never copy register-style code across to tidy later. Each function keeps its `@routine` tag and its `WHAT IT DOES`.
- **The frame.** Each `update` is one video frame of the original. A wait returns from `update`; the routine on the chain
  resumes from its saved state on the frame its wait runs out (decision 1).
- **The state.** Shared variables live in `src/system/state.h` as fields of `g`, under the area that mostly writes them, each
  with its original address in a comment. Search for the address before adding a field; edit with a small exact match.
  Variables one file uses stay `static` there.
- **Cross-area calls.** A converted routine keeps its name with real parameters, declared in its area's header. A missing
  one is added under `/* wanted */` in the owner's header; the owner implements it. No silent stubs: an unconverted routine
  prints its name once to stderr and is listed in the report.
- **Files.** Each session owns the directories in its brief and edits no other but `state.h` and `/* wanted */` lines. Save a
  file only when it compiles. Commit with `git commit -- <paths>`. No pushes. No attribution of any kind.
- **Reporting.** Lead with what is not done. Name interim forms as interim, with counts.

## 1. What this plan cannot promise

1. **This is a rewrite of about 85,900 lines, not an engine hookup.** 632 lines touch hardware (574 the finder sees, 38 it
   misses, 20 at cartridge RAM). The rest of the finder's 22,345 is register style (`abi_` 6,799, `cpu.` 6,007, `op_` 2,874)
   and data at SNES addresses (5,459 lines). Beside those: 826 actor resume points (`actor_yield` 619, `actor_wait` 207),
   2,502 pushed return addresses (`jsl_frame` 1,460, `call_frame` 1,042), 1,086 `goto`s, 1,495 flag tests, 672 `spc.` uses
   (hardware-touches.md section 8).
2. **The finder sees only part of it.** Its patterns miss 30 register lines (`W34SEL`, `BG12NBA`, `A2A`, `NTRL` ...) and have
   nothing for the beam, the region, the coroutine primitives or `goto`. A finder result of 0 is required, not sufficient.
3. **Frames will not line up with bsnes.** The original spends frames where the native game spends none: `UnpackTask` unpacks
   until the beam passes line 240 and then waits a frame (`video/unpack.c:405-415`); sound sets and songs go by the IPL
   handshake (`audio/spc_upload.c`). The native game does each in one update, so it reaches screens sooner. SRFX measured the
   same effect: 60 frames sooner to its logo, about 127 to its title (SRFX Progress, row 4). Also, the original's NMI uploads
   what the tasks queued in the frame before, so its picture trails its logic by a frame; `draw` after `update` does not.
4. **Sound is compared by voice events first**, sample for sample only where it aligns (SRFX section 1 item 5; the engine's
   `docs/conversion-guide.md` section 8 says zelda3's never aligned).
5. **Nothing can be seen until stage 4.** Power-on runs the logo, then the opening story, which places actors and loads
   level $35's set-up (`game/opening_story.c:299-302`), then the title. So boot to title needs the actor loop and the story's
   actor kinds, which belong to stage 5's areas.
6. **Linux first is a first.** SRFX's row 11 says its Linux build was never run; no doc in nativeshvc records a converted game
   running on Linux. The engine's Linux host opens an X11 window (`src/host/linux/host_linux.c:189`); this machine's session
   is Wayland, so it runs under XWayland. Its GPU is the host's, shared into this Hyper-V guest, and the engine draws through
   Mesa's Dozen (Vulkan over Direct3D 12), which reports itself "not a conformant Vulkan implementation" (BUILD.md). A wrong
   picture here may be the driver's, not the game's or the engine's; telling them apart needs another machine or the whole
   GPU given to the guest (BUILD.md).
7. **Few of the decomp's scripts can drive the native game.** 3 of the 378 scripts in `tools/aladdin/scripts` run from power-on
   by input alone (`attract`, `rand_menus1`, `rand_menus2`); the rest load saved states (602 `load` lines) or poke memory
   (2,621 `poke`, 677 `set`), which the native game, having no SNES addresses, cannot take. Each level needs a new play by
   input before its state can be compared.
8. **Dead code is not converted.** 20 `@unreachable` tags; the copier branches behind the cartridge-RAM reads (`actor/kind20.c:439`
   loops for ever); the PAL branch (`game/password_screen.c:137`); the input recorder's writes to cartridge RAM
   (`system/input_modes.c`). Each is listed with its proof, from the census (`docs/aladdin/ROUTINE_CENSUS.md`).

## 2. What is there

`src/aladdin`: 85,916 lines, 340 files (326 `.c`, 14 headers), 1,655 routines.

| Area | Lines | Files | Routines | `@actor` entries | Holds |
|---|---:|---:|---:|---:|---|
| `actor` | 47,012 | 183 | 950 | 340 | the actor loop, pool, spawning, motion, collision boxes, every enemy and object kind |
| `video` | 10,673 | 43 | 226 | 0 | uploads, VRAM queue, scroll layers, parallax, palettes, sprites, Mode 7, window effects, HUD |
| `game` | 8,793 | 31 | 129 | 12 | screens: logo, opening story, title, options, password, level load and end, Genie, ending |
| `player` | 7,261 | 28 | 129 | 49 | Aladdin's states and moves |
| `level` | 3,075 | 14 | 65 | 0 | level set-ups, events, scenes, tables |
| `audio` | 2,755 | 5 | 44 | 0 | the SPC700 driver (`sound_driver.c` 2,124, `dsp_write.c` 87), the S-CPU's queue and uploads |
| `map` | 2,471 | 4 | 65 | 0 | the level map, collision, tile behaviour and changes |
| `system` | 1,956 | 16 | 42 | 0 | power-on, reset, the NMI and IRQ, tasks, frame, joypads, random |
| `include` | 1,739 | 14 | 0 | 0 | `machine.h`, `hw.h`, `ops65.h`, `cpu_ops.h`, `spc.h`, the work-RAM names |
| `math` | 153 | 1 | 4 | 0 | multiply and divide |
| `input` | 28 | 1 | 1 | 0 | pad buttons |

What waits (`docs/aladdin/TASKS.md`, `docs/aladdin/ACTORS.md`, and calls counted in the C):

- **Tasks:** 8 slots of $18 bytes from $006F; 82 task entries; in the C, 136 calls of `TaskWait`, 65 of `TaskCreate`, 28 of
  `TaskExec` and 28 of `TaskExit`, in about 40 files (`game/title_task.c` 27, `game/ending.c` 22, `video/window_effects.c` 17
  ...). 27 of the 70 wait, exec and exit sites TASKS.md measured sit one to three calls below the task's body. `FrameEnd`
  (80:AE9A) ran 14,718 waits on one script.
- **Actors:** the player's record at $08DB and $50-byte records from $092B; each runs with D = its record and yields only from
  its own code (BRK one frame, COP A frames), so nothing of it is on the stack while it waits: its state is its record and
  its resume address (ACTORS.md, measured).

## 3. The converted game

### Layout

```
native/aladdin/
  CMakeLists.txt
  icon/icon.png                 the box art (on main, d9fcb02)
  assets/cart.dat               written by tools/extract_assets.py from the player's ROM; not in git
  tools/extract_assets.py       ROM -> assets/cart.dat, checked by SHA-1 (on main)
  tools/assets.csv              2,021 data ranges: name, start, end, reader (on main, made by make_assets_csv.py)
  src/system/   main.c (nshvc_game; on main, blank screen), state.h (g), the frame, tasks, power-on and reset, random
  src/data/     the only place cartridge offsets live; typed tables and readers
  src/audio/    the driver as music code, the sound queue, sample and song loads
  src/video/    the picture state draw reads, engine objects, uploads, per-line effects, palettes, sprites, Mode 7
  src/game/     screens: logo, story, title, options, password, level load and end, Genie, ending
  src/level/  src/map/  src/player/  src/actor/  src/input/  src/save/  src/math/
native/checks/                  record, replay and compare tools: not part of the game
```

`CMakeLists.txt`, as `docs/conversion-guide.md` section 6 gives it and SRFX's has it:

```cmake
cmake_minimum_required(VERSION 3.20)
project(aladdin C)
add_subdirectory(${NSHVC_ROOT} nshvc)
include(${NSHVC_ROOT}/cmake/NshvcGame.cmake)
file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS src/*.c)
nshvc_game(aladdin SOURCES ${game_sources} ASSETS assets ICON icon/icon.png TITLE "Aladdin")
target_include_directories(aladdin PRIVATE src)
target_compile_options(aladdin PRIVATE -std=gnu11 -Wall -Wextra)
```

No `SHADERS` (optional in `cmake/NshvcGame.cmake:122`): nothing in Aladdin needs a shader of its own (it has no chip
drawing). Built with
`tools/nshvc-build linux native/aladdin`.

`tools/extract_assets.py` (on main) follows SRFX's: it drops a 512-byte copier header if there is one, refuses anything whose
SHA-1 is not `bd5fea18ce7456cc0231a39473927d8ea9c6589b` (`tools/aladdin/build.py:18`), and writes an image of the cartridge
with `assets.csv`'s ranges at their places and zeros where code was: 1,047,228 B the decomp's traces read as data,
173,223 B no trace touched, 90,269 B of code left out (BUILD.md).

### The loop

`nshvc_game` with `update_hz` 60. `update` does, in order, the NMI's game work (frame counters, each waiting task counted
down, the sound command sent) and then the task scheduler's pass: in the order `system/task_scheduler.c` scans, each ready or
new task runs to its next wait, exec or exit; slot 0's turn reads the pads first (`InputRead`). `draw` sets the whole picture
from the game's state.

### How each Aladdin-specific thing converts

| In the original | In the native game |
|---|---|
| **Tasks and their waits** (`system/task.c`, `task_scheduler.c`) | `Task tasks[8]`: state, wait, parameter, and the slot's own bytes ($06-$17) as typed fields. Each task entry is a function that takes its task and returns waiting, exec'd or ended. A wait one to three calls deep returns "waiting" up the chain, and each routine on the chain keeps where it stopped in the task's state, as SRFX's 276 waiting functions became states (decision 1). Exec sets the task's entry and starts it on its next turn; exit frees the slot. The task stacks ($0300 up), the pushed registers and `jsl_frame`/`call_frame` (2,502 sites) go. |
| **Actors' yields** (`actor/actor_loop.c`, BRK and COP) | `Actor` records with the fields of ACTORS.md (state, wait, kind, flags, the kind's own bytes) and a resume point in place of the PC, P and bank at $02-$05. Each kind's code is a function of its record that switches on the resume point; BRK returns with a wait of 1, COP with its frames; a routine that ends its actor (`actor_leave`, 32) returns "ended" to the loop. Kind $08 rewrites its resume point to another entry (ACTORS.md): an assignment of the enum. `PlayerLoop` and `ActorLoop`, called from `FrameStep` (81:8311), become plain loops over the records. |
| **NMI** (`NmiHandler`, 80:8111) | its counters, task countdown and sound send at the start of `update`; its uploads as object writes where the game queues them (below); the registers it writes from shadows become the fields `draw` reads |
| **IRQ** (`IrqHandler`, 80:8332), set by `LevelScreenSetup` for every level | in `draw`: blank down to the IRQ's line, then `nshvc_line` for `NSHVC_LINE_BLANK`, `NSHVC_LINE_BRIGHTNESS`, `NSHVC_LINE_MAIN` and `_SUB` from the line after it, and the per-line effects from there. The line is `V_1C7D`'s value or 8 (hardware-touches.md section 5) |
| **HDMA tables** ($7E:F400 up, $7E:F800, the window effects, the level set-ups, the colour bands, the ending) | per-line `int32_t` arrays, one per field, filled where the game fills its tables, indirect entries resolved; `draw` sends each with `nshvc_lines` (hardware-touches.md section 3 lists every target and its field) |
| **DMA uploads** (always channel 0) | object writes when the data changes: `nshvc_tiles_write`, `nshvc_map_write_snes`, `nshvc_palette_write`, `nshvc_sprites_set` (hardware-touches.md section 4). Each screen kind holds its tile sets and maps as engine objects. The VRAM queue's records name destinations by VRAM address; one function turns an address into its object for the current screen, the one numeric part, as the zelda3 example does (conversion guide section 8). VRAM read back (`video/mode7.c`, `UploadBlockB`) reads the game's own arrays |
| **Sprites** (`video/oam.c` building `OAM_BUFFER`, OBSEL $03) | `video/oam.c`'s builders fill an `nshvc_sprite` list (8 x 8 and 16 x 16); `draw` calls `nshvc_sprites_set(list, n, 0)` |
| **Mode 7** (`video/mode7*.c`) | `nshvc_mode7_set` from the matrix the game computes; the multiplier it borrows (`Mode7ElementScale`) is a C multiplication; its tiles and map are the game's arrays, written with `NSHVC_TILES_MODE7` |
| **The sound driver** (`audio/sound_driver.c`, `dsp_write.c`) | music code: `nshvc_music.hz` 32,040, one driver tick every 256 samples (timer 0 at $40, `sound_driver.c:188`: 8 ms), the music and effects halves on alternate ticks as the driver has them. Voices through `nshvc_voice_*`; echo through `nshvc_echo` and `nshvc_echo_writes`; main volume and noise through `nshvc_sound_*`. It never waits, locks, allocates or reads files |
| **Sound commands and uploads** (`audio/sound_command.c`, `spc_upload.c`) | each command an `nshvc_music_send` message, in order, the echo wait gone; the game thread loads a sample set's BRR samples with `nshvc_sample_load` and passes the song's data, then sends "ready"; the lists at $85:8000 (`config/aladdin/spclists.csv`) are read through `data/` |
| **Work RAM** | one typed state `g` with records as records (actors, tasks, the VRAM queue); about 540 addresses already have names in the C (hardware-touches.md section 7), the starting point for `state.h`. Direct-page scratch ($000C up) becomes locals and arguments; the register shadows become the picture's fields |
| **ROM data** | `assets/cart.dat`, read through `nshvc_asset_*` once at `init`. As SRFX ruled under its 8.2 (2026-11-09): each table's place is written once, in `data/` only; game code sees typed tables and readers named for what they hold, and no bank, address constant or cartridge read; data that points to data in its own format is passed as an opaque reference `data/` resolves |
| **Calls by address** (93) | a direct call, or a `switch` on a named id |
| **Cartridge RAM, region, beam** | constants: the retail board's open-bus result, NTSC; the beam-timed work done in one update |

## 4. Stages

Each stage deletes what it replaces from the new tree in the same change: `machine.h`, `hw.h`, `ops65.h`, `cpu_ops.h`, `spc.h`,
`spc_ops.h` and the `wram*.h` headers are never brought over. Every run is listed and asked for first (section 7).

| # | Stage | Lines of the record | Done means | Checked by |
|---|---|---:|---|---|
| 0 | **Set up** (done: `b941e4e`, `d9fcb02`): `native/aladdin` with its `CMakeLists.txt`, the icon and an empty game | 0 | `tools/nshvc-build linux native/aladdin` builds; the window opens on this machine and closes; headless `--updates 60` exits 0 | the build; one windowed launch; the engine's log naming the Vulkan device |
| 1 | **Record and replay** of the original, for finding bugs only | 0 (tooling) | `snesbox` records, per frame of an input-only play, the per-line picture state, palette, sprite table and video-memory changes, the DSP writes and the port commands; `native/checks/replay` makes from a recording the API calls a converted game would, through the engine | the replay's pictures beside bsnes's on the attract play; a planted change shown to make it differ |
| 2 | **Data and state**: `src/data` over the `assets.csv` and `extract_assets.py` already on main, `state.h`, `math` | 1,892 (`math` 153, `include` 1,739, not brought over but read for names) | every table the game reads has a `data/` reader; `state.h` holds the shared fields with their addresses in comments | every extracted range against the ROM's bytes; a CSV of `g`'s fields to original addresses, for stage 5's state comparison; compiles |
| 3 | **Sound**: the driver as music code, the queue, the loads | 2,755 (`audio`) | the driver runs on the audio thread from commands; sample sets and songs load by message | the recorded port commands fed to the driver give the recorded voice events (key-ons, pitch, volumes, per tick; SRFX's `music_check`); then `tools/nshvc-check` sound against bsnes on the attract play. Expected: the same events, with differences only in write order inside a tick |
| 4 | **Loop, video, boot to title** (the first end-to-end) | about 13,700: `system` 1,956, `video` 10,673, `game/logo_screen.c` 119, `opening_story.c` 502, `title_task.c` 473; plus the actor loop and the kinds the story places (counted in this stage's first step, from `OpeningActorPlace`'s table) | the game runs from power-on through the logo and the opening story (or Start) to the title with its music, the finder reporting nothing in the tree | headless runs with `--shots` and `--sound`, looked at against the original's recording; `nshvc-check` sound on the boot. Expected: the same screens and music, reached sooner (section 1 item 3) |
| 5 | **Player, actors and levels** | about 67,500: `actor` 47,012, `player` 7,261, `level` 3,075, `map` 2,471, the rest of `game` 7,699 | every level, scene, the Genie screen, continue, password and ending playable by input; every live routine converted or listed dead | for each level, a new input-only play of the original (section 1 item 7); the native game's state against the recording at each frame through stage 2's CSV, each masked field with its reason; no crash or unconverted-routine message |
| 6 | **Input and saves** | about 324 (`system/joypad.c` 51, `controls.c` 68, `input_modes.c` 177, all counted in stage 4's `system` too, and `input/` 28) | keys and controllers through the game's own controls on `nshvc_input`; the player's settings (bindings, full screen) in an `nshvc_save_*` file; passwords as the original's progress | headless `--input` runs with keys and with a controller replayed; password entry by keys |
| 7 | **Linux packaging** | 0 | `tools/nshvc-build linux native/aladdin` gives the program, its icon and desktop file (`cmake/NshvcGame.cmake`), `cart.dat` beside it | launched from the package on this machine, windowed and full screen (F11, API section 5) |
| 8 | **Whole game** | | section 8 | the finder at 0 and the rest of section 8 checked over the whole tree; every play through `nshvc-check` |

Order: 0, 1, 2, then 3 and 4 side by side (they share no files), then 5 with 6 beside it, then 7 and 8.

**Time.** No estimate here: Aladdin has no measured stage yet. What SRFX records: its plan was written 2026-11-09 "from
reading the source before anything was built"; its history has the empty game at 11:52 that day (`48f4d347`), the replay of
the original's picture at 12:19 (`683c4c38`), its sound drivers against the original's voice events at 12:44 (`d5f7be36`)
and its title screen at 13:12 (`a18331ad`), with 265 commits on 2026-11-09 from several sessions at once; on 2026-11-14 its
plan says the game plays and is not finished. SRFX started from earlier branches and newer C it could reuse (its section 2);
Aladdin has none, so those times do not carry over.

## 5. Sessions after boot to title

After stage 4 lands, these sessions work at once, each owning only its directories (both under `native/aladdin/src`):

| Session | Owns | Lines of the record | Starts from |
|---|---|---:|---|
| actors A | `actor/` core: loop, pool, spawning, motion, boxes, the shared kind helpers (`actor_*.c`, `pool.c`, `spawn*.c`, `motion.c`, `hit_*.c` ...) | the core's files, counted when the split is made | stage 4's actor loop |
| actors B, C | `actor/kind*.c` by kind number, split by file list in the brief (each its own files) | the rest of `actor`, 47,012 in all with A | A's `actor.h` |
| player and map | `player/`, `map/` | 9,732 | `actor.h`, `state.h` |
| levels and screens | `level/`, `game/` but the stage 4 screens | 10,774 | stage 4's screens |
| video | `video/` (the window effects, parallax and HUD the levels add) | inside stage 4's 10,673 | stage 4 |
| input and saves | `input/`, `system/controls.c`, `joypad.c`, `input_modes.c`, `save/` | about 324 | stage 4's loop |

`state.h` is the one shared file: small exact edits only. A routine another session owns is asked for under `/* wanted */`
in its header. Sound (stage 3) is finished before this and owns `audio/` alone.

## 6. Recording and comparing

Built in stage 1, as SRFX's section 6, with `snesbox` (`tools/aladdin/snesbox`, the patched bsnes core `setup.sh` stages)
in place of `sfxbox`:

- **Recorded** from the original on input-only plays: per frame, the per-line picture state, the palette, the sprite table
  and video-memory changes, and the bsnes picture; every DSP write with its sample time, and the port commands; the work
  RAM at each frame.
- **Replay** (`native/checks/replay`): a small program on the engine that makes, from one recording, the API calls a
  converted game would. It tests the mapping and the engine with no game logic.
- **State comparison**: a CSV of `g`'s fields and their original addresses (stage 2); a check-only tool compares the native
  state with the recording at each frame, field by field. Addresses live only in that CSV.
- Recordings find bugs. A difference is settled from evidence: the conversion or the engine. If it is the engine, the
  conversion stops and the session that owns the engine is told; nativeshvc is never changed for one game.

## 7. Runs

Every run is asked for first, with its expected length; builds that only compile are not. Builds and runs go through
the machine's shared CPU and memory queue, claiming the slots and memory they use. The lengths SRFX estimated for its comparable runs (its section
7): recording all input-only plays about 10 minutes; replay against bsnes about 5; the driver's voice events about 2;
`nshvc-check` boot and title about 2.

## 8. Done means

All of these, for `native/aladdin`:

1. `tools/nshvc-find-hardware` reports nothing.
2. No `cpu.`, `abi_`, `spc.`, flag helpers, `rd8`/`wr8`, register-named carried values, `goto` over translated assembly, task
   stacks or SNES addresses in the game's C, under any name.
3. Every function has real parameters and returns and structured control flow; every wait is a state (decision 1).
4. No platform code; it builds through `nshvc_game()` and `tools/nshvc-build`.
5. Every live routine of the record has a converted form, or is listed dead with its proof.
6. Each stage's check has passed, and the final report says which plays match and which do not, and why.

## 9. What gets deleted

Never brought over: `machine.h`, `hw.h`, `ops65.h`, `cpu_ops.h`, `native.h`, `spc.h`, `spc_ops.h`, `wram*.h`, every `abi_`
wrapper, the interrupt vectors and handlers as such, the task stacks, the IPL upload. Kept: `src/aladdin`, `asm/aladdin`,
`config/aladdin`, `tools/aladdin` including `snesbox`, as the record and the reference.

## 10. What is not known

- **Whether SRFX's decision 5 holds here** ("Enhanced IS the native game", Jeff 2026-11-09: a wider window, pictures between
  updates and the rest). This plan keeps the original's 256 x 224 picture until Jeff says.
- **What persists.** The original keeps its options in work RAM, sealed with a sum (`KeptOptionsSeal`, `game/options_*.c`),
  and progress only as passwords. Whether the native game saves options or progress across launches is Jeff's call.
- **The window-effect register table's bank.** Its bytes make sense in bank $80 only; the routines' comments say DB = $82
  (hardware-touches.md section 3). A recording in stage 1 settles it.
- **The IRQ's line on every level**: `V_1C7D` or 8, unless the V counter latched in the NMI reads 5 to $7F
  (`system/interrupts.c:319-330`). Stage 1's recordings show which lines occur.
- **Which actor kinds the opening story places**, and so how much of `actor/` stage 4 needs.
- **The echo buffer.** The driver sets `ESA` and `EDL` per song (`sound_driver.c:1623-1626`); the engine takes only the
  delay. If a song's echo buffer overlaps its own data in sound RAM, the original's sound depends on that overlap and the
  native's will not. Stage 3's voice comparison shows it.
- **Slowdown.** If the gameplay frame (`FrameStep`, 81:8311) ever takes longer than a frame, the original slows there and the
  native game will not. Stage 1's recordings show whether frames are dropped.
- **The data's meaning.** `assets.csv` divides the ROM by whether the traces read it (1,047,228 B) or not (173,223 B), and
  names ranges by address (`data_8080ED`); what each table is, and its typed reader, is stage 2's work.
- **Computed addresses**: whether each of the 221 lines and the 141 record pointers resolves to a typed record or table by
  reading. Any that does not is reported.
- **Whether Dozen draws every capability the game uses correctly** (section 1 item 6).
