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
#define ZONE_PLACE 0x83879Bu       /* not yet filed: the zone's point beside the actor, to $000C and $000E */
#define TEST_BOX_X 0x001484u       /* the test box's centre */
#define TEST_BOX_Y 0x001486u

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




