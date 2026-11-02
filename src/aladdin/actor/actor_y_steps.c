/*
 * actor_y_steps.c: actors' heights stepped from tables in bank $80 (the
 * bank DB names while actors run): by the animation frame, and the bob
 * of the actor whose code is at $83:C363 (up 4 pixels a frame for 14
 * frames, back 1 a frame for 56, or the other way by its byte at $07).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

/* In the bank DB names: a word per animation frame, added to the y
   pixels when the frame changes (the actor at $83:BE01's). */
#define FRAME_Y_STEPS_47 0xD1CEu
/* 4 bytes an entry, by the actor's byte at $07 (0 or 1), the return's
   entries 2 on: a word added to the y pixels, then a byte added (with the
   carry) to their high byte. */
#define BOB_STEPS 0xD286u

/*
 * @routine 83:BDE9 ActorYByFrame47
 * WHAT IT DOES: When the actor's animation frame has changed, moves it up
 * or down by that frame's step, so its height follows the animation.
 * HOW: The frame is compared with the one kept in $47; when new, the
 * word of the frame-steps table (bank $80, $D1CE) by it is added to the
 * y pixels; then $47 = the frame.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: A low = the
 * frame. Same frame: Z set, C set, the rest unchanged. New: X = frame *
 * 2; N and Z from the frame; V and C from the addition; M 8-bit; Y and
 * the other flags unchanged.
 */
void abi_ActorYByFrame47(void)
{
    lda8(ea_dp(ACTOR_FRAME));
    cmp8_v(rd8(ea_dp(ACTOR_47)));
    if (IF_Z) goto keep;
    ACC8(asl);
    tax8();
    op_rep(0x21);
    lda16(ea_absx(FRAME_Y_STEPS_47));
    adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8(ea_dp(ACTOR_FRAME));
keep:
    sta8(ea_dp(ACTOR_47));
}

/* The step by entry A: (A * 4) into BOB_STEPS. */
static void bob_step(void)
{
    ACC8(asl);
    ACC8(asl);
    tax8();
    op_rep(0x20);
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(rd16(ea_absx(BOB_STEPS)));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8(ea_dp(ACTOR_Y + 2));
    adc8_v(rd8(ea_absx(BOB_STEPS + 2)));
    sta8(ea_dp(ACTOR_Y + 2));
}

/*
 * @routine 83:C3C5 ActorBobReturnStep
 * WHAT IT DOES: One frame of the bob's return: moves the actor by the
 * step of entry 2 or 3 (by its byte at $07) of the bob's table.
 * HOW: $07 plus 2, then on into ActorBobStep past its LDA.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as
 * ActorBobStep.
 */
void abi_ActorBobReturnStep(void)
{
    lda8(ea_dp(ACTOR_07));
    ACC8(inc);
    ACC8(inc);
    bob_step();
}

/*
 * @routine 83:C3CB ActorBobStep
 * WHAT IT DOES: One frame of the bob's first move: moves the actor by the
 * step of entry 0 or 1 (by its byte at $07) of the bob's table.
 * HOW: The entry's word is added to the y pixels (16-bit A), then its
 * third byte, with the carry, to their high byte.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: A low = the new
 * high byte; X = the entry's offset; N, V, Z and C from the last
 * addition; M 8-bit; Y and the other flags unchanged.
 */
void abi_ActorBobStep(void)
{
    lda8(ea_dp(ACTOR_07));
    bob_step();
}
