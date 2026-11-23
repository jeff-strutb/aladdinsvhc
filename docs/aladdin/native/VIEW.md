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
- When there is no free record or palette slot, the placement fails silently and is not retried until its band re-enters.

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
shape and section 2's rule. They change only between updates. Every rule below reads them, and no code keeps its own 256 or
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

### 4.4 Range point, boxes and kinds' constants

- The range point is the view's middle shifted as the original's was: `range = corner + ($80 + Δx, $80 + Δy)`.
- Every `view_box` and `range_box` half-size grows by Δx across and Δy down. The margins past the screen's edges stay the
  original's: an actor wakes, is drawn and becomes solid at the same distance past the edge as before, wherever that edge now
  is. This is the "animation and trigger distance" of the direction. Everything the player can see is drawn, can be hit, can
  hit back and runs its scripts.
- Each direct constant in 3.3 gets the same rule: across sizes + Δx, down sizes + Δy, and "screen middle" + Δx.
  `kind6d.c:47` (spawn just off screen) becomes $90 + Δx.
- Placements against the camera (`kind27.c:82`, `kind8d.c:55-56`, `kinda7.c:34`, `carrier_pair.c:42`, `kind97.c:70`,
  `kind79.c:124-128`, `kind6e.c:150-153`) are each read and given their meaning: "from the left edge" stays, "below the
  screen" becomes `+ H + 32`, and "the middle" becomes `+ W/2`. Three tables they use are not yet read (80:D8FE, kinda7's,
  kind6e's).

### 4.5 Spawning

- The window becomes camera −64 to camera + W + 64 across and camera −64 to camera + H + 96 down. These are the original's
  margins, with each axis kept apart.
- The initial band count becomes (W + 128) / 32.
- One band a frame stays as it is: the camera moves at most 3 px a frame across and 6 down, under one band's 32 px.

### 4.6 Sprites

- The 9-bit wrap goes. Screen x is a full int32.
- The y cull becomes −32 to H − 1.
- The 128-sprite budget and the 32-actor draw lists grow with the view. The engine has no count limit and per-line limits stay
  off. The budget is sized so that the original view never drops what it does today.

### 4.7 Player clamps and view edges

- `PlayerKeepOnScreenX/Y` (80:C9B9) becomes 16 to W − 16 and 24 to H − 8.
- `ViewEdgePush` and `ViewEdgeHurt` (`level/view_edges.c:19-68`) measure from the view's edges, and the hurt margin stays $30.

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

- Each level map has a margin as deep as the view shows, made where the level loads (the view changes only there): across, ceil(view.dx / 16) + 1 cells each side (2 at 288, 9 at 512, 17 at 768), the backdrop's 8 more (the original's own window goes 128 px past its layout in levels $0F and $10); the play field has none above or below (its window never leaves the layout in the 19 plays: the camera's limits), the backdrop 7 cells (the original's goes 96 px above level $10's). A wrapping band backdrop has none across. The top and bottom margins are made by the exact bands only. The level's (0, 0) is at the map's margin (`level_maps_margin`). The fill is deterministic at a given view.
- The bar: every pair of metatiles that touch, across or down, in a margin or across its seam with the level, should occur touching the same way in the level's own layout. A prop cut by the edge is complete when no seam pair is unseen. `FillReport.unseen` counts the pairs that are not, by side; the dump prints them (`NN-fill.txt`) and the fill4 output tabulates them (`EVIDENCE.md`). A pair is seen if it occurs anywhere in the level, so the bar does not catch a straight edge made of two tiles that each border the other somewhere; a 2 x 2 block bar would (measured for one region in `WHY.md`, not enforced).
- Left and right at a 512 view, levels 0 to $11, both layers (72 sides): 55 are at 0 unseen pairs and 48 at 0 unseen 2 x 2 blocks; 209 unseen pairs and 621 unseen blocks (of 22,244) remain. The sides that cannot be 0 are those arc consistency or the exhaustive search shows cannot be (`EVIDENCE2.md` in the fill4 output).
- The 2 x 2 block bar: every 2 x 2 block of touching cells in a margin and across its seam occurs in the level (a block with an empty place is not judged). The search takes it first as a constraint (a cell that makes a block the level never has is refused, a dead end backing out across depths, a smaller node budget), and where that finds nothing it takes the pairs alone and tries blocks last, so the broken blocks are few and not forced. The cost is under 70 ms a level in the worst case (`LOADTIME2.md`).
- The methods, in order: B (exact period, the nearest stretch that repeats), kept only where every pair it makes is the level's, then the exact search: constraint satisfaction over the side's cells (depth 1 to 64 by band), domains the metatiles a chain of the remaining cells can follow, constraints the level's across and down pairs (the seam included), arc consistency, then depth-first assignment in depth-major order with the value order by context (the copy the neighbours are part of first, then the best 8 x 7 cell context match, then a seeded hash), each choice propagated and a dead end backed out across depths. A side that cannot be made to the end is made to the deepest depth that can (so the pairs the level never has are the farthest from the seam), and the rest by a second search with the seam pairs let go. So a prop the edge cuts is copied whole from where the level has it whole: level 0's cut window at the left edge gets its left part from level columns 126 to 129.
- Empty places (negative screen entries) inside a layout stay blank: made like the margin they change the 256 x 224 pictures (the original draws them blank), so a view that shows them (level $0A's shaft beside its empty screens) shows them blank.
- No collision kind is looked at (the margins are pictures); the level's own map is not touched.
- At 256 x 224 the pictures are the original's: 19 plays, 4,569 pictures every 50 updates, compared with the ring-based game's, 0 differ.
- Load time (the game's own clock, play field and backdrop together): before 0.7 to 5.5 s a level, after at most 145 ms at a 512 view (162 at 288, 249 at 768, `LOADTIME2.md`). The saving: no top or bottom margin (70 to 97 percent of the cells), a margin 7 times shallower, the pair relation of the depth at a time path made only where it runs, and its states cut to 24.

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

## 7. Capacity: the binding limit

- **Records:** 28 in the main pool, shared by placed actors, children and shots (`state.h:239-241`). On static placements
  alone, the peak is 25 at 480 x 270 (level $0D). Children and shots come on top of that. When a pool is empty the spawn fails
  silently.
- **Sprite palette and graphics slots: 4** (`state.h:752`, sprite palettes 4 to 7, tiles $100 to $1C0). A class 2 kind takes
  one slot per kind and a class 4 kind one per actor, and a kind can need 1 to 3. Counted from placement data, the slots
  needed in some window exceed 4 in levels 0, 3, $0C, $0F and $15 at 400 x 224 or 480 x 270. **In those places a wider view
  means enemies that never appear.**
- The engine's palette is 256 entries, so there are only 8 sprite palettes. The slots take 4 to 7; what holds 0 to 3 was not
  read for this document. The slots cannot be raised by the game alone.

This is decision 1.

## 8. How it is checked

- **At 256 x 224, the game is unchanged.** Every stage below is checked by running all of `native/checks/plays/` at the
  original view. Actor state and the player, update by update, must be identical to `b36b712`'s native build
  (`compare.py`, native against native). This proves each rule reduces to today's value. It also covers the parts the
  original never had (the whole-level map, the sprite budget).
- **At wider views:** the same plays run at 288 x 224 (4:3, the narrowest; a window of 900 x 1600 shows it letterboxed), 400 x 224 and 480 x 224. They may differ, because more is
  live. The check is that every play still reaches its exit, and that the following counters stay at 0: no 9-bit wraps, no
  failed placements for want of a record or slot, no sprites dropped, no ring overwrite (the ring is gone). Pictures are taken
  at each level's start, at its widest point and at its locks, for Jeff's look in a window.
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
| V10 | Palette slots (decision 1) | the decision; maybe an engine request |

Stages V2, V3, V5 and V9 touch separate files and can run side by side.

## 10. Decisions for Jeff

Answered 2026-11-22:
- **1, palette slots:** (a), the engine is asked for sprite palettes. Jeff: any number of sprite palettes, not a larger
  fixed number (engine API 6.8, built on the engine's `sprite-palettes` branch, not merged).
- **2, wake distances measured from Aladdin:** scaled with the view, as the screen-relative ones are.
- **3:** no black: fill with matching art (section 5, analysis made, nothing built).
- **4:** still screens centred, with black around (built).
- **5:** whole-level maps (built).
- **The levels' dark top rows (8, 12 in level 2) are cropped** from the picture, not shown (built).
- **6, the view's size:** Jeff asked for it to be explained.

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
