/*
 * actor_waits_small.c: small actor routines: two routines actors run each
 * frame they wait (hit tests, then their end out of range), kind $82's
 * start (shown, and stays), a turn of the vertical speed, and a place on
 * the escort actor. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorDownedByShot(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorHitByShotCC(void);
void abi_ActorPictureSend(void);
void abi_ActorStompOrHurt(void);
void abi_AnimStart(void);
void abi_AnimStep(void);

#define WAIT_HITS 0x42u  /* counted up each frame a shot does not down it */

/*
 * @routine 83:AA47 ActorWaitHitsRange8
 * WHAT IT DOES: What an actor (entry $2D of the actor loop's wait table at
 * $83:8F4F) runs each frame it waits: its animation and picture, the
 * stomp-or-hurt and shot tests, then its end when it is out of range by
 * box 8.
 * HOW: AnimStep, ActorPictureSend, ActorStompOrHurt, ActorHitByShotCC; X =
 * 8, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_ActorWaitHitsRange8(void)
{
    abi_AnimStep();  /* 83AA47 jsl */
    abi_ActorPictureSend();  /* 83AA4B jsl */
    abi_ActorStompOrHurt();  /* 83AA4F jsr */
    abi_ActorHitByShotCC();  /* 83AA52 jsr */
    ldx8_v(0x08);  /* 83AA55 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83AA57 jmp */
}


/*
 * @routine 83:B630 ActorWaitHitsRange0
 * WHAT IT DOES: What an actor (entry $2F of the actor loop's wait table at
 * $83:8F4F) runs each frame it waits: the stomp-or-hurt and shot tests
 * ($42 counted up when no shot downs it), its animation, then its end when
 * it is out of range by box 0.
 * HOW: ActorStompOrHurt; ActorDownedByShot, C clear: $42 up; AnimStep; X =
 * 0, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0), or from 83:1FE0. Exit: as
 * ActorEndOutOfRange.
 */
void abi_ActorWaitHitsRange0(void)
{
    abi_ActorStompOrHurt();  /* 83B630 jsr */
    abi_ActorDownedByShot();  /* 83B633 jsr */
    if (IF_C) goto L_83B63A;  /* 83B636 bcs */
    MEM8(inc, ea_dp(WAIT_HITS));  /* 83B638 inc */
L_83B63A:
    abi_AnimStep();  /* 83B63A jsl */
    ldx8_v(0x00);  /* 83B63E ldx */
    abi_ActorEndOutOfRange(); return;  /* 83B640 jmp */
}


/*
 * @routine 84:951D Kind82Show
 * @actor
 * WHAT IT DOES: Actor kind $82: shown with animation 0 (bits $B0 of $09,
 * bit 1 of $08 set) and stays.
 * HOW: $08 bit 1 set, $09 bits $B0 set, AnimStart 0, BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $82 in state $0C (through the stub at
 * $83:92CF). Exit: never returns.
 */
void abi_Kind82Show(void)
{
    lda8(ea_dp(ACTOR_08));  /* 84951D lda */
    ora8_v(0x02);  /* 84951F ora */
    sta8(ea_dp(ACTOR_08));  /* 849521 sta */
    lda8(ea_dp(ACTOR_09));  /* 849523 lda */
    ora8_v(0xB0);  /* 849525 ora */
    sta8(ea_dp(ACTOR_09));  /* 849527 sta */
    lda8_v(0x00);  /* 849529 lda */
    abi_AnimStart();  /* 84952B jsl */
L_84952F:
    actor_yield(0x84952Fu);  /* 84952F brk */
    goto L_84952F;  /* 849531 bra */
}


/*
 * @routine 83:B109 ActorSpeedYTurn
 * WHAT IT DOES: Turns the actor's vertical speed round: the 24-bit speed
 * negated (its high byte inverted, the low word negated).
 * HOW: The speed's top byte EOR $FF; the low word EOR $FFFF plus 1
 * (16-bit); M 8-bit.
 * Entry: M 8-bit; D = the actor's record; called by JSR (83:B0FA). Exit: M
 * 8-bit; A = the new low word; N and Z from it.
 */
void abi_ActorSpeedYTurn(void)
{
    lda8(ea_dp(ACTOR_SPEED_Y + 2));  /* 83B109 lda */
    eor8_v(0xFF);  /* 83B10B eor */
    sta8(ea_dp(ACTOR_SPEED_Y + 2));  /* 83B10D sta */
    op_rep(0x20);  /* 83B10F rep */
    lda16(ea_dp(ACTOR_SPEED_Y));  /* 83B111 lda */
    eor16_v(0xFFFF);  /* 83B113 eor */
    ACC16(inc);  /* 83B116 inc */
    sta16(ea_dp(ACTOR_SPEED_Y));  /* 83B117 sta */
    op_sep(0x20);  /* 83B119 sep */
    return;  /* 83B11B rts */
}


/*
 * @routine 83:EB4B ActorOnEscort
 * WHAT IT DOES: Puts the actor on the escort actor: its x the escort's,
 * its y 4 above the escort's.
 * HOW: 16-bit copies of the escort's x and of its y less 4; M 8-bit.
 * Entry: M 8-bit; D = the actor's record; called by JSR (83:EB00, 83:EB17,
 * 83:EB2C). Exit: M 8-bit; A = the new y pixels.
 */
void abi_ActorOnEscort(void)
{
    op_rep(0x20);  /* 83EB4B rep */
    lda16(ea_abs(ACTOR_ESCORT + ACTOR_X + 1));  /* 83EB4D lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 83EB50 sta */
    clc();  /* 83EB52 clc */
    lda16(ea_abs(ACTOR_ESCORT + ACTOR_Y + 1));  /* 83EB53 lda */
    adc16_v(0xFFFC);  /* 83EB56 adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83EB59 sta */
    op_sep(0x20);  /* 83EB5B sep */
    return;  /* 83EB5D rts */
}
