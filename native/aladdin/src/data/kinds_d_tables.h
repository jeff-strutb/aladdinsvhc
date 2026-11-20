/*
 * kinds_d_tables.h: the cartridge's tables kinds session D's actor kinds
 * read (data/kinds_d_tables.c holds their places). Each reader gives an
 * entry by what it holds.
 */
#ifndef ALADDIN_KINDS_D_TABLES_H
#define ALADDIN_KINDS_D_TABLES_H

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

#endif
