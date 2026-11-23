# The view at any shape: analysis and plan

Written 2026-11-22 at `b36b712` from reading `native/aladdin/src`, the engine (`nativeshvc` at `2d4726f`, `docs/api.md` and
`src/engine/picture.c`) and tables read from the ROM. Nothing here was built or run. Stage 7 of [PLAN.md](PLAN.md): this is
the design shown to Jeff before it is built.

**The direction (Jeff, 2026-11-22):** resolution agnostic across landscape shapes, down to 4:3 ("Just don't support portrait. Landscape down to 4:3 dimensions only."), done as the view of the
level expanding, including the distances at which things spawn, wake, animate and trigger, not as a wider field of view.

**How this meets it, in one line:** the game's one hard-wired 256 x 224 view becomes a view of W x H game pixels chosen from the
window's shape. The camera, the map, the spawn and despawn windows, the "in view" test and the sprite list are all measured from
it, so what the player can see is live, and a 256 x 224 view gives exactly today's game.

## 1. What the engine gives

- `nshvc_screen.width/height` take any size. `origin_x/origin_y` place the original's 256 x 224 inside it. The picture is drawn
  at the window's pixels, so a larger view in game pixels is sharper, not stretched.
- Line changes count from `origin_y` with no upper bound. A row above the origin takes the picture's starting state
  (`picture.c`, the row loop). A table of H lines works at origin 0.
- Window edges are picture columns (int32). A split inside a line is `column + origin_x`.
- Maps, tile sets and sprite lists may be any size or count. `nshvc_limits` is off by default, and `layer_clip` keeps a layer to
  the original's 256 x 224.
- **The palette is 256 entries**, so sprites have 8 palettes of 16 colours. That limit matters (section 7).
- Precedent: the engine's Zelda 3 example (`examples/zelda3/src/video.c:290-342`) is a sprite game made wider with extra
  columns. It uses `origin_x`, `show_left/right` and `layer_clip` for its status bar. Stunt Race FX's widescreen was the
  other kind, a wider field of view for 3D (`stuntracefx-widescreen`, `ffc9492d`), and does not apply here.

## 2. The view's size

The view in game pixels, from the window's shape `a` (width / height), taken as at least 4:3 (Jeff, 2026-11-22):

- The picture shown is 216 lines (the original's 224 less the 8 a level crops) and round(216 · max(a, 4/3)) across, even, so at
  least 288. In a level the view is 224 tall (216 and the crop rows): H = 224, Δy = 0.
- A window narrower than 4:3 shows that 4:3 picture letterboxed; nothing is ever taller than the original's view. Portrait is
  not supported.
- `ALADDIN_VIEW=WxH` (a check) is clamped the same way: the height is the original's, the width at least 288, except exactly
  256 x 224, the identity check.

The original's 256 x 224 is always inside the view, so nothing the original showed is ever cut. A larger window at the same
shape shows the same view, sharper. Section 5 covers what happens where a level is smaller than the view. The rest of this
document writes Δx = (W − 256) / 2, 0 or more (Δy is 0).

Today `video_draw` sets no size: `nshvc_screen_set` is called with width and height 0 (`video/video.c:491`).

## 3. Where the game holds its view today

The 256 x 224 view is not in one place. It is built into five mechanisms, each with its own constants.

### 3.1 The camera (`video/scroll_update.c`, `video/camera.c`, `video/scroll_layer.c`)

- **Aim.** The player is kept at screen x `camera_aim_x` = $80 (`player/player_starts.c:103`, `player/frame.c:162`). The y "point"
  and deadzones come from the 10-byte windows at 80:C672, which are screen offsets for 224 lines (`scroll_update.c:33-75,107`).
  There is no lead and no look up or down.
- **Limits.** `limit_x = (columns − 1) · 256` and `limit_y = (rows − 1) · 256` (`scroll_layer.c:79-83`): the right edge of the
  view is the level's edge only because the view is 256 wide. Direct writers of the limits: `level_screen_setups.c:423`,
  `actor/kind81.c:136`, `level/level_events.c:209`. Scene locks (`g.scroll_x_min/max`) are left-edge positions for a 256 view:
  `level/level03_scene.c`, `level0d_scene.c`, `level10_scene.c`, `level/camera_trigger.c:66`.
- **Empty-block stops** use `x + $FF` and `y + $FF` as the far edges, test only the corner blocks, and snap to one 256 block
  (`camera.c:58-103`).
- **Self-running scrolls.** Level 6 holds the player at most +$F0 from the left edge (`scroll_update.c:161-167`).
- **Fall death** is `limit_y + $E6` (`player/frame.c:60-68`). It is derived from the limit, so it must be kept apart from the
  view's height, or deaths would move.

### 3.2 The map: a streamed ring

The play field and backdrop are written column by column and row by row into 64 x 32-tile (512 x 256 px) maps that wrap
(`video.c:41-44`; `scroll_layer.c:96,353,396-482,522,553`).

- The edges are drawn at −16 and +272 across, and at −16 and +232 down (80:C656).
- **Down, the ring is already full:** 256 px holds the −16 to +240 the original needs. Any H above about 224 overwrites rows
  still on screen.
- Across, W can reach about 464 before columns still on screen are overwritten.
- Everything the ring is filled from is in RAM: `g.screen_map.places` (32 x 32 screen places) and `g.level_tiles`
  (`map/level_map.c:18-37`, `system/state.h:560`). The largest level is 32 x 3 screens (8192 x 768 px). The tallest is $0A at
  13 x 8 screens.

### 3.3 The range point and its boxes: what is "near the screen"

Almost every off-screen test in the game measures from one point: `g.range_x/y = camera corner + $80` on both axes
(`game/frame_step.c:16,52`; `level/level_tables.c:14,64`; `game/ending.c:364` moves it). Down, +$80 is 16 px below the middle
of 224 lines. Around it are two ROM tables of half-sizes:

- **`view_box`** (80:BA02, `data/actor_tables.c:113`). `ActorsInView` sets an actor's "in view" flag from it
  (`actor/in_view.c:19-51`). Box 0, the default, covers −32 to 288 of the screen. **The flag is more than drawing:** an actor
  out of view is not drawn and is also not hit, not solid and cannot be stood on (`hit_box.c:16`, `actor_overlap.c:31,38`,
  `solid_actor.c:35`, `player/attach.c:25`, `course_platform.c:529`). Shots end when out of view (`thrown_shot.c:30`,
  `kind9a.c:69`, `kind_a1_a6.c:84`, `kind58_start.c:190`). Scripts and sounds wait for it (`kind70_start.c:74`, `kind3e.c:62`,
  `kind2f.c:158`, `kind6c.c:60`, `kind42.c:84`, `kind53.c:41`, `kind31.c:99`).
- **`range_box`** (80:CE73, `data/actor_a_tables.c:11`). This is the despawn test (`ActorInRange`, `ActorEndOutOfRange`,
  `ActorFreeOutOfRange`, `actor/actor_range.c:16-54`), at about 67 call sites. Its boxes run ±$E0 to ±$200.
- **Direct constants** in kinds, each a screen size in disguise: `kind7d.c:126` ±$E0, `kind79.c:227,230`, `kind86.c:116-118`,
  `kind5c.c:346` ±$140, `kind_a1_a6.c:153`, `kind_waits_rest.c:151`, `kind68.c:137`, `kind6e.c:230-247`,
  `actor_range.c:78-88` ±$100, `wait_step.c:22-27` +$120, `kind6d.c:47` $90 ("spawn just off screen"),
  `course_platform.c:398` (screen middle).

### 3.4 Spawning (`actor/placement.c`)

- Placements are indexed in 32 px bands (`:47-119`). At a level's start, 12 bands are placed from 64 px left of the camera
  (`:192-209`). After that, one band per axis comes in each time the camera has moved 32 px (`:215-241`).
- An actor is placed when its position is from camera −64 to camera +320 on both axes (`:153`), against the camera as it was at
  the last placing.
- `g.placed[n]` bit 0 means placed and bit 7 means never again (`:136,149`). Leaving the range clears bit 0, so the actor comes
  back when its band re-enters (`actor_end.c:99-107`).
- When there is no free record or palette slot, the placement fails silently and is not retried until its band re-enters. (A larger view now opens slots as needed and a scene's clear keeps what the original has not placed: section 4.5.)

### 3.5 The sprite list (`video/oam.c`)

- x is wrapped to 9 bits (`:94`): in a wider view, anything at screen x 256 or more would show on the left.
- y is culled outside −32 to 223 (`:86`).
- There is a budget of 128 sprites (`:13`), and an actor with more pieces than are left is dropped whole.
- There are four draw lists of 32 actors each (`in_view.c:38-45`).

### 3.6 Player-relative distances: gameplay, not view

The `ActorNearPlayer` family (`actor/player_near.c:35-80`, `near_box` at 80:BC43, up to ±$80, `near_span_x` up to $A0) wakes
actors by their distance from Aladdin, not from the screen. These are not the view (decision 2).

## 4. Design, by area

Each rule below reduces to today's value when W = 256 and H = 224.

### 4.1 One view, in one place

`g.view_w`, `g.view_h` and `g.view_dx` (Δx), `g.view_dy` (Δy) are set once a frame, before `update`'s work, from the window's
shape and section 2's rule. They change only between updates (and, in play, by 4.8). Every rule below reads them, and no code keeps its own 256 or
224. The view's size is not game state that the plays record: a play run at 256 x 224 must reproduce today's game exactly
(section 8).

### 4.2 Camera

- Aim x = $80 + Δx. Every y point and deadzone of 80:C672 is moved by Δy, keeping each one's distance from the middle.
- `limit_x = width − W` and `limit_y = rows · 256 − 32 − H`. The second keeps the original's lowest visible line; at H = 224
  it is today's `(rows − 1) · 256`.
- The direct writers (`level_screen_setups.c:423`, `kind81.c:136`, `level_events.c:209`) write positions in the same terms.
- Fall death is `(rows − 1) · 256 + $E6`, computed from the rows and not from the limit.
- Scene and room locks: where `scroll_x_min/max` holds a room the original's 256 view showed, the view is centred on that
  room, `left = room_left + 128 − W/2`, clamped to the level. Section 5 covers rooms narrower than the view.
- Empty-block stops: the far edges become `x + W − 1` and `y + H − 1`, every block along the edge is tested, and moving right
  or down snaps to `boundary − W`.
- Level 6's hold becomes +$F0 + 2Δx, keeping the same 16 px from the right edge.

### 4.3 Map: the whole level in one engine map

The ring cannot grow down, so the play field and the backdrop are each kept as a whole-level engine map. Engine maps may be
any size, and the largest is 1024 x 96 tiles of 8 px.

- The map is written from `g.level_tiles` once at level load, and again for each tile change.
- The engine draws only the tiles in view, so the cost follows what is shown (api.md §3).
- The ring, its column and row queue, and the 6 px trigger are removed. Today they are the reason W and H are bounded.
- Every writer of the ring is pointed at the level map instead: `TileChangesApply` (`map/tile_changes.c:83,94`),
  `LevelEndBlanks` (`game/level_end.c:54`), the screen-map patches (`level_screen_setups.c:94,300`), and the frozen layers
  (`level/level_backdrop.c:29`, `kind81.c:154`).
- Empty screen places draw blank tiles, not screen 0's (`scroll_layer.c:49` draws screen 0 today, which a wider view would
  show).
- The backdrop's art repeats every 512 px in the band levels (0, 2, 3, level 6's drift), so the backdrop map wraps at its own
  period. Per-line scroll stops masking to `& 0x3FF` (`video/lines.c:150`).

This is a change of mechanism within the direction, not a new direction. It is listed as decision 5 because it replaces
converted code.

### 4.4 The original frame (Jeff, 2026-11-22)

The game plays at the original's 256 x 224 against the original's camera. The view is a frame round that screen: the original's
screen sits at (Δx, Δy) in the view, and the engine draws the level's layers beyond its edges. The camera, the scroll limits and
stops, the player's clamps, the view-edge rules, the collision probe and the range point (the middle of the original's screen)
all have the original's values. What the view adds is liveness, in three kinds of use:

- **(a) Camera and player: the original's.** `ScrollUpdate` and its limits, `CameraStop*`, `PlayerKeepOnScreen*`, `ViewEdge*`,
  `ProbeOnViewOff`, `PlayerFallenTest`, the level 5 end scene's windows, the range point.
- **(b) Liveness over the whole view: scaled.** The spawn window (placement), the despawn tests (`range_box`,
  `ActorEndFarWithChildren`, `ActorEndIfAhead`, the kinds' own range tests), the in-view flag (bit 6 of the actor's flags: drawn,
  hit, solid and standable, and where a shot ends), the draw lists and the sprite budget. A box grows by Δx on each side across and
  Δy up and down round the same range point, so everything the view shows is live: an apple flies its whole arc and can hit
  whatever it reaches, an enemy in the frame is drawn, hittable and goes on with its untriggered behaviour.
- **(c) Triggers: the original's screen.** The player-proximity wakes (`near_box`, `near_span_x/y`: the table's values, not
  scaled), the screen-relative starts and waits (a script, sound or step that waited for the in-view flag; `kind6d` "spawn just off
  screen", `kind79` and `kind8d` starting below the screen, `course_platform`'s screen middle, the guard's dead zone) measured against the
  original's 256 x 224. A second flag, `Actor.on_screen`, is set by `ActorsInView` beside the in-view flag from the original's own
  `view_box`; `ACTOR_ON_SCREEN(a)` is the flag set and `on_screen` set, and the sites of kind (c) ask it. At 256 x 224 the two
  flags are the same test.

Section 8 lists the checks: at 256 x 224 the game is the original's; wider, the trigger log
(`ALADDIN_DEV_TRIGGERS=1`) shows no trigger fired past its original distance.

### 4.5 Spawning

The window is measured from the view's edges (camera less Δx, Δy), 64 pixels before them to 64 past across and 96 past down. The
initial band count follows the window's length. One band a frame stays as it is (the camera moves 3 pixels a frame across, 6
down, under a band's 32).

**Palette slots open as needed** (Jeff, decision 1: any number of sprite palettes, not only a larger fixed number; built
2026-11-23). A larger view starts with 4 slots scaled by its area and opens one more whenever a kind finds none free
(`video/slot_gfx.c` `slots_grow`; their tiles in sets of 16 slots, `video_slot_tiles`, a run of 2 or 3 never across two sets),
so no placement fails for want of a slot. At 256 x 224 the original's four stay, with its failures. Found by Jeff's play at
384 wide: level 3's market-end figure (kind $56) failed for want of a slot, its band never came again under the scene's
locked camera, and the level never ended.

**A scene's clear** (`ActorsClearForScene`, `actor/pool.c`; the scene tasks of levels 3, 4/$16, 5 and $10 call it where the
original calls `ActorsFreeAll`; built 2026-11-23, Jeff: fully seamless, game logic changed as needed). The wider window places
figures before the original does, and the original's clear assumes only what its own window has placed: at 384 wide, level 3's
clear removed kind $56 before the original would have placed it, and nothing placed it again. In a larger view the clear
removes what the original removes and nothing the view shows going:

- A figure whose placement the original's own placing has not reached is left as it is. `placement.c` follows the original's
  placing beside the view's (the same camera steps of 32, the original's 384 window and far band $120 from the original's
  camera) and marks what it reaches; it creates nothing.
- A figure the original has, shown in the view (its in-view flag) but off the original's screen, which the original removes
  unseen, stays outside play (`Actor.outside_play`): drawn and running, not counted with its kind, touching and touched by
  nothing (`actor_in_play`: hit, shots, solid, landing, riding, the pad), what it makes outside play too, and freed the frame
  its in-view box leaves the view, which is when it would stop being drawn anyway. If the pool has no free record when an
  actor is made, one kept outside play is freed first, so a scene's own figures are never short of one.
- Every other record is freed as the original frees it: those out of sight, and those on the original's own screen, which the
  original removes in sight too (seen at 256 in levels 4, 5 and 16).

At 256 x 224 the clear is the original's `ActorsFreeAll`. Measured over the 20 plays and Jeff's at 256, 384 and 512
(`ALADDIN_DEV_CLEAR=1` prints each clear): at 384 and 512 level 3's clear keeps kinds $56 and $57 (not placed yet by the
original) and keeps a pot (kind $4D, 32 pixels left of the original's screen) outside play until it leaves the view; Jeff's
play then ends level 3 at both.

### 4.6 Sprites

The 9-bit wrap goes: x is a full int, and a piece is kept from -32 above the view's top to its bottom. The budget (128) and the draw
lists (32) scale with the view's area. The sprites are the original's coordinates, moved by (Δx, Δy less the cropped rows) when
sent, as the layers are.

### 4.7 The display of the frame

`video_draw` sets the picture to the view and moves the original's screen to (Δx, Δy less the level's crop rows L). Down, the
engine's origin does it (layers, per-line tables). Across it is done in the layers' scrolls, the per-line BG2 scroll and the
sprites, since a split inside a line cannot be placed left of the engine's origin and the status line (hud_layer.c) needs one at
the view's edge. Windows: the original's columns plus Δx; an edge at the screen's own edge (0, 255) goes on to the picture's
edge, so a window that covered a line to its end covers the frame beside it, and an empty one stays empty (`lines.c`).
`colour_bands.c`, `parallax.c`, `circle_wipe.c`, `window_effects.c` and the level 6 line tables are the original's: the tables count
lines from the original's top, a line past the table's end holds its last value, and a row above the original's top takes the
picture's starting state. The status line stays at the corners of its band across the view's top (section 6.2).

### 4.8 The view follows the window, at any moment (2026-11-23; design before the build)

**Jeff, 2026-11-23:** "resizing the screen should properly adapt the screen for the new screen size, not just on level start. right now if i start say at 4:3 and go full screen to 16x9, the game stays cropped to 4:3." This replaces "the view changes only where a level loads" (4.1, 5.x, `view.h`, FRAMERATE.md 4.2): every other decision here stands.

**How this meets it, in one line:** at the start of an update the view is chosen again from the window by the rule of section 2, and what was built from the old view (the maps' margins, the sprite lists, the spawn window) is made again for the new one, so the picture takes the new shape on that update, in a level, on a still screen or with the pause menu open; nothing in the original's 256 x 224 frame's logic is touched.

**What reads the view, and what a change must redo** (from the code at `b91729e`, not from plays):

| What | Held how | On a change | Same as a level loaded at the new shape? |
|---|---|---|---|
| `view.w h dx dy square` | `view.c`, set by `view_choose`/`view_choose_still` | chosen again (`view_follow`) | yes |
| Camera: aim, limits, locks, stops, `range_x/y`, the player's clamps, fall death, the probes, `ViewEdge*`, level 5's windows | the original's values (4.4a): none reads `view` | nothing | yes |
| Gameplay frame's offset (the original's screen at dx, dy in the view) | read from `view` at each draw (`video_draw`, `lines.c`, `oam.c`) | nothing | yes |
| `view_box`, `range_box` and the kinds' range tests, the in-view flag and `on_screen`, the draw-list room (`in_view.c`) | read from `view` at each call | nothing: they follow at the next `ActorsInView` | yes |
| Per-line tables, windows, the HUD band, the crop's rows, BG2's per-line scroll | built by the game from `g` each frame, moved by `view.dx` when sent (`lines.c`, `hud_layer.c`) | nothing | yes |
| Still screens, Mode 7 | `video_draw` centres the original's 256 x 224 by `view.dx`, `show_*` black round it; no map, no spawn | the view alone (and the sprite lists) | yes |
| Sprite lists: the game's list and its owner, anchor arrays (`oam.c`, budget 128 scaled by area) and the picture's shown lists (`video.c`, sized to the most any view needed) | allocated by the view's area | the game's list is resized keeping its contents; the shown lists (the update before's sprites) are kept as they are, and grow only if the area is larger than any before | yes (the shown lists are the update before's world and are the same in both) |
| Palette and graphics slots (`slot_gfx.c`) | a count from the area when a level starts, opened as kinds need; never closed | nothing: a wider view opens more as kinds find none free; a narrower one keeps those open | close: a level loaded narrow has fewer open, and which slot a kind holds follows what was placed before it, so one figure can sit in slot 6 in a play that began at 512 and in slot 5 in one that began at 384 (level 3's kind $56, `ALADDIN_DEV_SLOTS`); its own bytes differ too, by when it was placed, and its picture with them |
| Level maps: margins and fill (`level_maps.c`, `level_fill.c`) | the map is the layout plus `ceil(dx/16)+1` cells each side across (backdrop 8 more), the fill built at load from the layout; one engine map object per size, never freed | made again from the layout's cells (kept at load) when that cell count changes: the margin is a function of the layout and the count only, so it is exactly what a load at the new shape makes. The layout's own words are kept as they are now, so a tile already changed (a wall broken, a door opened) stays changed | margin: yes; layout: the level as played (what a load would show before the play changed it, which is not the same picture and should not be) |
| Engine map objects (`map_of`, 96 sizes, no free) | by exact size | the map is built at a fixed padding of 32 cells each side across (more only past 32), so every width a window can have is one size per level and layer; a drag makes no new object | n/a |
| Spawn: `placing`, the bands, `g.placed` (`placement.c`) | the window is read from `view` live, but which bands are brought in is by the camera's steps from `placing`, set at load | `placing` set from the camera again and the bands of the new window brought in (`PlacementsStart` less the indexing and the clearing), at the step's placing point | close: see below |
| The original's own placing (`console`, `console_reached`) | by the original's camera | nothing | yes |
| Actors outside play (`outside_play`) | freed when their in-view box leaves the view | nothing | yes, by the same rule |
| Eased picture (`between.c`): "was" and "now" | `view`, `layer_map` and the shown lists are kept parts | the change is made before "was" is copied, so both hold the new view and nothing slides between two shapes | yes |
| Runahead | saved after the real step, put back at the next update's start | the change is made after the put-back and before the save, so the real world and every ahead world have the new view; no ahead step ends for it | yes |
| Pause menu | `update` returns before the step | the change is made before that return: the picture is the held world in the new shape | the spawn waits (below) |

**When.** In `update`, after `runahead_begin` and before `between_update_begin`, never inside a step or a load. A level's load takes the view as now (`video_view_level`) and clears anything waiting.

**What is rebuilt, in order.** (1) The target view is chosen from the window (still or level as the screen is now). (2) If it is the view already, nothing. (3) The view's fields set (the margins read them). (4) Level maps' margins made again for the layers whose cell count changes (a layer with no change, a band backdrop and a layer shown from a video map are left), the sprite lists resized. (5) The spawn window re-based: `placing` from the camera, flagged and done at `PlacementsScroll`'s next turn, the point in a frame where the game places, so no actor is made outside a frame (a level's end, a pause).

**What the picture shows on that update.** The new shape, from "was" = "now". The map margins are the fill for the new depth; the columns the view adds hold the level's art or its fill (counted in the checks). The actors the new window places are made in that update's step and drawn in the next picture: the one picture of that tick has the level's art in the new columns and the sprites the game already had. With the pause menu open the spawn waits for the first step after it shuts, so the new columns show art and no enemies until then (a choice for Jeff, below).

**The game's state, wider and narrower.** At 256 x 224 nothing changes. Wider: the placing of a new window brings in every placement in it that is not placed and not gone (`bring_one`'s own test): the figures a level loaded at that width would have placed, except those that were placed and have ended since, which come back only when their band is crossed, as in any play. Narrower: nothing is removed: what the narrower view no longer holds ends by the range tests of 4.4b (scaled by the new dx) and is placed again when its band returns; a figure kept outside play leaves when its in-view box leaves the view (`ActorsInView`). A scene's clear (4.5) keeps what the original has not placed, and keeps outside play what the original removes unseen, by `console`, which a change does not touch, so a change just before a scene's clear gives what a level loaded at the new shape gives. The rule needs nothing more of Jeff's: it is 4.5's rule applied at a moment other than a load. *Not the same as a level loaded at the new shape:* a figure the player killed stays gone (bit 7), one that ended out of range comes back with its band, not at once, and the records and palette slots are used in the order they were taken, so a trace of the actors differs in record numbers and order from a load at that shape (not in what each does).

**A drag.** The view is a function of the window's shape, so most steps of a drag give the same view (the width is even, 288 at 4:3 up by 2 for each 0.9 percent of the proportion). A change that needs no new margin (the cell count `ceil(dx/16)+1` is the same) is made at once: the sprite lists and the spawn only, expected under 0.1 ms. A change that needs the margin made again costs the fill, which was measured at load at 70 to 250 ms a level (VIEW.md 5.x, `LOADTIME2.md`), far over 16.7 ms: it is made once the target has held for 6 updates (0.1 s), so a drag of 120 updates makes none until it stops and one when it does, and a full-screen switch (the system animates the window for about half a second) one at its end. While it waits the picture is the old view fitted to the window by the engine (letterboxed), which is what shows today. The developer's changes (`ALADDIN_DEV_VIEW_AT`) are made on the update asked, with no wait.

**Checks, expected.** `same.sh` at 256x224: 21 runs, 0 differ. `wide.sh` 288, 384, 512 as before. `state.sh` passes. A change 288 to 512 and 512 to 288 at several updates in four plays: the player's line identical to the run that does not change except where a figure of the wider window touches him (listed); the pictures from the update after the change equal to a run started at the new shape where the same figures are placed, each other difference listed; the columns added hold only the level's art or the fill (counted); cost of one change under 0.1 ms without margins and the fill's time with; a drag of 120 updates one rebuild.

**Choices for Jeff** (nothing here departs from a decision; each is a case no rule settles):
1. *Pause menu open at a change:* the new columns show the level's art and no figures until the game steps again (proposed), or a figure-less frame is not enough and the game places and draws while paused (a step the pause holds back; not proposed).
2. *The wait of 6 updates before a change that needs new margins:* proposed to keep a drag from rebuilding at 70 to 250 ms a time. The alternative, at once on every change, is exact to the letter of "at once" and stalls the game for the fill at each 32 pixels of width a drag crosses.
3. *Palette slots stay open after a narrower view* (nothing can use fewer): kept.

**As built (2026-11-23).** Code: `video/view.c` (`view_target`, `view_apply`, `view_dev_at`: the rule's result without setting it, the developer's changes), `video/video.c` (`video_view_follow`, `sprite_lists_make(keep)`), `video/oam.c` (`OamResize`), `video/level_maps.c` (`layer_make`, `LevelMapsNeedResize`, `LevelMapsResize`, the layout's cells kept, the map padded), `actor/placement.c` (`place_window`, `PlacementsViewChanged`), `system/main.c` (one call in `update`, after `runahead_begin`, before `between_update_begin`). As designed, with these differences found while building:
- **The map is padded.** A level's map holds 32 cells of margin each side across (more only past a view of about 1,250 pixels), the fill as deep as the view shows and the rest blank, so one engine object serves every width of a level and layer (objects are never freed and the pool holds 96 sizes; a drag across 7 depths would have made 14 a level). `level_maps_words` gives the dump the fill's part of it alone. At 256, 288 and 512 across the pictures are the ones before the change: 6 plays (p0-0, p0-3, p3-0, boss16-2hits, p5-3, p4-2), a picture every 100 updates from 3900, built at `b91729e` against built now: 336 pictures at each of the three widths, 1,008 in all, 0 differ.
- **No window, or one with no size** (a minimised window reports none): the view stays as it is. The original's 256 x 224 is still the headless view at the start.
- **Placing.** The window is set again at the next `PlacementsScroll` (the frame's place for placing: not at a level's end or in a pause), from the camera, with the placements, `g.placed` and the original's own placing (`console`) left as they are. A change at an update puts the new figures into that update's step; their first picture is the next one.
- **The developer's variables** (CHECK.md): `ALADDIN_DEV_VIEW_AT`, `ALADDIN_DEV_WINDOW_AT` (the window's pixels, so the rule and its wait are used), `ALADDIN_DEV_VIEW`, `ALADDIN_DEV_NO_SPRITES`, `ALADDIN_DEV_SLOTS`; `native/checks/view/change.sh` and `change_report.py`.

**Checks run, and their numbers** (headless build of this tree against the build at `b91729e`; pictures from the Metal build of this tree):
- `same.sh` at 256x224: 21 runs, 0 differ (the traces, update by update: the player, every live actor with its own bytes, the video fields). `wide.sh` at 288x224, 384x224 and 512x224: 21 runs each, 0 failed, 0 triggers past their original limit. `runahead/state.sh`: 21 runs, 0 differ (the real world's hash after every update, with and without ahead steps).
- **Runahead through changes:** with ahead steps on and off, trace and real-world hash line for line the same through 3, 2 and 2 changes in p0-3 (6,900 updates), p1-0 (6,500) and boss16-2hits (9,400): 22,800 hashes. 4, 4 and 7 ticks a run ended early, for a level's load or the sound's (`runahead_guard`); the change itself calls no guard.
- **A change of shape in play against an unchanged run and against a level started at the new shape** (`change.sh`, 12 cases: p3-0 (level 8, 256 tall, margin fill) 288 to 512 at 4300 and 512 to 288 at 4600; boss16-2hits (level $10, the locked boss room) 288 to 512 at 8300 and 512 to 288 at 9000; p0-3 (level 3) 288 to 384 at 5200, 512 to 384 at 7200 and 384 to 288 at 9000; p5-3 (level $11's arena, narrower than the view) 288 to 512 at 6200 and 512 to 288 at 5600; p0-0 288 to 512 at 4004 and 5000; p1-1 512 to 288 at 7000), and 16 more by plays run to their ends without pictures (288 to 512 and back at 2 to 4 points in p0-3, p1-1, p2-0, p4-2, p5-3 and boss16-2hits). Every run exits 0. The player's line against the unchanged run's: 0 differ in 10 of the 12 and in every narrowing (all 13 narrowing runs of both sets); in the widening runs where it differs a figure of the wider window is in the changed run alone (p3-0 from 4690, p0-0 from 5467; at boss16-2hits 288 to 512 at 8000, update 8257: kinds $2D at x 528 and $6C at x 464, past a 288 view's reach, are in the changed run only and his health is 5 and not 6; at p5-3 4600, update 6178: his x is 752 and not 751, with a kind $A1 at x 361 and other pieces placed only in the changed run since 6138), and from there on the play is another play, as at any wide view. All four p0-3 plays (13,600 updates) end as the unchanged run does, at level 53 (the screen the play's exit leads to), and so do p1-1 and p2-0 from 512 to 288; p1-1 288 to 512 at 6000 and p2-0 288 to 512 at 7000 end at level 53 where their unchanged runs end at level 37 and at level 6 with health 0.
- **Pictures** (`change.sh`, a picture every update for 12 after the change, against the run started at the new shape): **every picture after a change has the new view's width** (0 of the wrong width in 12 cases of 12 pictures, 24 more at p0-0; the change's own update shows it: `u4005` after a change at 4004). Level layers alone (`ALADDIN_DEV_NO_SPRITES`): p0-0 at 4004, **24 of 24 identical** to the run started at 512 (with sprites too: 24 of 24; 70 updates after the level's load); p3-0 288 to 512, **12 of 12**; p0-3 512 to 384, **12 of 12** (level 3 at 384); p0-3 288 to 384, 9 of 12; in the other cases the player or the camera differ from the other run's by then (the figures of the wider window), and **all the differences are in updates where the two games' states differ: 0 of the others are unexplained**. With sprites each difference is explained by the actors' own bytes (placed by history: p0-3 512 to 384, kind $56 in slot 6 and not 5, 364 pixels at its place, equal to the unchanged 512 run's picture of it shifted by the 64 pixels: 0 differ). **The columns the new view adds: 0 all-black columns in the 6 widening cases of the pictures** (counted per side per picture; 112 columns a side at 288 to 512): the level's art or the fill. A still screen's added columns are black by design (6.1); the boot's copyright card (288 to 512 at 600) and title (512 to 288 at 2600), the Genie wheel (288 to 512 at 200) and the ending's story (512 to 288 at 300): the 8 pictures after each change identical to the run started at the new shape, 8 of 8 each.
- **The cost of one change** (a headless run, 19 plays, 288 to 512 and back, a level maps' fill made again each): 1.2 to 169 ms, median 15.8 ms, 19 of the 38 over 16.7 ms (four runs at once, so a little slow); the most at p1-0 (169 ms), p3-2 (146 and 97 ms), p6-0 (112 and 97 ms) and p4-1 (86 and 91 ms). A change that makes no fill: 0.005 ms median, 0.017 ms worst (the 105 of 112 steps of a drag that do not cross a margin depth), against 16.7 ms.
- **A drag of 120 updates** (288 to 512 at 2 pixels a step on p1-0, p0-0, p3-2): every update applied: 112 changes, 7 of them with the fill (14 maps made again), 669 ms in all on p1-0 (worst 153 ms; 7 changes over 16.7 ms), 99 ms on p0-0 (worst 25 ms; 2 over), 788 ms on p3-2 (worst 129 ms; 7 over). Through the rule as built (a window of 1440 x 1080 to 1920 x 1080 over 120 updates, the target held for 6 updates before a fill): 1 change, 2 maps made again, at update 4223 for a drag whose last step was at 4219: 78 ms on p1-0, 9 ms on p0-0, 108 ms on p3-2.
- **In a window** (the macOS build, `ALADDIN_DEV_VIEW=1`, level 3 by the title's level select, 4:3 window of 1,776 x 1,332 pixels, then the display's 5,760 x 3,240): `view 288` in the window; by the pause menu's FULL SCREEN item, with the menu open, `view 384` at update 4,367 (2.0 ms for the margins) and back to `view 288` at 4,807 (1.7 ms); by the system's own keys (control, command, F, sent by the System Events), `view 384` at 4,991 (2.0 ms) and `view 288` at 5,539 (1.7 ms); QUIT by the menu: exit 0. Each change was made by the rule after its wait for the window to settle. How it looks was not judged here. A first run whose keys missed QUIT was ended by its timeout (exit 142, a mistake of the input file, kept out of the numbers).
- **Not checked:** a drag by the mouse in a real window (the engine's resize events were driven only by the full-screen switches); the Windows and iOS builds; a change while a level loads (the view then waits for the load, by `level_loading`, which no run reached); a window's 4:3 and 16:9 pictures judged by eye.

**Found, not part of this change:** the same figure in a different palette slot is drawn in different shades (level 3's kind $56 in slot 5 and in slot 6, 364 of 1,400 pixels at update 7201); not looked into. A fresh load at a different width puts the figure in another slot (view 288: slot 1 of 5, 336 and 384: slot 5 of 7, 512: slot 6 of 8).

### 4.9 Runahead, saves and the title with the view following the window (2026-11-23, after merging the snapshot, the saves and the level select)

**State the change added or made changeable mid-level, and how each is covered:**

| State | Runahead | Snapshot (save slots) |
|---|---|---|
| `view` (`View`) | `between_keep(&view)`: saved with the real world, put back; changed only outside ahead steps (at an update's start, after the put-back) | a kept part, and `TAG_VIEW` with the slots open: `video_view_adopt` puts the saved view in first, which sizes the lists again |
| `layer_map[2]`, with `made`, `lcols`, `lrows`, `reach`, `pad`, `fill_ox` | kept (`between_keep`, a struct copy) | `level_maps_snapshot_*`: the record's numbers and the words; the six fields are made again at the load: the layout's cells from the loaded game state (`layer_layout`), `pad = mx / 2`, `fill_ox` from the adopted view (the view and the margins change only together, so it is that view's depth), `made = on` |
| `level_cells[2][]`, `field_px` (the layout as the level loaded it) | written only by a level's load, which is not run ahead | made again by the load from the game state it brings (a screen map place patched in play is then in them as it is: the margin beside such a place may differ from one a fresh level load made, a picture only) |
| the level maps' words | the log of `LevelMapsTile`/`LevelMapsPlace` (as before); a change of the view is made outside ahead steps | as before |
| engine map objects | one per level and layer (the padding): a change makes none | made by `map_of` for the saved size as before |
| `rebase_due` (placing) | `RUNAHEAD_KEEP`: saved and put back | the statics section (all `RUNAHEAD_KEEP`s) |
| sprite lists (`oam.c` budget and arrays, `video.c` shown lists) | the shown lists are kept parts; a change resizes them outside ahead steps and `between_save` takes the sizes it finds | `oam_snapshot_*` and the kept parts; `video_view_adopt` sizes them for the saved view |
| palette slots open | only ever grow | `slots_adopt` |
| `video_view_follow`'s wait (`follow_held`, the pending target) and `follow_at_once` | not game state: written only in `update`, after the put-back | not saved; a load sets the wait to none |
| the developer's lists (`ALADDIN_DEV_VIEW_AT`, `_WINDOW_AT`) | process state | not saved |

**A save at one shape loaded at another** (the window was resized between, or the save came from another machine). Today, as built: the load puts the saved view and everything made for it in (the maps with their margins and padding, the lists, the spawn window in the statics), so the world is whole at the saved shape; at the next update's start `video_view_follow` finds the window asking for another view and changes to it, **at once** (a load is a cut: no wait for the margins) by the rule of 4.8: the margins made again, the spawn window set again. The rule is *the loaded state is rebuilt for the current window*; no snapshot is refused for its shape. A save from a wide window loaded in a narrow one gives the narrow game: what the narrower view no longer holds ends by its range tests; one from a narrow window in a wide one places the figures the wider window holds (4.8). Headless, with no window and no `ALADDIN_VIEW`, there is no window to follow and the saved view stays (the exact checks of SAVES.md run so). A window never gives the original's 256 x 224, so a view of 288 or more is not taken down to it (a developer's `ALADDIN_VIEW=256x224` loading a save of a wider view keeps the saved one). **For Jeff:** the alternative is to keep the saved shape until the window changes again, which leaves a loaded game cropped to the shape it was saved in, as the game was before 4.8. Not proposed: it is the behaviour Jeff asked to end.

**Checks on the merged tree (main `f1aa676` into this branch, 2026-11-23; headless and Metal builds of it):** `same.sh` against main at 256x224: 21 runs, 0 differ; `wide.sh` at 288, 384 and 512: 21 runs each, 0 failed, 0 triggers past their limit; `runahead/state.sh`: 21 runs, 0 differ. `change.sh`, the 12 cases of 4.8 with and without sprites: every run exits 0, every picture from a change's update on has the new width (first picture included: the merge's stepped "was" made the first one the old width until `between_reset_part`), the layer pictures identical to the run started at the new shape in p0-0 24 of 24, p3-0 12 of 12, p0-3 512 to 384 12 of 12 and 288 to 384 9 of 12, and in the others every difference where the games' states differ (0 unexplained, with sprites too). `saves/exact.sh` (7 runs): 6 the same, the planted fault caught. `snapshot/cross.sh`: p0-0 at 4,300 in the title and in p2-1, boss16-2hits at 8,350, p3-1 at 8,850 (Mode 7), p2-1 at 6,000 in p0-1, and the one-process round trip: 0 of 200 (100) updates and 0 of 198 (98) pictures differ in each. `snapshot/pointers.sh` passes. **Saved at one shape, loaded at another** (`shape_load.sh`, 600 updates after the load): the same shape, p1-1 saved at 5000 and loaded in p3-0: player, video and actors 0 differ; saved at 288 and loaded in a 512 window: the player's line is the same for 414 updates and then differs (186 of 600; a figure of the wider window), video 0 differ; saved at 512 and loaded in a 288 window: player and video 0 differ (the actors differ from the load, as narrowing ends them); boss16-2hits at 8,300 loaded in p0-1: 288 to 512 and 512 to 288 both 0 differ in player and video; loaded at the title: 288 to 512 as p1-1 above, 512 to 288 0 differ.

**Save files made before this change** are refused whole at their load: the snapshot's layout mark folds in the registered statics' names and the kept parts' sizes, and `rebase_due` and the grown `layer_map` change it (SAVES.md: a file of another layout is refused, the game goes on).

**The title, the level select and the save screens after a resize while shown.** The title's whole-view branch of `video_draw` reads `view` at each picture and has no map or list of its own, so the view alone changes: the title at 2650 and the level select open at 2900 (by keys, `native/checks/view/level_select_open.txt`), 288 to 512 and 512 to 288: the 8 pictures from the change's update on are identical to the run started at the new shape (the first has the new width: 8 of 8 each). The level select and save screens (the pause menu's `menu_draw`, `level_screen.c`) are drawn over the picture from the window's size and their own state, not the view's, so a resize moves them with the window and does not touch their state; their pictures are not in the headless renderer (it draws no overlays), so they were not compared.

## 5. Where there is no more level to show

The level data sets the bounds.

- **Only 256 px tall:** levels 3 (in effect), 6, 7, 8, 9, $0D, $10, $11, and level $0A's top corridor.
- **Narrower than a wide view:** the snake's arena ($11, 768 px, locked to a screen), level $0A's 256 px shaft beside empty
  screens, level 4's shaft, the boss room of $10 (locked at $11A0 to $12A0), and the chain arena (kind $81, held at $200).

**Jeff (2026-11-22): no black.** Analyse how cleanly the space can be filled with matching pixel art, repeated or
generated. The analysis below was made from the whole-level pictures of the developer's map dump (`ALADDIN_DEV_MAPDUMP`, every
level the game loads, checked against the game's own pictures for levels 0, 4 and $10), measuring where the art repeats
exactly. It is a proposal; nothing of it is built.

**What the measurements show.**
- **Locked rooms in the middle of a level have real level on both sides.** Level 3's room ($0500-$05FF) is between streets, and
  level $0D's treasure room ($0A00-$0AFF) is between caves. A wider view centred on the room shows that real map. Nothing needs
  making; the lock stays a gameplay lock (the player cannot walk out of it). The arenas' left sides ($10's room, $11's arena,
  kind $81's) are the real corridors that led to them.
- **Past a level's end, the floors repeat exactly.** Measured band by band, the floor, brick and spike rows repeat every 32 px
  without one differing pixel next to the rooms of $10 and $11 and level 3's end. Continuing them by that period is seamless,
  and every pixel is the game's own.
- **Many backdrops repeat exactly across their whole width:** levels 0, 3 and 6 every 512 px, levels 8 and 9 every 768. The
  whole-level maps already wrap them, so they continue past the level's edge with no seam at all. The snake arena's backdrop
  (arches and pillar groups) repeats every 384 px with 717 of 98,304 pixels differing (0.7%, small ornaments): wrapping it at its
  768 px puts the seam where the art already meets itself.
- **Organic art has no period:** Jafar's cloud band (measured: no period up to 256 px with fewer than 6,000 differing pixels),
  the caves of $0D, the sky beside level $0A's shaft (its backdrop is mostly empty places, so it shows one flat colour). Plain
  repetition shows a seam there.
- The very edge of most levels is unique (an exit, an end wall, a sign): the whole-height columns of the last 256 px repeat in no
  level. Repetition must be taken band by band from just inside those features, not from the last column.

**Proposed fill, in order of preference, all decided per level at load and the same in every play:**
1. Real map where the level has it (locked rooms, arenas' approaches).
2. Exact band-by-band repetition: each 16 px band of the play field continued at its own measured period, taken from the
   nearest stretch that repeats exactly; backdrops that repeat continue by wrapping. Seamless by construction, every pixel the
   game's own, nothing new.
3. For bands with no period (organic art): new arrangements of the level's own 16 x 16 tiles, chosen so every tile sits only
   next to tiles it sits next to somewhere in that level (the adjacency rules read from the level's map), from a fixed seed.
   Every pixel is still the game's own; only the arrangement is new. Only decorative tiles are used (no collision behaviour), so
   the fill is scenery: nothing can be stood on, no actor is placed in it, and the camera's limits are the level's own (the fill
   shows only where the view is larger than the level or the room).
4. The level's backdrop alone, where the play field has nothing to give (level $0A's empty screens): what the original shows
   behind every empty place.

Each level's result is shown to Jeff as pictures before it lands. The view is never taller than the original's, so only the left
and right margins are seen; the top and bottom ones only where the original itself scrolls past the map's edge.

### 5.x Fill on four sides, completed by context (2026-11-22; landscape only)

- Each level map has a margin as deep as the view shows, made where the level loads and made again where the view follows the window and the cell count changes (4.8): across, ceil(view.dx / 16) + 1 cells each side (2 at 288, 9 at 512, 17 at 768), the backdrop's 8 more (the original's own window goes 128 px past its layout in levels $0F and $10); the play field has none above or below (its window never leaves the layout in the 19 plays: the camera's limits), the backdrop 7 cells (the original's goes 96 px above level $10's). A wrapping band backdrop has none across. The top and bottom margins are made by the exact bands only. The level's (0, 0) is at the map's margin (`level_maps_margin`). The fill is deterministic at a given view.
- The bar: every pair of metatiles that touch, across or down, in a margin or across its seam with the level, should occur touching the same way in the level's own layout. A prop cut by the edge is complete when no seam pair is unseen. `FillReport.unseen` counts the pairs that are not, by side; the dump prints them (`NN-fill.txt`) and the fill4 output tabulates them (`EVIDENCE.md`). A pair is seen if it occurs anywhere in the level, so the bar does not catch a straight edge made of two tiles that each border the other somewhere; a 2 x 2 block bar would (measured for one region in `WHY.md`, not enforced).
- Left and right at a 512 view, levels 0 to $11, both layers (72 sides): 55 are at 0 unseen pairs and 48 at 0 unseen 2 x 2 blocks; 209 unseen pairs and 621 unseen blocks (of 22,244) remain. The sides that cannot be 0 are those arc consistency or the exhaustive search shows cannot be (`EVIDENCE2.md` in the fill4 output).
- The 2 x 2 block bar: every 2 x 2 block of touching cells in a margin and across its seam occurs in the level (a block with an empty place is not judged). The search takes it first as a constraint (a cell that makes a block the level never has is refused, a dead end backing out across depths, a smaller node budget), and where that finds nothing it takes the pairs alone and tries blocks last, so the broken blocks are few and not forced. The cost is under 70 ms a level in the worst case (`LOADTIME2.md`).
- The methods, in order: B (exact period, the nearest stretch that repeats), kept only where every pair it makes is the level's, then the exact search: constraint satisfaction over the side's cells (depth 1 to 64 by band), domains the metatiles a chain of the remaining cells can follow, constraints the level's across and down pairs (the seam included), arc consistency, then depth-first assignment in depth-major order with the value order by context (the copy the neighbours are part of first, then the best 8 x 7 cell context match, then a seeded hash), each choice propagated and a dead end backed out across depths. A side that cannot be made to the end is made to the deepest depth that can (so the pairs the level never has are the farthest from the seam), and the rest by a second search with the seam pairs let go. So a prop the edge cuts is copied whole from where the level has it whole: level 0's cut window at the left edge gets its left part from level columns 126 to 129.
- Empty places (negative screen entries) inside a layout stay blank: made like the margin they change the 256 x 224 pictures (the original draws them blank), so a view that shows them (level $0A's shaft beside its empty screens) shows them blank.
- No collision kind is looked at (the margins are pictures); the level's own map is not touched.
- At 256 x 224 the pictures are the original's: 19 plays, 4,569 pictures every 50 updates, compared with the ring-based game's, 0 differ.
- Load time (the game's own clock, play field and backdrop together): before 0.7 to 5.5 s a level, after at most 145 ms at a 512 view (162 at 288, 249 at 768, `LOADTIME2.md`). The saving: no top or bottom margin (70 to 97 percent of the cells), a margin 7 times shallower, the pair relation of the depth at a time path made only where it runs, and its states cut to 24.

Runs and the copy fallback (2026-11-22). The pair and 2 x 2 block bars let a flat column pass (level 6's salmon floor tile repeated down a margin: every pair and block of it occurs in the level). A search that has to let the seam go also had no context to choose by. Three changes: a run bar (a tile may not repeat along a row or column for longer than the level's own longest run of it); candidates for a relaxed seam are sampled from the level's whole context; and a left or right side the search cannot make is laid with the level's own columns (the window with the fewest unseen pairs and runs at the seam), never one with an empty place. What the minimisation prefers is that the margin looks like the level (its own runs and columns), then that the pairs and blocks are the level's; a few unseen pairs of real art are better than a flat strip.

### 5.y Art the original's window never reaches (2026-11-22)

A layout can hold cells the original's 256 x 224 window can never show in that level (level $10's backdrop from x 4608 on: the flames of its last
screen). A wide view would show them, which the original never did. These cells count as outside the layout: the fill treats them as margin and
continues the reachable art over them (`level_maps.c` layout_reach_x, called by build_layer).

The reachable columns, by analysis of the code, not by sampling plays:
- The camera's x is at most the play field's layout width less 256 (`tail` in scroll_update.c keeps it to `scroll_limit_x`; a lock only lowers the
  limit, an arena raises it by one screen to that width), and at least 0. So the play field's window is the whole layout: nothing of it is cut.
- The backdrop's scroll x is set from the camera's by the level's handler (`parallax.c`, `g.layer_setup_mode`): half (modes 0, 6, 8), the same
  (2, 4, $14), or for $10 the same up to $1000 and half as fast past it. Its window therefore ends at the scroll at the greatest camera x, plus 256,
  rounded up to a metatile. The banded handlers ($0A, $0C, $0E) wrap their layer, and $12 does not scroll: nothing is cut.
- The window is the same height as the original's, so no row is cut (a row the original's window never reaches is never in the view either).

Where it cuts (layer 1, the backdrop; the width reached of the layout's, in pixels; art cells cut): level $01 1408 of 1536 (256); $04 2176 of 2304
(384); $08 and $09 2176 of 2304 (128 each); $0A 1792 of 2048 (256); $0C 1664 of 3072 (4,224: no backdrop layout of its own); $0E 2176 of 2304
(256); $10 4608 of 4864 (256); $12 4224 of 4352 (256); $16 2176 of 2304 (384); $18 2176 of 2304 (128); $19 1792 of 2048 (256); $1C 4224 of 4352
(256); $1E 2432 of 4608 (2,176); $1F 4224 of 8192 (3,968). Level $0F's backdrop is 1536 and the window reaches 1664: it goes past the layout, as
the margin's depth already allows for.

Level $10: the room's camera is $11A0 = 4512, so BG2's scroll is (4512 - 4096) / 2 + 4096 = 4304 and its window ends at 4559; the flames begin at
backdrop x 4608, 48 pixels past the original's right edge, which is 16 pixels inside a 384-wide view (dx 64): view x 368 to 384. At the level's
end the camera is 4608, BG2's scroll 4352, the window's end 4607: the flames are never in the original's window.

`ALADDIN_DEV_OVER=FILE` also writes, per level and layer, the window's least x and greatest right edge a play reached and the reach (checked: no
play's window exceeds the reach, native/checks).

### 5.z Strip quilting of the left and right margins (2026-11-22)

Cell-by-cell synthesis passes the pair and 2 x 2 block bars and still cuts props (a barrel loses its top: each cell is right with its neighbours and
the whole is not the level's). The left and right margins of a side whose rows are not all exact (B) or empty (E) are therefore made by quilting on the
metatile grid (`level_fill.c`, quilt_side):
- The margin is built outward from the level's edge by whole full-height strips of the level's own columns (12 wide), so a prop is whole down a strip.
- Each strip is chosen by how well its first 6 columns match the 6 already placed (the level's edge, or the strip before): the cost is the cells that
  differ, and the seam's.
- The strip is joined along the cheapest seam through those 6 columns, row by row (a dynamic programme over each row's switch point; the cost the
  level's unseen pairs, 100 each, and unseen 2 x 2 blocks, 30 each, the cut would make), so the cut goes round props.
- No strip's source overlaps another's, none is the same distance from its copy as another (no fixed period) and none is next to its own copy; among
  the strips costing within a few of the best one is picked by chance, so no two margins repeat.
- The exact-period continuation (B) stays where the level itself repeats exactly; a side with an empty place running in from its edge stays with the
  exact search; a quilt costing more than a few unseen pairs more than the margin it replaces is not used.
- Margin cells made this way are not mended cell by cell (repair), which would cut props again.

### 5.w Props cut in half, doubled, and outside the playable area (2026-11-23)

Jeff's report: "Some levels still have weird prop cutoffs (half barrels at start of 1-2, double exit sign end), etc." and, with a picture of level $0A's start at 512 wide (the shaft with its two golden walls): "the background needs to be the full width of the screen (repeating is fine if it can be) and the additional props outside the playable area gone".

**Names.** "1-2" is the level select's PART+1 - STAGE+1, so part 0, stage 1: level `$01` (play `p0-1.txt`, `fill_shots.sh` name L01). Jeff's picture is level `$0A` (part 3, stage 2, "4-3").

**Evidence.** Pictures of every playable level (`$00` to `$11`) at its start and exit at 288, 384 and 512 wide (`native/checks/view/fill_shots.sh`), and the map dump of every level the game loads (32), before (`ALADDIN_DEV_FILL_BASE=1` runs the fill as it was) and after, by `fill_audit.sh` (below). Sheets: `shots/fillfix-WxH-LNN.png`, one per level and view, four pictures stacked: start before, start after, exit before, exit after, the original's 256 x 224 frame outlined (dashed magenta on its left and right columns).

**The faults, before (512 wide; the audit's counts are over the level's whole map, so they are the same at every view where the margin is as deep).**

| # | Fault | Where | What it is | Cause |
|---|---|---|---|---|
| F1 | Exit sign twice | `$02`, `$06`, `$09` ends (seen in the pictures; the audit finds one-off copies in the other level ids that carry the same maps) | map art (two metatile cells: the sign's head and post), the level's one sign; the second is a margin copy 6 columns after the level's edge | A: quilting takes a strip of the level's own columns that holds the sign, to continue the ledge it stands on |
| F2 | One-off pieces of the level's art twice | by the audit, copies of cells the level has once, in the margins: `$00` 18, `$01` 23 (and 9 in its backdrop), `$02` 12, `$03` 15, `$04` 20, `$05` 7, `$06` 28, `$07` 5, `$08` 10, `$09` 9, `$0A` 31, `$0E` backdrop 3, `$0F` 1, `$11` backdrop 11 (202 over `$00` to `$11` at 512; 349 over all 32 levels) | map art | A, as F1 |
| F3 | Strip boundary inside a prop | 13 at 512 over `$00` to `$11` (19 over all 32): `$01` backdrop 7, `$05` 1, `$07` 1, `$09` 4, and in the maps `$12`, `$13`, `$17`, `$1C`, `$1F` | map art cut where two strips meet | B: the seam's cost counts pairs the level never has, not a prop cut in two whose halves are each seen |
| F4 | Props and a flat background outside the playable area | `$0A` start (Jeff's picture): genie heads, vases and rails left of the left golden wall; right of the right wall and left of the left, only the plain sky colour where the shaft's backdrop (white pillars, a cloud) stops at the walls | fill (quilt of the shaft's columns) left; the layout's own empty places right (the backdrop has the pillars in its first screen column only, then empty places; the field the same) | C: the left margin is made from a level whose edge is a wall; D: the backdrop's empty places stay blank |
| F5 | Half barrels at the start of 1-2 | `$01` start: level columns 0 to 1, rows 41 to 43 (the barrel at x 0 to 14 px is cut flat at the left, 7 px of it missing; the barrel next to it, x 18 to 39, is whole) | **map art**: the map begins in the middle of the first barrel. Its three cells (one-off) have no left neighbour in the level, so every pair they make with a margin cell is unseen | E: the map's edge cells are taken out in a wide view |
| F6 | The same, elsewhere | the audit counts the rows where the level's own first or last column holds scenery that no cell of the level sits beside on that side: `$00` 3 (left), `$01` 3 + 4, `$02` 8, `$04` 10 + 3, `$05` 3, `$06` 11 + 1, `$07` 5, `$08` 3 + 2, `$09` 3, `$0B` 10, `$0D` 2 + 4, `$10` 2 + 1 (165 rows over all 32 levels at 512 before) | map art | E |
| F7 | A sprite cut or missing at the frame's old edge | none seen in the 60 pictures per view (looked at level by level); actors were not counted by a tool | | |
| F8 | Other seams | the backdrops that repeat every 512 px (`$00`, `$03`) show the same stall twice in a view (a stall each 512 px) | backdrop art repeating by its own period; it is the original's own art repeated, not a cut | none (left as it is) |

**Why the fill cannot complete F5 and F6, and what is done instead.** A prop the level's map cuts at its own edge could be completed only by a cell whose art continues it. For the barrel: the missing 7 px would have to be a cell with a barrel's left edge at pixel 9 of the cell. A pixel search of level $01's whole map for the left 7 columns of the whole barrel (x 18 to 24, rows 659 to 690) finds it at x 18, 514 and 562 only, all at pixel 2 of a cell (x mod 16 = 2), never 9: completing it would be made-up art (a shifted or mirrored copy of tiles), which the rules forbid. **Jeff (2026-11-23): "Just get rid of half barrels anywhere. There's no reason to even have them, since we can layer props anyway if we want to partly occlude them".** So rule E takes them out.

**The rules (each holds for every level; no list of levels, no hand patch).**

- **A, one-offs.** A *one-off* is a metatile the layer's map holds in exactly one cell. A margin never holds one: (1) a quilt strip that holds a one-off keeps its place (it is still the best continuation of the ledge) and the one-off's cell in the margin is read as the nearest cell of the same row that is not a one-off, going out then in, the level's edge cell included, so a sign over a ledge simply is not there twice; (2) after all methods, a margin cell that is a one-off is mended like any cell that breaks pairs (`repair`), from the metatiles that are not one-offs, whichever method made it. Expected: F1, F2: no copy of a one-off in any margin of any level at any view.
- **B, no cut through a prop.** A *prop* is a 4-connected run of the layer's scenery cells (not blank, not drawn empty, and in the play field not solid) at most `Q_W - Q_O` = 6 columns wide: no strip is narrower than that, so a prop always fits in a strip. A strip's seam through a row is charged `Q_CUT` = 300 (three unseen pairs) where the cell before it continues into the next one of a prop, or the cell after it comes from the one before; the cheapest seam goes round props or takes them whole. Expected: F3 gone (0 boundaries inside a prop).
- **C, a wall ends the playable area.** A side of the play field whose edge column is solid in every row (a wall: `$0A`'s left edge, 128 of 128 rows; no other level's edge is: the fullest of the rest is `$0D`'s, 12 of 16) has no margin: the margin is empty, so the backdrop alone is shown past the wall. Expected: F4's props gone (level `$0A`: nothing of the field left of its left wall in any view).
- **D, the backdrop's empty places, tiled in a wide view.** In a wide view the backdrop layout's empty places (places the layout does not hold, where the original shows the plain colour) are made like the margins: in each row, a run of empty cells is filled by repeating the row's nearest non-empty place (16 cells) across it, the cell before it in the run, so the art repeats with the period of a screen; a row is tiled only where the pair of cells the repeat makes at its seam is one the level has side by side (`seen_across`), the others stay empty. The margins then continue from these cells. At 256 x 224 nothing changes (the voids are the original's; `FILL_TRIM_VOIDS` documents why the first attempt, which tiled them there too, changed pictures). Expected: `$0A` start at 512: the shaft's pillars and cloud across the whole width, where the original shows plain colour past the golden walls.
- **E, no half props at the level's edges (wide views).** In a wide view a cell of the left edge column that is scenery (not blank, not drawn empty, not solid) and that no metatile of the level sits left of anywhere in the level (its only neighbour is the one on its right: the map begins in the middle of the prop) is replaced by the level's commonest metatile that draws nothing; of the right edge column, one that nothing follows anywhere. Both layers. The cell is the map copy the engine draws, not the level's data: collision is not touched (only non-solid cells are). At 256 x 224 nothing changes. Expected: `$01` start at 512: the barrel at level x 0 to 14 px (rows 41 to 43, column 0) is gone and the whole barrel beside it stays; the same for every level's edge column.

**As built.** `video/level_fill.c`: `oneoff_cell`, the sanitising pass at the end of `quilt_side`, `repair` (A); `find_props`, `q_cut` and `Q_CUT` in `quilt_try` (B); `fill_wall` (C: the margin is not made, `walled()` skips it in the counts and in `repair`); `tile_voids` (the backdrop) and `trim_edges` (both layers), run when `LevelFill`'s `wide` argument is set, by `level_maps.c` in a wide view (D, E); `fill_audit` and `FillReport.oneoff_*`, `props`, `cuts`, `edge_cuts`, `edge_solid`, printed by the map dump (`NN-fill.txt`, the line "audit (VIEW.md 5.w)"). `ALADDIN_DEV_FILL_BASE=1` runs the fill without A to D (the check's "before"); `ALADDIN_DEV_AUDIT=1` prints each strip boundary inside a prop. `native/checks/view/fill_audit.sh DIR` totals a dump's audit lines; `native/checks/view/sheet_frame.py` makes the sheets.

**Checks, run by me (2026-11-23; the 21-run suites are the orchestrator's).**
- Audit (`fill_audit.sh` over the map dump of all 32 levels, before = `ALADDIN_DEV_FILL_BASE=1`): copies of one-offs in margins 349 -> 0 at 512, 235 -> 0 at 384, 108 -> 0 at 288; strip boundaries inside a prop 19 -> 2 at 512 (levels `$13` and `$1F`, backdrops, one row each; not in the plays or `fill_shots`), 9 -> 0 at 384, 6 -> 0 at 288; level-edge rows with a prop the edge cuts 165 -> 137 at 512, 164 -> 127 at 384, 152 -> 127 at 288. The rows that remain are cells that have a neighbour on that side somewhere in the level but not the one the margin gave them (the pair is unseen); they are not half props by the test of E and were not looked at one by one.
- `fill_shots.sh` unchanged: 60 pictures at 384, 0 failed. The sheets (`shots/fillfix-*`) are 18 levels x 3 views, rebuilt after rule E. Looked at by eye after the last build: `$01`, `$02`, `$04` at 512 (the others after the earlier rules: `$00`, `$05`, `$07`, `$09`, `$0A`, `$0B`).
- 256 x 224: the traces (`same.sh`'s command line, one run each) of plays `p0-1` (8,726 updates) and `p3-2` (10,940, level `$0A`) are byte for byte those of the build of `9f1df9f`. Not run: the other 19 runs, `wide.sh` at 288, 384, 512 (the machine lock was held by other sessions; the orchestrator runs them on landing).
- Not verified: the look in a window; the Direct3D 12 and Vulkan builds; level `$0A` at 288 and 384 by eye (rules C and D are in the map, by the audit and the 512 picture).


## 6. Screens and effects that are not the level

### 6.1 Still screens

The copyright card, logo, title, opening story, story cards, STAGE CLEAR, password, options, continue, the Genie wheel, the
ending story, credits and "The End" are each one 32 x 32-tile picture, or Mode 7 from such a map, with actors at fixed screen
places. None can show more without new art.

**Proposed (decision 4):** each is drawn as the original's 256 x 224, centred with `origin_x/y` = (Δx, 0), with
`show_*` black around it. Per-line tables and windows are offset by the origin. The ending's stages are real levels played
back, so they take the level view.

### 6.2 HUD

The HUD is BG3 text tiles in two groups: the left group at x 8-79, y 8-39, and the right group at x 168-247, y 16-31
(`video/hud.c`, 81:F58D). BG3 wraps, so a wider view would repeat it.

**Proposed:** draw the HUD as screen-anchored pieces, with the left group at the safe area's top-left corner and the right
group at its top-right (`nshvc_display_info`'s safe area). The rest of BG3 in play (pause text, messages) stays centred in
the original's 256 columns. Level $11's 33-line HUD band (80:B9C3-B9D4) becomes the first 33 lines of the view.

### 6.3 Effects

Every effect is keyed to 224 lines and 8-bit columns (`video/lines.c:15,57`; `g.window_edges` is uint8; effects 8, 10 and the
circle wipe are gated to < $100 and < $E0; `circle_wipe.c:59-80`, `iris_close_task.c:28-53`). In levels:

| Effect | In a W x H level view |
|---|---|
| Circle wipe, iris (level ends) | Centre in view terms; radius grows to cover the view's far corner; edges 16 bit. "Full" and "none" become 0..W−1 |
| Effect 8/11 (level 5's end), 10 (level 5's spot) | Same: boxes and spot around the view's centre, 16-bit edges |
| Level 5's IRQ split (lines 0 to 8, or 12, dark in every level: `video.c:483-490`, `level_screen_setups.c:145,488`) | The dark band stays at the view's top, at its line count |
| Level 6: colour gradient over the top 96 lines, 11 parallax bands of 182 lines, sprites off from line 181 (80:BA7E, 80:B991, 80:B99E) | Lines are not moved (Δy is 0), and the last band and the colour hold to H. The sprite cut at 181 is read before it is changed (it may hide the backdrop's art) |
| Parallax band tables (levels 0, 2, 3; `parallax.c:250-262`) | The band split covers H lines instead of 224. The last band already runs to $7FFF |

On still screens, effects keep the original's numbers inside the centred 256 x 224.

### 6.4 Mode 7

The title, the Genie wheel and the ending spin use Mode 7 on still screens. They are centred like the other still screens
(6.1). The plane outside is transparent and shows black.

### 6.5 The title's load-in uses the whole view (Jeff, 2026-11-23; written before it is built)

**The direction.** Jeff: "fix main menu animation load in (aladdin logo and sfx) to properly be resolution agnostic. Right now they get cropped by their original 8x7 instead of using the screen space available to them." This departs from decision 4 (still screens centred at 256 x 224 with black round them) for the title alone; Jeff replaced decision 4 for the title screen only (2026-11-23), asked whether his title direction should replace it: "Yes, replace it"; and on the form, "extend, art edges black" (the conflict was put to him with the pictures `shots/title-loadin-before-*.png`). Decision 4 stands for every other still screen; none shares the title's code path (the flag below is set by `TitleTask` alone, so the opening story, the Genie wheel and the rest are drawn as before). Everything the cartridge's data holds shows; where it holds nothing the picture is black. No art is made or repeated. The Genie wheel, the ending spin and the other still screens stay as decision 4 has them.

**How it meets that, in one line:** the title keeps the original's 256 x 224 as the frame its art and actors are placed in (centred, `origin` = (dx, dy)), and stops cutting the picture to that frame, so what the engine draws beyond it (the backdrop and its fixed colour, the brightness, Mode 7 outside the frame, the windows' ends) is shown.

**What the pictures show (updates 2556 to 2699 of the boot run, Start at 2400, at 256, 288 and 512 wide, before).** At 288 and 512 the title is the 256 x 224 frame with black bars: the white flash, the fades and the zoom end at the frame's columns (dx to dx + 255); at 256 the zoomed logo is cut by the frame itself.

**The cause, from the code.** `video_draw` (`video/video.c`, the branch for a screen that is not a level) sets `show_left = dx`, `show_right = dx + 256`, `show_top = dy`, `show_bottom = dy + 224`. The engine paints black outside that rectangle after everything else (`compose.c show_rects`, `picture.c:1223-1226`), so nothing outside the frame can show: not the fixed colour's white (colour maths reaches the backdrop, `g.blend_layers` $31/$21), not the fade's brightness, not the zoomed Mode 7 plane's parts beyond 256 columns. It is the only mechanism found that keeps the title to the frame:
- The Mode 7 layer's extent is the 1,024-pixel plane (128 x 128 cells, `video.c` `mode7_cells`), `outside` transparent (`mode7_outside`, set by `Mode7ScreenSetup`): its extent is not the frame.
- The title's menu and CAPCOM rows are not in the Mode 7 plane: the title's per-line effect (`window_effects.c`, target `LINE_MODE`, `lines.c video_lines_tile_mode`) puts the menu's lines in mode 1, where they are BG1's text map, a 32 x 32-tile map that wraps (`draw_layers`, `l.wrap = 1`): drawn across a view wider than 256 it repeats every 256 columns. (Found from the first build's picture, which showed the menu twice more at +-256; the analysis above had it in the plane, which was wrong.) It needs no cut at the frame; it needs `wrap` off, so the map is drawn once at the origin, where the frame is.
- The window edges are the frame's columns plus dx and an edge at the frame's own edge goes to the picture's edge (`lines.c` `video_window_left/right`), as in a level.
- Sprites (kinds $82 and $4A) are at the frame's coordinates plus dx (`video_draw`) and are not culled at 256 (`sprite_wrap` is not set).

**The change.** A flag `title_view` (`hud_layer.c`, beside `level_view`), set by `TitleTask` after `LevelRamSetup` (which calls `DisplayDefaults` again: set before it, the flag was cleared before any picture was drawn, shown by a probe, removed) and cleared by `DisplayDefaults`, kept for the picture as the update before's (`between_keep`) and with the world (`RUNAHEAD_KEEP`). While it is set `video_draw` keeps the origin at (dx, dy) and sets no `show_*`, and `draw_layers` draws BG1 once (`wrap` 0). Nothing else changes. At 256 x 224, dx = dy = 0 and `show_*` covered the whole picture, so the pictures are the original's.

**Expected pictures, in numbers** (view 512 x 224, dx = 128; 384: dx = 64; 288: dx = 16):
- The flash (updates about 2556 to 2575, the fixed colour at white): every column 0 to W - 1 and line 0 to 223 are the flash's colour, where before only columns dx to dx + 255 were.
- The fade in (brightness) and out (GAME START): the same, whole view.
- The zoom ($D0 to $100): columns dx to dx + 255 equal the 256 x 224 picture at the same update (the central 256 x 224 of each picture is compared); beyond them the plane's parts the zoom brings in, and black where the plane has none (left of plane x 0: outside, transparent; right of x 255: tile 0 of the plane, black if that tile is empty, and checked).
- Settled: the menu, CAPCOM and the logo at columns dx to dx + 255, centred, and columns 0 to dx - 1 and dx + 256 to W - 1 black.
- The central 256 x 224 of every picture at every shape: identical to the 256 x 224 run's picture at the same update, but where the zoomed plane's own pixels reach into it from outside the frame (none: the frame's pixels are the same plane's).



**As built (2026-11-23).** Exactly the change above, in `video/video.c` (`video_draw`'s still-screen branch: the four `show_*` set only while the title is not the screen; `draw_layers`: BG1's `wrap` 0 for the title), `video/hud_layer.c` (the flag), `video/screen_setup.c` (cleared), `game/title_task.c` (set after `LevelRamSetup`). Nothing changes for any other screen. Measured on the boot run with Start at 2400, a picture of every update (headless `--shots`):
- 256 x 224: all 2,700 pictures of updates 0 to 2699 byte for byte as before the change.
- Columns lit, updates 2540 to 2699, before then after: 288 wide 16 to 271, then 0 to 287; 384 wide 64 to 319, then 0 to 383; 512 wide 128 to 383, then 0 to 511; lines 0 to 223 each. 51 updates are lit at both edges of the view after, none before (the flash, the fades and the zoom).
- The central 256 x 224 against the 256 x 224 run's picture at the same update, updates 2540 to 2699: 0 of 160 differ at 288 and at 384; at 512, 3 updates (2576, 2592, 2605) differ by 1 pixel each, the same 3 updates before the change (not explained; not from this change).
- Later states at 512 (`shots/title-wide-*.png`; an 11,000-update run compared with 256's): the settled menu (updates 2600 to 3168: central picture equal, nothing lit outside the frame); the return to the title after the demo (4493 to 4912: the lamp, Abu, the smoke, the logo and the menu; central picture equal; Abu runs out to the view's edge); the return after the story (about 20,500 on, no 256 run to compare); the fade to black and the level's fade-in at GAME START (whole view). The demo and the levels are the level's view, as before.
- Checks (the build against b91729e's, run under the lock): `same.sh` at 256 x 224, 21 runs, 0 differ; `behind.sh` (BEHIND_SHIFT=0, whole pixels), 21 pairs, all same; `wide.sh` at 384 x 224, 21 runs, 0 failed, 0 triggers past their limit.
- Not checked: the Direct3D 12 and Vulkan builds; a window, and motion.

## 7. Capacity: the binding limit

- **Records:** 28 in the main pool, shared by placed actors, children and shots (`state.h:239-241`). On static placements
  alone, the peak is 25 at 480 x 270 (level $0D). Children and shots come on top of that. When a pool is empty the spawn fails
  silently.
- **Sprite palette and graphics slots: 4** (`state.h:752`, sprite palettes 4 to 7, tiles $100 to $1C0). A class 2 kind takes
  one slot per kind and a class 4 kind one per actor, and a kind can need 1 to 3. Counted from placement data, the slots
  needed in some window exceed 4 in levels 0, 3, $0C, $0F and $15 at 400 x 224 or 480 x 270. **In those places a wider view
  means enemies that never appear.**
- **Built (V10):** the slots are `ceil(4 * w * h / (256 * 224))` (at most 1,028, the slot tiles' 16-bit numbers), 4 at 256 x 224.
  Slots 1 to 4 are the original's. Slot s of 5 and up has sprite palette s + 3 (entries 256 + 16 (s - 5), engine 6.8) and 64
  sprite tiles of its own in a tile set of its own (`video_slot_tiles`), not in the 1,024-tile set. `Actor.look` and
  `g.kind_look` are 32 bits: the low 16 are the original's, bits 16 up the slot when it is 5 or more (`video/slot_gfx.h`).
  Measured on the plays at 400 x 224 and 480 x 270, failed placements for want of a slot went from 0 to 5 per run (levels 0,
  1, 3) to 0.

This is decision 1.

## 8. How it is checked

- **At 256 x 224, the game is unchanged.** Every stage below is checked by running all of `native/checks/plays/` at the
  original view. Actor state and the player, update by update, must be identical to `b36b712`'s native build
  (`compare.py`, native against native). This proves each rule reduces to today's value. It also covers the parts the
  original never had (the whole-level map, the sprite budget).
- **At wider views:** the game traces legitimately differ (more is live). `native/checks/view/wide.sh` runs the boot and every
  play at 288 x 224 (4:3, the narrowest; a window narrower than 4:3 shows it letterboxed), 384 x 224 and 512 x 224 with
  `ALADDIN_DEV_TRIGGERS=1`: every run must exit 0 with no trap text, and `trigger_check.py` must find every trigger that fired
  (a near test, or the original-screen flag) inside its original limits. Pictures are taken at each level's start, at its
  widest point and at its locks, for Jeff's look in a window.
- **Still screens and effects:** pictures at each view, compared with the 256 x 224 picture cut from their middle. They must
  be identical inside it and black outside.

## 9. Plan

Each stage is one area, landed with its 256 x 224 check passing.

| Stage | Work | Depends on |
|---|---|---|
| V1 | The view: `g.view_*` from the window's shape, `nshvc_screen` width, height and origin, `show_*` | none |
| V2 | Sprites: no 9-bit wrap, y cull to H, budget and draw lists | V1 |
| V3 | Whole-level maps for the play field and backdrop, all tile writers redirected, blank empty places | V1 |
| V4 | Camera: aim, y points, limits, locks, block stops, fall death, level 6's hold | V1, V3 |
| V5 | Range point, `view_box` / `range_box`, the kinds' constants, placements against the camera (after reading the three tables) | V1 |
| V6 | Spawn window and bands | V5 |
| V7 | Player clamps and view edges | V4 |
| V8 | Levels smaller than the view, and room locks (decision 3) | V4 |
| V9 | Still screens centred, HUD anchored, effects in view terms, level 6's and level $11's line tables | V1 |
| V10 | Palette slots (decision 1): built | the decision; engine 6.8 |

Stages V2, V3, V5 and V9 touch separate files and can run side by side.

## 10. Decisions for Jeff

Answered 2026-11-22:
- **1, palette slots:** (a), the engine is asked for sprite palettes. Jeff: any number of sprite palettes, not a larger
  fixed number (engine API 6.8, built on the engine's `sprite-palettes` branch, not merged).
- **2, wake distances measured from Aladdin:** the original's, not scaled (Jeff 2026-11-22: the game is the original's, the view a frame; enemies in the frame stay live but are not triggered past the original's distances).
- **3:** no black: fill with matching art (section 5, analysis made, nothing built).
- **4:** still screens centred, with black around (built). **Replaced for the title screen only, 2026-11-23** (Jeff: "Yes, replace it"; section 6.5): the title's picture uses the whole view.
- **5:** whole-level maps (built).
- **The levels' dark top rows (8, 12 in level 2) are cropped** from the picture, not shown (built).
- **6, the view's size:** the window's proportion (landscape: 216 shown lines and round(216 a) across; narrower: 256 across and round(256 / a) lines; both even, plus the level's crop rows).

Built so far: `src/video/view.h` and `view.c` hold the view, set from `ALADDIN_VIEW=WxH` (a developer's setting) until rule 6 is
settled, and `video_draw` sets the screen to it. `native/checks/view/same.sh` is section 8's check at 256 x 224.

The questions as they were put:

1. **Palette slots.** Wider views need more than 4 sprite kinds at once in five levels, and the engine allows only 8 sprite
   palettes. The options:
   - (a) Ask the engine for more sprite palettes, for example a sprite palette index past 7 with a larger palette. This keeps
     the original rule of one palette per kind slot.
   - (b) Keep 4 slots and accept that some enemies the original would have placed do not appear at wide views.
   - (c) Limit the view's width in those levels.
2. **Player-relative wake distances** (`near_box`, up to ±$80). Keep them as gameplay, so an enemy you can now see further off
   stays idle until you are as close as in the original. Or scale them with the view. The screen-relative ones scale, by the
   direction.
3. **Levels and rooms smaller than the view:** shrink the view on that axis, with black around it (proposed). The alternative
   is to show beyond the level's edge.
4. **Still screens:** centred 256 x 224 with black around (proposed). The alternative is to extend each with a colour or
   repeated art.
5. **The ring replaced by whole-level engine maps:** it removes the height limit and converted code with it. It is the one way
   found to show more than 224 lines.
6. **The view's size rule (section 2):** the original's 256 x 224 at least, grown along the window's longer side. The
   alternative is a fixed number of game pixels per window pixel, which shows more of the level on bigger screens. Whether
   to cap it is part of the same choice, given that the largest a level can use is its own size.

## 11. Not known yet

- What the bottom 32 px of each level's last row hold. The proposed `limit_y` never shows them, as the original.
- The tables at 80:D8FE (kind $97), kinda7's placement table and kind $6E's spawn table.
- Whether `view_box` half-sizes should grow evenly around the range point or only downward. The proposal grows them evenly,
  since the range point moves by Δy.
- Real counts of live actors and slots at wide views. The section 7 figures are from static placements. A probe run
  answers it, after V5 and V6.
- Level 6's sprite cut at line 181, and what level 7's 512 x 512 backdrop shows past 256 x 224.
- Whether a backdrop that does not repeat (levels 4, $12) looks right when wrapped.

## Level $11's floor: offset-per-tile (2026-11-23)

**Fault.** In the snake's arena the original draws the lines below the status band in mode 2 (line table $80:B9C3; BG3's y
$F0 there, $80:B9CA), so each BG2 column takes its vertical scroll from the text map's row 31, where ActorYHistoryPush (84:8EBD)
keeps the riser's last 32 heights: the floor is a wave across the screen. The native game sent the row but never gave the engine
an offset-per-tile map, so every column showed the newest height and Aladdin (whose y was the original's) seemed to stand on air
or sink into the ledge.

**Fix (Jeff's decisions, 2026-11-23).** Engine API 6.14 adds `nshvc_layer.offset_per_tile_own`: the offsets as a table of the
layer's own (row 0 across, row 1 down, column c - 1 for offset column c), not read through BG3 (in a level BG3 is the status
line's own map, hud_layer.c). `video/line_history.c` keeps the heights whole in a list longer than the row (kept with the world,
between.h; in snapshots and saves, so saves made before are refused as any layout change makes them) and builds BG2's table each
picture, in the terms of the map BG2 draws (in the arena the video map $4800, 512 x 256), on level $11 while a mode effect puts
lines in mode 2. A wider view continues the wave to its right edge with the older heights; left of the original's first offset
column the floor is the newest height, as the original's column 0 is (no height exists there). The row 30 entries ($4000) give
BG2's x in whole characters and are the same picture as none; any other would be reported once on stderr.

**Checked.** p5-3 on the original (lbox, a picture and work RAM a native update, 5150-5410) against the native at 256 x 224: the
player's x and y the same at all 261 updates; the floor's rows per 8-pixel column (rows 172-199, below the player's feet), native
update u against the original's u - 1 (the headless picture is the update before's, between.h): 0 rows off in all 32 columns in
65 of 65 updates sampled (5152-5408, every 4th). Before the fix the same measure gave up to 22 rows off. Native traces (player,
every actor) the same before and after the fix at 256 and 398 wide; runahead off against on, world hash every update, p5-3 to
6000: the same. At 398 and 512 the play is a wide view's own (the riser's y differs from the 256 play's from update 5207 in the
build before the fix too), so the wide pictures were looked at, not measured against the original. Engine: the scenes
offset-per-tile-own2 and -own4 (Metal) match bsnes, and fail with the flag off.
