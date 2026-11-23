# The level select screen

**How it meets Jeff's direction** ("a level select screen accessible from the main menu, which goes to a screen in a similar
aesthetic to the new save menu screen, where a player can go right to any level they've beaten before (grid of 4, pageable)
with 1x1 screenshots of that level (cropped close so graphic matches)"): the title's LEVEL SELECT opens a screen of the levels the
player has beaten, 4 to a page (2 rows of 2) on the save screen's own grid part, each cell a crop of that level at one game pixel to
one menu pixel and the level's number; choosing one starts it with a new game's counts. His answers (2026-11-23): a level started
from here starts with a new game's lives and counts; beating levels from here still unlocks the next ones; the beaten levels are kept in
one progress file, apart from the save slots.

Status: stage 1 (the parts that stand alone: the record, its file, the pictures, the start) is built and checked, section 8. Stage 2
(the screen on the save screen's grid part) is **not built**: section 6 says why and what it needs decided.

## 1. The levels, and their order

The game's progress is a sequence (the original's "part", `g.progress_sequence`) and a stage in it (`g.progress_stage`);
`progress_level(progress_first(part) + stage)` is the level. The order the game plays them is the table at `$80:B988`
(`progress_next_sequence`): part 0, 1, 2, 3, 4, **6**, 5, then 7 (the ending, whose stages are the story and the credits, not
levels). The original's own level select (`game/level_select.c`) shows part + 1 "." stage + 1 and steps up to part 6 and
`level_select_last_stage(part)` (the table at `$80:CE2E`: 3, 1, 1, 2, 2, 3, 0).

| # | the game's number | level | what the start picture shows | its exit |
|---|---|---|---|---|
| 1 | 1.1 | $00 | Agrabah market, Aladdin on the arch | spot |
| 2 | 1.2 | $01 | the ruined wall, the desert beyond | spot |
| 3 | 1.3 | $02 | the high wall, the sand | spot |
| 4 | 1.4 | $03 | the market again (`level03_scene`: he jumps in) | the scene's walk out |
| 5 | 2.1 | $04 | the Cave of Wonders, blue | the scene's landing (`PlayerSceneLand`) |
| 6 | 2.2 | $05 | the cave, by the water | its end scene (`level05_end_scene.c`) |
| 7 | 3.1 | $06 | the lava cave | spot |
| 8 | 3.2 | $07 | the lava cave, the carpet's ride | by the view (the escort exit) |
| 9 | 4.1 | $08 | the Genie's clouds ("GENIE" sign) | spot |
| 10 | 4.2 | $09 | the clouds, pink | spot |
| 11 | 4.3 | $0A | the pillars, the Genie's hands | spot |
| 12 | 5.1 | $0B | the dungeon | spot |
| 13 | 5.2 | $0C | black (the level's first picture has no scenery drawn by the two scroll layers) | spot |
| 14 | 5.3 | $0D | the dungeon, stone | spot |
| 15 | 7.1 | $12 | the palace's night sky | spot |
| 16 | 6.1 | $0E | the palace | spot |
| 17 | 6.2 | $0F | the palace's arches | spot |
| 18 | 6.3 | $10 | Jafar's first form (kind $98) | spot, after the fight |
| 19 | 6.4 | $11 | the snake, the last (kind $73) | `chain_piece.c`'s fade to the ending |

(The game's number is part + 1 . stage + 1; the third column is `g.level`. Rows 15 to 19 are in the order the game plays them:
part 6, the single stage $12, comes before part 5. That order is read from the game's table when the list is made
(`game/beaten.c` `list_make`), not written down.)

**Which entries count as levels.** All 19: each has an exit that moves the progress on, and each can be started from the title by the
original's select. Not in the list: levels $13 and $14 to $1B (no progress entry, or the ending's stages), $24 (the screen between
parts), $30, $39, $1C (story card, story, credits) and the title's $40. **No selectable level is a Mode 7 stage**: Mode 7 is built
only by the title and the Genie screen (`Mode7ScreenSetup`'s callers, `game/genie_screen.c` and the title), not by any level in the
progress. The Genie's screen (the bonus after a level) is a screen between levels, not a level. The bonus-looking levels $08 to $0A
are in the progress and are listed. This is a choice for Jeff (section 7).

**Names.** The game has no level names: its own words are `STAGE` and `AREA` (the original's select screen, text layout 6) and
the numbers it shows, part + 1 and stage + 1. The last part's title and part 5 stage 3's ("6.4") are text layouts $5E and $60,
which the cartridge image here holds as zeros (nothing in the traces reads them), so they cannot be drawn. A level is named by the
game's number, "STAGE 1-4" style (stage 2 settles the exact form with the grid's cell width): `part + 1`, "-", `stage + 1`.

## 2. What "beaten" is

A level is beaten when its exit is passed: the point where the game moves its progress on and takes the player out of it. The
original records it in no variable; the exit's own code sets `g.level_exiting`, adds to `g.stages_ended`, and moves
`g.progress_sequence` / `g.progress_stage` on. Where each level's exit ends, found by playing every level to it (section 8, the plays
and the hooks printed once in a test build), one call, `LevelBeaten()`, is placed at five places, each at the moment the exit can no
longer be undone:

| Call site | Levels (found by the plays) |
|---|---|
| `level/level_tables.c` `LevelExitCheck`, after `stages_ended++` | $00 $01 $02 $06 $07 $08 $09 $0A $0B $0C $0D $0E $0F $10 $12 |
| `player/scene.c` `walk_out_fade` | $03 |
| `player/scene.c` `land_fade` | $04 |
| `level/level05_end_scene.c`, after `stages_ended++` | $05 |
| `actor/chain_piece.c` `FC_FADE`, before the ending's story | $11 |

Each call marks `g.level` (the level being left) and reads nothing else; it changes none of the game's state (`same.sh`, section 8).
It is in the game's logic, so it marks the level however it was reached: from the title's GAME START, a continue, a level select, a
loaded save (the save screen's slots start the game at their progress and the same exits run), and the demo cannot reach an exit.

**Before anything is beaten** the screen shows no cells and one line, in the game's text tiles: `NO STAGES CLEARED YET`, with the
save screen's back control. A level that is not in the list is never marked.

**Unlocking.** A level started from the screen plays on as the game does (`g.progress_*` carry on to the next stage and part), so the
levels after it are beaten in turn and each is marked when its own exit is passed; the screen lists only levels that are marked, as
Jeff said ("any level they've beaten before").

## 3. The progress file

`levels` in the engine's save folder, beside `options` (`game/beaten.c`). Six bytes: its size (6), the format number (1), the beaten
levels as a 24-bit mask little endian (bit n: level n), and a check byte (`0xA5` xor the five bytes before it). A file of another
size (cut short or longer: the read asks for 7 bytes and wants exactly 6), another format number or a wrong check byte, or with a
bit that is not a level of the list, is left out whole: nothing is beaten, the game plays as ever, and the next beaten level writes
a fresh file. `nshvc_save_write` writes it to a name with `.new` and renames it over the file (`src/engine/files.c` `write_job`), so a
cut never leaves half a file. It is written when a level is newly marked, nothing otherwise; it is read once, at the first question.
It holds no picture (section 4).

## 4. The pictures

Choice analysed:

- **A. Taken by the game from its own picture** the first time a level is beaten or entered, stored in the file. The engine's
  only way to the picture is `nshvc_picture_read`: the last picture as the renderer drew it **at the window's size, with the overlays**,
  read back from the GPU (a stall), "NULL when this renderer keeps none" (`nativeshvc/include/nshvc/nshvc.h`). In a window or full
  screen that is not 256 x 224, so a crop could not be at one game pixel to one menu pixel without resampling, and the picture has the
  HUD, the pause overlay and the view's extra width in it. It would put pictures of the game's art in a save file, and the picture
  of a level the player reached by a loaded save would be whatever happened to be on screen. Only a headless run is exact, and only
  with no overlay.
- **B. Composed at the screen from the cartridge's level data.** The level is loaded as `LevelLoad` does (the same calls), the video
  memory writes are kept, and the play field and the backdrop are drawn from the screen map, the metatiles, the tiles and the
  palette as the load leaves them, in Mode 1's order. The developer's map dump (`system/dev_mapdump.c`) already did exactly
  this to write `LL-start.ppm`; its composition is now `video/level_picture.c`, which the dump and the game both use. No picture is
  stored: the pictures are the cartridge's own data drawn on the machine that has the cartridge (never committed, never in a save
  file), exact at any window size (the menu draws it at its own pixels, 1:1), and the same for a level reached any way.

**Chosen: B.** Costs: the load runs the level's set-up, so the screen must set itself up again afterwards and free the tasks and
actors the level's set-up started (section 6); the picture is the level's **scenery** at its start (both scroll layers), without
Aladdin, the enemies or the status line (sprites and text are not in it). It is a choice for Jeff (section 7): A would give a true
screenshot with sprites but only at the window's size.

**The crop.** The source is the 256 x 224 window of the start picture: the play field and the backdrop at the level's start scroll
(the backdrop after one `ParallaxUpdate`), from row (scroll y + 1), the picture's own one-line offset. The crop is
`LEVEL_PICTURE_W x LEVEL_PICTURE_H` = **96 x 72** pixels (`video/level_picture.h`; stage 2 sets the final size against the grid
cell), centred on where the player stands at the start (his x and y less the scroll) and moved inside the window if it would
leave it: `cx = clamp(px - 48, 0, 256 - 96)`, `cy = clamp(py - 36, 0, 224 - 72)`. Never scaled; one pixel of the picture is one
pixel of the menu. The 19 crops' origins in the window: $00 (0,136), $01 (0,132), $02 (0,100), $03 (0,136), $04 (0,100), $05 (0,76),
$06 (0,92), $07 (0,92), $08 (0,100), $09 (0,100), $0A (56,52), $0B (0,132), $0C (0,0), $0D (0,132), $0E (0,68), $0F (0,68),
$10 (0,132), $11 (0,116), $12 (0,76). The crop of the whole dump is the same bytes (checked, section 8).

Level $0C's start picture is black: both scroll layers draw nothing there at the start. Its cell would be black. Why the level's
first picture has no scenery is not analysed here. Listed in section 7.

## 5. Starting a level

`LevelSelectStart(i)` (`game/beaten.c`) sets what the title's GAME START takes with it: `NewGameCounts()` (title_task.c: the part
of the title's `new_game_counts` that the original calls "a new game's counts", moved out whole: progress 0.0, level 0, 3 lives start,
3 health marks, 10 apples, the hover, the flags and counts cleared), then `g.progress_sequence` and `g.progress_stage` of list
entry i, and `g.game_begun = 0`. The caller then goes on as the title does for GAME START: sound $4F, the fade, and
`TASK_ENTRY_GAME_START`. `GameStartTask` then takes its new game path (`NewGameState`, `LevelPlayStartTask`): `LevelFromProgress` gives
the level, `PlayStartCounts` the seed and full health. This is the game's own start path; it does not use the cheat byte or its keys,
and `level_select.c` is not changed (the cheat byte still opens it, as before). With the cheat byte set, `PlayStartCounts` gives 10
health marks, the hover and 99 apples as it does for the original's select; without it, a new game's 3 lives and 3 health (the
screen's start has no cheat). The original's select reached GAME START with `g.game_begun` 0 as well: it is 0 whenever the title is
up (boot, and the continue screen's "no" clears it).

## 6. Stage 2 is not built: the picture's load cannot be done at the screen without a way to put the game back

The save screen's grid (`pause/slot_grid.[ch]`, on the save branch, 96 x 72 pictures as 0xAARRGGBB, two caption lines) is an **overlay** drawn
over the game, which is held while it is open: the title's tasks do not run, so the title's graphics stay as they are
(`pause/save_screen.c`). `LevelPicture` cannot run while the title is held: it loads a level, which overwrites `g` (the tasks,
the actors, the palette, the progress), the video memory and the engine's tiles and maps (`write_words`) and the level fill's maps
(`video/level_maps.c`'s pool). Back at the title nothing would put those back: the title's picture is built once, in the opening
story's last scene (`game/opening_story.c`), and the game has no snapshot to restore (`system/snapshot` on the save branch has
nothing behind it). The ways found:

1. **Compose once at start-up**, in `init()` after `video_init`, then `PowerOn()` again for `g` and the tasks. The engine's tiles, maps and
   fill pool keep the last level's contents, not what a power-on has; only `same.sh`'s traces (video fields, not pixels) could be run
   here, and this Mac's headless build writes no pictures (the null renderer), so a leftover on a boot screen could not be seen.
2. **Compose when the save branch's snapshot exists**: snapshot, compose, restore. It needs the runahead session's mechanism.
3. **Keep the video memory (a 64 KB copy) while a level is played and compose from the live state** at the level's first update (no load at the
   screen, `g` and the engine untouched), storing the crop in the progress file. Needs `DevVramShadow` on during every level load, and
   the pictures of levels first entered by a loaded game would be missing.

No one of these is settled by analysis; this is for Jeff's direction (section 7, item 9). What the screen does once the pictures exist:
the cells are `SlotCell`s (`line1` "STAGE 1-2", `line2` empty, `used` 1, the picture), four to a page on the grid's 2 x 2, pages as the
grid has them; choosing a cell is `SLOT_CHOSEN`, whose index maps through `BeatenFind` to a list entry for `LevelSelectStart`, followed by the
title's chosen sequence (the sound, the fade, `TASK_ENTRY_GAME_START`).

## 7. Choices for Jeff to change

1. **Which entries are levels:** all 19 progress entries (parts 0 to 6), including the bonus-looking $08-$0A, the lava carpet ride $07, the
   scene levels $03 and $04, and both boss fights; none of the screens between them. The ending's stages are not.
2. **The names:** "STAGE n-m" from the game's own part and stage numbers (the game has no names for levels).
3. **The picture:** composed scenery of the level's start (no Aladdin, no enemies, no status line), not a true screenshot; and the
   level where it is black ($0C).
4. **The crop:** 96 x 72 pixels, centred on where Aladdin starts, never scaled (set against the grid's cell in stage 2).
5. **What shows first:** "NO STAGES CLEARED YET" until a level is beaten.
6. **Order:** the game's own play order (part 6, the single stage $12, before part 5).
7. **Beaten:** the level's exit passed; the two boss levels are marked at their exits (level $11's at the fade into the ending).
8. A level beaten from a loaded save or a continue counts, as from the screen.
9. **How the pictures are made without disturbing the title** (section 6): at start-up and reset, from the snapshot when it exists, or taken
   from the live game: which one, or another.

## 8. As built, stage 1 (2026-11-23)

Files: `game/beaten.c`/`.h` (the list, the record and the file, `LevelBeaten`, `LevelSelectStart`), `video/level_picture.c`/`.h` (the
picture, moved out of `system/dev_mapdump.c`, which uses it), the five `LevelBeaten()` calls above, `title_task.c`'s
`NewGameCounts` split out of `new_game_counts` (the same assignments; `LevelNumberText` now follows them), and the developer's
`ALADDIN_DEV_START_LEVEL=N` (the title's menu starts level N by `LevelSelectStart` at its first update) and `ALADDIN_DEV_BEATEN=1`
(`system/dev_start.c`: the levels the file gives, printed at the start).

Checks (macOS headless build, builds in `build/aladdin/native/base` = 71ea8d0 and `new` = this work; every run under `perl alarm`):

- **`same.sh` base (71ea8d0) against new, 256x224: 21 runs, 0 differ** (the boot run of 11,000 updates and the 20 plays, the player's line, every live actor with its own bytes and the video fields, update by update). The record writes a file and changes no game state.
- **A play to a level's end marks that level and only that level, across a restart.** Each `native/checks/plays/pPART-STAGE.txt` run to
  its last key + 600 with `ALADDIN_DEV_CHEAT=1` on an empty save folder: the file after it holds exactly the level the play selected for
  p0-0 to p5-1 and p6-0 (17 plays: $00 to $0F and $12, one mask bit each). p5-2 and p5-3 (the two boss fights) are not won in full by
  keys (Jeff's decision, CHECK.md); with their boss-flow patch (`ALADDIN_DEV_OWN`, one real stomp) each marks its own level: level 16
  ($10) and level 17 ($11) alone. `boss16-2hits` alone reaches no exit and marks none. Restart: p1-1 then p0-2 on one folder, a new
  run between them with `ALADDIN_DEV_BEATEN=1`: `05`, then `02 05`.
- **A bad file is ignored whole.** Each of: cut short (5 bytes), empty, one byte too long, format number 2, size byte 7, a wrong check byte,
  a bit above the levels, a level bit with a right check that is not in the list ($13): `beaten:` with nothing. A right file gives
  its levels.
- **A level started by `LevelSelectStart` gives the same trace as the original's select.** The traces (the player's line, every live
  actor with its own bytes, the video fields, `ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1`) from the level's first
  update (the first with the level's number and health), 600 updates, `ALADDIN_DEV_CHEAT=1` on both sides, for the levels $00, $04, $07,
  $0A, $0B, $0C, $0E, $10 (boss), $11 (boss) and $12: **0 differ in all 10**. The original's came at update 3933 (the select by keys,
  `select_input.py`), the new one's at 2656 (the menu's first update). There is no Mode 7 stage among the levels (section 1). Without the
  cheat byte the new start's first line: level $11 with `health=3 lives=3`.
- **The picture is the dump's.** `ALADDIN_DEV_MAPDUMP` for all 19 levels writes the same files as before the move (every one of the
  `LL-*` files byte for byte), and its `LL-picture.ppm` (`LevelPicture`) differs in 0 of 6,912 pixels from the same crop of the dump's
  own `LL-start.ppm`, for each of the 19.
