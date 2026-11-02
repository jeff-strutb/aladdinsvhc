/*
 * actor_steps.c: small steps of some actors' code: rising after a delay,
 * a height that follows the animation's frame, a flag each of eight
 * actors sets once, and a test picked by X.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorAccelerateY(void);
void abi_ActorStepYBack(void);

/* In bank $80 (DB): the y change for each frame of the animation
   (words), by ACTOR_FRAME. */
#define FRAME_Y_STEPS 0xD673u
/* In bank $80 (DB): a bit mask for each of 8 (by the low 3 bits of $07). */
#define BIT_MASKS 0xD316u
#define V_0359 0x000359u          /* eight flags, set by the actors that use these */
/* Jump tables in the code's bank: 4 cases by bits 3-4 of $07; the tests
   picked by X. */
#define FLAG_CASES 0xCF29u
#define TESTS_BY_X 0xFBA9u

/*
 * @routine 83:A17A ActorRiseAfterDelay
 * WHAT IT DOES: Counts its delay ($47, a word) down; once it is out, the
 * actor moves up by its vertical speed, and the speed grows by its
 * gravity while its whole part is below 2.
 * HOW: DEC on the word while it is not 0; else ActorStepYBack, then
 * ActorAccelerateY when the speed's whole low byte is below 2.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: counting: M 8-bit; A low byte = $47 OR $48 before; N
 * and Z from the decrement. Else as ActorStepYBack (speed 2 up: A = the
 * speed's whole low byte, C set, N and Z from its compare with 2) or
 * ActorAccelerateY leaves them. X, Y unchanged.
 */
void abi_ActorRiseAfterDelay(void)
{
    lda8(ea_dp(ACTOR_47));
    ora8_v(rd8(ea_dp(ACTOR_48)));
    if (IF_Z) goto rise;
    op_rep(0x20);
    MEM16(dec, ea_dp(ACTOR_47));
    op_sep(0x20);
    return;
rise:
    abi_ActorStepYBack();
    lda8(ea_dp(ACTOR_SPEED_Y + 1));
    cmp8_v(0x02);
    if (IF_C) return;
    abi_ActorAccelerateY();
}

/*
 * @routine 83:F031 ActorYByFrame
 * WHAT IT DOES: When the actor's animation frame has changed, moves it up
 * or down by that frame's step, so its height follows the animation.
 * HOW: The frame is compared with the one kept in $49; when new, the
 * word of the frame-steps table (bank $80, $D673) by it is added to the
 * y pixels; then $49 = the frame.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: Y = the frame;
 * same frame: Z set, C set, the rest unchanged. New: M 8-bit; A = the new
 * y pixels (16 bits); X = frame * 2; N, V, Z, C from the addition.
 */
void abi_ActorYByFrame(void)
{
    ldy8(ea_dp(ACTOR_FRAME));
    cpy8_v(rd8(ea_dp(ACTOR_49)));
    if (IF_Z) goto keep;
    tya8();
    ACC8(asl);
    tax8();
    op_rep(0x20);
    clc();
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(rd16(ea_absx(FRAME_Y_STEPS)));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
keep:
    sty8(ea_dp(ACTOR_49));
}



/*
 * @routine 83:FBA6 ActorTestByX
 * WHAT IT DOES: Runs the actor test X picks, from the table at $83:FBA9,
 * for callers in any bank.
 * HOW: JMP (table,X); the tests end in RTL.
 * Entry: M 8-bit, X 8-bit; X = the test's offset (0 or 2); called by JSL.
 * Exit: as the test leaves them.
 */
void abi_ActorTestByX(void)
{
    jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(TESTS_BY_X + cpu.x)));
}
/*
 * @routine 83:CF10 ActorFlagOnce
 * WHAT IT DOES: Each of eight such actors (by the low 3 bits of $07) has a
 * flag in $0359: when its flag is set already, it is set again and back;
 * else the case by bits 3-4 of $07 runs (from the table at $83:CF29).
 * HOW: The flag's mask (bank $80, $D316) goes in $41 and is tested
 * against $0359; set: TSB; clear: JMP through the case table.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR. Exit:
 * set already: A low byte = the mask; Z clear; X = the actor's number
 * (0-7). Else as the case leaves them.
 */
void abi_ActorFlagOnce(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x07);
    tax8();
    lda8(ea_absx(BIT_MASKS));
    sta8(ea_dp(ACTOR_41));
    and8_v(rd8(ea_abs(V_0359)));
    if (!IF_Z) goto mark;
    lda8(ea_dp(ACTOR_07));
    and8_v(0x18);
    ACC8(lsr);
    ACC8(lsr);
    tax8();
    jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(FLAG_CASES + cpu.x)));
    return;
mark:
    lda8(ea_dp(ACTOR_41));
    tsb8(ea_abs(V_0359));
}
