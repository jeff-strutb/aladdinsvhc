/*
 * actor_y_steps.c: actors' heights stepped from tables in bank $80 (the
 * bank DB names while actors run): by the animation frame, and the bob
 * of the actor whose code is at $83:C363 (up 4 pixels a frame for 14
 * frames, back 1 a frame for 56, or the other way by its byte at $07).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint8_t frame = rd8(dpa(ACTOR_FRAME)), was = rd8(dpa(ACTOR_47));

    a8_load(frame);
    cmp8_of(frame, was);
    if (frame != was) {                   /* a new frame: its y step */
        cpu.x = (uint8_t)(frame << 1);
        op_rep(0x21);
        cpu.a = op_adc16(rd16(dba(FRAME_Y_STEPS_47) + cpu.x), dp16(ACTOR_Y + 1), &cpu.p);
        dp16_set(ACTOR_Y + 1, cpu.a);
        op_sep(P_M);
        a8_load(frame);
    }
    wr8(dpa(ACTOR_47), frame);
}

/* The step by entry A: (A * 4) into BOB_STEPS. */
/* The y moved by bob step `step` (4 bytes an entry: the y word's step,
   then the top byte's). */
static void bob_step(uint8_t step)
{
    uint16_t y;
    uint8_t top;

    cpu.x = (uint8_t)(step << 2);
    op_rep(P_M);
    flag_set(P_C, 0);
    y = op_adc16(dp16(ACTOR_Y + 1), rd16(dba(BOB_STEPS) + cpu.x), &cpu.p);
    dp16_set(ACTOR_Y + 1, y);
    op_sep(P_M);
    top = op_adc8(rd8(dpa(ACTOR_Y + 2)), rd8(dba(BOB_STEPS + 2) + cpu.x), &cpu.p);
    wr8(dpa(ACTOR_Y + 2), top);
    cpu.a = (uint16_t)((y & 0xFF00) | top);
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
    bob_step((uint8_t)(rd8(dpa(ACTOR_07)) + 2));
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
    bob_step(rd8(dpa(ACTOR_07)));
}
