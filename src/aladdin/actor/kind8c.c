/*
 * kind8c.c: actor kind $8C, a cloud that rises as kind $5D's does and that Aladdin bounces off, for as
 * long as its maker lasts and a little after. Its wait is Kind8CWaitWork (kind_waits_84.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorBouncesPlayerLong(void);
void abi_ActorEndPlaced(void);
void abi_AnimStart(void);
void abi_Kind5DParentGoneLong(void);
void abi_Kind5DRise(void);
void abi_Kind8CBounced(void);
void abi_SoundQueueAdd(void);

#define K8C_BOUNCED 0x40u             /* set once Aladdin has bounced off it */
#define K8C_TIMER 0x41u               /* frames left of its part */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:9979 Kind8CStart
 * @actor
 * WHAT IT DOES: Actor kind $8C (placed once in levels $08 and $18, at x
 * 16383: made by other code): a cloud that rises as kind $5D's does
 * (Kind5DRise), and that Aladdin bounces off (sound $3B, animation $12)
 * for as long as its maker lasts and for $57 frames after, then it
 * ends.
 * HOW: Kind5DRise; $08 = 2, $09 = $80 (its kind's wait runs), the
 * palette byte ($2C) = 6; AnimStart $11. Each frame (BRK)
 * Kind5DParentGoneLong: gone, to the last part; else
 * ActorBouncesPlayerLong, bounced: K8C_BOUNCED ($40) = 1,
 * Kind8CBounced, K8C_TIMER ($41) = $B0, and each frame (BRK) the same
 * tests (Kind8CBounced on a bounce) and K8C_TIMER down to 0. Last part:
 * $08 bit 4 set; K8C_TIMER = $57; each frame (BRK)
 * ActorBouncesPlayerLong (Kind8CBounced on a bounce) and K8C_TIMER down
 * to 0; JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML
 * of kind $8C's stub (Kind8CEnter, $83:9337) from the actor loop in the
 * start state.
 * Exit: never returns: ends in ActorEndPlaced.
 */
void abi_Kind8CStart(void)
{
    jsl_call(0x849979u, abi_Kind5DRise);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8_v(0x11);
    jsl_call(0x84998Bu, abi_AnimStart);
L_84998F:
    actor_yield(0x84998Fu);
    jsl_call(0x849991u, abi_Kind5DParentGoneLong);
    if (!IF_Z)
        goto L_8499BD;
    jsl_call(0x849997u, abi_ActorBouncesPlayerLong);
    if (IF_Z)
        goto L_84998F;
    lda8_v(0x01);
    sta8(ea_dp(K8C_BOUNCED));
    call_frame(0x99A3u, abi_Kind8CBounced);
    lda8_v(0xB0);
    sta8(ea_dp(K8C_TIMER));
L_8499A8:
    actor_yield(0x8499A8u);
    jsl_call(0x8499AAu, abi_Kind5DParentGoneLong);
    if (!IF_Z)
        goto L_8499BD;
    jsl_call(0x8499B0u, abi_ActorBouncesPlayerLong);
    if (IF_Z)
        goto L_8499B9;
    call_frame(0x99B8u, abi_Kind8CBounced);
L_8499B9:
    MEM8(dec, ea_dp(K8C_TIMER));
    if (!IF_Z)
        goto L_8499A8;
L_8499BD:
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x10);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x57);
    sta8(ea_dp(K8C_TIMER));
L_8499C7:
    actor_yield(0x8499C7u);
    jsl_call(0x8499C9u, abi_ActorBouncesPlayerLong);
    if (IF_Z)
        goto L_8499D2;
    call_frame(0x99D1u, abi_Kind8CBounced);
L_8499D2:
    MEM8(dec, ea_dp(K8C_TIMER));
    if (!IF_Z)
        goto L_8499C7;
    abi_ActorEndPlaced();
}

/*
 * @routine 84:99DA Kind8CBounced
 * WHAT IT DOES: What kind $8C does as Aladdin bounces off it: its
 * animation $12 and sound $3B.
 * HOW: AnimStart $12; SoundQueueAdd $3B; RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
 * Kind8CStart. Exit: M and X 8-bit.
 */
void abi_Kind8CBounced(void)
{
    lda8_v(0x12);
    jsl_call(0x8499DCu, abi_AnimStart);
    lda8_v(0x3B);
    jsl_call(0x8499E2u, abi_SoundQueueAdd);
}
