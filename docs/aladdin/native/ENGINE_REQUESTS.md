# What the game needs from nativeshvc

Requests from the Aladdin conversion to the engine's owner, through Jeff. Each is general, for every game, and names
no game in the engine.

## 1. A voice's sample changed without a key-on (Jeff, 2026-11-19): API 6.6, `nshvc_voice_sample`

**What the game needed.** The sound driver sets a voice's instrument, and so its DSP source (SRCN), about one tick
(8 ms) before the note's key-on: `src/aladdin/audio/sound_driver.c:1324-1329` (VoiceInstrumentFrom's write of DSP
register 4, reached from sequencer command $08 and from the restore of a music voice after an effect,
`sound_driver.c:240-243`). On the SNES the old note, still fading, goes on into the new source's loop when it next
reaches its end, because the DSP reads the current source's directory entry there. API 6.5 changed a voice's sample
only at key-on (`nshvc_voice_key_on`), so the old note would have kept its own loop.

**What the engine gained.** `void nshvc_voice_sample(uint32_t v, nshvc_sample s)`: the voice's sample from then on.
Key-on starts it, and a playing voice goes on into its loop point, its filter history kept, when it next ends the
block with the end flag. `nshvc_voice_key_on` sets the voice's sample too. In nativeshvc: `include/nshvc/nshvc.h`,
`src/engine/sound.c` (a voice's `source` in place of the sample queued for key-on), `docs/api.md` (section 6.3 and the
stability rules), `README.md`, `docs/native-engine-plan.md`, `tests/scenes/voice_sample.c`.

**API version.** 6.6, nativeshvc `fce4dc0`.

**Jeff's approval**, 2026-11-19, in this session, answering whether to accept the difference or add an engine call:
"if you cloned nativesvhc, you can add this engine call in nativesvhc as a new spec and i'll commit. I own nativesvhc
so that's fine (if it's proper console architecture)", then "we already cloned nativesvhc in
projects\strutb\nativesvhc. do it there, properly". The change was made in that checkout; it is nativeshvc's commit
`fce4dc0` (committed on Jeff's instruction), API 6.6.

**How it was checked on the engine's own tests.** `tests/scenes/voice_sample.c`, headless on Linux: a voice keyed on
sample A and given sample B before A's first end is the same, level, output and end flag, in each of 64,079 samples,
as a voice playing one sample made of A's blocks then B's looped ones; a control voice left on A differs in 58,016,
so the test sees the change; the run's left (the switched voice) and right (the joined sample) are the same in all
64,080 samples at full precision (`voice_sample --check`). Not run: the other examples' sound before and after.

**Where the game uses it.** `native/aladdin/src/audio/driver.c` (`out_source`, from `instrument_set` and
`driver_start`). The game's build needs API 6.6.

## 2. Places with a fraction of a pixel (Jeff, 2026-11-22): API 6.10, made on nativeshvc branch `subpixel`, not committed

**What the game needs.** The game draws each picture between its last two updates (docs/aladdin/native/FRAMERATE.md,
Jeff's decisions F3 and F4). Its places are fixed point with 8 bits of fraction (an actor's 16.8, a layer's 24.8), but
API 6.8 takes whole game pixels: `nshvc_sprite.x`, `.y`, `nshvc_layer.scroll_x`, `.scroll_y`, `nshvc_mode7.scroll_x`,
`.scroll_y` and the per-line scrolls (`NSHVC_LINE_SCROLL_X`, `_Y`) are `int32_t`. At a window scale of 4 a slow mover or
a slow camera then moves 4 window pixels at once every few updates, so the eased pictures still step. Jeff, asked
whether to include the engine change: "Yes, change the engine".

**What is asked.** For every game, and a no-op at 0, so every picture of every game today is unchanged:
- `nshvc_sprite`: `int16_t x_sub, y_sub` (256ths of a game pixel, added to `x`, `y` where the sprite is drawn; the
  console's line rules, the sprites a line and the wrap, still go by the whole `x`, `y`).
- `nshvc_layer`: `int16_t scroll_x_sub, scroll_y_sub` (256ths, added to the scroll where the layer is drawn: a layer pixel
  is then a screen pixel plus a fraction, drawn by the sharp-texel rule the engine already has; offset-per-tile, the
  scroll's whole characters, mosaic's blocks still by the whole scroll).
- `nshvc_mode7`: `int16_t scroll_x_sub, scroll_y_sub`, the same for the plane.
- the per-line scrolls: `NSHVC_LINE_SCROLL_X_SUB + n`, `NSHVC_LINE_SCROLL_Y_SUB + n` (256ths, per line, as the whole ones).
A game that never sets them sees nothing new. The windows and per-line windows stay whole (their edges are the
console's columns).

**How it would be checked** (in nativeshvc): every example and scene test's pictures identical before and after with
the new fields 0; a scene with a sprite and a layer at x_sub 0, 64, 128, 192 at window scale 4 moving by 0, 1, 2, 3
window pixels; Metal here, Vulkan and Direct3D 12 on their machines (Jeff's).

**What was made** (Jeff's "yes" to doing it here, 2026-11-22), as asked: `x_sub`, `y_sub` (sprites), `scroll_x_sub`,
`scroll_y_sub` (layers, Mode 7), `NSHVC_LINE_SCROLL_X_SUB` = 100 and `NSHVC_LINE_SCROLL_Y_SUB` = 104 (+ layer). In
nativeshvc: `include/nshvc/nshvc.h` (6.10), `src/engine/compose.c` (the layers' offsets and their char range by floor and
ceil of the fractional offset; strips split on a fraction; Mode 7's fraction through the matrix after the rounded
products; a sprite piece of ten words), `src/engine/picture.c` (the line fields), `src/engine/shaders/sprites.vert` (the
piece drawn moved by its fraction, kept to its strip by the whole place), the compiled programs regenerated by
`src/render/shaders/build.sh` (Metal and Vulkan; Direct3D 12's `rr_dxil.h` was left stale then, wrongly thought to
need a machine with dxc: this Mac has a dxc build in `nativeshvc/build/tools/dxc`, and `rr_dxil.h` regenerated with it
from 6.10's programs is the same byte for byte as the copy another session regenerated on 2026-11-23), `docs/api.md`,
`tests/scenes/subpixel.c`. 6.9 is the display's scale on its own branch.

**How it was checked**, on Metal (Apple M4 Pro), headless:
- regenerating the compiled programs on unchanged main reproduces the checked-in ones byte for byte (so the diff is the
  change's alone);
- the engine's 118 scenes (made without bsnes: an empty-ROM stand-in for asar, the engine's scene files are the same)
  drawn by main's engine and the new one, at the console's size and at 4 times: 236 pictures, 0 differ;
- the engine's own tests `depth`, `palettes`, `mesh`: ok on both; `m7check` over the scenes: the same output on both;
- the game (branch `smooth60`) built on main's engine and on the new one, fractions all 0: 21 runs' pictures (9,812
  pairs), 0 differ;
- `subpixel` (new): a sprite, a layer, Mode 7 and a per-line scroll at fractions 0, 64, 128, 192 at 4 window pixels a
  screen pixel, each edge checked inside and just outside: ok; a planted fault (sprites' x fraction dropped) fails it
  (7 failures).
Not run: Vulkan, Direct3D 12, the windowed path; `feature`, `pixel`, `cost` (they need inputs or a game's shaders).

**Where the game uses it.** `native/aladdin/src/video/video.c` (`sprites_ease`, `draw_layers`, `draw_mode7`),
`video/scroll.c`, `video/lines.c`: the eased places' fractions, from the 16.8 and 24.8 places the game keeps.

## 3. A brightness between the 16 levels (Jeff, 2026-11-23): API 6.11, nativeshvc branch `brightness-sub`

**What the game needs.** Jeff's answer on the stepped effects (FRAMERATE.md, R4: "Item 1") is that each is eased from
its step before to its newest. A fade steps the brightness a level every 2 updates; eased, a picture's brightness lies
between two levels, and the engine takes only the 16 (`nshvc_screen.brightness`, `NSHVC_LINE_BRIGHTNESS`, 0-15).

**What was made.** `nshvc_screen.brightness_sub` and `NSHVC_LINE_BRIGHTNESS_SUB` (108): 256ths of the way from
`brightness` to the level above. The colour maths (`src/engine/shaders/maths.glsl`) dims each colour as the console does
at both levels and mixes the two, rounded; at 0, and at brightness 15, the picture is the console's. The fraction rides
in bits 16-23 of the draw's colour word (`compose.c`, `colour_word`; still a whole number a float holds exactly), set
from the screen and the line field (`picture.c`). The compiled programs regenerated for Metal, Vulkan and Direct3D 12
(with this Mac's dxc). `docs/api.md`, `tests/scenes/brightness_sub.c`.

**How it was checked**, on Metal, headless: the engine's 118 scenes at the console's size and at 4 times drawn by 6.10's
engine and 6.11's: 236 pictures, 0 differ; `subpixel` ok; `brightness_sub` (new): a 15-bit colour at levels 0, 3, 4, 7,
11, 14, 15 with fractions 0 to 255, on the screen and from a line, each pixel against the value worked out from the
console's dimming: ok, and a planted fault (the rounding dropped from the expected values) fails it (3 scenarios).
On Vulkan, through MoltenVK on this Mac (the null host): `brightness_sub`, `subpixel`, `depth`, `palettes`, `mesh` ok,
and the 118 scenes the same as Metal's, 236 pictures, 0 differ. Direct3D 12: the game cross-built for Windows, not run.

**Where the game uses it.** `video/video.c` (`video_draw`: the screen's and the lit line's brightness from
`between_brightness`, `video/between.c`).

## 4. Pictures timed by the display (Jeff, 2026-11-23: "Yes, plan it"): a plan, not made

**Why it was asked.** The windowed run (FRAMERATE.md R5) found 19 of 3,800 pictures drawn 5 to 17 ms off the refresh:
the game's `between` is the clock when `draw` is called, and the engine's loop calls it when the render thread asks
(`src/engine/loop.c`, `run_live`: `hs_want`, then `between` from `game_ns()`), not when the picture will be shown.

**Measured first** (macOS, Metal, 60 Hz display, p0-0 to 4,200 updates in a window; a temporary log of each drawable's
`presentedTime` from Metal's presented handler, removed after):
- while the window was shown, 2,870 of 2,876 presented pictures were one refresh apart; each picture's moment (the
  update before plus between) stepped with its real presentation time to 0.065 of an update (sd, about 1 ms); 11 were
  more than half an update off, all at the edges of the spans when the window was not shown. So timing by the display
  would correct about 0.4% of pictures, by up to a refresh, where the window comes and goes;
- a picture reaches the screen 49 ms after it is drawn (median; 35 to 66): three refreshes, the drawables in flight
  (`CAMetalLayer`'s default 3). With the easing's own up to one update, the game's newest state is on screen 50 to 67
  ms after its update;
- for spans of the run (updates 600-1,200 and 3,300-3,900) Metal presented nothing (`presentedTime` 0: the window not
  on screen, covered), and the engine then drew about two pictures an update (7,922 pictures for 4,200 updates in the
  first run), none shown.

**Three changes, each Jeff's to choose** (engine, every renderer; headless and the scene checks untouched):
- **D1, pictures timed by the display.** The render thread learns the display's refresh times from what was shown
  (Metal: the presented handler's `presentedTime`; Vulkan: `VK_GOOGLE_display_timing`, which MoltenVK has, or
  `VK_EXT_present_timing`; Direct3D 12: `IDXGISwapChain::GetFrameStatistics`) and, when it asks for a picture, gives
  the game thread the time that picture will be shown. `between` is then that time less a fixed delay (the pipeline's
  depth, so the moment shown is never past the newest update: Jeff's F3, no guessing ahead), not the clock at `draw`.
  Where no times come back (a renderer without them, the window not shown), as now. Gains: the 0.4% above.
- **D2, less delay.** Two drawables in flight, not three (Metal `maximumDrawableCount` 2; Vulkan `minImageCount` the
  surface's least, not one more; Direct3D 12 a waitable swap chain with `SetMaximumFrameLatency(1)`). Expected: drawn to
  shown about 33 ms, not 49. Risk: a picture that takes the renderer longer than a refresh misses one (the engine's
  `render_ms` would be measured on Jeff's games first, at 60 and 120 Hz).
- **D3, no drawing for nothing.** When the last pictures were not shown (`presentedTime` 0, or the host says the window
  is covered), the render thread asks for a picture once an update at most, as it already does for a minimised window.

**How each would be checked.** The windowed run with the draw log and a presented-time log (kept in the engine as a
developer's switch): D1, no picture's moment more than a quarter update off its presentation step in the shown spans
(now 14); D2, drawn to shown median about 33 ms and still every presented picture one refresh apart; D3, at most one
picture an update while nothing is shown. The headless checks unchanged by construction: the 236 scene pictures,
`same.sh`, `behind.sh`. Vulkan through MoltenVK here; Direct3D 12 and 120 Hz on Jeff's machines.

**Recommendation.** D2 and D3 first: D2 is the larger gain (16 ms off every picture's delay), D3 saves the wasted
drawing; D1 corrects few pictures and is the largest change (a timing path in each renderer), so last, if wanted.

**Made** (Jeff, 2026-11-23: "Do both D2 and D3", then "Do D1 too"; nativeshvc branch `present-pacing`, not committed):
- D2: Metal `maximumDrawableCount` 2; Vulkan `minImageCount` the surface's least, at least 2; Direct3D 12 a swap chain
  with the frame latency waitable object, `SetMaximumFrameLatency(1)` and a wait on it before each shown picture.
- D3: Metal counts pictures not seen (presented time 0); three running, `rr_draw` says the display did not pace it and
  the loop asks once an update; Direct3D 12 the same on `DXGI_STATUS_OCCLUDED`. Vulkan: no signal for it, not made.
- The developer's `NSHVC_DEV_PRESENT_LOG` (Metal: the presented handler; Direct3D 12: the frame statistics) and
  `NSHVC_DEV_FRAME_LATENCY` (Direct3D 12).

**Measured** (Metal, this Mac's 60 Hz display, p0-0 to 4,200 updates in a window, the window shown throughout):
- drawn to shown 32.3 ms median (was 49), 99th percentile 32.4; 4,193 of 4,195 steps one refresh apart, 2 of two
  refreshes (update 3529, the Mode 7 level select, where the GPU takes 4 to 17 ms a picture);
- draw times now locked to the refresh (handed-to-shown varies 0.4 ms, against 5 to 17 ms before): each picture's moment
  follows its presentation to 0.032 of an update (sd); 5 more than a quarter update off, at the two missed refreshes;
- the window hidden (the app hidden by AppleScript at 15 s): one picture an update for 2,502 of 2,541 hidden updates
  (38 had two, one four), against about two an update without D3 in the same run; shown again once the app was brought
  to the front.

**Windows** (Jeff's VM 192.168.1.22, AMD RX 6950 XT): the engine's `brightness_sub`, `subpixel`, `depth`, `palettes`,
`mesh` ok on Direct3D 12 and on Vulkan. Windowed, the game ran to its end on the new Direct3D 12 path (no hang on the
waitable object), but the VM's desktop is a virtual display that shows about 4 pictures a second whatever the queue
(the build before D2 the same: 409 pictures in 72 s) and gives no frame statistics, so D2's and D3's timing there is
not measured: it needs a Windows machine with a real display.

**D1, corrected.** As planned above, `between` from the presentation time less a fixed delay would show a moment up to
an update older than the one before the newest, which the game does not keep (Jeff's F3: between the last two updates):
it would need a third update kept, a refresh more delay, undoing D2. What D1 is for, pictures drawn at an even time
against the refresh, is what the request timing gives: with D2 on Metal the draw times are locked to the refresh (above),
and Direct3D 12's waitable object starts each picture at the swap chain's signal by construction. Measured on Metal, no
picture is off its moment but at a missed refresh, which no timing of the request can give back. Jeff, 2026-11-23, asked whether to
count D1 as done by D2 or build a display-link timing anyway: "Done by D2". No separate D1 is made.

## Considered, no request

- **The echo buffer's address (ESA).** The driver sets it (`sound_driver.c:1623`), and the engine has no call for
  it. Jeff asked what is right for the engine: the engine has no sound memory, so an echo address means something
  only where the echo shares memory with data, which the engine's model does not have; the delay (EDL), the buffer's
  size, is the console capability, and `nshvc_echo` takes it. The original's buffer ($E800-$FFFF) overlaps none of the
  sound data, which ends at $E7F9, so dropping it changes no sound.
- **When inside a driver tick a voice change lands.** The original's writes fall over its tick's SPC700 time (its
  key-ons 20-29 samples after the tick begins); the native driver's calls come at a tick's start. That is the
  original's processor speed, not a console capability: no request (`docs/aladdin/native/SOUND.md`).
- **The engine's mixer against bsnes.** The original's own DSP writes, given to the engine before the sample after
  the one each was made in, differ from bsnes's output by 2.7 % to 118.5 % of the energy per run
  (`docs/aladdin/native/SOUND.md`). Whether that is the whole-sample timing of the replay or the mixer is not
  established; it is for the engine's owner to look at, and no request is made until it is.
