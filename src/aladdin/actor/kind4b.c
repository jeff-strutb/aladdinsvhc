/*
 * kind4b.c: actor kind $4B, a figure that sticks to another record for
 * 24 frames, its animation following that record's. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_AnimResume(void);
void abi_AnimStart(void);
void abi_AnimStep(void);

#define K4B_FRAMES 0x40u          /* frames left */
#define K4B_SHOWN 0x41u           /* the $07 its animation was last set from */
#define K4B_ANIM_0 0x05           /* its animations start here, by $07 */
#define K4B_FOLLOWED_40 0x0040u   /* the followed record's byte its $07 follows */

/*
 * @routine 83:CE2A Kind4BStart
 * @actor
 * WHAT IT DOES: Actor kind $4B, a figure that sticks to the record its
 * ACTOR_NEXT names for 24 frames, then ends: each frame it takes that
 * record's place, and its animation follows that record's $40 (animation
 * 5 plus it).
 * HOW: ACTOR_09 = $10; AnimStart with $07 + 5; K4B_SHOWN = $FF; the
 * palette byte ($2C) = 6; K4B_FRAMES = $18. Each frame: with 16-bit
 * registers its x and y pixels from the record at ACTOR_NEXT; that
 * record's $40 into its $07 when it differs; $07 unlike K4B_SHOWN: kept
 * there, AnimResume with $07 + 5; AnimStep; BRK; K4B_FRAMES down; at 0,
 * ActorEndPlaced. No script changes the record's $40 meanwhile (the
 * store to $07): written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $4B
 * in state $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind4BStart(void)
{
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_09));
    clc();
    lda8(ea_dp(ACTOR_07));
    adc8_v(K4B_ANIM_0);
    abi_AnimStart();
    lda8_v(0xFF);
    sta8(ea_dp(K4B_SHOWN));
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8_v(0x18);
    sta8(ea_dp(K4B_FRAMES));
    do {
        op_rep(0x30);
        ldx16(ea_dp(ACTOR_NEXT));
        lda16(ea_absx(ACTOR_X + 1));
        sta16(ea_dp(ACTOR_X + 1));
        lda16(ea_absx(ACTOR_Y + 1));
        sta16(ea_dp(ACTOR_Y + 1));
        op_sep(P_M);
        lda8(ea_absx(K4B_FOLLOWED_40));
        cmp8_v(rd8(ea_dp(ACTOR_07)));
        if (!IF_Z)
            sta8(ea_dp(ACTOR_07));
        op_sep(P_X);
        lda8(ea_dp(ACTOR_07));
        cmp8_v(rd8(ea_dp(K4B_SHOWN)));
        if (!IF_Z) {
            sta8(ea_dp(K4B_SHOWN));
            clc();
            adc8_v(K4B_ANIM_0);
            abi_AnimResume();
        }
        abi_AnimStep();
        actor_yield(0x83CE71u);
        MEM8(dec, ea_dp(K4B_FRAMES));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}
