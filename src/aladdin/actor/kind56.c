/*
 * kind56.c: actor kind $56 of level 3, the monkey at the market's end:
 * it waits until Aladdin is right of x $0528, runs left a while, and waits
 * for his walk-out scene ($00BE); then it runs at him, gives its sign
 * ($00BE = $80) and runs off. Its wait is Kind56Wait (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorSpeedXSet(void);
void abi_ActorMoveX(void);
void abi_ActorSnapToGround(void);

#define K56_LOOKS 0x16u         /* its look table (word) */
#define K56_STEPS 0x40u         /* frames left of its first run */
#define K56_LOOK_TABLE 0xD3B4u  /* its look table's address */
#define K56_X 0x05B0u           /* where it starts (x and y pixels) */
#define K56_Y 0x01B0u
#define K56_PLAYER_X_HIGH 0x08E8u /* Aladdin's x pixels, high byte */
#define K56_PLAYER_X_LOW 0x08E7u  /* and low byte */
#define K56_CUE 0x00BEu         /* set by Aladdin's walk-out scene; it sets it to $80 in turn */
#define K56_RUN_SPEED 0x30u     /* ActorSpeedXSet's entry for its runs */
#define K56_NEAR 0xECu          /* added to its x's low byte: it runs until that is below his */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:D8E2 Kind56Start
 * @actor
 * WHAT IT DOES: Actor kind $56: put at x $05B0, y $01B0 facing left, it
 * waits until Aladdin is right of x $0528, then (after 76 frames) runs
 * right 240 frames and turns; once his walk-out scene sets $00BE it runs
 * at him until it is near (its x's low byte less $14 below his), poses,
 * sets $00BE to $80, and runs off for good, 4 pixels higher.
 * HOW: $08 bit 0 and $09 bits 7 and 4 set; its look table $D3B4; x pixels
 * $05B0, y $01B0; facing and flip 1; AnimStart 0. Each frame (BRK first)
 * until his x high byte is 5 and its low byte $28 or more. COP $4C;
 * facing, flip 0; AnimStart 1; ActorSpeedXSet $30; K56_STEPS $F0 frames
 * (BRK first) of ActorMoveX, ActorSnapToGround. $08 bits 0-1 = 2; facing,
 * flip 1; AnimStart 2; each frame (BRK first) until $00BE is set. COP
 * $40; AnimStart 1; ActorSpeedXSet $30; each frame (BRK first)
 * ActorMoveX, ActorSnapToGround until its x low byte + $EC is below his.
 * AnimStart 3; COP $80; $00BE $80; COP $10; facing, flip 0; y pixels less
 * 4; AnimStart 1; ActorSpeedXSet $30; each frame (BRK first) ActorMoveX.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $56 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind56Start(void)
{
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(K56_LOOK_TABLE);
    sta16(ea_dp(K56_LOOKS));
    lda16_v(K56_X);
    sta16(ea_dp(ACTOR_X + 1));
    lda16_v(K56_Y);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x00);
    jsl_call(0x83D905u, abi_AnimStart);
    for (;;) {
        actor_yield(0x83D909u);
        lda8(ea_abs(K56_PLAYER_X_HIGH));
        cmp8_v(0x05);
        if (!IF_Z)
            continue;
        lda8(ea_abs(K56_PLAYER_X_LOW));
        cmp8_v(0x28);
        if (IF_C)
            break;
    }
    lda8_v(0x4C);
    actor_wait(0x83D91Bu);
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    lda8_v(0x01);
    jsl_call(0x83D923u, abi_AnimStart);
    ldy8_v(K56_RUN_SPEED);
    jsl_call(0x83D929u, abi_ActorSpeedXSet);
    lda8_v(0xF0);
    sta8(ea_dp(K56_STEPS));
    do {
        actor_yield(0x83D931u);
        jsl_call(0x83D933u, abi_ActorMoveX);
        jsl_call(0x83D937u, abi_ActorSnapToGround);
        MEM8(dec, ea_dp(K56_STEPS));
    } while (!IF_Z);
    lda8(ea_dp(ACTOR_08));
    and8_v(0xFC);
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x02);
    jsl_call(0x83D94Fu, abi_AnimStart);
    do {
        actor_yield(0x83D953u);
        lda8(ea_abs(K56_CUE));
    } while (IF_Z);
    lda8_v(0x40);
    actor_wait(0x83D95Cu);
    lda8_v(0x01);
    jsl_call(0x83D960u, abi_AnimStart);
    ldy8_v(K56_RUN_SPEED);
    jsl_call(0x83D966u, abi_ActorSpeedXSet);
    do {
        actor_yield(0x83D96Au);
        jsl_call(0x83D96Cu, abi_ActorMoveX);
        jsl_call(0x83D970u, abi_ActorSnapToGround);
        clc();
        lda8(ea_dp(ACTOR_X + 1));
        adc8_v(K56_NEAR);
        cmp8_v(rd8(ea_abs(K56_PLAYER_X_LOW)));
    } while (IF_C);
    lda8_v(0x03);
    jsl_call(0x83D980u, abi_AnimStart);
    lda8_v(0x80);
    actor_wait(0x83D986u);
    lda8_v(0x80);
    sta8(ea_abs(K56_CUE));
    lda8_v(0x10);
    actor_wait(0x83D98Fu);
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    op_rep(0x20);
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(0xFFFC);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8_v(0x01);
    jsl_call(0x83D9A3u, abi_AnimStart);
    ldy8_v(K56_RUN_SPEED);
    jsl_call(0x83D9A9u, abi_ActorSpeedXSet);
    for (;;) {
        actor_yield(0x83D9ADu);
        jsl_call(0x83D9AFu, abi_ActorMoveX);
    }
}
