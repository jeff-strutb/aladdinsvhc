# The native game at main's tip: what was proven, what was not

Written 2026-11-21 at `fe93c0dc` from the writers' files (`build/aladdin/writers/NATIVE-STATUS.txt`, `NATIVE-FAULTS.txt`,
`NATIVE-ACTOR-CHECK.txt`), the session hand-overs, [DEPARTURES.md](DEPARTURES.md), [CHECK.md](CHECK.md),
[BUILD.md](BUILD.md) and `native/checks/plays/`. A claim is here only when a source says it was run. Where two sources
disagreed the later one is used. A number a source does not give is left out.

## What it is

`native/aladdin` is Aladdin (SNES) as a native C game on the nativeshvc engine (API 6.7). Each `update` is one 60 Hz frame of
the original's work; `draw` describes the picture from the game's state. The game calls the engine only through `nshvc_*`.
No processor, PPU, DMA or sound chip is imitated. The decomp in `src/aladdin` (README bars 100.0%, 92,250 of 92,250 B,
ledger `bdf23ac7`) is the record the C was written from. The original is used only to find bugs: recordings of it are
compared with the native game and are never a target. Linux (Vulkan and headless) is the platform that was built and run.

## What was proven, and how

### Levels played by keys, start to exit

The plays are `--input` files made by the developer's player (`system/dev_bot.c`, an archive search over keys). Level ids
count from 0: part-stage 0-0 to 0-3 are levels 0 to 3, 1-0 is 4, 1-1 is 5, 2-0 is 6, 2-1 is 7, 3-0 to 3-2 are 8 to 10,
4-0 to 4-2 are 11 to 13, 5-0 to 5-3 are 14 to 17, 6-0 is 18.

| Reached by keys | Levels | Count |
|---|---|---:|
| Start to the real exit | 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 18 | 16 |
| Start to the scene, STAGE 1 CLEAR and the story card | 3 | 1 |
| Start to the boss room, then the end by patched health | 16, 17 | 2 |

Level 4 ends by walking past its right edge (x 4080). Level 5 ends by the lamp scene, then STAGE CLEAR. Level 9 ends at the
Genie screen by a real level end. Level 6 needed the search with no try allowed to lose health (`GE_HURT=0`): exit at x 4585,
health 10 throughout. Level 18 reaches its exit, the bonus clear, the password screen and the ending's story cards.

The two boss levels are not won in full. Jeff decided that they are proven as flow: the boss's health is set by a patch
(`ALADDIN_DEV_OWN`) so that one real stomp by keys is the last. Three sessions could not beat either boss by keys.

| Level | Play | What the one real stomp led to (`boss-flow.sh`) |
|---|---|---|
| 16 (5-2), Jafar's first form | `boss16-2hits.txt` | Defeat 8379 to 8436, exit 8540, STAGE 6 CLEAR, level 17 loads at 9660. Run to 10500, exit 0. |
| 17 (5-3), the snake | `p5-3.txt` | Beaten at 5577, the finale, the ending story, the credits, "The End" held to 40000. Exit 0. |

Level 16's play lands two of the six stomps unpatched (hits 6 to 5 at 8380, 5 to 4 at 9752). Level 17's search landed 4 of 7.

### Plays identical to the original, update by update

Method (CHECK.md): the play is run on the original with one logic frame per native update, and `compare.py` gives the first
update where the player (x, y, health) or the actor list (kind, x, y) differs. "Identical" means every update up to the
number in the table. A death or restart ends a comparison.

| Level | Play | Last identical update | Note |
|---|---|---:|---|
| 0 | p0-0 | 6430 | the level's end; native loads level 1 at 6431 |
| 1 | p0-1 | 8133 | the original's level end |
| 2 | p0-2 | 8163 | the whole play |
| 3 | p0-3 | 7803 | the level's end |
| 4 | p1-0 | 9253 | |
| 5 | p1-1 | 9088 | 9089 differs by one update: departure 4 |
| 6 | p2-0 | 10893 | the exit, x 4585 |
| 7 | p2-1 | 6387 | |
| 8 | p3-0 | 8799 | one life, to the exit |
| 9 | p3-1 | 8403 | |
| 10 | p3-2 | 10099 | |
| 11 | p4-0 | 5877 | |
| 12 | p4-1 | 7179 | |
| 13 | p4-2 | 7407 | |
| 14 | p5-0 | 7695 | the level's end |
| 15 | p5-1 | 6092 | |
| 16 | p5-2 | 8482 | kind $97 counted as $53: departure 3 |
| 17 | p5-3 | 6469 | the play's end; the original's recording ends there |
| 18 | p6-0 | not compared | no source lists a comparison of this play |

Level 17 with the boss's health patched, from update 3936 to 7301 (the finale's jump and landing to 7476), is identical to the
original given the same patch. The level change is at native update 7477, which is the original's frame 7602.

Level 16's patched flow (`boss-flow.sh 16`) was not compared with the original: no source says so.

### The ending, compared

Two runs of the original were compared with the native game by cue and by frame. (1) The ending from the decomp state
`stage_a0_s0`, no input, 30,000 video frames. (2) The ending from level 17's real end, with the same health patch given to
the original, run to frame 28,808.

| | Result |
|---|---|
| Cues in order and counts | equal in both runs: F6 02, spin zoom start, iris create at +375, iris 64 frames, sound $1C one frame after, F6 02, F6 01 |
| Carpet scene, player x and y | equal by play-time tick for 15,350 ticks |
| Text pages 0 to 13, credits, TOTAL, "The End" | look the same side by side at 8 paired frames |
| Scene lengths | native is 10 to 52 frames shorter (departure 6); in run (2) the cues are earlier by 54, 106, 116 and 131 frames |
| First timed text page | 3 logic frames shorter (run 1); 733 against 742 video frames (run 2) |

Not compared: the credits' TOTAL with items (the state has none).

### Actor kinds, summary

The EVERY KIND table (`NATIVE-ACTOR-CHECK.txt`, landed `86f21dd9`) compares each of the 133 rows of `actor/kinds_*.c` with
the original's per-frame work-RAM dumps. The recordings are all 244 of the decomp's coverage scripts (cut into sections at each
load and level change) and the stage plays of levels 4 to 18. The check models the original's cut loops. Fields compared per
frame: every field and own byte of each record, the free lists, the palette slots and the kind counts.

The table gives no per-kind verdict count. These counts are of its rows, made when this file was written:

| Rows | Count |
|---|---:|
| Kinds in the table | 133 |
| NOT MET: no recording makes the kind live ($00, $07, $14 to $17, $64, $B9; $00 and $07 are given in the player's own sections) | 8 |
| Only "differs after the script poked a record" ($2C $33 $3A $3E $41 $50 $86 $98): nothing followable | 8 |
| A DIFFERS line "not explained by" restarts, pokes or cut frames (see below) | 22 |
| The other rows: exact sections, with at most restart, poke or cut-frame notes | 95 |

What the later files say of the table's own open list:

| Row | State at the tip |
|---|---|
| $63 hit by an apple (1882) | fixed (R3, `2d1875a4`): 4,399 fields differ before, 0 of 143,079 after |
| $7D (253, 981) | not a fault (R7): a backdrop read one frame behind, and a loop cut |
| $6D (137, 1108) | not checkable (R8): the script turns on copier RAM, no retail hardware does |
| $6E 1559, $83 1927, $81 3528 | superseded (R10): plays p2-1 and p5-3 cover the same levels; p5-3 is now identical to 6469 |
| $9E (kind9e_states 3449) | OPEN: 20 fields, one level; a pick the Genie's task sets and clears inside one frame is not in an end-of-frame dump, so the method cannot see it |
| $60 (pw4 2053) | the check's artifact, not the kind |

Not checkable by this method (the table says what each would take): kinds whose record is made only by re-kinding or poking
a live record ($44, $84's start, $5F's start, $5C's path, $22's and $87's second entries, $64), $9B after frame 5283 (the story
task's cues), $A4's ways (a unit call), $69 (no script makes one live). Kind $8B's hold of Aladdin is unchecked: no play
reaches it. Also open in the file: kind $46 with another kind in `rand_pw1_2` and $1428 on a platform in `rand_pw5_2`
(player's checks; no later source says they were closed).

Other exact results: the attract demo's window (0 of 606,824 fields differ, all registered kinds but
Aladdin's, at `2dc14405`); the ending's stage 8 (the player and kinds $27 $35 $49 $6C $A0: 476,462 fields); kinds $4C, $8B,
$8C, $92 (57,892, 40,490, 33,613 fields, 0 differ).

### Screen effects

The table is `native-pictures2.md` (landed `ed508de2`, no game fix needed) with the picture compares of `n5-video`. The
original's pictures have filtered columns; edges were read on the even column after the same gamma.

| Effect | What | Result |
|---|---|---|
| 0, 1, 5 | none | no creator in the ROM; never visible |
| 2, 7 | circle wipe, Genie screen | compared: same phases (zoom 93 to 162 against 94 to 161, middle 50 to 205 both, settled 15/16 to 239); wheel equal |
| 3, 4 | the title menu's waves and mode band | looked at only: same logo, flash and menu; not measured |
| 6 | title intro ribbon (kind $2E), window shape 82:BBAE | compared at an offset of 446 frames: left 84, top 0, bottom equal, right +1 in 12 of 12 samples; flash 64 frames on both |
| 8, 11 | level 5's end | compared: shrink 1 px a frame on both, same top; native 1 px smaller on three sides: departure 4 |
| 9 | level 18 and the credits | not compared: its window starts $0200 lines below a camera that stays at y 0, so it is never on screen |
| 10 | level 5's spot | compared: window x 120 to 136 and its height equal; tint within 1 to 3 levels |
| 12 | ending story scene 5, opening story | compared: picture box 32,23 to 223,129 and the blue area identical |
| iris | ending story end | compared: both shrink 1 px a frame, edges within 1 to 2 px (20 bbox pairs) |
| text bands | credits | compared: reveal steps and rows 65 to 197 equal, text boxes within 1 px |
| circle wipe, kind $69 (level 13) | | looked at, not measured: no circle visible on either side |

The IRQ screen of level 5 was lit one row early; the original's rows 0 to 7 are dark. Fixed in `32eeff06`.

### Passwords

All seven passwords were accepted by keys and led to parts 1 to 7: levels 4, 6, 8, 11, 14, 18 and 17. The passwords are not
compared with the original's.

## Faults found by this checking, and fixed

| Fault | Commit |
|---|---|
| The work-RAM clear at a game start missed 82 fields (stage 8 of the ending kept stage 7's scroll kind and killed the player) | `e0d1199a` |
| `RandomFromList` took X where the original uses X / 2: Jafar's first form stopped at its first pick (also $A1/$A6 and the chain's picks) | `56e4efad` |
| `PlayerSceneJumpIn` and `PlayerSceneWalkOut` were left unconverted on a wrong note: level 3's scene never ran | `c5a384cb` |
| Kind $68's timing and its overlapped byte | `22cc5db9`, `c4b4e918` |
| Kind $4E's spring left +$41 at 6, not 5 | `b4edf6c2` |
| The top speed cap's direction | `486c4ed7` |
| Kind $63 hit by an apple put the wrong record in state $8C; the apple's course read the byte at + 2 x pose; the landing boxes were read from bank $82, not $80 (the swinging platforms of kind $33); the escort's fly start (A5:EFDE) was not converted | `2d1875a4` |
| `g.sprite_flicker` was never counted (the player's every-eighth-update puffs came every update) | `a4c5b33d` |
| The escort's restart test used the wrong carry (193 pixels on the ground) | `3c4ec70e` |
| The snake's head sent its pictures without the kind's graphics offset: scrambled | `e8f2c300` |
| Kind $92's marker left the escort standing: `GuardSceneStart` converted; then `GuardSceneWalkOut` | `58ad9519`, `cdfbcc6e` |
| The ending story's scene 5 never ended: cues kept in two places | `689d74b7` |
| The ending: kind $9B's flight did not set the story task's frames left; kind $24 waited on bytes nothing wrote (no iris, no sound $1C); the iris took the wrong parameter | `818bb9be` |
| Chain: `ChainPieceEnd` ran a wait routine the original skips (one path step too many) | `68e971c1` |
| `PlayerSceneCued` (81:E154) was unconverted: the fight-won scene of level 17; its `g.play_wait` | `b9ed8649` |
| Chain: a slide's step was read one byte early (the head and pieces moved by garbage in the fight-won scene) | `fe93c0dc` |
| Level 5's IRQ screen lit one row too early | `32eeff06` |

Not faults, found on the way: the "ACTION" text (the original's own blinking task); the garbage after level 5's STAGE CLEAR (the
developer's start leaking into level $24); a wrong-way ledge (departure 1); the title's black after START (departure 5).

## Departures 1 to 6

Each is an accident of the original that the native game leaves out (Jeff's rule, 2026-11-20: not reproduced unless leaving it
out hurts gameplay). Detail and evidence: [DEPARTURES.md](DEPARTURES.md).

| # | Departure | Kind |
|---|---|---|
| 1 | A ledge that faces the wrong way no longer clears the fraction of his horizontal speed (no play reaches it) | game logic |
| 2 | Kind $47's course maker no longer stalls the game for 85 frames at level 5's start | game logic |
| 3 | A carrier-pair sender keeps its own kind (the original turns it into $53) | game logic |
| 4 | Starting a song costs the starting task one video frame in the original (SongPlay's upload) | console speed |
| 5 | Unpacking a screen's graphics takes the original's task about a hundred video frames; native unpacks at once | console speed |
| 6 | The ending's scenes run 10 to 52 frames shorter native (the original's scene loads and lag frames) | console speed |

Departures 4, 5 and 6 are the console's speed, not game logic. Departure 4 was measured on p3-0, p1-1, p0-0 and p2-0.

## What is NOT done or not proven

- **Windows** was cross-built (mingw-w64, Vulkan, clean) and never run: no Windows machine, no Wine. **macOS** is not built.
  Linux was built fresh and booted headless for 3200 updates, exit 0, but not run in a window on that commit (BUILD.md, `a92d22da`).
  Nothing is packaged. There is no CI.
- **The sound** was never judged by ear. The driver was checked sample by sample (41 runs, 330,680 ticks, 0 differ); the mixed
  sound is not sample-exact.
- **The bosses** (levels 16 and 17) were not won in full: patched health and one real stomp each.
- **Level 16** has no play to its exit by keys. **Level 17** has none either. The ending was reached from level 17's end only
  after the patch.
- **Effects 3 and 4** were only looked at. Kind $69's circle was looked at, not measured. Effect 9 has no picture to compare.
- **The compare tool cannot cross a restart or a continue screen** (a restart-aware compare is not built). Plays end their
  comparison at a death.
- **Level 18's play** (p6-0) has no listed comparison with the original. Level 16's patched flow has none either.
- **Open rows of the EVERY KIND table:** $9E (20 fields), the 8 not-met kinds, the not-checkable kinds above, and kinds $46 and
  $1428 in the player's checks.
- **Departure 4's other song starts** (the level's exit, kinds $81, $8F, $94, $6B, the chain's end song, the level scenes)
  follow from the code and are not measured, except the places the departure names as seen.
- **Passwords** are not compared with the original's.
- **The story card** copies code as graphics only with an act past 7 in one poked run (README, older); whether real play reaches
  it is open in the plan. No later source settles it.
- **The enhanced game** (widescreen, higher frame rates) is not designed.

## How to rerun each check

Run from the repository root. Every command is queued: `ALADDIN_CPU_RUN` set to the machine's CPU queue command and `cpu-run -j N -m SIZE --`.
The native binary has no `--help`: an unknown flag runs the game uncapped. Run it only as `--headless --updates N`.

| Check | Command |
|---|---|
| Build (Linux, headless) | the two `cmake -S native/aladdin ...` lines in [BUILD.md](BUILD.md) |
| Clean check | `python3 native/aladdin/tools/clean_check.py native/aladdin/src` (350 files, 0 findings at this commit) |
| Data check | `python3 native/aladdin/tools/data_check.py "reference/aladdin/Aladdin (USA).sfc"` |
| Boot, 11,000 updates | `cd build/aladdin/native/headless && ./aladdin --headless --updates 11000` (exit 0) |
| Replay a play | `native/checks/plays/run.sh native/checks/plays/p0-0.txt 6500` (add `DIR EVERY FROM` for pictures from the Linux build) |
| The two boss flows | `native/checks/plays/boss-flow.sh 16` and `native/checks/plays/boss-flow.sh 17` |
| A play on the original, update by update | `native/checks/plays/lbox.sh DIR`, then `LH_FROM=3936 LH_DUMP=8192 python3 native/checks/plays/to_box.py PLAY.txt OUT.box LAST 1 3900`, run it in `DIR/lbox`, then `python3 native/checks/plays/compare.py ORIGDIR nat.txt 3936 LAST 2` (CHECK.md) |
| Native side of that | `ALADDIN_DEV_ACTORS=1 ALADDIN_DEV_TRACE=1 ./aladdin --headless --updates N --input PLAY.txt > nat.txt` |
| Make a play | `native/checks/plays/make.sh PART STAGE LAST` with `ALADDIN_DEV_BOT_GE=10` (CHECK.md, `ways.txt`) |
| Record a person's play | `ALADDIN_DEV_RECORD=FILE` |
| A screen by a developer's start | `ALADDIN_DEV_SCREEN=genie|continue|level-end|story-card|ending-story|ending|credits` (CHECK.md) |
| Actor check of a kind | build `native/checks/actors` (cmake), then `native/checks/actor_check_kinds.sh DIR FIRST LAST KIND[,KIND...]` (CHECK.md "Checking every actor kind") |
| Sound driver | `native/aladdin/tools/check.sh attract` (CHECK.md) |
| Pictures | `native/checks/pictures/winbox.py` (its README) |
| Windows cross-build | the mingw-w64 lines in BUILD.md |

The CHECK.md table of the two boss flows was written before `b9ed8649` and `fe93c0dc`; its ending-story times (8740 and later)
are older than the 7477 given above.
