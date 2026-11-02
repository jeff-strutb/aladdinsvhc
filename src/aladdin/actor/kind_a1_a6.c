/*
 * kind_a1_a6.c: actor kinds $A1 in state $4C (hit: it flies off) and $A6
 * (it stays until far above or below the point actors are kept in range
 * of). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);

#define RANGE_Y 0x1A46u  /* the y of the point actors are kept in range of (word) */

/*
 * @routine 84:A9FC KindA1Hit
 * @actor
 * WHAT IT DOES: Actor kind $A1 in state $4C (hit): it flies off (motion
 * $1F), after 8 frames when its $41 is set, until it can no longer be hit,
 * and ends.
 * HOW: Animation 0, $08 bit 4 set; $41 nonzero: COP 8; ActorMotionPreset
 * $1F; each frame ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A1 in state $4C (through the stub at
 * $83:9412). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA1Hit(void)
{
    lda8_v(0x00);  /* 84A9FC lda */
    abi_AnimStart();  /* 84A9FE jsl */
    lda8(ea_dp(ACTOR_08));  /* 84AA02 lda */
    ora8_v(0x10);  /* 84AA04 ora */
    sta8(ea_dp(ACTOR_08));  /* 84AA06 sta */
    lda8(ea_dp(ACTOR_41));  /* 84AA08 lda */
    if (IF_Z) goto L_84AA10;  /* 84AA0A beq */
    lda8_v(0x08);  /* 84AA0C lda */
    actor_wait(0x84AA0Eu);  /* 84AA0E cop */
L_84AA10:
    ldy8_v(0x1F);  /* 84AA10 ldy */
    abi_ActorMotionPreset();  /* 84AA12 jsl */
L_84AA16:
    actor_yield(0x84AA16u);  /* 84AA16 brk */
    abi_ActorFly();  /* 84AA18 jsl */
    bit8_v(rd8(ea_dp(ACTOR_09)));  /* 84AA1C bit */
    if (IF_V) goto L_84AA16;  /* 84AA1E bvs */
    abi_ActorEndPlaced(); return;  /* 84AA20 jml */
}


/*
 * @routine 84:AC3F KindA6Stay
 * @actor
 * WHAT IT DOES: Actor kind $A6: shown (animation 0, flipped by its $07)
 * where it was placed until it is 256 or more pixels above or below the
 * point actors are kept in range of, then ends.
 * HOW: $08 = 3, AnimStart 0, flip = $07; each frame (BRK first) RANGE_Y
 * less its y plus $0100, unsigned below $0200: again; else ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A6 in state $0C (through the stub at
 * $83:943E). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA6Stay(void)
{
    lda8_v(0x03);  /* 84AC3F lda */
    sta8(ea_dp(ACTOR_08));  /* 84AC41 sta */
    lda8_v(0x00);  /* 84AC43 lda */
    abi_AnimStart();  /* 84AC45 jsl */
    lda8(ea_dp(ACTOR_07));  /* 84AC49 lda */
    sta8(ea_dp(ACTOR_FLIP));  /* 84AC4B sta */
    op_rep(0x20);  /* 84AC4D rep */
L_84AC4F:
    actor_yield(0x84AC4Fu);  /* 84AC4F brk */
    sec();  /* 84AC51 sec */
    lda16(ea_abs(RANGE_Y));  /* 84AC52 lda */
    sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 84AC55 sbc */
    clc();  /* 84AC57 clc */
    adc16_v(0x0100);  /* 84AC58 adc */
    cmp16_v(0x0200);  /* 84AC5B cmp */
    if (!IF_C) goto L_84AC4F;  /* 84AC5E bcc */
    op_sep(0x20);  /* 84AC60 sep */
    abi_ActorEndPlaced(); return;  /* 84AC62 jml */
}
