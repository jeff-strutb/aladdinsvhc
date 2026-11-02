/*
 * actor_overlap.c: an actor's own tests of what touches it: the records of
 * the second free list overlapping its hit box, and the test box's centre
 * inside a zone placed beside it by its facing.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
#include "actor.h"
#include "wram.h"

#define SECOND_POOL 0x09CBu        /* the five records of the second free list, $50 apart */
#define SECOND_POOL_LEFT 0x001490u /* the records left to test, less one (word) */
/* In the bank DB names: actors' half-width and half-height (words) by
   ACTOR_06 (HIT_BOXES); the second pool's by the low 4 bits of its kind
   (SECOND_POOL_BOXES); a zone's half-width and height (bytes) by Y, the
   zone's offset added to $64 (ZONES). */
#define HIT_BOXES 0xC29Au
#define SECOND_POOL_BOXES 0xC580u
#define ZONES 0xBDA6u
#define ZONE_FIRST 0x64
#define TEST_BOX_X 0x001484u       /* the test box's centre */
#define TEST_BOX_Y 0x001486u
#define TEST_BOX_HALF_W 0x001488u  /* the test box's half-width and half-height */
#define TEST_BOX_HALF_H 0x00148Au

void abi_PlayerEnter8CFacing(void);
void abi_ActorZoneHasTestPoint(void);

/* A word of the bank DB names. */
static uint16_t db16(uint16_t addr) { return rd16(dba(addr)); }

/* `a` - `b` + `sum` against `twice`, unsigned: nonzero when below (the
   original's SEC, SBC, CLC, ADC and CMP, their flags kept). */
static int within(uint16_t a, uint16_t b, uint16_t sum, uint16_t twice)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(a, b, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, sum, &cpu.p);
    cpu.a = d;
    cmp16_of(d, twice);
    return d < twice;
}

/*
 * @routine 83:8578 ActorTouchesSecondPool
 * WHAT IT DOES: Tells whether one of the five records of the second free
 * list, in use and able to be hit, overlaps the actor's hit box; the
 * actor tests none when its own byte at $08 has bit 7 set or it cannot be
 * hit.
 * HOW: The records from $09CB, $50 apart, state nonzero and bytes $08-$09
 * & $4080 = $4000; each record's box (by its kind's low 4 bits) and the
 * actor's (by ACTOR_06) give the sums of half-widths and of half-heights
 * (scratch words $000C-$0013, the height's less 4), and the difference of
 * positions plus a sum must be below its double, unsigned, x then y.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM,
 * the boxes' bank. Exit: C clear when one overlaps: Y = its record, X =
 * its box's offset, A = the last sum (16 bits), N and Z from its compare,
 * V from its addition. None: C set; Y = $0B5B, $1490 = $FFFF, A and X as
 * the last record's tests leave them. M 8-bit, X 16-bit either way. Not
 * tested: C set, A low = its byte at $08, N, V and Z as LDA and BIT of its
 * bytes leave them, X and Y unchanged; M 8-bit.
 */
void abi_ActorTouchesSecondPool(void)
{
    uint16_t y, x, mine, sum;

    a8_load(rd8(dpa(ACTOR_08)));
    if ((uint8_t)cpu.a & 0x80)
        goto none;
    bit8_of((uint8_t)cpu.a, rd8(dpa(ACTOR_09)));
    if (!(cpu.p & P_V))
        goto none;
    op_rep(0x30);
    cpu.x = 0x0004;
    wr16(dba((uint16_t)SECOND_POOL_LEFT), 0x0004);
    y = SECOND_POOL;
    do {
        /* in use, can be hit, not out of play */
        a16_load(db16((uint16_t)(ACTOR_STATE + y)) & 0x00FF);
        if (!cpu.a)
            goto next;
        cpu.a = db16((uint16_t)(ACTOR_08 + y)) & 0x4080;
        cmp16_of(cpu.a, 0x4000);
        if (cpu.a != 0x4000)
            goto next;
        /* the sums of half-widths and half-heights (less 4), doubled */
        x = (uint16_t)((db16((uint16_t)(ACTOR_06 + y)) & 0x000F) << 2);
        cpu.x = x;
        mine = (uint16_t)((dp16(ACTOR_06) & 0x00FF) << 2);
        flag_set(P_C, 0);
        sum = op_adc16(db16((uint16_t)(SECOND_POOL_BOXES + x)), db16((uint16_t)(HIT_BOXES + mine)), &cpu.p);
        wr16(dba(SCRATCH), sum);
        wr16(dba(SCRATCH + 2), (uint16_t)(sum << 1));
        flag_set(P_C, 0);
        sum = op_adc16(db16((uint16_t)(SECOND_POOL_BOXES + 2 + x)), db16((uint16_t)(HIT_BOXES + 2 + mine)), &cpu.p);
        flag_set(P_C, 1);
        wr16(dba(SCRATCH + 4), op_sbc16(sum, 0x0004, &cpu.p));
        wr16(dba(SCRATCH + 6), (uint16_t)(sum << 1));
        if (within(dp16(ACTOR_X + 1), db16((uint16_t)(ACTOR_X + 1 + y)), db16(SCRATCH), db16(SCRATCH + 2))
            && within(dp16(ACTOR_Y + 1), db16((uint16_t)(ACTOR_Y + 1 + y)), db16(SCRATCH + 4), db16(SCRATCH + 6))) {
            cpu.y = y;
            op_sep(P_M);
            return;
        }
next:
        flag_set(P_C, 0);
        y = op_adc16(y, 0x0050, &cpu.p);
        cpu.a = y;
        cpu.y = y;
        dec16_at(dba((uint16_t)SECOND_POOL_LEFT));
    } while (!(cpu.p & P_N));
none:
    op_sep(0x21);
}

/*
 * @routine 83:8570 ActorTouchesSecondPoolLong
 * WHAT IT DOES: ActorTouchesSecondPool for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorTouchesSecondPool; called by JSL. Exit: as
 * ActorTouchesSecondPool.
 */
void abi_ActorTouchesSecondPoolLong(void)
{
    abi_ActorTouchesSecondPool();
}

/* A zone's signed byte at `at` as a word. */
static uint16_t zone_offset(uint16_t at)
{
    uint16_t v = db16(at) & 0x00FF;

    return v & 0x0080 ? (uint16_t)(v | 0xFF00) : v;
}

/*
 * @routine 83:879B ActorZonePlace
 * WHAT IT DOES: Places zone Y beside the actor: the zone's point, ahead of
 * the actor by its facing, in $000C (x) and $000E (y).
 * HOW: The zone's signed x offset (byte 2 of its record) is added to the
 * actor's x pixels when its flip byte is zero and taken from them
 * otherwise; its signed y offset (byte 3) is added to its y pixels.
 * Entry: M 8-bit; Y = the zone's offset (the zones' table index); D = the
 * actor; DB a bank whose $0000-$1FFF is work RAM, the zones' bank; called
 * by JSR. Exit: M 16-bit; A = the point's y; N, V, Z, C from its addition;
 * X, Y unchanged.
 */
void abi_ActorZonePlace(void)
{
    uint16_t dx = zone_offset((uint16_t)(ZONES + 2 + cpu.y));

    /* x ahead of the actor by its facing, y below */
    op_rep(P_M);
    if (!rd8(dpa(ACTOR_FLIP))) {
        flag_set(P_C, 0);
        wr16(dba(SCRATCH), op_adc16(dx, dp16(ACTOR_X + 1), &cpu.p));
    } else {
        wr16(dba(SCRATCH), dx);
        flag_set(P_C, 1);
        wr16(dba(SCRATCH), op_sbc16(dp16(ACTOR_X + 1), dx, &cpu.p));
    }
    flag_set(P_C, 0);
    cpu.a = op_adc16(zone_offset((uint16_t)(ZONES + 3 + cpu.y)), dp16(ACTOR_Y + 1), &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
}

/*
 * @routine 83:8752 ActorZoneHasTestBox
 * WHAT IT DOES: Tells whether the test box overlaps zone Y beside the
 * actor.
 * HOW: ActorZonePlace; the zone's half-width plus the test box's ($1488)
 * in $0010, doubled in $0012; its half-height plus the test box's ($148A)
 * in $0014, doubled in $0016; the point's x less the test box's centre,
 * plus the half-width sum, must be below the doubled sum, unsigned, then
 * the same for y.
 * Entry: M 8-bit; Y = the zone's offset; D = the actor; DB a bank whose
 * $0000-$1FFF is work RAM, the zones' bank; called by JSR. Exit: C clear
 * when they overlap; A = the last sum (16 bits), N and Z from the compare
 * that decided, V from its addition; M 8-bit; X, Y unchanged.
 */
void abi_ActorZoneHasTestBox(void)
{
    uint16_t sum;

    abi_ActorZonePlace();
    flag_set(P_C, 0);
    sum = op_adc16(db16((uint16_t)(ZONES + cpu.y)) & 0x00FF, db16((uint16_t)TEST_BOX_HALF_W), &cpu.p);
    wr16(dba(SCRATCH + 4), sum);
    wr16(dba(SCRATCH + 6), (uint16_t)(sum << 1));
    flag_set(P_C, 0);
    sum = op_adc16(db16((uint16_t)(ZONES + 1 + cpu.y)) & 0x00FF, db16((uint16_t)TEST_BOX_HALF_H), &cpu.p);
    wr16(dba(SCRATCH + 8), sum);
    wr16(dba(SCRATCH + 10), (uint16_t)(sum << 1));
    if (within(db16(SCRATCH), db16((uint16_t)TEST_BOX_X), db16(SCRATCH + 4), db16(SCRATCH + 6)))
        within(db16(SCRATCH + 2), db16((uint16_t)TEST_BOX_Y), db16(SCRATCH + 8), db16(SCRATCH + 10));
    op_sep(P_M);
}

/*
 * @routine 83:8743 ActorZoneHurtsPlayer
 * WHAT IT DOES: When the test box overlaps zone Y beside the actor, the
 * player is hurt (PlayerEnter8CFacing).
 * HOW: ActorZoneHasTestBox; C clear: PlayerEnter8CFacing, then C cleared.
 * Entry: as ActorZoneHasTestBox. Exit: no overlap: as ActorZoneHasTestBox
 * (C set). Hurt: C clear; the rest as PlayerEnter8CFacing leaves it.
 */
void abi_ActorZoneHurtsPlayer(void)
{
    abi_ActorZoneHasTestBox();
    if (cpu.p & P_C)
        return;
    abi_PlayerEnter8CFacing();
    flag_set(P_C, 0);
}

/*
 * @routine 83:87E8 ActorZoneHasTestPointLong
 * WHAT IT DOES: ActorZoneHasTestPoint for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorZoneHasTestPoint; called by JSL. Exit: as
 * ActorZoneHasTestPoint.
 */
void abi_ActorZoneHasTestPointLong(void)
{
    abi_ActorZoneHasTestPoint();
}
/*
 * @routine 83:87EC ActorZoneHasTestPoint
 * WHAT IT DOES: Tells whether the test box's centre is inside zone Y
 * beside the actor: a box placed by the actor's position and facing.
 * HOW: $64 added to Y; $83:879B puts the zone's point (top, centre) in
 * $000C and $000E; the centre's x less the point's, plus the half-width,
 * must be below the width, and its y less the point's below the height,
 * unsigned. The half-width, width and height are kept at $0010, $0012
 * and $0016.
 * Entry: M 8-bit, X 8-bit; Y = the zone's offset; D = the actor; DB a
 * bank whose $0000-$1FFF is work RAM, the zones' bank. Exit: C clear when
 * inside; A = the last difference (16 bits), N, Z and C from the test
 * that decided, V from the last subtraction; Y = the zone's offset plus
 * $64; X unchanged; M and X 8-bit.
 */
void abi_ActorZoneHasTestPoint(void)
{
    uint16_t half, d;

    flag_set(P_C, 0);
    a8_load(op_adc8((uint8_t)cpu.y, ZONE_FIRST, &cpu.p));
    cpu.y = (uint8_t)cpu.a;
    abi_ActorZonePlace();
    /* the zone's half-width, width and height */
    half = db16((uint16_t)(ZONES + cpu.y)) & 0x00FF;
    wr16(dba(SCRATCH + 4), half);
    wr16(dba(SCRATCH + 6), (uint16_t)(half << 1));
    cpu.a = db16((uint16_t)(ZONES + 1 + cpu.y)) & 0x00FF;
    wr16(dba(SCRATCH + 10), cpu.a);
    /* the centre's x less the point's, plus the half-width, below the
       width; its y less the point's below the height */
    flag_set(P_C, 0);
    d = op_adc16(db16((uint16_t)TEST_BOX_X), half, &cpu.p);
    flag_set(P_C, 1);
    cpu.a = d = op_sbc16(d, db16(SCRATCH), &cpu.p);
    if (!(cpu.p & P_C))
        goto out;
    cmp16_of(d, db16(SCRATCH + 6));
    if (cpu.p & P_C)
        goto out;
    flag_set(P_C, 1);
    cpu.a = d = op_sbc16(db16((uint16_t)TEST_BOX_Y), db16(SCRATCH + 2), &cpu.p);
    if (!(cpu.p & P_C))
        goto out;
    cmp16_of(d, db16(SCRATCH + 10));
    op_sep(0x30);
    return;
out:
    op_sep(0x31);
}
