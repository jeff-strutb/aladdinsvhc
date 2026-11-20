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
#include "data/data.h"              /* DataRef */

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

HitBox second_pool_box(uint8_t n);  /* $80:C580, by a second-pool kind's low 4 bits (4 bytes): half-width, half-height */
/* a zone beside an actor ($80:BDA6, 4 bytes from `at`): half-width,
   half-height, and its place from the actor's: across along its facing,
   down (signed) */
typedef struct Zone { uint8_t half_w, half_h; int8_t ahead, down; } Zone;
Zone zone_at(uint8_t at);
DataRef shot_probes(void);
/* ActorSpawnRing's points ($80:D36E, 4 bytes from `at`): each from the last, across and down */
typedef struct RingStep { uint16_t dx, dy; } RingStep;
RingStep ring_step(uint8_t at);
/* FlagDropMaybe's and FlagDropAlways' child sorts by chance ($80:D31E and
   $80:D33E, 32 bytes each; the first's 0: none) */
HitBox solid_box(uint8_t at);
uint16_t frame_y_step(uint8_t frame);   /* $80:D1CE, a word by frame: ActorYByFrame47's step down */
uint16_t ahead_5(uint8_t facing);       /* $80:D176, a word by facing: 5 pixels ahead */       /* $80:BD66, 4 bytes from `at`: a solid actor's half-width, half-height */
uint8_t drop_sort_maybe(uint8_t chance);
uint8_t drop_sort_always(uint8_t chance);          /* the shots' probe list ($80:C9FF; Actor.probes) */

/* the speed tables of A5:EA93 ActorSpeedXFromA5 and A5:EAA3
   ActorSpeedYFromA5, read in bank $80 ($80:CFB7, $80:CFCF): 3 bytes an
   entry (fraction, pixels low, high), entry at byte offset `at` */
uint32_t a5_speed_x(uint8_t at);
uint32_t a5_speed_y(uint8_t at);

/* ActorRiseWhenInReach's and PlayerInReach's reach table ($80:D9B0, 8 bytes
   an entry at byte offset `at`): his reach across and down (0: not tested),
   the rise's start speed and step (each in 16ths) and its top speed */
typedef struct ReachEntry { uint8_t across, down, speed, step, limit; } ReachEntry;
ReachEntry reach_entry(uint8_t at);

/* ActorHopFromTable's hops ($80:D9BF, 5 bytes an entry at byte offset `at`):
   its gravity, the vertical speed's low two bytes (the third is $FF, up)
   and the horizontal speed's two bytes */
typedef struct HopEntry { uint8_t gravity; uint16_t up; uint16_t across; } HopEntry;
HopEntry hop_entry(uint8_t at);

/* ActorYByFrame's step down by the animation frame ($80:D673, a word by frame) */
uint16_t frame_y_step_83(uint8_t frame);

/* the floor's clearance ActorClampAboveFloor takes from the floor line ($80:D7C4, a word) */
uint16_t floor_clearance(void);

/* ActorMoveAlong's direction table (bank $82, from $01B6 + direction): the
   step along it as three bytes each across and down (low, middle, high) */
typedef struct DirectionStep { uint8_t x[3], y[3]; } DirectionStep;
DirectionStep direction_step(uint16_t direction);

#endif
