/*
 * slot_gfx.h: the sprite palette and graphics slots past the original's four
 * (docs/aladdin/native/VIEW.md, stage V10).
 *
 * The original gives each live class 2 kind (or class 4 actor) a slot: sprite
 * palettes 4 to 7 and the sprite tiles $100, $140, $180, $1C0. A view larger
 * than 256 x 224 has ceil(4 * w * h / (256 * 224)) slots. Slot 5 and up have
 * a sprite palette of their own (slot s: sprite palette s + 3, entries
 * 256 + 16 (s - 5), which g.palette holds from entry 256 on) and a 64-tile range
 * of their own in a tile set of their own (video_slot_tiles), not in the
 * original's 1,024-tile set, whose tiles past 512 are the backgrounds' wrap.
 *
 * A look (Actor.look, g.kind_look) is 32 bits: the low 16 are the original's
 * (tile base in bits 0-8, sprite palette in bits 9-11, bit 15 loaded); bits
 * 16 up are the slot's number when it is 5 or more, else 0. An actor of such a
 * slot draws its tiles from the slot's range and, while its palette bits are
 * 7 (what the slot's look has), the slot's palette; effects that put another
 * palette in the bits (a flash) show that one, as in the original.
 */
#ifndef ALADDIN_VIDEO_SLOT_GFX_H
#define ALADDIN_VIDEO_SLOT_GFX_H

#include <stdint.h>

#define SLOTS_STD 4             /* the original's */
#define SLOTS_MAX 1028          /* the most: the slot tiles' numbers are 16 bits (64 x 1,024 of them), a view's sides are 4,096 at most */
#define SLOT_TILES 64           /* the sprite tiles of a slot */
#define SLOT_PALETTE_BITS 0x0E00 /* a look's palette bits, 7 for an actor in the slot's own palette */
#define PALETTE_ENTRIES (256 + 16 * (SLOTS_MAX - SLOTS_STD))  /* g.palette: the SNES's 256, then the slots' */

/* The VRAM word address of a slot's tiles in the queue's `to` (VramQueueEntry): from here on, 16 words a tile of the slot tile set. */
#define VRAM_SLOT_TILES 0x10000u

unsigned slots_count(void);                 /* the view's slots, from 4 (set at init) */
static inline unsigned look_slot(uint32_t look) { return look >> 16; }   /* 5 and up, or 0: not in a slot past the original's */
uint32_t slot_look(unsigned slot);          /* slot 1-4: palette_slot_look; 5 and up: loaded, palette bits 7, tile base 0, the slot in the high half */
uint32_t look_vram(uint32_t look, unsigned tile);   /* the VRAM word address of `tile` of the look's tile range */
uint32_t vram_add(uint32_t address, uint16_t step); /* address + step: 16 bits for the original's range, none past it */
unsigned look_palette_number(uint32_t look);        /* the look's palette: 8 to 15, or 16 and up for a slot's own (palette 16 + slot - 5) */

#endif
