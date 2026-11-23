/*
 * between.h: pictures between updates (docs/aladdin/native/FRAMERATE.md, sections 4 and 6; Jeff's decisions F1, F3, F4).
 *
 * The engine draws once a display refresh and passes `between`, how far real time is past the last update towards the
 * next (0 to 1). A picture is one moment: the world the update before left, its moving parts eased `between` of the way
 * towards the newest. So a picture is never ahead of the game and never shows what no update made; it is up to one
 * update (1/60 s) behind.
 *
 * How the update before's world is kept:
 *  - the game state (g) and the video state the picture reads (registered with between_keep) are copied as each update
 *    begins: that copy is "was", the live state "now";
 *  - what an update writes to the engine's tiles, maps and palette is held (between_tiles_write and the rest, which the
 *    video code calls in place of the engine's) and written as the next update begins, so the engine's art is always
 *    the world "was" names;
 *  - a picture swaps "was" in, draws, and swaps "now" back. The game never sees "was".
 * Headless pictures are at between 0: the update before's whole picture.
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

/* runahead (system/runahead.c): g and the kept parts saved, and put back (the held engine writes of what ran between are dropped) */
void between_save(void);
void between_restore(void);
uint64_t between_saved_hash(uint64_t h, int parts);   /* FNV-1a of what between_save kept; with `parts`, each part's on stderr */

/* a part of the video state the picture reads that an update changes: kept as "was" with g (register at init) */
void between_keep(void *p, size_t bytes);
/* a kept part moved and grown (its contents so far kept): `old` is now `p`, `bytes` long */
/* the developer's state hash leaves this kept part to its owner (it holds pointers) */
void between_unhashed(const void *live);
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

/* the engine's content writes, held while an update runs (written at once outside one) */
void between_tiles_write(nshvc_tiles t, uint32_t first, const void *data, uint32_t count, nshvc_tile_format f, size_t bytes);
void between_map_write(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const nshvc_map_entry *entries);
void between_map_write_snes(nshvc_map m, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *words);
void between_palette_write(uint32_t first, uint32_t count, const uint16_t *bgr15);

#endif
