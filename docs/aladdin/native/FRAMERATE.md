# The frame rate: a 60 Hz game, drawn smooth at any display rate

Status: analysis and plan, 2026-11-22. Nothing here is built. Part of stage 7 of [PLAN.md](PLAN.md) (decision 3: "pictures
between updates" is designed and shown to Jeff before it is built). Jeff's direction: "update game tick rate to 60hz as well as
all animations in game interpolated to a smooth 60hz".

**How this meets the direction, in one line:** the game's logic stays one 60 Hz update (it already is); every picture is drawn
from the state between the last two updates by the engine's `between`, so everything that moves, scrolls, fades or wipes moves
smoothly at the display's rate; and the animations get new in-between art (decision F2, Jeff: "Make new art"), drawn by the picture only, so the game's logic still reads the original's frames.

## 1. What is true now (measured)

Measured on main's headless build (`build/aladdin/native/headless`, macOS) with the play `native/checks/plays/p0-0.txt`
(level 0, 3,530 updates in the level), `ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=1`, every update logged.

| What | Measured | Meaning |
|---|---|---|
| The game's tick | `game.update_hz = 60` (`system/main.c`); `PlayLoop` waits 1 frame a turn and runs one `FrameStep` (`game/play_loop.c`, `game/frame_step.c`) | **The logic already runs at 60 Hz**, as the original's does (the original's own rate is 60.10 Hz; lag frames are gone natively) |
| The player's x | changes on 1,719 of 3,529 updates; when moving, every update (run of 1: 1,670) | motion is stepped every update |
| The camera (scroll x) | changes on 1,614 updates; 1,577 runs of one update | the camera is stepped every update |
| Other actors' places | 5,796 changes over 21,467 actor-updates; of the steps of actors that are moving, 3,610 come after one update, 254 after 2 to 8 (kinds $2F, $50, $08, $12, $30...) | 93% of motion is per update; the rest are slow movers under 1 px a frame |
| Places as drawn | 8.8 fixed point in the game (`a->x >> 8`), drawn at whole pixels: `nshvc_sprite.x/y` and `nshvc_layer.scroll_x/y` are `int32_t` game pixels (`nativeshvc/include/nshvc/nshvc.h:253,274`) | a slow mover steps a pixel every 2-8 updates; at 4x scale that is a 4-pixel jump |
| Actors' cels (the art's frames) | held 1 update 405 times, 2: 154, 3: 275, **4: 616**, 5: 203, 6-8: 316, 9-20: 160 | the art is drawn at 7.5-20 pictures a second, by the animation tables (`actor/anim.c`: a time and a picture per frame word) |
| `draw(between)` | ignored (`system/main.c`: `(void)between`) | every picture is the newest update's |
| The engine's clock | updates at exactly 60.000 Hz by the game thread's clock; pictures at the display's refresh, unsynchronised (`nativeshvc/src/engine/loop.c:440-485`); `between` is how far real time is past the last update | on a 59.94 Hz display one update is skipped about every 17 s; on 120 Hz (ProMotion) each update shows on 2 refreshes but the phase drifts, so some show on 1 and some on 3: judder |

Below 60 Hz in the logic, by design of the original:
- `g.play_wait` ($03FB): `PlayLoop` waits 8 frames a turn during the chain scene's jump (`PlayerSceneCued`): play at 7.5 Hz,
  a slow motion. ($3F for the copier check, never set natively.)
- Stepped effects: the fade a step every 2 frames (`video/brightness_wait.c`), the window effects every 2
  (`video/window_effects.c`), colour bands and text bands every 4, text typed a character every 4, the Genie wheel's slowing
  every 4th frame, the ending's zoom every 16 and 24, palette cycles by their tables.

## 2. What "smooth 60 Hz" needs

Since the tick is already 60 Hz, what a player sees as not smooth comes from four things, in order of how much they show:

1. **Unsynchronised pictures** (the engine's clock against the display's). Fixed by drawing each picture at `between`.
2. **Whole-pixel places.** At a window scale of 3x-5x a slow mover or a slow scroll jumps several window pixels at once.
   Fixed by drawing places with their fraction (the game keeps 8 bits of it), which needs the engine.
3. **Steps of several frames** (the slow motion and the stepped effects). Fixed by easing between the steps' values.
4. **Cels held 3-8 updates.** The art itself. Not fixable by easing: see F2.

## 3. Decisions for Jeff

| # | Question | Options | Proposed |
|---|---|---|---|
| F1 | The tick. It is already 60 Hz. Is anything else meant by "update game tick rate to 60hz" (for example the chain scene's 7.5 Hz slow motion, or the stepped fades)? | (a) the logic stays as it is, 60 Hz, and the slow motion and stepped effects are eased in the picture (3.3, 4.5); (b) the slow motion's logic also runs at 60 Hz (each step's motion split in 8, a change to the game's logic: a departure in DEPARTURES.md) | **Jeff, 2026-11-22: "For now do everything else at 60"**: (a), the logic as it is, everything else eased |
| F2 | The cels. The art has 7.5-20 pictures a second; easing cannot make a picture between two cels. A cross-fade would need blending the engine does not offer for sprites beyond the SNES's colour maths (its draw lists say "There is no blended drawing", `nativeshvc/docs/api.md:309`), and would smear pixel art if it could. | (a) the cels keep the original's timing; everything else is eased; (b) new in-between art for the animations (an art pipeline: generated or drawn pictures, new sprite tiles, longer animation tables, judged by Jeff picture by picture; a large job of its own, per kind) | **Jeff, 2026-11-22: "Make new art"**: (b), section 7; then, no method found (7.6): "For now do everything else at 60. Then we will circle back to more research": (a) for now |
| F3 | Latency. Easing between the last two updates shows the world up to one update (16.7 ms) late, SRFX's way (`stuntracefx/native/stuntracefx/src/video/between.c`). | (a) ease between the last two (exact, +0-16.7 ms); (b) extrapolate past the newest (no added delay; wrong at every turn, jump and stop) | **Jeff, 2026-11-22**: (a), between the last two updates |
| F4 | Sub-pixel places. Needs an engine change (section 5). | (a) yes: places drawn with their fraction at the window's pixels; (b) whole game pixels, eased only | **Jeff, 2026-11-22**: (a), yes, the engine changed for it |

## 4. The design (F1, F3, F4 as decided; F2 (a) for now)

**4.1 The rule.** Game state is never changed by a picture. `update` is unchanged; only `draw(between)` changes. A picture
shows the update before's world, its moving parts eased `between` of the way towards the newest. Headless pictures are at
`between` 0, so every check that compares game state or headless pictures (`native/checks/view/same.sh`, the 21 runs) is
unchanged by construction except for the pictures, which show the update before (section 6 says how that is checked).

**4.2 What is kept per update.** At the end of each update's picture work (`NmiPictureWork`), the drawn state is copied as
"was" before the next update writes "now": the sprite list, the layers' scrolls with their fractions (`g.layers[i].scroll_x/y`,
`g.bg_x/bg_y` 24.8), BG offsets, the per-line tables (`video/lines.c`), window edges, Mode 7's centre, angle and zoom, the
brightness and the colour maths' fixed colour. Art, maps and palettes are not eased: a picture uses the update before's
(SRFX's whole-frame rule), so the art and the places always agree.

On the view branch (`view-frame-int`) the values are kept before the view's own offsets are added, and the offsets are added
after easing: the layers' scrolls before `level_maps_margin()` and `view.dx`; the per-line BG2 x before the margin and dx
(`video/lines.c`); the sprite list before `video_draw` moves it by dx; the crop's rows (`video_cropped_rows()`, 8, or 12 in
level 2) as they are, not eased. `view.w/h/dx/dy` change only at a level's load, so they are the same in "was" and "now".
None of the view sessions touch `main.c`'s `draw`.

Jeff's view decisions this keeps: the view from the window's proportion (216 shown lines, landscape, 4:3 at its narrowest);
gameplay in the original 256 x 224 frame against the original camera; the dark top rows cropped; the HUD inside a band no
wider than 16:9 (it does not ease); and at 256 x 224 the game identical to the original (`same.sh` against base-b36b712,
21 runs, 0 differ). Easing is in the picture only and changes no game state.

**4.3 Sprites are matched by owner, not by index.** SRFX matches sprites by their place in the list. Aladdin's list is rebuilt
every update from four draw lists and the player, and its order changes as actors are made, ended or flicker
(`video/oam.c:OamDrawActors`). So `draw_actor` tags each sprite with its owner (the actor's record and the piece's index);
a sprite's eased place is its owner's eased place plus the piece's offset in the newest cel. An owner missing from "was"
(newly made) is drawn at its newest place; one missing from "now" is not drawn. A place that moved more than a limit (32 game
pixels, as SRFX's `SPRITE_FAR`) is placed anew: a respawn, a room change, a teleport.

**4.4 The camera and layers** ease as the sprites' origin does, with SRFX's wrap rule for a scroll kept modulo a period, and
a snap (the level's start, a room change, `kind $81`'s freeze, a still screen) set anew. The status line (HUD) does not ease.
The per-line scrolls (parallax bands, level 6's colour band) ease line by line.

**4.5 Stepped effects ease between their steps.** A value a task steps every N frames (the fade's brightness, the iris and
window edges, colour bands, Mode 7's zoom and angle) eases over the N frames from its last step to its next. That needs each
such effect to say its step length, so "was" and "now" are the step's two ends, not the last two updates: a small record per
effect, set where the task steps (`video/brightness_wait.c`, `video/window_effects.c`, `video/colour_bands.c`,
`video/mode7_rotate.c`, `game/ending.c`). Brightness has 16 levels on the SNES; an eased fade needs the engine to take a
brightness between them (section 5). Palette cycles and text typing stay stepped: they are discrete art changes, as cels.

**4.6 The slow motion** (`g.play_wait` 8): each 8-frame step's places ease over its 8 frames, by the same owner match, with
"was" kept at the step's start.

## 5. What the engine needs (requests, through ENGINE_REQUESTS.md and the engine's session)

The engine as of 2026-11-22: API 6.8 (any number of sprite palettes) on engine main 21a6c32; 6.9 (`nshvc_display.scale`) on
branch `display-scale` ded0ea8, not merged; nothing else queued. Sub-pixel places are a new request, after 6.9.

- Sprite places and layer scrolls with a fraction: `nshvc_sprite.x/y` and `nshvc_layer.scroll_x/y` are `int32_t` game pixels.
  Proposed: a sub-pixel part (for example 8 bits, the game's own 8.8) drawn at the window's pixels. The engine already has
  float places for draw lists (`nshvc.h:321`, "with sub-pixel precision"), so the renderer can place quads at fractions.
- Per-line scroll values with a fraction (`nshvc_line_field` values are `int32_t`).
- A brightness between the 16 steps (if 4.5's eased fade is wanted).
- Whether the engine's clock should lock to the display when the refresh is a multiple of 60 (an option, not needed if
  `between` is used).

## 6. Stages and checks

| Stage | Work | Check, with the expected result |
|---|---|---|
| R0 | Coordinate (the view coordinator's answer, 2026-11-22): the view work is not on main; it is on the local integration branch `view-frame-int` (8f38960, worktree `aladdinsvhc-wt-view`), and view-fill4 is still editing `video/level_fill.c`, `video/level_maps.c/h` and the margin lines of `video/video.c`. This work branches from `view-frame-int` once view-fill4 has landed there, never from main | view-fill4 merged into `view-frame-int`; no file of this work open in another session |
| R1 | Kept state (4.2), owner tags (4.3), `draw(between)` passed to `video_draw`; whole pixels | `same.sh`: game state identical in all 21 runs (draw-only change). Headless pictures equal the reference's pictures one update earlier (between 0 shows the update before): checked byte for byte on the 21 runs |
| R2 | Easing of sprites, camera, layers, per-line scrolls (4.3, 4.4) | a test mode writing pictures at between = 0.25, 0.5, 0.75: every eased sprite's place lies between its two updates' places, monotonically; no sprite drawn with a missing owner; a planted error (an owner swapped) is caught |
| R3 | Engine: sub-pixel places (section 5), then the game passes fractions | pictures at 4x scale: a slow mover (kind $2F, $50) moves 1 window pixel per refresh step where it moved 4 every 4 updates |
| R4 | Stepped effects and the slow motion (4.5, 4.6) | the fade's brightness and the iris's radius monotonic across their steps' frames in the test pictures; the chain scene's jump eased over its 8-frame steps |
| R5 | Run in a window on Jeff's Mac at 60 Hz and 120 Hz (ProMotion), on level 0 and level 6 | needs Jeff's eye; the numbers we can measure: no picture repeated or skipped against `between` in a logged run |

Not covered by any check here: how it looks to a player; F2's option (b).

## 7. New in-between art (decision F2: Jeff, 2026-11-22, "Make new art")

### 7.1 How much art (measured)

Read from the cartridge's animation tables (the 90 animation sets of kinds $00-$A8, `data/actor_tables.c`'s
`kind_anim_set` and `anim_set_table`; scratch script, not in the tree): **533 animations, 1,840 frame entries, 1,359
distinct pictures.** A frame's time (updates it is held): 1: 46, 2: 156, 3: 195, 4: 260, 5: 158, 6: 115, 7: 117, 8: 222,
9-16: 158, 17-24: 52, over 24: 362 (held poses: standing, waiting).

Counting the motion frames (held 2 to 16 updates), as distinct transitions (set, picture, next picture, time): 1,123.
New pictures needed for each, by the rate the animation is drawn at:

| Animation drawn at | New pictures (distinct transitions) | Against the frame entries |
|---|---|---|
| 60 Hz (a picture every update) | **5,176** | 6,365 |
| 30 Hz | 2,253 | 2,761 |
| 20 Hz | about 1,700 | 1,684 |

Not counted: pictures a kind sets without an animation table, the level's animated tiles (tile slots, palette cycles), the
Mode 7 and still-screen art, and the held poses over 16 updates.

### 7.2 The new art is drawn by the picture, never read by the logic

The game's logic reads the original's frames: kind $20 turns on pictures $09 and $0C (`actor/kind20.c:251`), the guards on
their struck and throwing pictures (`actor/kind42.c:134-140`), kind $52 on its hurt picture (`actor/kind52.c:238`), and
`actor_steps.c`, `player/held.c`, `shot_hit.c` and others on the frame. So the animation tables are not changed: an actor's
frame, picture and time stay the original's, and at 256 x 224 the game stays identical (`same.sh`, 21 runs, 0 differ).

The picture picks the art: an actor's cel is looked up by (its set, the picture it shows, the picture it goes to, how far
into the frame's time it is, with `between`). Because a picture shows the update before eased towards the newest (F3 (a)),
the picture it goes to is known, not guessed, even when the logic switches animation. Where no new art exists for a
transition, the original picture shows, so the art can be added a set at a time.

The new art lives beside the cartridge's, in a new asset file made by a tool from the art's source pictures (PNG): its own
engine tile sets (`nshvc_tiles_create`), the sprite pieces of each new picture (as the original's: x, x mirrored, y, tile),
and the lookup. The actor's palette is the kind's own; a new picture that needs more colours takes a palette slot of its
own (engine 6.8, any number of sprite palettes). Pictures the original streams into a tile slot (the player's,
`video/actor_pictures.c`) are drawn from the new tile sets directly, not through the slot.

### 7.3 Decisions for Jeff

| # | Question | Options | Proposed |
|---|---|---|---|
| F5 | The animation's rate | 60 Hz (5,176 new pictures), 30 Hz (2,253), 20 Hz (about 1,700); the rest of the picture (motion, camera, effects) is eased at the display's rate in every case | 60 Hz, as Jeff's direction says |
| F6 | Who makes the pictures | (a) pixel artists by hand; (b) generated by an in-betweening model, then cleaned to the palette and the pixel grid by hand; (c) generated only | **Jeff, 2026-11-22: "Generate, then hand-clean"**: (b). Section 7.5. Later the same day: "Fixing by hand just won't be an option. I'm not an artist myself." So no hand cleaning is planned: a draft is used only if it is good enough as it comes out of the tool and its automatic clean-up; what happens to the rest is open (section 7.6) |
| F7 | The pictures' size | (a) the original's pixel size; (b) finer than the original's (the engine's tiles take more texels to a pixel, `nshvc_tiles_create`'s `texels_per_pixel`), which would also mean redrawing the original pictures to match | (a) |
| F8 | The rights to new art made from the original's | the art is Capcom's and Disney's; new pictures drawn from it are derived work | Jeff's to settle before the art is shipped; it does not block the tools or the pilot |

### 7.4 Stages

| Stage | Work | Check, with the expected result |
|---|---|---|
| A1 | The art's sheets: a tool writes every animation of every set as PNG pictures in its palette, with each frame's time and its transitions, and a picture of where each piece sits | 90 sets, 533 animations, 1,359 pictures written; each written picture redrawn from the PNG equals the game's own drawing of it pixel for pixel. **Done in part, 2026-11-22: see 7.6** |
| A2 | The new-art file and its tool, the lookup, the drawing (7.2), with no new art yet | game state identical in all 21 runs; pictures identical to R1's (no transition has new art) |
| A3 | Pilot: the player's run and one enemy's walk, by F6's way, at F5's rate | the logic's records unchanged; shown to Jeff side by side with the original at 60 Hz, in a window; Jeff judges |
| A4 | The rest, a set at a time, the player's set first (125 pictures), then the kinds every level has, then by how often a set is on screen in the plays | each set: game state identical; every new picture within its palette and its pieces' box; Jeff's look |

Order with sections 4-6: R1 (pictures one update behind) comes before A2, since the lookup needs the picture the actor goes to.

### 7.5 Generated, then cleaned by hand (decision F6: Jeff, 2026-11-22, "Generate, then hand-clean")

**What is known about generating in-betweens for pixel art** (read 2026-11-22):
- General video interpolators (RIFE and its kind) blur edges and break the palette on pixel art; the sprite work below says
  so and measures it.
- A sprite-specific U-Net ("DeepInterframes", arXiv 2610.16423, and its previous/next variant 2610.27253, one group):
  two RGBA frames in, the middle frame out, sprites 32-256 px. On 42 test triplets, mean FLIP (lower is better) 0.258
  against RIFE's 0.313 (RIFE not retrained on sprites). Trained on only 280 triplets. Code is public
  (github.com/NethamGH/deepinterframes_neural_sprite_inbetweening: MIT licence by its repository, TensorFlow, 15 KB of
  code; no trained weights and no data released, so it is trained here, on Aladdin's own triplets). Stated
  failures: exaggerated squash and stretch and turns over 45 degrees (FLIP 0.45-0.62), smoke and fire, ghosting where a
  part is hidden. The output is continuous colour: neither the palette nor the pixel grid is kept by the model.
- ToonCrafter (SIGGRAPH Asia 2024, github.com/ToonCrafter/ToonCrafter, Apache-2.0 by its repository; the weights'
  model card on Hugging Face states no licence): a diffusion model that makes up to 16 frames between two cartoon frames
  at 512 x 320, steerable by sparse sketches; about 24 GB of GPU memory (less in community versions). Weights 10.5 GB
  (`model.ckpt`) plus 1.5 GB for sketch guidance. It needs torch 2.0, xformers and decord: xformers is built for CUDA
  and decord has no macOS arm64 build, so it likely needs a machine with an NVIDIA GPU (not verified). Better at large motion and hidden parts; made for drawn cartoons, not pixel art, so its output needs
  scaling down and the palette put back.
- Commercial services exist (for example Ludo.ai's keyframe tool); using one sends the game's art to an outside service,
  which needs Jeff's yes and is not proposed.

**The game's own art is the test.** Every animation already has frames in sequence: for three consecutive frames A, B, C of
one animation, B is a real picture between A and C. The scratch count finds the triplets in the 533 animations; each model
is given A and C and scored on how close its picture is to the real B (FLIP, and the share of pixels exactly B's palette
index once quantized). This picks the method by measurement on Aladdin's own pictures before any production, and gives the
fine-tuning data too: the same triplets train the sprite U-Net on Aladdin's style (held-out triplets kept for the score).

**The pipeline, per transition (picture P to picture Q, held t updates, t - 1 new pictures at 60 Hz):**
1. Export (A1): P and Q composed from their pieces at 1x, RGBA, with the kind's palette and each piece's box.
2. Generate: the chosen model's pictures at the t - 1 times between (a middle-frame model by halving; ToonCrafter in one
   run). A transition met with several times is generated once at its longest and the shorter take every other.
3. Clean by tool: each pixel to the nearest colour of the kind's palette (the 15 colours and clear), the alpha to on/off,
   lone pixels removed, pixels outside the union of P's and Q's boxes flagged. Transitions the score marks as hard (a large
   change between P and Q, measured as the share of pixels that differ) are flagged to be drawn by hand from the start.
4. Clean by hand: each transition as an Aseprite file (P, the drafts, Q as layers, onion skin, the palette locked),
   cleaned by an artist, read back by the tool. The tool cuts each finished picture into 8 x 8 and 16 x 16 pieces for the
   engine and writes the new-art file (7.2).
5. Look: a developer switch plays any set's animations at 60 Hz beside the original's, in the game and on a sheet, for
   Jeff's judgement.

**Not known yet, and what the pilot (A3) settles:** which model wins on Aladdin's triplets; how many minutes of hand cleaning
a picture takes (the cost of the 5,176); whether the models run on the Mac's GPU (the sprite U-Net is small, about 206 MB a
model and 8-10 s a frame on a T4; ToonCrafter's memory may not fit) or need a GPU machine; the licence of the sprite U-Net's
code, before it is used for the game's art.

**Decisions this leaves for Jeff:** F5 (60 Hz proposed), F7 (the original's pixel size proposed), F8 (the rights), who
cleans by hand (an artist Jeff engages; this plan cannot supply one), and whether a GPU machine or service may be used if
the Mac is not enough. F1, F3 and F4 (sections 3-6) still wait on his yes.

### 7.6 Stage A1 and the test, as run (2026-11-22, branch `art-inbetween`)

**How the pictures are taken.** Not from the cartridge's tables: the game itself writes them (`system/dev_celdump.c`,
`ALADDIN_DEV_CELDUMP=DIR`). After each update, every actor the update drew is composed from its pieces exactly as
`video/oam.c` draws it, from a shadow of everything the game wrote to video memory and from its palette; each distinct
picture is written once, and a timeline line per actor per update (its kind, set, animation, frame, picture, time,
mirroring, place). The game is not changed: the dump only reads, and does nothing while the variable is unset. So
pictures whose tiles a kind streams (the player's), palette variants and every set's own tile layout come out as the
player sees them, without re-deriving any of it.

**Checked against the game's own pictures.** p0-0, updates 4,200-4,300, with a picture written every update: of the
dumped pictures' 196,982 pixels, **97.04% equal the game's picture**, 2.60% differ where another actor is drawn over
them, and 0.36% differ with no actor over them (kinds $08 and $20: Abu and a barrel, at the bottom of the screen; the
level's front layer drawn over them; Abu looked at side by side, the same picture). Found on the way, both mine: the
first version tested a piece's 16 x 16 bit after masking it off (pieces came out 8 x 8); a written picture `uN` is the
picture after update N - 1, and pictures in a level are 216 lines, the original's from line 8.

**What the plays give.** The 19 start-to-exit plays and the boss play (`native/checks/plays`), 20 runs, 2-11 s each,
all exit 0: **1,624 distinct pictures** (counting palette variants), **1,021 of the 1,359 (set, picture) pairs** the
tables hold (75%), **77 of the 90 sets**, 896 transitions (907 entries with their times; 2-16 frames held), and **800
triplets** (three pictures in a row of one animation, the middle one's time known). The most: set $01 (Abu) 100, $00
(the player) 95, $03 (the guards) 84. The pictures no play showed are pictures of animations the plays did not reach;
they are found the same way by more plays, or from the tables (a second source to write if the plays cannot reach them).

**The bar a model must pass** (`tools/art/score.py`, over the 800 triplets; the real middle picture scores shape 1,
exact 1, error 0, palette 1):

| Method | Shape (silhouette IoU) | Exact (b's pixels right) | Error (RGB, 0-441) | Palette |
|---|---|---|---|---|
| hold: the first picture, what the game shows now | 0.585 | 0.345 | 122.5 | 0.975 |
| blend: the two averaged, put back on the palette | 0.611 | 0.380 | 116.5 | 0.969 |

`docs/aladdin/native/shots/framerate-triplets.png`: six triplets (the player's run twice, Abu, the barrel rolling):
the first picture, the real middle, the third, and the blend, which shows why a blend is not an in-between.

**Tools, in `native/aladdin/tools/art`:** `cels.py OUT DUMP...` (the pictures as RGBA PNG at 1x, `cels.csv`,
`transitions.csv`, the triplets), `score.py TRIPLETS METHOD` (the bar, and any model's pictures by `file:DIR`). Plain
Python, no packages.

**The sprite U-Net, set up (Jeff's yes, 2026-11-22).** Downloaded: its repository (commit 25cf52d, 1,448 lines, read in
full before running: model, losses, loaders, scripts; no network or shell use) into the session's scratch folder, and
TensorFlow 2.21 (223.5 MB wheel, 1.4 GB installed) in a Python 3.12 environment there; nothing installed system-wide.
On the CPU a pass took 170 s. On the GPU (Jeff: "run it on the GPU"): `tensorflow-metal` 1.2.0 (1.4 MB) does not load
with TensorFlow 2.21 (`_pywrap_tensorflow_internal.so` not found: the plugin is built against an older layout; Apple's
forums report the same for 2.20 and later, and 2.19.1 working), so a second environment holds TensorFlow 2.19.1 with
it; the GPU is found and used. A pass takes 30 s (5.7 times faster), the loss after two passes 0.635 against the CPU's
0.608, the same course. `tools/art/dif_run.py` drives it on
Aladdin's triplets without changing its model, with two departures from its scripts for pixel art (no stretching to
256 x 256; a key colour, (153, 255, 34), 86 from every colour of the art, for clear pixels; reasons in its header).
Split by whole animations: 640 triplets to train (173 animations in all), 160 to test. A pass over the 640 takes 170 s,
so the published 100 passes (batch 16) take about 50 minutes on the GPU. One pass, drafted and scored end to end (pipeline check
only): shape 0.176, exact 0.035, noise where the figure is. On the 160 test triplets the bar is: hold shape 0.605,
exact 0.366, error 106.5; blend shape 0.623, exact 0.394, error 102.0.

**No budget and no artist (Jeff, 2026-11-22):** "Paying isn't an option. This is a hobby project as a job applicant."
Cloud tools with an in-between feature exist (PixelLab's interpolation, 64 x 64 at most, which 127 of the 160 test
triplets fit; Ludo.ai's keyframe animation, API in beta), but both need paid credits: out. Methods are local and free.

**Sliding the original pieces (a calculated in-between, measured).** The cel dump also writes each picture's pieces
(their place, size, flips and own pixels). Over the 907 motion transitions the plays show, of the first picture's
pixels 16.5% are in pieces the next picture has at the same place, 4.7% in pieces it has at another place (could
slide), and **78.8% are in pieces it does not have at all: the art is redrawn**. 40 transitions carry over 100%, 68 at
least 90%, 187 at least half. Sliding pieces cannot make most in-betweens; at most it serves the few transitions where
a part moves whole.

**The sprite U-Net at pass 40** (its best saved state, on the 160 test triplets): shape 0.568, exact 0.113, error
146.1: worse than the blend on all three; its drafts are brown smears over roughly the figure's place.

**The sprite U-Net after its 100 passes (GPU, 50 min; loss 0.379 at the end, flat from pass 30 though the learning
rate was halved five times):** on the 160 test triplets shape 0.565, exact 0.114, error 147.7: **worse than the blend
on all three** (0.623, 0.394, 102.0), and no triplet reaches 80% exact. Its raw output (before the palette) is as far
from the real middle picture on 40 triplets it trained on (mean absolute error 0.213) as on 40 it never saw (0.209):
it has not learned the in-between at all, not merely failed to generalise. `shots/framerate-unet-raw.png`: three
training triplets (the run) and three test ones (the climb): first, real middle, last, its raw middle; the raw middle
is a red-brown double image over the key colour. As planned before the result, the method is dropped; it is not tuned
by trial (no evidence says which change would make it work, and the published results are on a different, unreleased
data set).

**Where this leaves F2.** Of the free, local methods measured, none makes an in-between good enough to use without
hand work: the blend ghosts (exact 0.394), piece sliding applies to the 4.7% of pixels that move whole, the U-Net fails.
No one-time-purchase tool was found that makes in-betweens locally (Retro Diffusion's local extension has no
animation; its in-betweening is on its paid website). The choice is Jeff's.

## 8. As built (branch `smooth60`, 2026-11-22; not committed)

**R1, the update before's world.** `video/between.c`: as each update begins, the engine writes the last update held
are made and g and the video state the picture reads (registered with `between_keep`: the shown sprites with their
owners and anchors, the scroll latches, the level maps' state, the status line's switch, the view, Mode 7's switches)
are copied as "was"; while an update runs, every write to the engine's tiles, maps and palette is held
(`between_tiles_write` and the rest, in `video.c`, `level_maps.c`, `hud_layer.c`; Mode 7's art moved from the
picture to the update's end); a picture copies "was" in, draws, and puts the live state back. One choice made in
building it: the picture is the whole update before's world, its art included (SRFX's way), not the newest art at
eased places, which would draw an actor's old pieces with its new tiles for a streamed picture (the player's).
Checked: `native/checks/view/same.sh` against main ca8e418, 21 runs, 0 differ (game state unchanged); new
`native/checks/between/behind.sh`: every picture equals the reference's one update earlier, byte for byte, 9,811
pairs over the 21 runs (every 25th update), and 4,511 more every 7th update over the boot and p0-0; 0 differ.

**R2, easing.** Sprites move by their actor's drawn screen place (oam.c records each sprite's owner and the place it
was drawn from), eased from the update before's to the newest's: exact at both ends, no wobble against the screen, and
whatever the origin took in follows (BG2's shifts as well as the camera). An actor not drawn at the newest update, or
whose record is another actor's, or that moved more than 32 + 64 pixels, stays where it was. Layers ease by their
24.8 places (a jump past 64 pixels stays); per-line scrolls, window edges and Mode 7's A by line; Mode 7's matrix
and centre as numbers. Checked: `ALADDIN_DEV_BETWEEN_CHECK=1` (before each picture, the easing at between 1 against
the newest update and at 0.5 against both) on p0-0, p2-0 and p5-2: 567,000 places, 0 wrong; a planted error (one
pixel more for sprites moving more than 2) was caught (163 wrong at between 1 in 8,000 updates). The picture check
above was rerun after easing: 21 runs, 0 differ. Not checked by a count: the per-line easing and Mode 7's.

**R3, places between pixels** (engine 6.10, ENGINE_REQUESTS.md section 2, nativeshvc branch `subpixel`, not committed;
the game's build points `NSHVC_ROOT` at that checkout until Jeff commits it). Sprites pass their actor's true place on
the screen (oam.c records each sprite's anchor in 256ths: the actor's 16.8 less the origin's, the camera's 24.8 and the
shift), eased, as whole pixels and a fraction; layers and Mode 7 pass their 24.8 places' eased fractions; per-line
scrolls (the parallax bands) are eased in 256ths. The status line stays on whole pixels. A picture at between 0 shows the
update before's true places, so `behind.sh` is run with `ALADDIN_DEV_WHOLE_PIXELS=1` (whole pixels, the R2 path).
Checked: the easing check with fractions and without, on p0-0, p2-0 and p5-2: 567,000 places each way, 0 wrong (with
fractions it compares true places in 256ths); `behind.sh` with whole pixels on the new engine: 21 runs, 0 differ. Seen:
`shots/framerate-between-4x.png`, one moment of p0-0 (update 4310, the camera moving) at between 0, 0.25, 0.5, 0.75 and 1
at 4 window pixels a screen pixel, a guide line at one window column: the scene moves a step at each quarter (about
190,000 window pixels change from one to the next). `ALADDIN_DEV_BETWEEN_AT=F` draws every picture at between F.

**R4, the stepped effects and the slow motion** (Jeff, 2026-11-23: "Item 1", each eased from its step before to its
newest, running behind by up to a step; the chain scene's jump eased too). Measured first (developer's step log,
`ALADDIN_DEV_STEPS`, on the boot, p0-0, p2-0, p3-0, p4-0, p5-2, p6-0, the end-* plays and boss-flow 17 to 15,000):
- the brightness moves only a level at a time (2,183 steps, none of 2 or more); within a fade every 2 updates, but one
  fade each way in level 36 every update;
- the window edges move every update in most effects (already eased by R2), every 3 in level 13's exit (4022-4120),
  once a 7 in level 5's;
- the chain scene's slow motion: the play loop turns every 8 updates (its task's wait at the update's end is 8 on the
  turn, 7 to 1 between), and between turns nothing drawn changes (the places' hash, the brightness and the windows);
  `g.play_wait` stays 8 after the scene (into the ending's story, level 57), where no play loop turns. The scene fades
  out during the slow motion (7383-7411) and level 57 fades in from 7479, 10 updates after the last turn (7469).

The rule, for three "tracks" (`video/between.c`): a track steps when its value changes; a picture shows it eased from its
step before to its newest over G updates, G the updates between those two steps, at most the track's limit, so it is
exact at each step and runs behind by up to G - 1 updates more than the rest.
- The world (everything R1-R3 eases). It steps every update but in the slow motion: `PlayLoop` tells between.c when it
  turns (`between_play_turn`, no game state); while the last turn was under `g.play_wait` (more than 1) updates ago,
  an update is no step. "Was" and the newest are kept at the world's steps (at the update's end now, not its start),
  and the held engine writes are written a step late, so the art is always the world "was" names. Limit: the play
  loop's wait. Outside the slow motion the world is exactly R3's.
- The brightness: a step of one level eased over up to 2 updates (any other change shown at once). The engine takes a
  brightness between its 16 levels (6.11: `nshvc_screen.brightness_sub` and `NSHVC_LINE_BRIGHTNESS_SUB`, 256ths of the
  way to the next level, the console's own result at 0).
- The windows: the window edges each line shows, and the frame's own, eased line by line over up to 3 updates.

Expected, before building: the game unchanged (`same.sh` 21 runs, 0 differ); with the steps off
(`ALADDIN_DEV_NO_STEPS=1`, every track eased over one update), the pictures R3's; the easing check 0 wrong; a new check
(`ALADDIN_DEV_STEP_CHECK=FILE`): each track's eased value at between 1 is the next update's at between 0 (no jump between
pictures) everywhere but where a step comes sooner than the one before, and a fade's brightness moves 1/8 of a level a
quarter update; in the chain scene the player moves 1/32 of a turn's motion a quarter update after the first turn.

As built: `video/between.c` (the tracks; "was" and the newest now kept at the world's steps' ends, the held writes
written a step late), `game/play_loop.c` (`between_play_turn` at the play loop's turn: no game state), `video/video.c`
(the brightness and the frame's window edges from the tracks), `video/lines.c` (the window lines eased with their track;
the developer's samples), engine 6.11 (ENGINE_REQUESTS.md section 3). Found on the way and fixed: R2 eased a layer by its
24.8 place alone, so a layer moved by its shift or offset stepped (the chain scene scrolls BG1 2 pixels an update by
`g.layer_offset_y`, `bg_y` still: 896 places wrong at between 1 on main too); a layer now eases as sent, its pixels with
the shift or offset added and its place's fraction (`video/scroll.c`).

Checked, on all 37 plays (the boot, every p*, boss16-2hits, every end-* and boss-flow 17 to 30,000):
- the easing check: 3,760,506 places, 0 wrong (at between 1 and 0.5);
- the step check, 437,951 pictures: the world 0 jumps; the brightness 0 jumps, and 21 where a fade's steps came every
  update (its first step eased over 2, the next an update later); the windows 0 jumps, and 6 after a step sooner than the
  one before. Not counted as jumps, as R2 has them: a window line whose step is more than 64 pixels (a line of window
  appearing, shown at its step's end) and a channel starting or stopping (the effect's start and end);
- the speeds: in fades stepping every 2 updates every quarter update moves the brightness 32/256 of a level (55,050
  samples, no other size), every update 64/256; in the chain scene's slow motion (112 updates eased over 8) the player
  moves 8/256 of a pixel each quarter update, a turn's 1 pixel over its 8 updates;
- `same.sh` against main c6803c7: 21 runs, 0 differ (the game unchanged);
- `behind.sh` with the steps off (`ALADDIN_DEV_NO_STEPS=1`, `BEHIND_SHIFT=0`) against main's own pictures on engine 6.10:
  21 runs, 10,010 pairs, 0 differ (R3's pictures, the layer fix aside, which is the same at between 0). Its first run
  found a fault in the windows' track, fixed: a window line whose step was too far to ease stayed at its old value after
  its step ended, until the windows stepped again (p1-1 and p4-2, 21 pictures); at a step's end the newest is now shown
  exactly.

Not checked by a count: how it looks in a window (R5).

**Not done.** Colour bands and text bands (every 4), Mode 7's zoom in the ending (every 16 and 24) and the Genie
wheel's slowing step as before. R5: a run in a window at 60 and 120 Hz, by Jeff's eye.
