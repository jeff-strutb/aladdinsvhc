# Save games: the slot screen, in game and from the title

Meets Jeff's direction (2026-11-23, "add save game support ... a pageable grid of 6 save slots (3 in a row on each page), using
screenshots of the game (1x1 cropped close so graphic matches) where the save happened ...") by one reusable grid screen,
drawn in the pause menu's own manner, holding a full snapshot of the exact moment per slot. **Status: design only. Two
things below (sections 2 and 5) block the build and need Jeff's or the orchestrator's decision.**

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

## 2. The screenshot: BLOCKER

The only API that returns pixels is `nshvc_picture_read`: the last finished picture **at the window's size, with the overlays**,
a GPU read-back (a stall, once per save is fine). So the game's own 256 x 224 picture, without the menu over it, does not exist
as an array. Options:
- (a) take it in the update in which the menu opens (before its first `draw` sets the overlays): the picture then is the
  game's, at window size; find the game's rectangle with `nshvc_window_to_picture`, and sample nearest. Exact (a pure pixel
  for pixel copy) only at a whole-number scale; headless it is exactly 256 x 224. In a window at another scale it is a resample,
  not "1:1". Also the picture is the eased one (a blend of two updates) at that moment.
- (b) an engine request (`ENGINE_REQUESTS.md`): "the picture at the screen's own size, without overlays" (`nshvc_picture_read_screen`).
  This is what gives Jeff's "graphic matches" in every window. **Recommended**; (a) as the stopgap.
The save keeps the crop (96 x 72 x 4 bytes = 27,648 bytes) taken in the menu-opening update, so what the slot shows is the
frame the player paused on, which is the state the snapshot holds (the menu holds the game).

## 3. Crop rule
Centred on the player's screen position (`g.actors[0]` less the camera), clamped so the 96 x 72 window lies inside the
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

## 5. The snapshot: BLOCKER
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
