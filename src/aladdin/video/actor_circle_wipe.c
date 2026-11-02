/*
 * actor_circle_wipe.c: a circle wipe that grows round an actor: one step a
 * call, the window table copied where the HDMA reads it. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_CircleWipeStep(void);

#define WIPE_RADIUS 0x45u  /* the circle's radius, one up each step (word) */
#define LAYER0_X 0x1773u  /* layer 0's x pixels (word) */
#define LAYER0_Y 0x1777u  /* layer 0's y pixels (word) */

/*
 * @routine 83:EAAF ActorCircleWipeStep
 * WHAT IT DOES: One step of a circle wipe round the actor: the radius
 * grows by one and the circle is drawn round its place on screen (8 pixels
 * above its y), then the $200-byte window table is copied from $7E:5000 to
 * $7F:F500.
 * HOW: $45 up (8-bit); X = its x less LAYER0_X, Y = its y less LAYER0_Y
 * less 8, A = $45 (16-bit); CircleWipeStep; MVN $7E:5000 to $7F:F500, $200
 * bytes, DB kept; M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * (83:EA38, 83:EA56). Exit: M and X 8-bit; A = $FFFF, X = $5200, Y = $F700
 * (low bytes).
 */
void abi_ActorCircleWipeStep(void)
{
    MEM8(inc, ea_dp(WIPE_RADIUS));  /* 83EAAF inc */
    op_rep(0x30);  /* 83EAB1 rep */
    sec();  /* 83EAB3 sec */
    lda16(ea_dp(ACTOR_X + 1));  /* 83EAB4 lda */
    sbc16_v(rd16(ea_abs(LAYER0_X)));  /* 83EAB6 sbc */
    tax16();  /* 83EAB9 tax */
    sec();  /* 83EABA sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83EABB lda */
    sbc16_v(rd16(ea_abs(LAYER0_Y)));  /* 83EABD sbc */
    sec();  /* 83EAC0 sec */
    sbc16_v(0x0008);  /* 83EAC1 sbc */
    tay16();  /* 83EAC4 tay */
    lda16(ea_dp(WIPE_RADIUS));  /* 83EAC5 lda */
    abi_CircleWipeStep();  /* 83EAC7 jsl */
    phb();  /* 83EACB phb */
    ldx16_v(0x5000);  /* 83EACC ldx */
    ldy16_v(0xF500);  /* 83EACF ldy */
    lda16_v(0x01FF);  /* 83EAD2 lda */
    mvn(0x7E, 0x7F);  /* 83EAD5 mvn */
    plb();  /* 83EAD8 plb */
    op_sep(0x30);  /* 83EAD9 sep */
    return;  /* 83EADB rts */
}
