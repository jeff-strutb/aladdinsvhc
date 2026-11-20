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

/* ---- kind $08 (A5:E515 GuardStart, A5:EAE6 GuardStepAhead, A5:F19D
   GuardLedgeJump): read in bank $80 (the same offsets in bank $A5 are not
   these tables: its speeds and points there make no sense) ---- */

DataRef k08_probes(void) { return ref(0x80, 0xCF8F); }
DataRef k08_pictures(void) { return ref(0x80, 0x8909); }
uint16_t k08_step_x(uint8_t at) { return cart16(cart_offset(0x80, (uint16_t)(0xCFF4 + at))); }
uint16_t k08_step_y(uint8_t at) { return cart16(cart_offset(0x80, (uint16_t)(0xD010 + at))); }
uint8_t k08_jump_motion(uint8_t left) { return at8(0x80, (uint16_t)(0xD054 + left)); }

/* ---- kind $A4 (84:AA24 KindA4Start): $80:D972, a word an entry
   (2, 2, 1, 1, 0, -1, -1, -2, -2 for entries 0-8) ---- */

uint16_t ka4_shake(uint8_t entry) { return cart16(cart_offset(0x80, (uint16_t)(0xD972 + 2u * entry))); }

/* ---- kind $53 (83:D566 Kind53Held) ---- */

DataRef k53_probes(void) { return ref(0x80, 0xD38C); }
/* $80:D394, 4 bytes a frame: x then y, words */
int16_t k53_held_x(uint8_t frame) { return (int16_t)cart16(cart_offset(0x80, (uint16_t)(0xD394 + 4u * frame))); }
int16_t k53_held_y(uint8_t frame) { return (int16_t)cart16(cart_offset(0x80, (uint16_t)(0xD396 + 4u * frame))); }

/* ---- kind $6A (83:EB5E Kind6AStart): $80:D65B, a byte by $07 ---- */

uint8_t k6a_pause(uint8_t spawner) { return at8(0x80, (uint16_t)(0xD65B + spawner)); }

/* ---- kind $69 (83:E9F7 Kind69Start): the word $D657 it stores at +$16 ---- */

DataRef k69_probes(void) { return ref(0x80, 0xD657); }

/* ---- kind $A7 (84:AC66 KindA7Start): $80:D99E, 4 bytes by $07 ---- */

uint16_t ka7_place_x(uint8_t v07) { return cart16(cart_offset(0x80, (uint16_t)(0xD99E + 4u * v07))); }
uint16_t ka7_place_y(uint8_t v07) { return cart16(cart_offset(0x80, (uint16_t)(0xD9A0 + 4u * v07))); }

/* ---- kind $A5 (84:AB9C KindA5Start), in bank $80 ---- */

DataRef ka5_probes(void) { return ref(0x80, 0xD984); }
uint16_t ka5_place_x(uint8_t n) { return cart16(cart_offset(0x80, (uint16_t)(0xD988 + 2u * n))); }
uint8_t ka5_height(uint8_t n) { return at8(0x80, (uint16_t)(0xD98E + n)); }

/* ---- kind $47 (83:C96E PlatformCourseStart and its pieces), in bank $80 ---- */

PlatformBox k47_box(uint8_t box)
{
    uint16_t at = (uint16_t)(0xD2D9 + box);
    PlatformBox b;

    b.x_off = cart16(cart_offset(0x80, at));
    b.width = cart16(cart_offset(0x80, (uint16_t)(at + 2)));
    b.y_off = cart16(cart_offset(0x80, (uint16_t)(at + 4)));
    b.height = cart16(cart_offset(0x80, (uint16_t)(at + 6)));
    b.top_off = cart16(cart_offset(0x80, (uint16_t)(at + 8)));
    return b;
}

unsigned k47_turn_zones(void) { return at8(0x80, 0xD2F7); }
uint16_t k47_turn_from(unsigned zone) { return cart16(cart_offset(0x80, (uint16_t)(0xD2F8 + 6 * zone))); }
uint16_t k47_turn_to(unsigned zone) { return cart16(cart_offset(0x80, (uint16_t)(0xD2FA + 6 * zone))); }

uint32_t k47_dip_rate(unsigned rising)
{
    uint16_t at = (uint16_t)(0xD2ED + 3 * rising);

    return (uint32_t)at8(0x80, at) | (uint32_t)at8(0x80, (uint16_t)(at + 1)) << 8 | (uint32_t)at8(0x80, (uint16_t)(at + 2)) << 16;
}

void k47_course_step(unsigned direction, uint8_t x[3], uint8_t y[3])
{
    uint16_t at = (uint16_t)(0xD304 + 8 * direction);
    unsigned i;

    for (i = 0; i < 3; i++) {
        x[i] = at8(0x80, (uint16_t)(at + i));
        y[i] = at8(0x80, (uint16_t)(at + 3 + i));
    }
}
