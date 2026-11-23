# The pause menu, controller mapping, full screen and quit

Jeff, 2026-11-22 and 2026-11-23 ("it should open from start. The original pause should be deprecated."): the pause menu and controller mapping of Stunt Race FX in Aladdin, in Aladdin's own look, with the same
controller picture; full screen by default with a window available; a way to quit. Start and Escape open it, in place of the
original's pause.

## What it does

- **Opening.** Start during play, where the original paused (`game/pause.c`, `PauseCheck`: full brightness, no demo, nothing
  blocking, the cooldown run out), and Escape on any screen. Start or Escape shuts it. The original's pause is retired: `PauseCheck` no longer freezes tasks or shows PAUSED (the `PAUSED` text layout, the frozen task states and the pause mode are not used). `PauseCheck` keeps the original's tests
  and asks the menu to open (`menu_request`) in place of freezing the tasks and showing PAUSED.
- **Holding the game.** While the menu is open `update` does not step the game (`system/main.c`: `menu_update` returns 1), so
  the tasks, the frame counts and the picture stand where they were, as the original's frozen tasks left them. The sound goes
  on: the menu queues the original's pause sound ($0E, not on level $12) and the music pause ($F3), and sends the queue itself
  each held update (`SoundCommandSend`); it queues $F4 and sets the original's cooldown ($10) when it shuts. Buttons held as it
  shuts reach the game only once let go (`controls_swallow`). With no menu opened the game is exactly as before.
- **Items.** The twelve pad buttons, each tag naming what the button does in the game (JUMP, DASH, THROW, HOVER as the options
  screen gives them, LOOK UP, CROUCH, GO LEFT, GO RIGHT, MENU for Start, UNUSED) over the key bound to it (the controller button when a controller was the last
  device used), a line from each tag to its button on the controller; RESUME; DEFAULTS (the game's own keys and buttons back); SAVE MENU (the save slots' screen, SAVES.md); FULL SCREEN ON or OFF (not offered where the window
  cannot switch); QUIT. The items lie RESUME DEFAULTS / SAVE MENU FULL SCREEN / QUIT.
- **Remapping.** Pad 1 only: the game reads one pad and pad 2's buttons do nothing in it. A button chosen (B or A, or a click)
  asks for the next key, or the next controller button on the first controller; Escape or a click elsewhere cancels. A key or
  button another had goes to the button whose own was taken, so each keeps one of each. Escape, Tab, F11 and F12 cannot be
  taken. Right Shift is also Select until the key is given to another button.
- **Input.** The pad's directions move among the items (tags in two columns, then RESUME and DEFAULTS, FULL SCREEN and QUIT),
  B or A chooses, a click of the pointer chooses and presses what it is on (a tag, its button on the controller, an item).
- **Full screen.** Full screen until the player chooses a window, then as they left it: the choice is saved, and a switch made
  by the system's own keys (the View menu, F11, Alt+Enter) is saved too (`system/screen_mode.c`, as Stunt Race FX's
  `display.c`). Nothing is offered where the engine cannot switch (`full_screen_switch` 0).
- **Quit.** QUIT asks once more (it reads REALLY QUIT; moving off it or shutting the menu undoes the ask), then saves the
  settings and ends the run through `nshvc_quit`. Stunt Race FX's desktop menu has no quit and its items do not ask; the
  iPhone menu quits the race at once. Here a second press is asked because the original's progress is passwords, so a quit
  from the middle of a level loses the level.
- **Saved.** One file, `options` in the engine's save folder (`system/settings.c`): full screen and the keys and controller
  buttons. A file of another size, or with bindings that are not valid, is ignored whole. This is the first file the game
  saves (plan decision 4 kept progress to passwords; Jeff allowed this file, 2026-11-23). Everything the game saves (this file, and the saved games and beaten-levels files on the branches that add them) goes through the engine's save API, so it all follows the folder below. The folder is the game's own, not the system's: see "Where the game saves" below.

## Where the game saves

Jeff, 2026-11-23: "make sure save data goes in the game's directory, not OS folders, so it's properly portable." The game
sets `nshvc_game.save_in_game_folder = 1` (`system/main.c`; the engine's API 6.12, `nativeshvc/docs/api.md` section 8, no engine
change), so what it saves sits beside the program and copying the two together carries the options (and, once merged, the
saved games and progress) to another place or machine.

| Host | Folder |
|---|---|
| macOS | `aladdin-save` beside `aladdin.app` (not inside: writing in a signed bundle breaks its signature). A bare executable: `save` beside it |
| Windows | `save` beside `aladdin.exe` |
| Linux | `save` beside the program (the folder `/proc/self/exe` is in) |
| iOS (and Android) | the exception: an app cannot write beside itself, so the flag is ignored and the files are in the app's own container (Application Support on iOS) |
| headless | `build/aladdin/save` under the folder the program is run from, unchanged (the flag is ignored) |
| any host with `PORT_SAVEDIR` | that folder, whatever else is set; nothing is chosen, copied or reported |

The engine makes the folder at the first start, checks it by writing and removing a test file, and writes its own
`.carried-over` marker and (on Windows) `log.txt` in it. A packager keeps `aladdin-save` or `save` when it replaces a build and ships a build without one.

**Open for Jeff (engine behaviour the game does not control; neither is optional in the engine today).**

1. **The fallback.** When the place cannot be written the saves go to the old system folder instead, and the engine
   writes one line to stderr (`log.txt` on Windows): `nativeshvc: cannot write in <folder>, so saves stay in <system folder>`. The
   player is not told in the game. It triggers when the folder cannot be made or a file cannot be created in it: macOS, `aladdin.app` run from a
   mounted disk image, from the read-only copy Gatekeeper runs a quarantined app from (app translocation, until the player moves it), or
   from `/Applications` by a player who is not an administrator; Windows, `Program Files` without rights or a read-only drive;
   Linux, a read-only install, an AppImage (its mount is read-only, the folder beside the program is the mount) or a package-owned folder.
   This departs from "not OS folders". The alternative is to tell the player and not save, which needs an engine
   change (a new request in `ENGINE_REQUESTS.md`); not built, since it is Jeff's call.
2. **The carry-over.** The first time the game's folder is used, flat files in the old system folder are copied into it
   (never over a file there, never deleted or changed in the old folder), and `.carried-over` is left so it happens once. A
   Mac that has run the game before therefore gets its old `options` in `aladdin-save` at the next start. If Jeff does not want this, it is also an engine change.

## Where the game saves: as built (2026-11-23)

Built against engine `134e2a2` (API 6.12; every build folder of this worktree has `NSHVC_ROOT` at `../nativeshvc`, a
checkout on its main). The game's change is the one line in `system/main.c`. The check scripts `native/checks/view/same.sh` and
`native/checks/pause_check/hold.sh` now give each run its own `PORT_SAVEDIR` inside their output folder: before, every
headless run created `build/aladdin/save` beside the build it ran from, shared by all runs of that build.

Checked (macOS, arm64, Metal build and the headless build; the options written by the pause menu's code path: Escape, Right,
B on DEFAULTS in the first level of play p0-0, which calls `settings_save`; `CFFIXED_USER_HOME` pointed at a scratch folder so
the system folder was a scratch one):

- Headless run of the app's program from a scratch folder: `options` (39 bytes) and `.carried-over` appear in `aladdin-save` beside the
  .app, and nothing in the scratch system folder. The same in a window (`--updates 6520`, no `--headless`). The real
  `~/Library/Application Support/aladdin/options` was only read by the migration check (a first run with it as the source copied it: "carried 1 save file(s)") and keeps its md5 and date.
- The folder copied elsewhere with the app, its `options` changed to bind B to V: with Z the menu stays open (the game held:
  6,433 trace lines), with V it shuts (6,507); the original folder with Z shuts (6,507). So the copy reads the file beside it.
- Read-only folder with the app and no `aladdin-save`: the stderr line above, and `options` written in the system folder; nothing beside the app.
- `PORT_SAVEDIR`: the file is written there and `aladdin-save` is not made.
- `same.sh` (the same headless build on both sides, since the flag does nothing there): 21 runs, 0 differ; no `build` folder is left beside the build and no run shares a save folder with another (42 folders, all empty). `hold.sh`: exit 0, 299 updates compared, the game held for 51.
- Windows (Direct3D 12, cross-built, run headless on the VM): `save\options` (same 39 bytes, md5 identical to the Mac's) and `.carried-over`, `log.txt` beside `aladdin.exe`; `%APPDATA%\aladdin` gained nothing (it holds the `log.txt` of earlier runs). The folder copied to another place with its `options` changed (B bound to V) read it: Z left the file as it was, V rewrote it to the defaults (md5 as the original's).

Not run: Windows read-only place (Program Files) and Windows in a window; Linux and iOS (read from the engine's code only: Linux `/proc/self/exe`'s folder, iOS the container); a mounted disk image or app translocation on macOS (the read-only case above stands for them: the engine tests only whether it can write); Stunt Race FX or the engine's own tests (the engine was not changed); the saved-games and beaten-levels files, which are on unmerged branches (their code calls only `nshvc_save_read` and `nshvc_save_write`, read with `git grep`, so they follow the folder).

## Look

The text is the game's 8 x 8 text tiles (unpack set 1, the tiles the original's options screen put at $5000), the hand is that
screen's cursor (tiles $7E, $02-$04, $12-$14, the title menu's mark too), and the colours are the palettes that screen loaded (set $08: the gold of its
items, the cream, the pink of its title, the glove's blue), all read from the cartridge through the data layer (the options screen itself is gone from the title menu: DEPARTURES.md section 7)
(`pause/menu_art.c`). The frame is drawn from the gold's colours. The controller is Stunt Race FX's drawing, the same
`SNES_controller.svg` and `pad_art.c`; its label letters are `pad_glyphs.c`'s capitals, part of that picture.

The title menu's second and third choices are SAVED GAMES (the save slots' screen) and LEVEL SELECT, which does nothing yet (DEPARTURES.md section 7); the pause menu is the game's only menu of settings.

The save slots' screen is built from this menu's drawing kit (`menu_art.h`: panel, text, hand, box, blit) and its panel (352 x 282), in `slot_grid.[ch]` (a reusable grid) and `save_screen.[ch]` (the slots). The text tiles are the original's, whose punctuation is not at ASCII's places; `menu_art.c` maps the marks it needs (: . , - ? ! ' ( ) = ), so a `-` (a key with no name) no longer shows the multiply sign.

Not done: the password screen's flourishes (a graphic of the screen, not text) are not in the frame, and Stunt Race FX's
blurred picture behind its panel is not used: the picture stands under a darkening.

## Layout and scale

The panel is 352 x 282 game pixels. It is drawn at a whole number of image pixels to a game pixel (1 to 6) and laid at a whole
number of window pixels to a game pixel where the window allows, so its pixels stay square and sharp; in a window too small
for that it is scaled down. It does not depend on the view's shape (256 x 224 or wider): it is the window's.

## Checks

- `native/checks/view/same.sh` at 256x224 with no menu opened: 21 runs, 0 differ.
- `native/checks/pause_check/hold.sh BIN`: a play run as it is and with the menu opened (Escape) and shut again at an update
  where no key is held; the game does not step while open and the traces after it are the plain run's, update for update.
- `native/checks/pause_check/check.py` with `build.sh`: the controller held against rsvg-convert's drawing of the SVG (56
  pixels over 16 levels, none over 32, of 600,000, outside the letters, which rsvg-convert does not draw).
- `pause_check menu` (`pause_check.c`): the menu over a game picture in a window of a given size, for the pictures in
  `build/aladdin/native/pause/`.
- By hand, in a window on a Mac: Escape opens it, Down and B move and choose, FULL SCREEN switched to full screen and back
  and the saved file followed, QUIT asked then ended the program.
