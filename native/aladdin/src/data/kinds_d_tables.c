/*
 * kinds_d_tables.c: the places of the tables kinds session D's actor kinds
 * read (kinds_d_tables.h), in the data bank each routine's entry gives
 * (the record's comments).
 */
#include "data/kinds_d_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }
static DataRef ref(uint8_t bank, uint16_t addr)
{
    DataRef r;

    r.at = cart_offset(bank, addr);
    return r;
}

/* ---- kind $30 (83:B684 Kind30Start) ---- */

/* its probe list: the word $D17A it stores at +$16, read by the map's tests
   in their data bank $80 (four bytes a probe: x, y offsets as words) */
DataRef k30_probes(void) { return ref(0x80, 0xD17A); }

/* $80:D182: a byte a progress sequence, the bit of each (01 02 04 08 10 20) */
uint8_t k30_level_mark_bit(uint8_t sequence) { return at8(0x80, (uint16_t)(0xD182 + sequence)); }

/* $80:D188: a byte a spawn point, the preset ($27 $72 $73 for spawn points 0-2) */
uint8_t k30_fall_motion(uint8_t spawn_at) { return at8(0x80, (uint16_t)(0xD188 + spawn_at)); }

/* ---- kind $20 (83:97AF Kind20Start, 83:9A15 ActorMapAtOffset) ---- */

/* the words $D05A and $890F it stores at +$16 and +$29, read in bank $80 */
DataRef k20_probes(void) { return ref(0x80, 0xD05A); }
DataRef k20_pictures(void) { return ref(0x80, 0x890F); }

/* $80:D062: per entry (4 bytes), the word for facing right then left:
   entry 0 11 and -11, entry 4 7 and -7 */
int16_t k20_ahead_x(uint8_t at) { return (int16_t)cart16(cart_offset(0x80, (uint16_t)(0xD062 + at))); }

/* ---- kind $5C (83:E000 Kind5CStart, 83:E13F Kind5CSpeedPick) ---- */

/* the word $890B it stores at +$29, in bank $80 */
DataRef k5c_pictures(void) { return ref(0x80, 0x890B); }

/* $80:D428: three words, each a path's place in bank $80; at a path, 4
   bytes a point (x word, the child's kind, 0), $FF ending it */
uint16_t k5c_path(uint8_t n) { return cart16(cart_offset(0x80, (uint16_t)(0xD428 + 2u * n))); }
uint16_t k5c_path_x(uint16_t path, uint8_t at) { return cart16(cart_offset(0x80, (uint16_t)(path + at))); }
uint8_t k5c_path_kind(uint16_t path, uint8_t at) { return at8(0x80, (uint16_t)(path + at + 2u)); }
bool k5c_path_end(uint16_t path, uint8_t at) { return at8(0x80, (uint16_t)(path + at)) == 0xFF; }

/* $80:D408: 4 bytes an entry, a word (the limit) and a byte (the step) */
uint16_t k5c_speed_limit(uint8_t at) { return cart16(cart_offset(0x80, (uint16_t)(0xD408 + at))); }
uint8_t k5c_speed_step(uint8_t at) { return at8(0x80, (uint16_t)(0xD408 + at + 2u)); }

/* ---- kind $5D (83:E3CB Kind5DRise) ---- */

/* $80:D4A8: three bytes, each path's offset from $D4A8; there a word for
   each cloud */
uint8_t k5d_heights_at(uint8_t path) { return at8(0x80, (uint16_t)(0xD4A8 + path)); }
uint16_t k5d_height(uint8_t at) { return cart16(cart_offset(0x80, (uint16_t)(0xD4A8 + at))); }

/* ---- kind $87 (83:F77F Kind87Start) ---- */

/* the words $D852 and $D856 it stores at +$16, in bank $80 */
DataRef k87_probes(void) { return ref(0x80, 0xD852); }
DataRef k87_child_probes(void) { return ref(0x80, 0xD856); }

/* $80:D843: by step a byte, the offset of its move in the words at $D84C
   (1, 2, -2) */
int16_t k87_shake_x(uint8_t step)
{
    return (int16_t)cart16(cart_offset(0x80, (uint16_t)(0xD84C + at8(0x80, (uint16_t)(0xD843 + step)))));
}

/* $80:D859: by a child's $07, its turn's frames (0, 6, 12, 6, 12 for 1-5) */
uint8_t k87_child_wait(uint8_t n) { return at8(0x80, (uint16_t)(0xD859 + n)); }
