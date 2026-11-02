/*
 * actor_overlap.c: an actor's own tests of what touches it: the records of
 * the second free list overlapping its hit box, and the test box's centre
 * inside a zone placed beside it by its facing.
 */
#include "machine.h"
#include "ops65.h"
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
    lda8(ea_dp(ACTOR_08));
    if (IF_N) goto none;
    bit8_v(rd8(ea_dp(ACTOR_09)));
    if (!IF_V) goto none;
    op_rep(0x30);
    ldx16_v(0x0004);
    stx16(ea_abs(SECOND_POOL_LEFT));
    ldy16_v(SECOND_POOL);
    do {
        lda16(ea_absy(ACTOR_STATE));
        and16_v(0x00FF);
        if (IF_Z) goto next;
        lda16(ea_absy(ACTOR_08));
        and16_v(0x4080);
        cmp16_v(0x4000);
        if (!IF_Z) goto next;
        lda16(ea_absy(ACTOR_06));
        and16_v(0x000F);
        ACC16(asl);
        ACC16(asl);
        tax16();
        phy16();
        lda16(ea_dp(ACTOR_06));
        and16_v(0x00FF);
        ACC16(asl);
        ACC16(asl);
        tay16();
        lda16(ea_absx(SECOND_POOL_BOXES));
        clc();
        adc16_v(rd16(ea_absy(HIT_BOXES)));
        sta16(ea_abs(SCRATCH));
        ACC16(asl);
        sta16(ea_abs(SCRATCH + 2));
        clc();
        lda16(ea_absx(SECOND_POOL_BOXES + 2));
        adc16_v(rd16(ea_absy(HIT_BOXES + 2)));
        pha16();
        sec();
        sbc16_v(0x0004);
        sta16(ea_abs(SCRATCH + 4));
        pla16();
        ACC16(asl);
        sta16(ea_abs(SCRATCH + 6));
        ply16();
        sec();
        lda16(ea_dp(ACTOR_X + 1));
        sbc16_v(rd16(ea_absy(ACTOR_X + 1)));
        clc();
        adc16_v(rd16(ea_abs(SCRATCH)));
        cmp16_v(rd16(ea_abs(SCRATCH + 2)));
        if (IF_C) goto next;
        sec();
        lda16(ea_dp(ACTOR_Y + 1));
        sbc16_v(rd16(ea_absy(ACTOR_Y + 1)));
        clc();
        adc16_v(rd16(ea_abs(SCRATCH + 4)));
        cmp16_v(rd16(ea_abs(SCRATCH + 6)));
        if (!IF_C) {
            op_sep(0x20);
            return;
        }
next:
        clc();
        tya16();
        adc16_v(0x0050);
        tay16();
        MEM16(dec, ea_abs(SECOND_POOL_LEFT));
    } while (!IF_N);
none:
    op_sep(0x21);       /* at $83:856D, before the entry */
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
    lda8(ea_dp(ACTOR_FLIP));
    op_rep(0x20);
    if (IF_Z) {
        lda16(ea_absy(ZONES + 2));
        and16_v(0x00FF);
        bit16_i(0x0080);
        if (!IF_Z)
            ora16_v(0xFF00);
        clc();
        adc16_v(rd16(ea_dp(ACTOR_X + 1)));
        sta16(ea_abs(SCRATCH));
    } else {
        lda16(ea_absy(ZONES + 2));
        and16_v(0x00FF);
        bit16_i(0x0080);
        if (!IF_Z)
            ora16_v(0xFF00);
        sta16(ea_abs(SCRATCH));
        sec();
        lda16(ea_dp(ACTOR_X + 1));
        sbc16_v(rd16(ea_abs(SCRATCH)));
        sta16(ea_abs(SCRATCH));
    }
    lda16(ea_absy(ZONES + 3));
    and16_v(0x00FF);
    bit16_i(0x0080);
    if (!IF_Z)
        ora16_v(0xFF00);
    clc();
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_abs(SCRATCH + 2));
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
    abi_ActorZonePlace();
    lda16(ea_absy(ZONES));
    and16_v(0x00FF);
    clc();
    adc16_v(rd16(ea_abs(TEST_BOX_HALF_W)));
    sta16(ea_abs(SCRATCH + 4));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 6));
    lda16(ea_absy(ZONES + 1));
    and16_v(0x00FF);
    clc();
    adc16_v(rd16(ea_abs(TEST_BOX_HALF_H)));
    sta16(ea_abs(SCRATCH + 8));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 10));
    sec();
    lda16(ea_abs(SCRATCH));
    sbc16_v(rd16(ea_abs(TEST_BOX_X)));
    clc();
    adc16_v(rd16(ea_abs(SCRATCH + 4)));
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));
    if (!IF_C) {
        sec();
        lda16(ea_abs(SCRATCH + 2));
        sbc16_v(rd16(ea_abs(TEST_BOX_Y)));
        clc();
        adc16_v(rd16(ea_abs(SCRATCH + 8)));
        cmp16_v(rd16(ea_abs(SCRATCH + 10)));
    }
    op_sep(0x20);
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
    if (IF_C)
        return;
    abi_PlayerEnter8CFacing();
    clc();
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
    tya8();
    clc();
    adc8_v(ZONE_FIRST);
    tay8();
    abi_ActorZonePlace();
    lda16(ea_absy(ZONES));
    and16_v(0x00FF);
    sta16(ea_abs(SCRATCH + 4));
    ACC16(asl);
    sta16(ea_abs(SCRATCH + 6));
    lda16(ea_absy(ZONES + 1));
    and16_v(0x00FF);
    sta16(ea_abs(SCRATCH + 10));
    clc();
    lda16(ea_abs(TEST_BOX_X));
    adc16_v(rd16(ea_abs(SCRATCH + 4)));
    sec();
    sbc16_v(rd16(ea_abs(SCRATCH)));
    if (!IF_C) goto out;
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));
    if (IF_C) goto out;
    lda16(ea_abs(TEST_BOX_Y));
    sec();
    sbc16_v(rd16(ea_abs(SCRATCH + 2)));
    if (!IF_C) goto out;
    cmp16_v(rd16(ea_abs(SCRATCH + 10)));
    op_sep(0x30);
    return;
out:
    op_sep(0x31);
}
