/*
 * kind3a.c: actor kind $3A's start, a hold Aladdin hangs from: once he
 * comes into its zone it takes him (his code sent to $D6F8) and keeps him
 * at the place its picture sets, with his frame matching; when he is in
 * the air or hurt again it lets go and, 13 frames on, can take him again.
 * Its wait is Kind3AWait (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"

void abi_Kind33Look(void);
void abi_AnimStep(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_Kind3AHoldBox(void);
void abi_Kind3AHoldSpot(void);

#define K3A_SHOWN 0x40u         /* the picture it last gave Aladdin a frame for ($FF: none) */
#define K3A_42 0x42u            /* set to 1 at its start; purpose not yet established */
#define K3A_LET_GO 0x43u        /* frames left of its pause after letting him go */
#define K3A_ZONES 0x0Au         /* its zones: ActorZoneHasTestPoint's zone is ($27 + this) * 4 */
#define K3A_HELD_CODE 0xD6F8u   /* where it sends Aladdin's code while it holds him */
#define K3A_PAUSE 0x0Du         /* frames of its pause after letting him go */
#define K3A_STATE_8C_COUNT 0x141Bu /* counted up as Aladdin enters state $8C */
#define K3A_V_145A 0x145Au      /* negative: no hold */
#define K3A_V_0380 0x000380u    /* bit 2 set: no hold */
#define K3A_HOLD_X 0xD1FEu      /* bank $80: by picture, the hold's offset from its x (words) */
#define K3A_HOLD_Y 0xD210u      /* and from its y */
#define K3A_FRAMES 0xD1DEu      /* bank $80: by its $2F, Aladdin's frame while held (facing right) */
#define K3A_FRAMES_LEFT 0xD1EEu /* and facing left */
#define K3A_HOLD_SPOT 0x145Du   /* the place kept for Aladdin while held (words: x, y) */
#define K3A_BOX_RIGHT 0x14u     /* V_1A08 while he is held facing right */
#define K3A_BOX_LEFT 0x1Eu      /* and facing left */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:BEB3 Kind3AStart
 * @actor
 * WHAT IT DOES: Actor kind $3A: a hold Aladdin hangs from. When he comes
 * into its zone (not hurt, $145A not negative, $0380 bit 2 clear) it takes
 * him: his code goes to $D6F8 and it keeps him at the place its picture
 * sets, his frame by its $2F as he faces. When he is in the air ($141C) or
 * hurt ($141B) it lets go, waits 13 frames and can take him again.
 * HOW: Kind33Look; K3A_42 1, K3A_SHOWN $FF. Each frame (BRK first)
 * AnimStep; the tests above, then ActorZoneHasTestPointLong with zone
 * ($27 + 10) * 4: C clear, taken. Taken: his resume address K3A_HELD_CODE;
 * Kind3AHoldSpot; each frame (BRK first) AnimStep, Kind3AHoldBox; $141C
 * or $141B nonzero: K3A_LET_GO 13 frames (BRK first) of AnimStep, V_1A08
 * 0, and the zone test again. Else his x and y = its own plus the hold's
 * offsets by its picture, his frame time $20; its $2F other than
 * K3A_SHOWN: kept, his frame from K3A_FRAMES (K3A_FRAMES_LEFT when his
 * flip is set), his picture marked changed, his frame time 1.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $3A in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind3AStart(void)
{
    call_frame(0xBEB5, abi_Kind33Look);
    lda8_v(0x01);
    sta8(ea_dp(K3A_42));
    lda8_v(0xFF);
    sta8(ea_dp(K3A_SHOWN));
free:
    actor_yield(0x83BEBEu);
    jsl_call(0x83BEC0u, abi_AnimStep);
    lda8(ea_abs(K3A_STATE_8C_COUNT));
    if (!IF_Z)
        goto free;
    lda8(ea_abs(K3A_V_145A));
    if (IF_N)
        goto free;
    lda8(ea_long(K3A_V_0380));
    bit8_i(0x04);
    if (!IF_Z)
        goto free;
    lda8(ea_dp(ACTOR_PICTURE));
    clc();
    adc8_v(K3A_ZONES);
    ACC8(asl);
    ACC8(asl);
    tay8();
    jsl_call(0x83BEDEu, abi_ActorZoneHasTestPointLong);
    if (IF_C)
        goto free;
    lda8_v(K3A_HELD_CODE & 0xFF);
    sta8(ea_abs(PLAYER + ACTOR_03));
    lda8_v(K3A_HELD_CODE >> 8);
    sta8(ea_abs(PLAYER + ACTOR_03 + 1));
    call_frame(0xBEF0, abi_Kind3AHoldSpot);
held:
    actor_yield(0x83BEF1u);
    jsl_call(0x83BEF3u, abi_AnimStep);
    call_frame(0xBEF9, abi_Kind3AHoldBox);
    lda8(ea_abs(V_141C));
    if (!IF_Z)
        goto let_go;
    lda8(ea_abs(K3A_STATE_8C_COUNT));
    if (IF_Z)
        goto keep;
let_go:
    lda8_v(K3A_PAUSE);
    sta8(ea_dp(K3A_LET_GO));
    do {
        actor_yield(0x83BF08u);
        jsl_call(0x83BF0Au, abi_AnimStep);
        MEM8(dec, ea_dp(K3A_LET_GO));
    } while (!IF_Z);
    stz8(ea_abs(V_1A08));
    goto free;
keep:
    lda8(ea_dp(ACTOR_PICTURE));
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(K3A_HOLD_X));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_abs(PLAYER + ACTOR_X + 1));
    lda16(ea_absx(K3A_HOLD_Y));
    clc();
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));
    op_sep(0x20);
    lda8_v(0x20);
    sta8(ea_abs(PLAYER + ACTOR_FRAME_TIME));
    ldx8(ea_dp(ACTOR_FRAME));
    cpx8_v(rd8(ea_dp(K3A_SHOWN)));
    if (IF_Z)
        goto held;
    stx8(ea_dp(K3A_SHOWN));
    lda8(ea_absx(K3A_FRAMES));
    ldy8(ea_abs(PLAYER + ACTOR_FLIP));
    if (!IF_Z)
        lda8(ea_absx(K3A_FRAMES_LEFT));
    sta8(ea_abs(PLAYER + ACTOR_FRAME));
    lda8_v(0xFF);
    sta8(ea_abs(PLAYER + ACTOR_REDRAW));
    lda8_v(0x01);
    sta8(ea_abs(PLAYER + ACTOR_FRAME_TIME));
    goto held;
}

/*
 * @routine 83:BF57 Kind3AHoldBox
 * WHAT IT DOES: Sets V_1A08 for Aladdin held: $14 facing right, $1E
 * facing left (his flip set).
 * HOW: Y = K3A_BOX_RIGHT, or K3A_BOX_LEFT with his flip nonzero; STY.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: Y = the value;
 * A low = his flip; N and Z from it.
 */
void abi_Kind3AHoldBox(void)
{
    ldy8_v(K3A_BOX_RIGHT);
    lda8(ea_abs(PLAYER + ACTOR_FLIP));
    if (!IF_Z)
        ldy8_v(K3A_BOX_LEFT);
    sty8(ea_abs(V_1A08));
}

/*
 * @routine 83:BF64 Kind3AHoldSpot
 * WHAT IT DOES: Keeps the place Aladdin is held at: its x and y plus the
 * hold's offsets by its picture, in K3A_HOLD_SPOT.
 * HOW: X = its picture times 2; 16-bit, carry clear: K3A_HOLD_X,x plus
 * its x pixels, K3A_HOLD_Y,x plus its y pixels.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; A = the y (16 bits); X = the picture
 * times 2; C from the addition.
 */
void abi_Kind3AHoldSpot(void)
{
    lda8(ea_dp(ACTOR_PICTURE));
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(K3A_HOLD_X));
    adc16_v(rd16(ea_dp(ACTOR_X + 1)));
    sta16(ea_abs(K3A_HOLD_SPOT));
    lda16(ea_absx(K3A_HOLD_Y));
    clc();
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_abs(K3A_HOLD_SPOT + 2));
    op_sep(0x20);
}
