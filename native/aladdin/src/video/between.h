/*
 * between.h: pictures between updates (docs/aladdin/native/FRAMERATE.md, sections 4 and 6; Jeff's decisions F1, F3, F4).
 *
 * The engine draws once a display refresh and passes `between`, how far real time is past the last update towards the
 * next (0 to 1). A picture is one moment: the world the update before left, its moving parts eased `between` of the way
 * towards the newest. So a picture is never ahead of the game and never shows what no update made; it is up to one
 * update (1/60 s) behind.
 *
 * How the update before's world is kept:
 *  - the game state (g) and the video state the picture reads (registered with between_keep) are copied at the end of
 *    each of the world's steps: the copy before is "was", the newest "now";
 *  - what an update writes to the engine's tiles, maps and palette is held (between_tiles_write and the rest, which the
 *    video code calls in place of the engine's) and written a step later, so the engine's art is always the world
 *    "was" names;
 *  - a picture puts "was" in, draws, and puts the live state back. The game never sees "was".
 * Headless pictures are at between 0: the update before's whole picture.
 *
 * Stepped effects (FRAMERATE.md, R4; Jeff's "Item 1", 2026-11-23): a track steps when its value changes, and a picture
 * shows it eased from its step before to its newest over the updates between those two steps (up to a limit), so it is
 * exact at each step and runs behind by up to a step. The world steps every update, but in the chain scene's slow
 * motion only when the play loop turns (between_play_turn): it is eased over the play loop's wait. The brightness
 * (a fade's steps of one level, over up to 2 updates) and the window edges (over up to 3) are tracks of their own.
 */
#ifndef VIDEO_BETWEEN_H
#define VIDEO_BETWEEN_H

#include <stddef.h>
#include <stdint.h>

#include "nshvc/nshvc.h"

/* main.c: around each update, and each picture */
void between_update_begin(void);
void between_update_end(void);
void between_draw(float between);

/* the play loop's turn (game/play_loop.c): the world steps */
void between_play_turn(void);

/* a part of the video state the picture reads that an update changes: kept as "was" with g (register at init) */
void between_keep(void *p, size_t bytes);
/* a kept part moved and grown (its contents so far kept): `old` is now `p`, `bytes` long */
void between_rekeep(void *old, void *p, size_t bytes);

/* while a picture draws (from "was"): whether it is eased, by how much, the live ("now") state and a kept part's now */
struct GameState;
int between_easing(void);
float between_amount(void);
const struct GameState *between_now(void);
const void *between_now_of(const void *live);
/* how far a place moves in the picture: was and now in fixed point (frac_bits of fraction) wrapping at 2^bits, eased
   between of the way the shorter way round; returns floor(eased) - floor(was) in whole units, or 0 when not drawing or
   when they are more than `far` whole units apart (a place set anew: a scene's cut, a respawn) */
int32_t between_ease(int64_t was, int64_t now, unsigned frac_bits, unsigned bits, int32_t far);
/* whether a picture passes its places' fractions (the engine's _sub fields, 6.10): while drawing, but for the
   developer's ALADDIN_DEV_WHOLE_PIXELS */
int between_fractions(void);
/* the eased place itself, in the same units as was and now (wrapping at 2^bits, the shorter way round; was when they
   are more than `far` apart or when not drawing) */
int64_t between_ease_fx(int64_t was, int64_t now, unsigned bits, int64_t far);

/* while a picture draws: its brightness in 256ths of a level (the level in bits 8-11; the fraction for the engine's
   brightness_sub); outside one, g's */
uint32_t between_brightness(void);
/* while a picture draws: the states of the windows' step before the newest and of the newest, and how far between
   them it is (0 to 1); and where the easing starts: `from` of the way from the state of the step before those (`prev`)
   to `was` (1 but where the newest step came before the easing to `was` had ended); 0 when not drawing */
int between_windows(const struct GameState **prev, float *from, const struct GameState **was, const struct GameState **now,
                    float *at);
/* while a picture draws: Mode 7's matrix (a, b, c, d), centre and scales (in 256ths of the game's 256ths), each eased
   with its own steps (the matrix and centre over up to 8 updates, the scales up to 24); 0 when not drawing */
int between_mode7(int16_t matrix[4], int16_t centre[2], int32_t scale_fx[4]);
/* the easing in hand's amount set (a track's, for its easing); the amount before returned */
float between_amount_set(float at);

/* the engine's content writes, held while an update runs (written at once outside one) */
void between_tiles_write(nshvc_tiles t, uint32_t first, const void *data, uint32_t count, nshvc_tile_format f, size_t bytes);
void between_map_write(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const nshvc_map_entry *entries);
void between_map_write_snes(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *words);
void between_palette_write(uint32_t first, uint32_t count, const uint16_t *bgr15);

#endif
