/*
 * kinds_d_tables.c: the places of the tables kinds session D's actor kinds
 * read (kinds_d_tables.h), in the data bank each routine's entry gives
 * (the record's comments).
 */
#include "data/kinds_d_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }

/* ---- kind $30 (83:B684 Kind30Start) ---- */

/* its probe list: the word $D17A it stores at +$16, read by the map's tests
   in their data bank $80 (four bytes a probe: x, y offsets as words) */
DataRef k30_probes(void)
{
    DataRef r;

    r.at = cart_offset(0x80, 0xD17A);
    return r;
}

/* $80:D182: a byte a progress sequence, the bit of each (01 02 04 08 10 20) */
uint8_t k30_level_mark_bit(uint8_t sequence) { return at8(0x80, (uint16_t)(0xD182 + sequence)); }

/* $80:D188: a byte a spawn point, the preset ($27 $72 $73 for spawn points 0-2) */
uint8_t k30_fall_motion(uint8_t spawn_at) { return at8(0x80, (uint16_t)(0xD188 + spawn_at)); }
