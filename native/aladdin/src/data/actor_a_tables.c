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

HitBox second_pool_box(uint8_t n)
{
    HitBox b;

    b.half_w = at16(0x80, (uint16_t)(0xC580 + 4u * n));
    b.half_h = at16(0x80, (uint16_t)(0xC582 + 4u * n));
    return b;
}

Zone zone_at(uint8_t at)
{
    Zone z;

    z.half_w = at8(0x80, (uint16_t)(0xBDA6 + at));
    z.half_h = at8(0x80, (uint16_t)(0xBDA7 + at));
    z.ahead = (int8_t)at8(0x80, (uint16_t)(0xBDA8 + at));
    z.down = (int8_t)at8(0x80, (uint16_t)(0xBDA9 + at));
    return z;
}

DataRef shot_probes(void)
{
    DataRef r;

    r.at = cart_offset(0x80, 0xC9FF);
    return r;
}

RingStep ring_step(uint8_t at)
{
    RingStep r;

    r.dx = at16(0x80, (uint16_t)(0xD36E + at));
    r.dy = at16(0x80, (uint16_t)(0xD370 + at));
    return r;
}

uint8_t drop_sort_maybe(uint8_t chance) { return at8(0x80, (uint16_t)(0xD31E + (chance & 0x1F))); }
uint8_t drop_sort_always(uint8_t chance) { return at8(0x80, (uint16_t)(0xD33E + (chance & 0x1F))); }

HitBox solid_box(uint8_t at)
{
    HitBox b;

    b.half_w = at16(0x80, (uint16_t)(0xBD66 + at));
    b.half_h = at16(0x80, (uint16_t)(0xBD68 + at));
    return b;
}
