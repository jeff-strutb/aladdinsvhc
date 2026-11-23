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
- Pad: directions move among cells; Left at column 0 or Right at column 2 turns the page (wrapping); L and R turn it too;
  B or A chooses; Start, Escape or the Back item go back to where it was opened from. Keys as the pause menu's bindings;
  the pointer chooses what it is on and presses it; a click on the page line's arrows turns the page.
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
It must come from the runahead session's mechanism. In its worktree (uncommitted, `system/runahead.[ch]`) it is an in-memory
undo (`RUNAHEAD_KEEP` statics, a write log, `between_save`), not "save to a buffer / restore from a buffer / layout mark"; it
covers one ahead step only ("the engine's art written by a second step would have to be put back too: not built"), and does not
yet include the sound or engine tiles/maps/palettes as a whole. I do not write a second list of state. The slot code takes
`snapshot_save(buf*, len*)`, `snapshot_load(buf, len)`, `snapshot_mark()` behind `system/snapshot.h`; those wait for runahead.
Pointers or engine handles in it cannot go to a file as they are; to be named once the code is committed.

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

- **Files.** `pause/slot_grid.[ch]` the reusable grid (spec: cols, rows, pages, picture size, title, a cell callback; events chosen, answer, back; `slot_grid_ask` for prompts; overlays 3 to 5); `pause/save_screen.[ch]` the slots' screen; `system/saves.[ch]` and `save_crop.c` the files and the crop; `system/snapshot.[ch]` the interface, **with nothing behind it** (`snapshot_available` is 0). `pause/menu_art.[ch]` gained a public drawing kit; the pause menu's items are RESUME DEFAULTS / SAVE MENU FULL SCREEN / QUIT (three rows, 17 pixels apart).
- **Layout as built.** 96 x 72 pictures in 104 x 80 boxes (a 4-pixel frame), two caption lines under each (`LEVEL 1.2`: the game's own part and stage as its level number reads; `TIME 0:01:07`), hint at y 246, page line "L  PAGE 1 OF 4  R" at y 262. The font's punctuation is the original's (menu_art.c maps it), so there are no `<` `>` `/`: the shoulder letters stand for the page turn and the chosen answer is in brackets.
- **File as built.** 36-byte header (`ASVG`, format 1, size, layout mark, snapshot length, part, stage, six time digits, update count), the crop (27,648), the snapshot, a FNV-1a 64 hash of the rest. Slot files `save_00` to `save_23`. Read with one byte more than a file can hold, so a longer file is refused too.
- **Flow.** Pause menu SAVE MENU: empty slot saves at once; used slot asks OVERWRITE, LOAD or CANCEL, then `REALLY OVERWRITE SLOT n?` or `REALLY LOAD SLOT n?` with YES and NO; an unreadable slot offers the overwrite ask. Title SAVED GAMES: a used slot asks `REALLY LOAD SLOT n?`; empty does nothing. Back (Start or Escape) puts a prompt away first. A load shuts the pause menu without its cooldown and queues the music's go-on; the pause menu's open sound and music pause were in the snapshot's moment only if the save was made with the menu open (the usual case): to settle when the snapshot exists.
- **The title.** The title's task asks (`save_screen_request_title`), the menu's update opens the screen from the next update, the game is held while it is open (the title's tasks do not run), and a load replaces the state from outside the tasks. "The title's tasks ended cleanly" is therefore not needed as a step: the snapshot holds the task state whole.
- **Checks run** (CHECK.md): the slot files (14 lines ok), the screens' pictures (`shots/saves-*.png`: empty page, used page, overwrite or load prompt, both confirms, from the game and the title; the crop equals the picture's pixels, 0 of 6,912 differ), `hold.sh` passes, `same.sh` at 256x224 with no menu opened: 21 runs, 0 differ (base 71ea8d0).
- **Not run, because the snapshot does not exist:** save and load exact on three plays, the planted fault, the load from the title and mid-play, the real timings and file size (a snapshot's length). The picture read in a window is not seen.
- **Engine limits found.** `nshvc_save_write` reports no failure and removes the old file before renaming the new one; a crash between the two loses the slot, and a write that fails after the temporary opened is not seen (the file is then refused whole on the next read). An atomic rename and a result code in the engine would close it.
- **Left unreached:** `game/password_screen.c` and its task entry stay in the tree, not deleted, so the task tables are not touched.
