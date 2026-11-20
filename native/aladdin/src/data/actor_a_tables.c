/*
 * actor_a_tables.c: the places of the tables the actors' shared routines
 * read (actor_a_tables.h).
 */
#include "data/actor_a_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint8_t bank, uint16_t addr) { return cart8(cart_offset(bank, addr)); }
static uint16_t at16(uint8_t bank, uint16_t addr) { return cart16(cart_offset(bank, addr)); }

ViewBox range_box(uint8_t at)
{
    ViewBox b;

    b.x_off = at16(0x80, (uint16_t)(0xCE73 + at));
    b.x_span = at16(0x80, (uint16_t)(0xCE75 + at));
    b.y_off = at16(0x80, (uint16_t)(0xCE77 + at));
    b.y_span = at16(0x80, (uint16_t)(0xCE79 + at));
    return b;
}

HitBox hit_box(uint8_t kind)
{
    HitBox b;

    b.half_w = at16(0x80, (uint16_t)(0xC29A + 4u * kind));
    b.half_h = at16(0x80, (uint16_t)(0xC29C + 4u * kind));
    return b;
}

HitBox near_box(uint8_t at)
{
    HitBox b;

    b.half_w = at16(0x80, (uint16_t)(0xBC43 + at));
    b.half_h = at16(0x80, (uint16_t)(0xBC45 + at));
    return b;
}

uint16_t near_span_x(uint8_t at) { return at16(0x80, (uint16_t)(0xBC7B + at)); }
uint16_t near_span_y(uint8_t at) { return at16(0x80, (uint16_t)(0xBC85 + at)); }

FrontBox front_box(uint8_t at)
{
    FrontBox b;

    b.half_h = at16(0x80, (uint16_t)(0xBC8F + at));
    b.off_right = at16(0x80, (uint16_t)(0xBC91 + at));
    b.off_left = at16(0x80, (uint16_t)(0xBC93 + at));
    return b;
}

uint8_t direction_by_band(uint8_t i) { return at8(0x80, (uint16_t)(0xBBE3 + i)); }
uint8_t direction_by_step(uint8_t i) { return at8(0x80, (uint16_t)(0xBC03 + i)); }
