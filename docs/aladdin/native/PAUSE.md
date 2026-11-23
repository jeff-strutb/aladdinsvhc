# The pause menu, controller mapping, full screen and quit

Jeff, 2026-11-22: the pause menu and controller mapping of Stunt Race FX in Aladdin, in Aladdin's own look, with the same
controller picture; full screen by default with a window available; a way to quit. Start and Escape open it, in place of the
original's pause.

## What it does

- **Opening.** Start during play, where the original paused (`game/pause.c`, `PauseCheck`: full brightness, no demo, nothing
  blocking, the cooldown run out), and Escape on any screen. Start or Escape shuts it. `PauseCheck` keeps the original's tests
  and asks the menu to open (`menu_request`) in place of freezing the tasks and showing PAUSED.
- **Holding the game.** While the menu is open `update` does not step the game (`system/main.c`: `menu_update` returns 1), so
  the tasks, the frame counts and the picture stand where they were, as the original's frozen tasks left them. The sound goes
  on: the menu queues the original's pause sound ($0E, not on level $12) and the music pause ($F3), and sends the queue itself
  each held update (`SoundCommandSend`); it queues $F4 and sets the original's cooldown ($10) when it shuts. Buttons held as it
  shuts reach the game only once let go (`controls_swallow`). With no menu opened the game is exactly as before.
- **Items.** The twelve pad buttons, each tag naming what the button does in the game (JUMP, DASH, THROW, HOVER as the options
  screen gives them, LOOK UP, CROUCH, GO LEFT, GO RIGHT, MENU for Start, UNUSED) over the key bound to it (the controller button when a controller was the last
  device used), a line from each tag to its button on the controller; RESUME; DEFAULTS (the game's own keys and buttons back); FULL SCREEN ON or OFF (not offered where the window
  cannot switch); QUIT.
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
  saves (plan decision 4 kept progress to passwords; Jeff allowed this file, 2026-11-22).

## Look

The text is the game's 8 x 8 text tiles (unpack set 1, the tiles the options screen puts at $5000), the hand is the options
screen's cursor (tiles $7E, $02-$04, $12-$14), and the colours are the palettes that screen loads (set $08: the gold of its
items, the cream, the pink of its title, the glove's blue), all read from the cartridge through the data layer
(`pause/menu_art.c`). The frame is drawn from the gold's colours. The controller is Stunt Race FX's drawing, the same
`SNES_controller.svg` and `pad_art.c`; its label letters are `pad_glyphs.c`'s capitals, part of that picture.

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
