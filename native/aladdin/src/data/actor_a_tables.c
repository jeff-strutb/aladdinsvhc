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

uint16_t frame_y_step(uint8_t frame) { return at16(0x80, (uint16_t)(0xD1CE + 2u * frame)); }
uint16_t ahead_5(uint8_t facing) { return at16(0x80, (uint16_t)(0xD176 + 2u * facing)); }

/* ---- A5:EA93 ActorSpeedXFromA5, A5:EAA3 ActorSpeedYFromA5 (written by
   n5-kinds-d for the demo's guard; the guard runs them with the data bank
   $80: at $A5 the same offsets are other data) ---- */

static uint32_t at24(uint8_t bank, uint16_t addr)
{
    return (uint32_t)at8(bank, addr) | (uint32_t)at8(bank, (uint16_t)(addr + 1)) << 8
           | (uint32_t)at8(bank, (uint16_t)(addr + 2)) << 16;
}

uint32_t a5_speed_x(uint8_t at) { return at24(0x80, (uint16_t)(0xCFB7 + at)); }
uint32_t a5_speed_y(uint8_t at) { return at24(0x80, (uint16_t)(0xCFCF + at)); }

/* ---- the reach and hop tables of 83:FCA5 and 83:FD2C: the traces read
   $80:D9B0-D9B4 and $80:D9BF-D9E7 as data; the same offsets in banks $81
   and $83 are code ---- */
ReachEntry reach_entry(uint8_t at)
{
    ReachEntry e;

    e.across = at8(0x80, (uint16_t)(0xD9B0 + at));
    e.down = at8(0x80, (uint16_t)(0xD9B1 + at));
    e.speed = at8(0x80, (uint16_t)(0xD9B2 + at));
    e.step = at8(0x80, (uint16_t)(0xD9B3 + at));
    e.limit = at8(0x80, (uint16_t)(0xD9B4 + at));
    return e;
}

HopEntry hop_entry(uint8_t at)
{
    HopEntry e;

    e.gravity = at8(0x80, (uint16_t)(0xD9BF + at));
    e.up = at16(0x80, (uint16_t)(0xD9C0 + at));
    e.across = at16(0x80, (uint16_t)(0xD9C2 + at));
    return e;
}

/* ---- 81:A73D ActorMoveAlong, 83:F031 ActorYByFrame, 84:8F3C ActorClampAboveFloor:
   the traces read $80:D673-D68A, $80:D7C4-D7C7 and the six direction tables
   (bank $82 as the record names it) as data; $81:D673 and $83:D673 on are code ---- */
uint16_t frame_y_step_83(uint8_t frame) { return at16(0x80, (uint16_t)(0xD673 + 2u * frame)); }
uint16_t floor_clearance(void) { return at16(0x80, 0xD7C4); }

DirectionStep direction_step(uint16_t direction)
{
    DirectionStep s;
    uint16_t at = (uint16_t)(direction + 0x01B6);

    s.x[0] = at8(0x82, (uint16_t)(0xC000 + at));
    s.x[1] = at8(0x82, (uint16_t)(0xC409 + at));
    s.x[2] = at8(0x82, (uint16_t)(0xC812 + at));
    s.y[0] = at8(0x82, (uint16_t)(0xCC1B + at));
    s.y[1] = at8(0x82, (uint16_t)(0xD024 + at));
    s.y[2] = at8(0x82, (uint16_t)(0xD42D + at));
    return s;
}

uint16_t a5_spot_x(uint8_t spot) { return cart16(cart_offset(0x80, (uint16_t)(0xD02C + spot))); }
uint16_t a5_spot_y(uint8_t spot) { return cart16(cart_offset(0x80, (uint16_t)(0xD02E + spot))); }

/* ---- the by-$40 tables (84:8A48 ActorAnimBy40, 84:8D8F ActorStartBy40) and 84:8CD2's: read in
   the data bank $80 the actor code runs with (the traces read $80:D714-D723, $80:D6EB-D70A,
   $80:D70B-D713 and $80:D6B9-D6BA as data; the same bytes in $81 and $83 are code, and
   $84:D6EB-D70A are never read) ---- */
uint8_t anim_by_40(uint8_t index) { return at8(0x80, (uint16_t)(0xD714 + index)); }
uint16_t start_by_40_word(uint8_t index) { return at16(0x80, (uint16_t)(0xD6EB + 2u * index)); }
uint8_t start_by_40_byte(uint8_t index) { return at8(0x80, (uint16_t)(0xD70B + index)); }
uint8_t parity_bits(uint8_t parity) { return at8(0x80, (uint16_t)(0xD6B9 + parity)); }
