# The frame rate: a 60 Hz game, drawn smooth at any display rate

Status: analysis and plan, 2026-11-22; what is built is in sections 8 and 9. Part of stage 7 of [PLAN.md](PLAN.md) (decision 3: "pictures
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

**Not done.** R4: the stepped effects (the fade's brightness has 16 steps the engine
takes whole; the wipes step every 2 updates, so easing between consecutive updates halves their steps but does not
remove them) and the chain scene's slow motion (8 updates a step: easing it needs the picture 8 updates behind in that
scene, or the logic split). R5: a run in a window at 60 and 120 Hz, by Jeff's eye.

## 9. Runahead: the picture answers the controls with no added delay (2026-11-23, branch `runahead-7b2cca`)

Jeff's direction: "Implement runahead so that there is zero input lag on controls". Status: built for one ahead step (9.10); the
checks of 9.9 were run on this Mac; how it feels in a window has not been judged.

**How this meets the direction, in one line:** each tick the real world steps once with the pad as read, exactly as before (the
logic is the original's, unchanged, and `same.sh` stays 21 runs, 0 differ); then the world is saved, stepped one more update with
the same pad held, and the picture's "was" and "now" (decision F3 (a), easing between the last two updates, unchanged) are the
two newest worlds, so the first picture drawn after a press shows the game's response; the next tick puts the saved real world back.

### 9.1 What is measured: updates from a pad change to the response

Headless Metal build, level 0 (`native/checks/plays/p0-0.txt` to its start of play, update 3903), Aladdin standing at (58, 432)
from update 4039; a key pressed at update T = 4100 against the same run with no key; `native/checks/runahead/latency.sh`, the
developer trace `ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=1`, a picture every update. The trace line of update u is written after
update u's step; the picture written at update u is the last one drawn, which is the picture after update u - 1 at between 0
(`DevShot`, system/dev_bot.c).

| Action (key) | First change of the player's state | The player's place first changes | First picture that differs, no runahead | With 1 ahead step |
|---|---|---|---|---|
| run right (Right) | T (picture $62 to $00, the run's start) | x at T + 2 (58 to 60) | T + 2 | T + 1 |
| run left (Left) | T (the turn, picture $63) | x at T + 5 (58 to 57; the turn lasts 4 updates) | T + 2 | T + 1 |
| jump (B) | T (picture $18, y 432 to 427 in the same update) | y at T | T + 2 | T + 1 |
| sword (A) | T (picture $6C) | none | T + 2 | T + 1 |
| crouch (Down) | T (picture $30) | none (his box is lower from T) | T + 2 | T + 1 |
| pause menu move (Down in the menu) | the menu's own state, `menu_update` | | not measurable by a picture: the menu's overlays are the window's, not in `nshvc_picture_read` | unchanged: no ahead steps while the menu is open |

So the original's own logic already answers a press in the update that reads it (the pad is read in `TaskScheduler`, the same
update as the step that moves the player: `d = 0` for the pose and for a jump's rise; a run's x starts 2 updates later, and a left
turn's 5, by the original's own start-up animation). What adds delay today is the picture, which (F3 (a)) shows the world one
update behind: the first picture after update T shows world T - 1 at between 0 and eases towards world T; its cels, which are
not eased (F2 (a)), show world T from the next update's picture on. In the headless numbers above that is the one update (16.7
ms) between T + 2 and T + 1. With the picture drawn at between 0.5 or 1 the first picture that differs is also T + 1 for the
jump without runahead (its eased height starts to move at once), and T + 2 for a run's cel; with runahead both are T + 1, and
the eased height is one update further on: from the trace, 5 pixels higher at between 0.5 (424.5 against 429.5).

### 9.2 N: one ahead step

N = 1. From the table: the response is in the world of the update that reads the press, and the picture shows the world
N - 1 updates before the newest ahead world (its "was") at between 0 and the newest at between 1, so the picture at between 0
shows the real world of the update just run, which has the response. N = 1 makes immediate, in that sense: the pose of every
action measured (run start, turn, jump, sword, crouch), a jump's rise, and everything a menu task of the game answers in the
update that reads the pad. It does not make immediate: a run's first step in x (T + 2) or a left turn's (T + 5), which the
original's logic itself delays; N = 3 would show the run's first x step in the first picture, at the price of two more steps that
can be wrong when the pad changes (9.7); not built, Jeff's to decide (9.11). The pause menu's cursor moves already at once (it is
drawn at once from its own state) and is not changed.

What N = 1 does not remove: the time between a press and the next update that reads it (up to one update, 16.7 ms, plus the
display's refresh): the engine reads the pad once per update.

### 9.3 The tick

`update` in `system/main.c` (three calls added):
1. `runahead_begin()`: if the last tick ran ahead, the saved real world is put back (g, the registered video parts, the
   registered statics, the level maps' words the ahead step changed) and the writes the ahead step held for the engine are dropped.
2. As before: `between_update_begin`, the pause menu, the real step, the developer's trace, shots and dumps, `between_update_end`.
3. `runahead_after(input, step)`: saves the world (g, the parts `between_keep` registered, the statics `RUNAHEAD_KEEP` registered);
   runs one more `between_update_begin`, `step(input)`, `between_update_end` with the same input; leaves the world as it
   is. The picture then eases "was" (the real world, as the real update left it) towards "now" (the ahead world).

The ahead step is the same code as the real one (`NmiHandler`, `TaskScheduler`, `NmiPictureWork`) with the pad held: the
pad's presses (`g.pad_pressed`) are made from the pad held at the last read, so a key pressed in the real step is held, not pressed
again, in the ahead step.

The engine's art needs nothing for one ahead step. `between.c` already holds an update's tile, map and palette writes and
writes them as the next update begins. The ahead step's begin writes the real update's, as the next real update's begin would;
its own are held and dropped with the real world (`between_restore`). So the engine's art at the next real update's begin is
what it would have been without runahead. No engine change is needed (ENGINE_REQUESTS.md section 3).

### 9.4 What a step reads and writes, and what is done with it

From the code (grep of the sources) and from the compiled object files (`nm` of a build with `-mno-global-merge`: every
writable static of the game: 441 symbols, 80 of them once-only notes).

| State | Written by a step? | Decision |
|---|---|---|
| `g` (`GameState`, 247,472 B: the tasks, the actors, the random number `g.random`, the pad, the sound queue, the palette, the VRAM queues, the line tables...) | yes | saved and restored (`between_save`/`between_restore`) |
| the video parts `between_keep` registered (the shown sprite lists and owners and anchors, the scroll latches, `view`, the level maps' records, the status line's switch, Mode 7's switches) | yes | saved and restored with `g` |
| the statics steps change: the placement tables and `placing` (actor/placement.c), the platform course (`course`, `course_end`, `course_live`, `course_steps`, `course_at`), the guards' `pace_once`, `kept_x`, `kept_y`, the camera's `target_x`, `target_y`, Mode 7's sine `table` and its source arrays (`kept_tiles`, `kept_map`), `mode7_cells`, `mode7_pixels` and their changed flags, `level_loading`, the task chain's `chain_depth` and `exec_pending`, the pad's `swallowed` and `source` | yes | saved and restored (`RUNAHEAD_KEEP`) |
| the level maps' words (`field_words`, `back_words`: a tile change or a patched place writes them) | yes, 4 words a tile change | logged before each write (`runahead_log`) and put back |
| statics a step rewrites from `g` before it reads them (the sprite lists of `oam.c`, `uploads.c`'s `text_bytes`, `lines.c`'s per-channel scratch, `anim.c`'s `out`) | scratch | nothing |
| the engine's tiles, maps and palette | through `video/between.c` | held by the ahead step and dropped (9.3) |
| sound and music: `send_command`, `send_upload` (audio/sound.c), which every command and song goes through, and what they reach, the music thread | the real step sends | an ahead step sends nothing (`runahead_ahead()`); the queue it advances is in `g` |
| a level's load (`LevelLoad`, `LevelMapsBuild`), the view's change that resizes the sprite lists (`view_changed`), the sound's first load (`SoundSetsLoad`) | rare | not run ahead: each returns at once and ends the ahead (`runahead_guard()`); the real world is put back and the picture holds the real world that tick (9.6) |
| the pause menu, the options file, the screen mode, the input record, the traces, shots, dumps, the bot | in `update`, outside `step` | real update only; the menu open means no ahead step |
| the cartridge data, the tables, once-only notes (`told`, `said`: 80 flags) | read only, or notes | notes an ahead step reaches first are lost (stderr of an ahead step goes to /dev/null so a developer's trace is not doubled); `unconverted` (system/unconverted.c) is told by the real step only |
| `nshvc_quit` on a step's internal error, `exit` on out of memory | errors | would end the game from an ahead step as from a real one; not guarded |

`runahead_init` turns ahead steps off when `ALADDIN_DEV_BOT`, `ALADDIN_DEV_TRIGGERS`, `ALADDIN_TASK_TRACE`, `ALADDIN_DEV_MAPDUMP`
or `ALADDIN_DEV_CELDUMP` is set: they drive or log the steps by the update number.

### 9.5 Sound

Sound and music come from the real update only, so a jump's sound is sent in the update that reads the press, as it is now. It is
not earlier than it was; the picture is, by one update, so the sound is no longer one update behind the picture's response.
Nothing the driver does (voices, the sequencer) is touched.

### 9.6 What is not built, and what ends the ahead

- **Two or more ahead steps.** Each later ahead step's begin would write the one before's art to the engine, which has no
  read back (ENGINE_REQUESTS.md section 3). It needs a copy of everything the game has written (a shadow of the tiles, maps and
  palette; the level maps' words alone are up to 2 MB) or an engine snapshot. `ALADDIN_RUNAHEAD=2` and more are taken as 1 with a note.
- **A tick that reaches a load, a view change or the sound's first load in the ahead step** puts the real world back and the
  picture holds the real world that tick (its "was" and "now" are both the real update's: the same as a still picture). Counted
  by `ALADDIN_DEV_RUNAHEAD_REPORT` (9.9).
- **Windows.** The stderr redirect of an ahead step is not done there (`#ifdef _WIN32`).

### 9.7 What a player sees when the pad changes during ahead steps

The ahead step holds the pad as the real step read it. If the pad changes at the next update, the ahead world was wrong, and the
picture corrects: the next tick's picture starts from the real world, which has the change. That is inherent to runahead. For one
ahead step it is one update of the world, never a state: a press shows its pose in the first picture after it (the ahead did not
expect it, so the picture before it showed the old pose and, between its two ends, the motion going on); a let go shows the
motion stopping one picture later than the real world stopped, and the picture before it was one update ahead of the real
world on the old motion. Everything that moves is eased towards a world that may be one update wrong, so a hit, a pick-up or a
turn is shown in place one update early, or not at all if the real world did not make it (an enemy's blow the ahead step landed
and the real update did not shows for at most one tick). What the game keeps, and what every real update computes, is the real
world: the ahead world is dropped (9.9, `state.sh`: the real world's hash after every update is the same with and without it).
Measured on p0-0, updates 4,090 to 4,500, at between 0.5 and 1: of 409 pictures, the 384 where the pad did not change at that
update are byte for byte the picture without runahead one update on; of the 25 where the pad changed, 18 are the same and 7
differ (the correction); at between 0 all 25 are the same, since a picture at between 0 shows only the real world.

### 9.8 Cost

Measured on this Mac (Apple M4 Pro), headless Metal-less build, release, `ALADDIN_DEV_RUNAHEAD_REPORT=1`, the time of everything
`update` does but the engine's own work (from `runahead_begin` to the end of `runahead_after`): 

| Plays (12,000 updates each, level 0 to level 5-1) | Without runahead | With 1 ahead step |
|---|---|---|
| mean of a tick (the whole update's work) | 0.009 to 0.011 ms (the real step alone: the same) | 0.030 to 0.032 ms (the real step 0.014 to 0.016) |
| median / 99th percentile | 0.006 to 0.007 / 0.027 ms | 0.027 to 0.028 / 0.049 to 0.052 ms |
| worst | 10.9 to 24.0 ms (a level's load), 0 or 1 tick over 16.7 ms | 9.7 to 23.1 ms (the same loads) |

An ahead step with the save and the put-back costs about 0.02 ms (copies of `g`, 247 KB, in and out, and the step), 0.1% of the 16.7 ms an update has. The
ticks over 16.7 ms are the same ones without runahead (a level's load, in the real step; the ahead step never runs through
one: it ends there, 9.6). The same run in a window (900 updates of the boot, 1,600 pictures, exit 0): mean 0.041 ms, 99th
percentile 0.117, worst 0.299. The drawing of a picture is the engine's and is not in these numbers.

The budget is one update, 16.7 ms.

### 9.9 Checks and their results

All on this Mac, from this branch, with the plays of `native/checks/plays` (the boot of 11,000 updates and 20 plays: 21 runs).

| Check | Result |
|---|---|
| Runahead off: `view/same.sh` of the previous game's build (`base-b36b712`) against this build, 256x224 | 21 runs, 0 differ |
| Runahead off: `between/behind.sh` with `BEHIND_SHIFT=0`, the Metal build of the commit this branch is cut from (c6803c7) against this branch's: every 25th picture's hash | 21 runs the same, 10,010 pairs, 0 differ. (The script's own form compares a build without pictures between updates one update on; no such Metal build was at hand, so this is the same claim another way: this branch's pictures with runahead off are the previous commit's, byte for byte) |
| Runahead on: `runahead/state.sh`, the real world after every update, with and without ahead steps | 21 runs, 250,581 hashes of the real world (g, 12 video parts, 28 statics, the level maps' words) and 2,038,719 lines of the developer's trace in all (the player, every live actor with its own bytes, the video fields): the two logs are the same, line for line, in every run. In a run of 12,000 updates the ahead step ended early 4 to 10 times (a load or a view change in it) |
| Planted leak: `g.random` left as the ahead step made it when the real world is put back | caught: p0-0 differs from update 4,251 (the first time the random number is read in an ahead step: a hash line), 349 lines differ by update 4,600; removed, the same run is identical again. (Planted in `between_restore`, not kept) |
| Runahead on: the ahead picture is the true future when the pad does not change (`runahead/future.sh`, p0-0 updates 4,090 to 4,500, a picture every update at between 0.5, then 1) | 384 of 384 pictures at updates where the input did not change are the picture without runahead one update on, byte for byte, at both; at the 25 where it changed 18 are the same, 7 differ (9.7); at between 0, 409 of 409 |
| Latency again with runahead on (`runahead/latency.sh`, the five actions of 9.1) | the first picture that differs is T + 1 in all five (T + 2 without); the player's state changes at T as before |
| A window: the Metal build, 900 updates, runahead on by default | exit 0, 1,600 pictures; one tick ended early |
| `clean_check.py` | no finding in the files this work touches (43 in others, as before) |

Not checked: how it feels in a window with a pad or keys (Jeff's); Linux, Windows and the other hosts' builds (the code has no
platform part but the stderr redirect, `#ifdef _WIN32`); a play with the ahead step ended early at every tick; the `told` notes of
9.4; the pause menu's pictures (outside `nshvc_picture_read`).

**Found on the way, not from this work.** `g.line_data` (the per-line tables) differs from run to run of one build with runahead off:
p1-1 (level 1-1), from update 9,089, bytes 9 to 11, 26 and 27 of the table. Two identical off runs give different values (for
example 222, 109 against 149, 107 at bytes 10 and 11); nothing in the traces or pictures shows it. The cause was not found (a
write watch on the bytes did not trigger; the values are not read from the cartridge by `window_table_entry`). The state
hash leaves `line_data` out for that reason, so a leak in it would not be seen.

### 9.10 As built

Built on `runahead-7b2cca` (cut from c6803c7), the design above for N = 1, on in a window and off in a headless run (`ALADDIN_RUNAHEAD`,
CHECK.md). The new code: `system/runahead.c` and `.h` (the tick's two hooks, the registered statics and the undo log, the guard, the
timing and the developer's state hash), `video/between.c` (`between_save`, `between_restore`, the hash). Edits in the game:
`system/main.c` (an include and three calls: `runahead_init`, `runahead_begin`, `runahead_after`), `audio/sound.c` (the sends and the sets' load),
`game/level_load.c`, `video/level_maps.c` (`LevelMapsBuild`, the words' log), `video/video.c` (`view_changed`, the Mode 7 arrays),
`system/unconverted.c`, and `RUNAHEAD_KEEP` after 28 statics in 10 files. No engine change (ENGINE_REQUESTS.md section 3). The game's
logic is not changed: with runahead off the game state and the pictures are the previous commit's (9.9). One departure found
while checking and kept out of the hash, in the note at the end of 9.9.

How it was found out, for the next change: a static that is missed shows as a different hash at the first update that reads
it (`placing` in actor/placement.c was one: a placed actor came a tick late; the hash showed it at update 4,170 of p0-0). The list in
9.4 was made from the compiled objects after that, not from a grep, which had missed the struct statics.

### 9.11 Decisions for Jeff

1. **N.** One ahead step makes the first picture after a press show the response (pose, a jump's rise, a crouch, a sword). A run's
   first step in x comes 2 updates after the press (a left turn's 5) by the original's own start-up animation: showing it in the first
   picture needs N = 3, with two more steps that can be wrong when the pad changes, and the engine's art for them (9.6: a shadow
   of what the game has written, or an engine snapshot). Not built; Jeff's to say whether that is wanted.
2. **Two or more ahead steps** need the same shadow or an engine snapshot (9.6).
3. **The rest of the latency**: a press waits up to one update (16.7 ms) for the next update that reads it, and then the display's
   refresh. Reading the pad more often than the update rate is the engine's (it reads once per update); not asked of it here.
4. **Whether the pause menu's pictures are checked headless**: its overlays are the window's, so a picture file does not show them;
   it needs an eye or an engine call that reads the whole window.

## 10. The snapshot: the whole game at one moment in a flat buffer (2026-11-23)

Jeff's requirement, through the save-game session (docs/aladdin/native/SAVES.md on the save session's branch): a save is the exact
moment; loading resumes at the very update and place, from the title or from the middle of another level, with sound and music
going on correctly; a file from a build with another state layout is refused. The interface the slots call is
`system/snapshot.h` (`snapshot_available`, `snapshot_max`, `snapshot_save`, `snapshot_load`, `snapshot_layout_mark`); this section
is the design behind it. **How this meets the direction in one line:** the snapshot is the runahead's save and restore (9.3, 9.4)
written to a buffer through the same two registered lists (the video parts of `between_keep` and the statics of `RUNAHEAD_KEEP`),
with what could not go to a file turned into what can; the logic, F3 (a) easing and runahead are not changed.

### 10.1 What a snapshot is, and when it is taken

The real world after a complete update (a tick's real step, no ahead world: `runahead_begin` has put the real world back, which the
pause menu's and the title's update already is). A load puts that world in, and the next update is the one after it. It is not
taken or put back inside a step.

### 10.2 What each piece is, and the decision for it

| Piece | In a file as it is? | Decision |
|---|---|---|
| `g` (247,472 B) | no: pointers | copied, then each pointer field replaced by a code and back on load: `Actor *` fields as an offset in `g` (`chain_head`, `chain_tail`, `free_pool` 28, `free_pool_b` 5, `draw_list` 4 x 192, each actor's `next`); NULL its own code. The list is the pointer fields of every type `GameState` holds, which the compiler's own record layouts give (`-fdump-record-layouts`: pointers in `GameState`, `Actor`, `VramQueueEntry`, and nothing else; checked by `native/checks/snapshot/pointers.sh`, which fails if a new pointer member appears) |
| each actor's `code` (a function pointer) | no | an id: the hash (FNV-1a, 32 bits) of the function's name, from one table of every function an actor can run in (`actor/code_table.c`, made by `tools/gen_code_table.py`: every start, second and third entry of every row of `kinds_*` and every function `ActorRunAs` is given: 146 functions). Complete by a check, not by hand: `gen_code_table.py --check` reads the sources for each name in a `KindCode` row and each second argument of `ActorRunAs` and fails if the file is not what they give; and a save fails, naming the address, if a live actor's `code` is not in the table (the check scans every update of the 21 plays). Ids are of names, so they hold across rebuilds; two names with one hash end the run at start |
| each VRAM queue entry's `from` | no | a code: `g.unpacked` or another array of `g` as an offset in `g`; the cartridge as an offset in the cartridge image; anything else fails the save, naming it (the plays' scan shows whether anything else occurs). The queue is empty after each update's picture work unless the NMI is off, so most saves hold none |
| the video parts of `between_keep` (sprite lists, anchors, scroll latches, `view`, Mode 7 switches...) | plain data, but sized by the view | their live bytes, in registration order, each with its size; `layer_map` is not among them (below) |
| the frame's sprites the game built (`oam.c`'s lists, as far as `g.sprite_count`) | state, found by the check | saved (section OAM). They are not scratch: `video_vblank` copies them to the shown lists whenever `g.frame_ready` is set, and in a scene whose update builds none it stays set, so they outlive their update. The first cross-process run of a Mode 7 scene (p5-2) showed it: the shown lists differed for 34 updates with every picture the same |
| the statics of `RUNAHEAD_KEEP` (28) | plain | their bytes, by name and size, from the same registry runahead uses (`runahead_keep_at`); not a second list |
| the level maps (`layer_map[2]`: handle and pointer; `field_words`, `back_words`) | handle and pointer: no | saved as numbers (on, wraps, size, margins, which words array) and the words (`w x h` 16-bit each, whole: a tile change or a patched place has changed them, so they are not the level's). On load the engine map of that layer and size is taken from the map pool as a level load takes it (made if the process has none: `map_of`) and the words are written to it. The engine's level map is nothing but those words (every writer is `between_map_write_snes` of the words), so rebuilt-from-words is equal to what the engine held |
| the engine's tiles, maps and palette | engine state, no read back | **a shadow kept by the game**: every write the game makes to an engine object it also makes to a copy in `video/between.c` (at the moment the write is held, so a held write is in it), by a stable name (`tiles.low`, `tiles.high`, `tiles.text`, `slot_tiles.N`, `map.back`, ..., `palette`). The snapshot holds the copies; a load writes each whole object. The slot sets are as many as the view needs: a file's set this process lacks is made by the view the load adopts, one the file lacks is zeroed. Not shadowed: the level maps (words above), Mode 7's tiles and map (written from `mode7_pixels` and `mode7_cells`, which are saved: a load sets their changed flags), everything the picture sets each time it draws. No engine change |
| the view (`view`, `slots_open`, sprite list sizes) | process and window state, not in `g` | saved; a load adopts the saved view and remakes what is sized from it as a level load does (`sprite_lists_make`, `video_slot_sets_make`), so a state saved in one window proportion loads in a window of another and keeps its own view until the next load of a level or still screen (as a window resized in a level does, VIEW.md). VIEW.md says the view changes only where a level loads and where a still screen starts: a load of a snapshot is taken as such a place; Jeff's to overrule (10.7) |
| the sound driver (`d`: the song and instrument data with the songs loaded, the 16 channels, tempo, fade, echo, voices' registers) and `music.c`'s tick phase | on the audio thread | saved from there: in a window by a request the audio code answers at its next sample (it first takes every message the game sent before, so the driver is as the game last left it); headless by reading it (one thread). The samples' engine handles are the process's, not saved. A load keeps the state and, at the start of the next update, sends it as messages in the music queue (in order with the commands: 9 parts of 8,000 bytes; the queue is 262,144 bytes), so the driver is put in between the messages of the updates before and those of the update after, exactly where the save was taken (a load that posted at once would let the driver run one update of sound before the first step); the audio code replaces `d` (keeping this process's sample handles), keys every voice off, and sends every voice's sample, envelope, pitch and volume, the echo, the main volume and the flags to the engine again (the driver sends only what changed, so without this the engine's voices would stay as the title or the other level left them) |
| what the engine's voices were doing (a note's phase, envelope level, the echo's buffer) | engine state, no read back | **not restored**: a load resumes the song at its place in the music (every channel's position, time, tempo, fade) but a note that was sounding is silent until the next note; no tail, no echo of what came before. Jeff's choice (10.7) |
| the pause menu's open sound and the music pause | in `g.sound_queue` and `d` | in the state the snapshot is taken in (the save session's matter: SAVES.md) |
| anything a level's load sets once that a load from the title lacks | | the statics and `g`; the engine's art by the shadows; the level maps; the sound data in `d`; the view; nothing else is set at a load (the nm list of 441 statics, 9.4, read for it: the rest are scratch, dev tools, notes and once-only caches) |

### 10.3 The layout mark

A 32-bit FNV-1a over what the layout is made of: `sizeof` of `GameState`, `Actor`, `Task`, `VramQueueEntry`, `View`, `Driver`,
`Channel`, `nshvc_sprite`, `LevelMap`; the offsets of every pointer field; each `between_keep` part's name and (for the fixed ones)
size, in order; each `RUNAHEAD_KEEP` static's name and size, in order; each shadowed object's name and geometry (not the slot sets, which follow the view); the number of
sections and the format number. Nothing is kept by hand: adding a registered static or a field of `g` changes it. A change that
keeps every size (two fields of one type swapped) is not seen; said so.

### 10.4 What a load does

1. Checks the format, the mark, every section's length, the codes (an unknown code id, a pointer code out of range): on any fault
   nothing is changed and it returns 0.
2. Puts in `g` (pointers rebuilt), the video parts, the statics, the view and what is sized from it, the level maps, the engine's art
   from the shadows (whole objects), Mode 7's changed flags, the sound.
3. Rebases the picture: "was" is the loaded world, nothing is held, so the first picture after a load is the loaded world, not a mix
   of it and the one before.

### 10.5 Checks (to run; results go in 10.8)

1. Runahead's own: `state.sh` 21 runs and `same.sh` 21 runs, 0 differ, with the snapshot code in.
2. One process: save at update u, run on, restore, run on again: the trace and the state hash from u on equal the first run's.
3. Across processes: a file written at update u of a play; a fresh process started at the title, or another level's play, restores it
   and is given the same keys: the trace from u on update for update, every 25th picture, and the sound driver's state hash;
   on at least four plays including a Mode 7 stage and a boss.
4. A planted omission (a registered static, then a shadowed object, then a pointer code) caught by 3.
5. Not covered: what it sounds like (10.2), a different window's view loading (not run on a second view), and what the save
   screen does.

### 10.6 Decisions made from the code (not asked)

Shadow of the engine's art instead of an engine read-back; the audio state through a request the audio code answers (the engine has
no way back from the audio thread: api.md section 1, "the engine never hands the game anything from the render or audio threads", so the
game's own code answers a flag; no engine change); function pointers as name hashes; the view adopted from the file.

### 10.7 For Jeff

1. **Sound after a load:** the song resumes at its place but a note that was sounding is silent until the next one (the engine has no
   read back of a voice's phase; keying them on again would restart a note and would play a finished drum hit once more). The other
   choice is to key on the voices the driver thinks are on: no silence, but re-attacked notes and possibly a repeated hit.
2. **The view a load brings:** the saved view stays until the next level or still screen (a window of another shape then shows it
   fitted, as a resized window does in a level). The other choice is to refuse a file saved in another view.

### 10.8 As built and results (2026-11-23)

Built on `runahead-7b2cca`: `system/snapshot.[ch]` (the interface; the buffer: sections G, KEPT, STAT, LMAP, VIEW, ART, OAM, SOUND), `video/between.c`
(the shadows, `between_art_*`, `between_rebase`, `between_layout`, names for the kept parts), `video/level_maps.c`, `video/oam.c`, `video/video.c`,
`video/slot_gfx.c` (the view's put-in), `audio/music.c` and `driver.c` (the state, the restore through the queue, the registers again),
`actor/code_table.c` made by `tools/gen_code_table.py`, `actor/code_id.c`, `data/data.c` (a pointer into the cartridge as its offset),
`system/dev_snapshot.c` (`ALADDIN_DEV_SNAP_SAVE=FILE@U`, `ALADDIN_DEV_SNAP_LOAD=FILE@L`), the checks in `native/checks/snapshot`. No engine change.
The buffer holds one snapshot of 585,012 to 701,460 bytes in the plays below (`snapshot_max` 2.7 MB: the level maps can hold 2 MB of words);
the save takes 0.30 to 0.43 ms and the load 0.38 to 0.43 ms in the headless Metal build (the engine's own upload in a window is not in it);
a save in a window waits for the audio thread's next sample: 2.9 ms in the one run measured (a load 0.56 ms).

| Check | Result |
|---|---|
| Runahead's own, with the snapshot code in: `runahead/state.sh` | 21 runs, 0 differ |
| `view/same.sh`, this build against main (b91729e), runahead off | 21 runs, 0 differ |
| `snapshot/pointers.sh` and `gen_code_table.py --check` | the pointer members of every type `GameState` holds are the 8 the snapshot walks; 146 functions, the rows and `ActorRunAs` name no other |
| One process: `cross.sh` with `SNAP_SAME=1`, p0-0, save at 4,300, load at 4,600 | 200 updates: the trace, the state hash and 198 pictures the same as the run that never loaded |
| Two processes, `snapshot/cross.sh` (A plays and saves; B is a fresh process that loads at update 300 at the title, or at update 4,000 to 6,000 in another play, and is then given A's keys): trace and state hash of K updates, a picture every update | p0-0 at 4,300 (title and in p2-1 at 5,000): 200 and 200, 0 differ, 0 of 198 pictures; boss16-2hits at 8,350 (Jafar's first form; title and in p3-0 at 5,000): 200 and 200, 0 differ; p3-1 at 8,850 (a Mode 7 stretch, 8,754 to 8,995; title and in p0-0 at 6,000): 100 and 100, 0 differ; p5-2 at 25,300 (a Mode 7 scene, 25,239 to 25,457): 150, 0 differ; p2-1 at 6,000 (in p0-1 at 4,000): 200, 0 differ |
| Runahead on in both processes (p0-0 at 4,300, title) | 200, 0 differ, 0 of 198 pictures |
| A view that differs: saved with `ALADDIN_VIEW=384x224` and loaded in a 256x224 process, and the reverse | 200 and 200, 0 differ, 0 of 198 pictures: the loading process keeps the saved view |
| A planted omission (each caught): `placing` (a registered static) left out of the list; `tiles.high` (a shadowed object) not put back; `chain_head` and `free_pool` (pointer fields) not put back | 190 of 200 updates differ from update 4,310; the trace is the same but 54 of 198 pictures differ; the loading process crashes at once (a segmentation fault) |
| The window's path (Metal build, not headless): save at 600, load at 700 of the boot, one process | exit 0; the save goes through the audio thread's flag |

**What the sound check is.** In the headless runs the driver's whole state (the Driver and the tick's phase, handles left out) is part of the state hash:
it is the same, update for update, from the first update after the load (the loading process's first update has it a sample window late,
which `cross.sh` allows for and says). That shows the song resumes at its place. It does not show what it sounds like: the audio samples were
not compared, and by 10.2 a note that was sounding is silent after a load until the next one.

**Not covered:** what a save and load sounds like in a window; the save and load calls from the save screens (that session's);
a load into a process with another window shape run in a window (the view is adopted in the headless runs above); a snapshot of another
build's layout (the mark refuses it, shown only by its code, not by a file from another build); the level maps' pool running out (96 sizes: the
load then reports it and goes on); `snapshot_load` after a failure part-way (the checks come first; the level maps' pool is the one step after
them that can fail). A same-size reorder of two fields in `g` is not seen by the mark (10.3).

**The interface for the save session:** `system/snapshot.h`: `snapshot_available()` (1), `snapshot_max()` (allocate this many bytes), `snapshot_save(buf, cap)` (bytes written,
0 on failure), `snapshot_load(buf, len)` (1 on success; 0 changes nothing), `snapshot_layout_mark()`. Call them from an update's start, as the
pause menu's and the title's updates are, after `runahead_begin` (the real world is back). After a load the update that called it runs no step.

