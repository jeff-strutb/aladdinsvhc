/*
 * kinds_d_tables.h: the cartridge's tables kinds session D's actor kinds
 * read (data/kinds_d_tables.c holds their places). Each reader gives an
 * entry by what it holds.
 */
#ifndef ALADDIN_KINDS_D_TABLES_H
#define ALADDIN_KINDS_D_TABLES_H

#include <stdint.h>

/* ---- kind $30, the pickups ---- */

/* the progress sequence's bit in g.level_marks_taken: whose health mark is taken */
uint8_t k30_level_mark_bit(uint8_t sequence);
/* types 6 and 7: the motion preset of their first drop, by their spawn_at */
uint8_t k30_fall_motion(uint8_t spawn_at);

#endif
