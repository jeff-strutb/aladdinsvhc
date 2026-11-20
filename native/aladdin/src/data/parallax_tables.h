/*
 * parallax_tables.h: the cartridge's tables the parallax reads (video/parallax.c).
 */
#ifndef ALADDIN_PARALLAX_TABLES_H
#define ALADDIN_PARALLAX_TABLES_H

#include <stdint.h>

/* the band sets (BandTable 81:C848), by the set's byte offset (0 or 2):
   its bands' count times 2 ($80:C602), and its edges ($80:C606): the
   first two words the sets' starts (byte offsets into the edges), then
   each set's lines in the level where a band starts, closed by $7FFF */
uint16_t band_set_count2(uint16_t set);
uint16_t band_edge_word(uint16_t at);           /* the word at $80:C606 + at */
uint16_t band_table_place(uint16_t buffer);     /* the two band tables ($80:C628 + buffer): $F400 or $F480 */

#endif
