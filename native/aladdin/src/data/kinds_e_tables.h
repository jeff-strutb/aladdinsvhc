/*
 * kinds_e_tables.h: the cartridge's tables kinds E's actors read
 * (data/kinds_e_tables.c holds their places). Each reader gives an entry by
 * what it holds.
 */
#ifndef ALADDIN_KINDS_E_TABLES_H
#define ALADDIN_KINDS_E_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* ---- picture lists (an actor's +$29: the list ActorPictureSend sends its
   own pictures from) ---- */

DataRef k21_pictures(void);     /* kind $21's */
DataRef k98_pictures(void);     /* kind $98's */

/* ---- kind $2E, level 5's closing scene ---- */

/* part 2's place beside Aladdin: by `at` (his frame times 2, a byte), the
   offsets added to his x and y, pixels */
uint16_t k2e_beside_x(uint8_t at);
uint16_t k2e_beside_y(uint8_t at);

/* ---- probe lists (an actor's +$16: the map's probes read them) ---- */

DataRef k31_probes(void);       /* kind $31's */
DataRef k98_probes(void);       /* kind $98's */

/* ---- kind $35, the gem ---- */

/* a set gem's bit in g.gem_sets_found, by `at` (its +$07's low digit, a
   byte offset into the table of words) */
uint16_t k35_set_bit(uint8_t at);

#endif
