/*
 * actor_tables.c: the places of the tables the actors read (actor_tables.h).
 *
 * The figures of the opening story read their tables in bank $80: the
 * decomp's comments on kinds $95 and $96 say the data bank is $84, but the
 * code reads through whatever bank its caller left, and the attract play's
 * trace (build/aladdin/box/attract, read with tools/aladdin/codemap.py)
 * read $80:D8B6-$80:D8FD and none of $84:D8B6-$84:D8FD.
 */
#include "data/actor_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }
static uint16_t at16(uint8_t bank, uint16_t addr) { return cart16(cart_offset(bank, addr)); }
static uint32_t at24(uint8_t bank, uint16_t addr)
{
    uint32_t o = cart_offset(bank, addr);
    return (uint32_t)cart8(o) | (uint32_t)cart8(o + 1) << 8 | (uint32_t)cart8(o + 2) << 16;
}

/* ---- placements: by level a word at $82:8A7A, the list's offset from
   there; at it a count byte, then 6-byte entries ---- */
#define PLACEMENT_BANK 0x82
#define PLACEMENT_LISTS 0x8A7A

static uint16_t placement_list(uint16_t level)
{
    return at16(PLACEMENT_BANK, (uint16_t)(PLACEMENT_LISTS + 2u * level));
}

unsigned placement_count(uint16_t level)
{
    return at8(PLACEMENT_BANK, (uint16_t)(PLACEMENT_LISTS + placement_list(level)));
}

Placement placement_at(uint16_t level, unsigned n)
{
    uint16_t e = (uint16_t)(PLACEMENT_LISTS + placement_list(level) + 1u + 6u * n);
    Placement p;

    p.kind = at8(PLACEMENT_BANK, e);
    p.byte = at8(PLACEMENT_BANK, (uint16_t)(e + 1));
    p.x_word = at16(PLACEMENT_BANK, (uint16_t)(e + 2));
    p.y_word = at16(PLACEMENT_BANK, (uint16_t)(e + 4));
    return p;
}

/* ---- creating ---- */

uint8_t kind_setup_class(uint8_t kind) { return at8(0x82, (uint16_t)(0xB000 + kind)); }

uint16_t kind_graphics_bytes(uint8_t kind)
{
    uint16_t entry = at16(0x82, (uint16_t)(0xAC18 + 2u * kind));

    return (uint16_t)(at16(0x82, (uint16_t)(0xAC58 + entry)) & 0x3FFF);
}

uint16_t palette_slot_look(uint8_t slot) { return at16(0x82, (uint16_t)(0xAC06 + 2u * slot)); }

uint8_t level_escort(uint16_t level) { return at8(0x80, (uint16_t)(0xB79E + (level & 0xFF))); }

/* ---- animation: each kind's set at $81:EF6F; per set, its picture list
   at $A6:8000 (3 bytes: an offset less $8000, then a bank past $A6) and its
   table at $A5:C000 (a word, its offset from $C000), each a count word and
   then the data ---- */

uint8_t kind_anim_set(uint8_t kind) { return at8(0x81, (uint16_t)(0xEF6F + kind)); }

const uint8_t *anim_set_pictures(uint8_t set, uint16_t *bytes)
{
    uint16_t entry = (uint16_t)(0x8000 + 3u * set);
    uint32_t o = cart_offset((uint8_t)(0xA6 + at8(0xA6, (uint16_t)(entry + 2))),
                             (uint16_t)(at16(0xA6, entry) + 0x8000));

    *bytes = cart16(o);
    return cart_at(o + 2);              /* a list running past its bank goes on in the next: the next bytes of cart.dat */
}

const uint8_t *anim_set_table(uint8_t set, uint16_t *words)
{
    uint16_t at = (uint16_t)(0xC000 + at16(0xA5, (uint16_t)(0xC000 + 2u * set)));

    *words = (uint16_t)(at16(0xA5, at) / 2 + 1);
    return cart_at(cart_offset(0xA5, (uint16_t)(at + 2)));
}

/* ---- motion ---- */

uint32_t speed_x_entry(uint8_t at) { return at24(0x80, (uint16_t)(0xBAAB + at)); }
uint32_t speed_y_entry(uint8_t at) { return at24(0x80, (uint16_t)(0xBB20 + at)); }
uint32_t speed_triple(uint8_t at) { return at24(0x80, (uint16_t)(0xD8D2 + at)); }

/* seven tables of a byte per preset: speed x high, middle, fraction; speed
   y high, middle, fraction; gravity */
MotionPreset motion_preset(uint8_t p)
{
    MotionPreset m;

    m.speed_x = (uint32_t)at8(0x80, (uint16_t)(0xC059 + p)) | (uint32_t)at8(0x80, (uint16_t)(0xBFCC + p)) << 8
              | (uint32_t)at8(0x80, (uint16_t)(0xBF3F + p)) << 16;
    m.speed_y = (uint32_t)at8(0x80, (uint16_t)(0xC200 + p)) | (uint32_t)at8(0x80, (uint16_t)(0xC173 + p)) << 8
              | (uint32_t)at8(0x80, (uint16_t)(0xC0E6 + p)) << 16;
    m.gravity = at8(0x80, (uint16_t)(0xC28D + p));
    return m;
}

/* ---- spawning and view ---- */

uint16_t spawn_offset_x(uint8_t at) { return at16(0x80, (uint16_t)(0xDBBB + at)); }
uint16_t spawn_offset_y(uint8_t at) { return at16(0x80, (uint16_t)(0xDC51 + at)); }

ViewBox view_box(uint8_t at)
{
    ViewBox b;

    b.x_off = at16(0x80, (uint16_t)(0xBA02 + at));
    b.x_span = at16(0x80, (uint16_t)(0xBA04 + at));
    b.y_off = at16(0x80, (uint16_t)(0xBA06 + at));
    b.y_span = at16(0x80, (uint16_t)(0xBA08 + at));
    return b;
}

/* ---- the story figures ---- */

uint8_t k95_flight_frames(uint8_t step) { return at8(0x80, (uint16_t)(0xD8F6 + step)); }

Kind95Hops k95_hops(uint8_t set)
{
    Kind95Hops h;

    h.pose = at8(0x80, (uint16_t)(0xD8B6 + set));
    h.delay = at8(0x80, (uint16_t)(0xD8BA + set));
    h.count = at8(0x80, (uint16_t)(0xD8BE + set));
    h.up_frames = at8(0x80, (uint16_t)(0xD8C2 + 2u * set));
    h.up_speed = at8(0x80, (uint16_t)(0xD8C3 + 2u * set));
    h.down_frames = at8(0x80, (uint16_t)(0xD8CA + 2u * set));
    h.down_speed = at8(0x80, (uint16_t)(0xD8CB + 2u * set));
    return h;
}

uint8_t k96_first_wait(uint8_t variant) { return at8(0x80, (uint16_t)(0xD8FC + variant)); }

/* ---- vectors: by direction set, its first vector (a word at $BBB9 in the
   data bank: $80, as every task runs and no caller of the vector routines
   changes it); per vector, six tables of a byte in bank $82, $409 apart:
   x fraction, pixels low, high; y fraction, pixels low, high ---- */

uint16_t vector_set_first(uint8_t at) { return at16(0x80, (uint16_t)(0xBBB9 + at)); }

uint32_t vector_x(uint16_t v)
{
    return (uint32_t)at8(0x82, (uint16_t)(0xC000 + v)) | (uint32_t)at8(0x82, (uint16_t)(0xC409 + v)) << 8
         | (uint32_t)at8(0x82, (uint16_t)(0xC812 + v)) << 16;
}

uint32_t vector_y(uint16_t v)
{
    return (uint32_t)at8(0x82, (uint16_t)(0xCC1B + v)) | (uint32_t)at8(0x82, (uint16_t)(0xD024 + v)) << 8
         | (uint32_t)at8(0x82, (uint16_t)(0xD42D + v)) << 16;
}
