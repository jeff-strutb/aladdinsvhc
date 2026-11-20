/*
 * actor_a_tables.h: the cartridge's tables the actors' shared routines
 * read (actors A; data/actor_a_tables.c holds their places). The shared
 * routines run with the data bank $80 their task left (no actor code
 * changes it but for a few lines of its own), so their tables are bank
 * $80's: the bytes there are data, the same offsets in banks $81-$84 code.
 */
#ifndef ALADDIN_ACTOR_A_TABLES_H
#define ALADDIN_ACTOR_A_TABLES_H

#include <stdint.h>

#include "data/actor_tables.h"      /* ViewBox */

ViewBox range_box(uint8_t at);
typedef struct HitBox { uint16_t half_w, half_h; } HitBox;
HitBox hit_box(uint8_t kind);       /* $80:C29A, by kind (4 bytes): its box's half-width and half-height */      /* $80:CE73, 8 bytes a box: x offset, x span, y offset, y span (words) */

/* round the player or a point ($80:BC43, 4 bytes a box): half-width, half-height */
HitBox near_box(uint8_t at);
uint16_t near_span_x(uint8_t at);   /* $80:BC7B, 2 bytes a span: a half-width */
uint16_t near_span_y(uint8_t at);   /* $80:BC85: a half-height */
/* in front of an actor ($80:BC8F, 6 bytes a box from `at`): its half-height,
   and across from the actor's x its offset facing right and facing left;
   its width is the two offsets added */
typedef struct FrontBox { uint16_t half_h, off_right, off_left; } FrontBox;
FrontBox front_box(uint8_t at);
uint8_t direction_by_band(uint8_t i);   /* $80:BBE3: by quadrant * 4 + band */
uint8_t direction_by_step(uint8_t i);   /* $80:BC03: by octant * 8 + step */

#endif
