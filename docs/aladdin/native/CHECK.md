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
