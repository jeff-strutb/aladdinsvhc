# Save games: the slot screen, in game and from the title

Meets Jeff's direction (2026-11-23, "add save game support ... a pageable grid of 6 save slots (3 in a row on each page), using
screenshots of the game (1x1 cropped close so graphic matches) where the save happened ...") by one reusable grid screen,
drawn in the pause menu's own manner, holding a full snapshot of the exact moment per slot. **Status: built except the snapshot (section 5), which waits for the runahead session's mechanism; see "As built" at the end.**

PLAN.md decision 4 ("progress is passwords") is amended by this direction, which is recorded there as his, quoted.

## 1. Screen (game pixels; the panel is the pause menu's 352 x 282, laid out by `menu_art_layout`)

- Title line (2x text, pink) "SAVE GAME" (from the pause menu) or "SAVED GAMES" (from the title); page line "PAGE 1 / 4".
- 2 rows of 3 cells. A cell is a **96 x 72** picture (the crop, 1 game pixel to 1 panel pixel, so the panel's own scale
  keeps it whole-number sharp) in a frame in the menu's gold, with a caption under it, two 8 x 8 text lines (24 px):
  `LEVEL NAME` is not on the cartridge as a string, so line 1 is `LEVEL nn` (the game's level number) and line 2
  `TIME h:mm:ss` (`game/play_time.c`'s count). An empty slot shows the frame with `EMPTY` and no picture.
- Cell pitch 112 x 112: columns at x = 16, 128, 240 (gaps of 16); rows at y = 36 and 148 (picture 72 + caption 24 + 16 gap);
  a hint line at y = 262 ("B BACK   L R PAGE" or the prompt). Total height 270, inside 282.
- Input (the menus' own, never the bindings; see PAUSE.md): the arrows, d-pad or stick move among cells; Left at column 0 or Right at column 2 turns the page (wrapping); PgUp and PgDn or LB and RB turn it too; Enter or the controller's Start chooses and answers a prompt; Escape or the controller's Back goes back to where it was opened from (or puts the prompt away), in the game and from the title alike. The hint line names these as the device in use does (Jeff, 2026-11-23). the pointer chooses what it is on and presses it; a click on the page line's arrows turns the page.
- The prompts take the cell's place of the hint line and read as QUIT's: first press on a used slot shows `OVERWRITE?` / `LOAD?`
  choices; the choice chosen asks once more (`REALLY OVERWRITE`, `REALLY LOAD`), moving off or Back undoes the ask.
  In game: an empty slot saves at once (no ask); a used slot offers OVERWRITE and LOAD. From the title: used slot offers LOAD
  only, with the one confirm; an empty slot does nothing.
- Pages: **4 pages, 24 slots** (a choice, section 7).
- Reusable part `pause/slot_grid.[ch]` (for level select): `SlotGrid` of cells `{ uint32_t *picture (96x72 RGBA or NULL), char
  line1[16], line2[16], int used }`, pages, selection, `slot_grid_update(in)` returning an event (none, chosen cell, back,
  page turned), `slot_grid_draw`, and `slot_grid_confirm(grid, text)` for the ask. It owns no save logic.

## 2. The screenshot
The only pixel read-back is `nshvc_picture_read`: the last finished picture at the window's size, with the overlays, a GPU
read-back (a stall; once per menu opening is fine). The game has no renderer of its own (the engine draws from the objects the
game sets). So the picture is read in the update in which the pause menu opens, before its first `draw` sets the overlays: it is
then the game's, with no menu over it. The game's rectangle in the window is found through `nshvc_window_to_picture` (the
window pixel at the middle of each game column and row, nearest). That is a pixel for pixel copy headless and at whole-number
window scales, and a nearest resample at others (Jeff decided to read it, 2026-11-23). The crop is kept in the slot, so what
the slot shows is the frame the player paused on, the state the snapshot holds (the menu holds the game).

## 3. Crop rule
Centred on the player: his feet's place in the picture (`g.actors[0]` less the camera) lifted 24 pixels (`SAVE_CROP_LIFT`, so his body is the centre), clamped so the 96 x 72 window lies inside the
picture (256 x 224, or the wider view). At the pause menu's open the picture is full; on a screen with no player (title
tasks, a story) the centre of the picture.

## 4. File format (one file per slot, `save_NN`, the engine's save folder; written through `nshvc_save_write`)
Little endian: magic `ASVG`, format number (1), total size, state layout mark (a hash of the snapshot's enumerated state
list: names and sizes, from the snapshot mechanism), crop 96 x 72 RGBA, caption values (level, play time frames, update
count), snapshot length, snapshot, a hash of everything before it. Refused whole (shown as empty-and-unreadable, never loaded)
when the size, magic, format, layout mark or hash differ or the file is cut short. The engine writes `NAME.new` then
`remove` + `rename` on its writer thread: a failed open of the temporary leaves the old slot; but `remove` then `rename` is not
atomic, and `nshvc_save_write` reports nothing back, so "a failed write leaves the old slot intact" holds for open failures
only; a crash between the two loses the old slot. An engine request (atomic rename, a result) would close this.
Expected size: 27.7 KB + snapshot (measured at build; `g` plus the tile, map and palette parts).

## 5. The snapshot
From the runahead session's mechanism (`system/snapshot.[ch]`, FRAMERATE.md section 10; merged here): one buffer of the whole
state, with its own magic, format, layout mark (made from the sizes and registered lists, no hand-kept number) and length.
Called from an update's start (the pause menu's, the title's, the developer's driver) after `runahead_begin`; a load is the last
thing its update does and the next update is the one after the save. `snapshot_max()` changes with the level, so the file reader
does not use it (a fixed 6 MB read cap; the header's size and the hash decide). The song resumes at its place, a note that was
sounding is silent until the next (the engine cannot read a voice back).

## 6. Sound and load
Saved with the world: the sound queue and the music's state (playing track, position) so the music resumes at the same note.
Load from the title: the title's tasks end cleanly (each ends its own task state, as the game's start does), the music
stopped, then the snapshot is put in and the next update is the one after the save. In game: same, at once.
Timings: a save is the read-back (about 5 ms) plus a memcpy and one async file write; a load a read of the file; both far
under a 16.7 ms update.

## 7. Choices for Jeff
Number of pages (4, 24 slots); crop 96 x 72; caption `LEVEL nn` over `TIME h:mm:ss`; slot names `save_00`..; OVERWRITE/LOAD
labels; wrap on page turns.

## As built (2026-11-23)

- **Files.** `pause/slot_grid.[ch]` the reusable grid (spec: cols, rows, pages, picture size, title, a cell callback; events chosen, answer, back; `slot_grid_ask` for prompts; overlays 3 to 5); `pause/save_screen.[ch]` the slots' screen; `system/saves.[ch]` and `save_crop.c` the files and the crop; `system/snapshot.[ch]` the runahead session's snapshot (merged); `system/dev_saves.c` the developer's save and load at an update. `pause/menu_art.[ch]` gained a public drawing kit; the pause menu's items are RESUME DEFAULTS / SAVE MENU FULL SCREEN / QUIT (three rows, 17 pixels apart).
- **Layout as built.** 96 x 72 pictures in 104 x 80 boxes (a 4-pixel frame), two caption lines under each (`LEVEL 1.2`: the game's own part and stage as its level number reads; `TIME 0:01:07`), hint at y 246, page line "L  PAGE 1 OF 4  R" at y 262. The font's punctuation is the original's (menu_art.c maps it), so there are no `<` `>` `/`: the shoulder letters stand for the page turn and the chosen answer is in brackets.
- **File as built.** 36-byte header (`ASVG`, format 1, size, layout mark, snapshot length, part, stage, six time digits, update count), the crop (27,648), the snapshot, a FNV-1a 64 hash of the rest. Slot files `save_00` to `save_23`. Read with one byte more than a file can hold, so a longer file is refused too.
- **Flow.** Pause menu SAVE MENU: empty slot saves at once; used slot asks OVERWRITE, LOAD or CANCEL, then `REALLY OVERWRITE SLOT n?` or `REALLY LOAD SLOT n?` with YES and NO; an unreadable slot offers the overwrite ask. Title SAVED GAMES: a used slot asks `REALLY LOAD SLOT n?`; empty does nothing. Back (Escape or the controller's Back) puts a prompt away first. A load shuts the pause menu without its cooldown and queues the music's go-on (the music was paused by the menu when the save was made).
- **The title.** The title's task asks (`save_screen_request_title`), the menu's update opens the screen from the next update, the game is held while it is open (the title's tasks do not run), and a load replaces the state from outside the tasks. "The title's tasks ended cleanly" is therefore not needed as a step: the snapshot holds the task state whole.
- **Checks run** (CHECK.md), headless build, runahead off:
  - the slot files (14 lines ok); the screens' pictures (`shots/saves-*.png`; the crop equals the picture's pixels, 0 of 6,912 differ);
  - `hold.sh` passes; `same.sh` at 256x224 with no menu opened, merged tree against main c9a5241: 21 runs, 0 differ;
  - **exact** (`native/checks/saves/exact.sh`; saved at the start of update U, loaded at the start of update V, then the same keys; the trace
    of the player, every live actor with its own bytes and the video fields compared line for line from the load on, 600 updates each):
    | A (saved) | B (loaded into) | result |
    |---|---|---|
    | p1-1 at 5000 | the title, no keys, at 2200 | same (600 updates) |
    | boss16-2hits at 8300 (Jafar's fight, level 16) | p0-1 mid-play at 4500 | same |
    | p1-1 at 5000 | p3-0 mid-play at 5000 | same |
    | the Genie screen at 500 (Mode 7 on for all 600 updates) | p1-1 mid-play at 4800 | same |
    | p1-1 at 4720 | the title, through SAVED GAMES by keys (Down, then Z three times: open, slot 1, YES) | same |
    | p1-1 at 4720, saved through the pause menu by keys (Escape, Down, Z, Z) | the title, through SAVED GAMES | same |
    | p1-1 at 5000, planted fault (the player's x left as it was before the load) | p3-0 mid-play | traces differ from the first update: caught |
  - a snapshot file is 612,086 bytes (the Genie screen) to 755,952 bytes (a level); the runahead session measured a save at 0.3 to 0.4 ms and a load at about 0.4 ms (2.9 ms in a window, where the save waits for the audio thread).
- **Held buttons.** Loading through the screens swallows the buttons held at that moment until they are let go (as shutting the pause menu does), so a load made with Z held and a game state that holds Z differ for one press; the exactness runs choose an update where this is not so.
- **Not run:** the picture read in a window (the screenshot's resample at a window scale that is not whole); the sound after a load, by ear; the exactness checks in a window (runahead on, headless, is below).
- **Engine limits found.** `nshvc_save_write` reports no failure and removes the old file before renaming the new one; a crash between the two loses the slot, and a write that fails after the temporary opened is not seen (the file is then refused whole on the next read). An atomic rename and a result code in the engine would close it.
- **Left unreached:** `game/password_screen.c` and its task entry stay in the tree, not deleted, so the task tables are not touched.

## With runahead on (2026-11-23)

**Analysis from the code, before any run.** `update` begins with `runahead_begin`, which puts the real world back when the last tick ran ahead
(`live`: `between_restore` restores `g` and the kept parts, `restore_statics` the registered statics and the undo log). Everything that saves or
loads comes after it in the same update (`DevSaves`, the pause menu's `menu_update`, the save screen), and a held update (the menu open) returns before
`runahead_after`, so no ahead step runs while the menu is open and `live` is 0. So a save, by the menu or by `ALADDIN_DEV_SAVE_AT`, always sees the
**real world** (the state after the last real step), not the ahead one. A load puts a snapshot in at such a point and ends its update without
`runahead_after`: `live` is 0, so the next `runahead_begin` restores nothing over it (the pending `g_saved`, the undo log and the kept copies are
rewritten by the next `between_save`/`save_statics`, after the next real step); the picture's tracks start at rest on the loaded world
(FRAMERATE.md 10.9). Expected: the real world's trace after a load is the same as with runahead off.

**Result.** `exact.sh` with `EXACT_RUNAHEAD=1` (A, the menu-save run and B all run ahead by 1; each A run reports 1,100 to 5,600 ticks ahead): the six cases are the same
(600 updates each) and the planted fault is caught, as with it off.

**The crop.** The picture read when the menu opens is not the saved state's: `between` shows the last ahead world eased with the real one, and the
first held update leaves it showing "was" = the ahead world, "now" = the real one. Measured with `crop_check.sh` (Metal build, headless): taken at the open
update, the crop of the saved state's picture differs by 902 of 6,912 pixels with runahead off (it was one update early: the world before the saved one)
and 0 with it on. Now the picture is taken in the second update after the menu opened, when both of `between`'s worlds are the real one, and the menu's
overlays are held back until then (two pictures, 33 ms), so the read has no menu over it: 0 of 6,912 pixels differ with runahead off and on (both
taken at update 4722 for a menu opened at 4720). `ALADDIN_DEV_SAVE_AT`'s picture (a developer's tool) is read at once and is not exact.
