/*
 * kinds_d_tables.h: the cartridge's tables kinds session D's actor kinds
 * read (data/kinds_d_tables.c holds their places). Each reader gives an
 * entry by what it holds.
 */
#ifndef ALADDIN_KINDS_D_TABLES_H
#define ALADDIN_KINDS_D_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#include "data/data.h"

/* ---- kind $30, the pickups ---- */

DataRef k30_probes(void);           /* its probe list (Actor.probes; map/'s tests read it) */

/* the progress sequence's bit in g.level_marks_taken: whose health mark is taken */
uint8_t k30_level_mark_bit(uint8_t sequence);
/* types 6 and 7: the motion preset of their first drop, by their spawn_at */
uint8_t k30_fall_motion(uint8_t spawn_at);

/* ---- kind $20, the guard ---- */

DataRef k20_probes(void);       /* its probe list (Actor.probes) */
DataRef k20_pictures(void);     /* its picture list (Actor.own_pictures) */
/* the x offset of a point ahead of it: `at` its facing times 2 plus an
   entry's byte offset (0: 11 pixels, 4: 7 pixels); a signed word */
int16_t k20_ahead_x(uint8_t at);

/* ---- kind $5C, the Genie's head of level 8 ---- */

DataRef k5c_pictures(void);     /* its picture list (Actor.own_pictures) */
/* path `n` (0-2, by Aladdin's lives): its place, the word the original keeps
   at +$16 */
uint16_t k5c_path(uint8_t n);
/* a path's point at byte offset `at` (4 bytes a point): its x, pixels; and
   the kind of the child made there. A point whose first byte is $FF ends
   the path: k5c_path_end */
uint16_t k5c_path_x(uint16_t path, uint8_t at);
uint8_t k5c_path_kind(uint16_t path, uint8_t at);
bool k5c_path_end(uint16_t path, uint8_t at);
/* by distance (`at`: a byte offset, 4 an entry): its speed limit (8.8) and
   the speed's step a frame */
uint16_t k5c_speed_limit(uint8_t at);
uint8_t k5c_speed_step(uint8_t at);

/* ---- kind $5D, its cloud ---- */

uint8_t k5d_heights_at(uint8_t path);   /* path 0-2: its heights' offset */
uint16_t k5d_height(uint8_t at);        /* at that offset plus twice the cloud's number: the y it rises to */

/* ---- kind $87, the catcher ---- */

DataRef k87_probes(void);       /* the catcher's probe list */
DataRef k87_child_probes(void); /* a child's */
int16_t k87_shake_x(uint8_t step);      /* the shake's step 1-8: its move across, pixels */
uint8_t k87_child_wait(uint8_t n);      /* child $07 1-5: frames it waits its turn (0 none) */

/* ---- kind $08, the guard ---- */

DataRef k08_probes(void);       /* its probe list */
DataRef k08_pictures(void);     /* its picture list */
/* the point its look ahead tests: by `at` (its speed's middle byte and its
   facing, a byte offset of words), its x and y offsets */
uint16_t k08_step_x(uint8_t at);
uint16_t k08_step_y(uint8_t at);
uint8_t k08_jump_motion(uint8_t left);  /* its jump up's motion preset by the probes left */

/* ---- kind $A4, the Genie's screen's figures ---- */

uint16_t ka4_shake(uint8_t entry);      /* way 2's shake of the screen's y, entry 8 down to 0 (a signed word) */

/* ---- kind $53, a thing held then dropped ---- */

DataRef k53_probes(void);       /* its probe list */
/* where it is held, by its holder's animation frame: x and y pixels from the
   holder's place (signed words) */
int16_t k53_held_x(uint8_t frame);
int16_t k53_held_y(uint8_t frame);

/* ---- kind $6A, its spawners ---- */

uint8_t k6a_pause(uint8_t spawner);     /* the frames a spawner ($07 0-3) waits between its children */

/* ---- kind $69 ---- */

DataRef k69_probes(void);       /* its probe list */

/* ---- kind $A7, a figure of the ending ---- */

/* its place by its $07 (4 bytes: x, y words): from the play field's scroll
   for $07 above 0 */
uint16_t ka7_place_x(uint8_t v07);
uint16_t ka7_place_y(uint8_t v07);

/* ---- kind $A5, the bombs and their column ---- */

DataRef ka5_probes(void);       /* the bomb's probe list */
uint16_t ka5_place_x(uint8_t n);        /* the column's x by its $07 less one */
uint8_t ka5_height(uint8_t n);          /* a piece's y (the pixels' low byte) by its $07's low four bits */

/* ---- kind $47, the carpet course's platforms ---- */

/* the platform's boxes ($80:D2D9, 10 bytes a box): x offset, width, y offset,
   height, top offset (words) */
typedef struct PlatformBox { uint16_t x_off, width, y_off, height, top_off; } PlatformBox;
PlatformBox k47_box(uint8_t box);
/* the stretches where the course turns ($80:D2F7: their count, then 6 bytes
   each): the first x pixel and the x pixel past it */
unsigned k47_turn_zones(void);
uint16_t k47_turn_from(unsigned zone);
uint16_t k47_turn_to(unsigned zone);
/* the rates the dip sinks (0) and rises (1) at ($80:D2ED, 24 bits each) */
uint32_t k47_dip_rate(unsigned rising);
/* the step along the course a direction (0, 1) takes at a speed unit
   ($80:D304, 8 bytes a direction): its across and down bytes */
void k47_course_step(unsigned direction, uint8_t x[3], uint8_t y[3]);

#endif
