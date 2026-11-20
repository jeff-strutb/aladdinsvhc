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

#endif
