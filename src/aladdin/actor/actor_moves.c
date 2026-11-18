/*
 * actor_moves.c: small steps of an actor's motion: speeding up and
 * moving, moving along a direction from the ROM's table, keeping its
 * place; and a byte's sign spread to a word.
 *
 * A direction's step is kept as two 24-bit values, x then y, at $0C-$0E
 * and $10-$12; each is a byte at a time multiplied by the speed.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_ActorBobAnimate(void);
void abi_AnimStep(void);
void abi_ActorBob(void);
void abi_ActorSpeedUpMove(void);
void abi_ActorMoveByVectorOver(void);
void abi_ActorMoveAlong(void);
void abi_ActorMoveScaled(void);
void abi_ActorKeepSpot(void);
void abi_SignExtend(void);
#include "hw.h"
#include "actor.h"
#include "wram.h"
void abi_ActorMove(void);
void abi_MultiplyBy(void);

/*
 * @routine 81:A4AF ActorSpeedUpMove
 * WHAT IT DOES: Speeds the actor up by its acceleration, then moves it.
 * HOW: The 24-bit horizontal speed plus the acceleration (a fraction),
 * then ActorMove.
 * Entry: M 8-bit; D = the actor; called by JSL. Exit: as ActorMove.
 */
void abi_ActorSpeedUpMove(void)
{
    uint8_t v;

    flag_set(P_C, 0);
    v = rd8(dpa(0x19));
    wr8(dpa(0x19), op_adc8(v, rd8(dpa(0x1C)), &cpu.p));
    wr8(dpa(0x1A), op_adc8(rd8(dpa(0x1A)), 0x00, &cpu.p));
    a8_load(op_adc8(rd8(dpa(0x1B)), 0x00, &cpu.p));
    wr8(dpa(0x1B), (uint8_t)cpu.a);
    abi_ActorMove();
}

/* The step at $0C-$12 times the speed: each axis's low and middle bytes
   by the multiplier, the two products overlapped into 24 bits; then added
   to the actor's x and y. Shared with motion.c. */
void actor_scale_and_move(uint8_t speed)
{
    uint8_t a, v;

    wr8(dba((uint16_t)WRMPYA), speed);
    a8_load(rd8(dba(SCRATCH)));
    abi_MultiplyBy();
    a8_load(rd8(dba(SCRATCH + 1)));
    wr16(dba(SCRATCH), cpu.y);
    abi_MultiplyBy();
    flag_set(P_C, 0);
    a8_load(op_adc8((uint8_t)cpu.y, rd8(dba(SCRATCH + 1)), &cpu.p));
    wr8(dba(SCRATCH + 1), (uint8_t)cpu.a);
    a8_load(rd8(dba(SCRATCH + 4)));
    abi_MultiplyBy();
    a8_load(rd8(dba(SCRATCH + 5)));
    wr16(dba(SCRATCH + 4), cpu.y);
    abi_MultiplyBy();
    flag_set(P_C, 0);
    wr8(dba(SCRATCH + 5), op_adc8((uint8_t)cpu.y, rd8(dba(SCRATCH + 5)), &cpu.p));
    flag_set(P_C, 0);
    v = rd8(dpa(0x0B));
    wr8(dpa(0x0B), op_adc8(v, rd8(dba(SCRATCH)), &cpu.p));
    v = rd8(dpa(0x0C));
    wr8(dpa(0x0C), op_adc8(v, rd8(dba(SCRATCH + 1)), &cpu.p));
    v = rd8(dpa(0x0D));
    wr8(dpa(0x0D), op_adc8(v, rd8(dba(SCRATCH + 2)), &cpu.p));
    flag_set(P_C, 0);
    v = rd8(dpa(0x0E));
    wr8(dpa(0x0E), op_adc8(v, rd8(dba(SCRATCH + 4)), &cpu.p));
    v = rd8(dpa(0x0F));
    wr8(dpa(0x0F), op_adc8(v, rd8(dba(SCRATCH + 5)), &cpu.p));
    v = rd8(dpa(0x10));
    a = op_adc8(v, rd8(dba(SCRATCH + 6)), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    wr8(dpa(0x10), a);
    op_sep(0x30);
}

#define VECTOR_SETS 0xBBB9u   /* in the data bank: a set's first vector (word), the facing added */
#define DIVISOR 0x001Au       /* ActorMoveByVectorOver's divisor */

/* One part of ActorMoveByVectorOver: the step's fraction and pixels at
   `step` made positive by its high byte's sign, divided by the divisor,
   the sign given back, then added to the 24-bit position at `pos` (direct
   page) with the high byte as its top byte. Entered M 16-bit, X 8-bit;
   left M 8-bit. */
static void vector_part_over(uint16_t step, uint8_t pos)
{
    op_rep(0x20);
    lda16(ea_abs(step));
    ldy8(ea_abs((uint16_t)(step + 2)));
    if (IF_N) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    sta16(ea_abs(WRDIVL & 0xFFFF));
    ldy8(ea_abs(DIVISOR));
    sty8(ea_abs(WRDIVB & 0xFFFF));
    lda16(ea_abs(RDDIVL & 0xFFFF));  /* after eight NOPs in the original */
    ldy8(ea_abs((uint16_t)(step + 2)));
    if (IF_N) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    op_sep(0x20);
    clc();
    adc8_v(rd8(ea_dp(pos)));
    sta8(ea_dp(pos));
    xba();
    adc8_v(rd8(ea_dp((uint8_t)(pos + 1))));
    sta8(ea_dp((uint8_t)(pos + 1)));
    tya8();
    adc8_v(rd8(ea_dp((uint8_t)(pos + 2))));
    sta8(ea_dp((uint8_t)(pos + 2)));
}

/*
 * @routine 81:A687 ActorMoveByVectorOver
 * WHAT IT DOES: Moves the actor by a vector from direction set X divided
 * by Y, the set's left vector when it faces left. No code refers to it.
 * HOW: The vector (a word at $BBB9 + X in the data bank, plus the facing)
 * to the step at $0C-$0E (x) and $10-$12 (y). For each part, its fraction
 * and pixels (16 bits) made positive by the high byte's sign, divided by
 * the divisor with the CPU's divider (eight NOPs while it works), the
 * quotient given the sign back, then added to the 24-bit position with the
 * high byte (the sign) as its top byte.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank with the vector sets
 * at $BBB9 and the CPU's registers at $4200; X = the set's byte offset;
 * Y = the divisor; called by JSL. Exit: M and X 8-bit; X = the vector's
 * low byte; Y = the y step's high byte; A low byte = the y position's high
 * byte; N, V, Z, C from its ADC; $1A the divisor; $0C-$12 the vector.
 */
void abi_ActorMoveByVectorOver(void)
{
    sty8(ea_abs(DIVISOR));
    op_rep(0x30);
    clc();
    lda16(ea_dp(ACTOR_FACING));
    and16_v(0x00FF);
    adc16_v(rd16(ea_absx(VECTOR_SETS)));
    tax16();
    op_sep(0x20);
    lda8(ea_longx(0x82C000u));
    sta8(ea_abs(SCRATCH));
    lda8(ea_longx(0x82C409u));
    sta8(ea_abs(SCRATCH + 1));
    lda8(ea_longx(0x82C812u));
    sta8(ea_abs(SCRATCH + 2));
    lda8(ea_longx(0x82CC1Bu));
    sta8(ea_abs(SCRATCH + 4));
    lda8(ea_longx(0x82D024u));
    sta8(ea_abs(SCRATCH + 5));
    lda8(ea_longx(0x82D42Du));
    sta8(ea_abs(SCRATCH + 6));
    op_sep(0x10);
    vector_part_over(SCRATCH, ACTOR_X);
    vector_part_over(SCRATCH + 4, ACTOR_Y);
}

/*
 * @routine 81:A73D ActorMoveAlong
 * WHAT IT DOES: Moves the actor along direction Y of the ROM's direction
 * table ($1B6 on), at speed A.
 * HOW: The direction's step, a byte at a time from the six tables in bank
 * $82, to $0C-$12; then as ActorMoveScaled.
 * Entry: M 8-bit, X 16-bit; A = the speed; Y = the direction; D = the
 * actor; called by JSL. Exit: as ActorMoveScaled.
 */
void abi_ActorMoveAlong(void)
{
    uint8_t speed = (uint8_t)cpu.a;
    uint16_t at;

    /* the direction's step, a byte at a time from the six tables */
    op_rep(P_M);
    flag_set(P_C, 0);
    at = op_adc16(cpu.y, 0x01B6, &cpu.p);
    cpu.a = at;
    cpu.x = at;
    op_sep(P_M);
    wr8(dba(SCRATCH), rd8(0x82C000u + at));
    wr8(dba(SCRATCH + 1), rd8(0x82C409u + at));
    wr8(dba(SCRATCH + 2), rd8(0x82C812u + at));
    wr8(dba(SCRATCH + 4), rd8(0x82CC1Bu + at));
    wr8(dba(SCRATCH + 5), rd8(0x82D024u + at));
    wr8(dba(SCRATCH + 6), rd8(0x82D42Du + at));
    actor_scale_and_move(speed);
}

/*
 * @routine 81:A748 ActorMoveScaled
 * WHAT IT DOES: Moves the actor by the step at $0C-$12 multiplied by
 * speed A: x by the step's x, y by its y.
 * HOW: Each step's low and middle bytes times the speed (MultiplyBy, the
 * products overlapped into 24 bits), then added to the actor's x and y.
 * Entry: M 8-bit; A = the speed; $0C-$0E = the step's x; $10-$12 = its y;
 * D = the actor; called by JSL. Exit: M 8-bit, X 8-bit; $0C-$12 the
 * scaled step.
 */
void abi_ActorMoveScaled(void)
{
    op_rep(P_X);
    actor_scale_and_move((uint8_t)cpu.a);
}

/*
 * @routine 81:9D56 ActorKeepSpot
 * WHAT IT DOES: Keeps the actor's place: its x and y pixels into $4C and
 * $4E of its record.
 * HOW: Two 16-bit copies.
 * Entry: D = the actor; called by JSL. Exit: M 8-bit; A = the y pixels.
 */
void abi_ActorKeepSpot(void)
{
    op_rep(P_M);
    dp16_set(0x4C, dp16(0x0C));
    a16_load(dp16(0x0F));
    dp16_set(0x4E, cpu.a);
    op_sep(P_M);
}

/*
 * @routine 81:945F SignExtend
 * WHAT IT DOES: Spreads the sign of A's low byte to its high byte.
 * HOW: Bit 7 clear: A & $FF; set: A | $FF00.
 * Entry: M 16-bit; called by JSR. Exit: A the word; N and Z from it.
 */
void abi_SignExtend(void)
{
    a16_load(cpu.a & 0x0080 ? (uint16_t)(cpu.a | 0xFF00) : (uint16_t)(cpu.a & 0x00FF));
}

/*
 * @routine 84:A7AD ActorBob
 * WHAT IT DOES: Bobs the actor up and down a pixel a frame, five frames
 * each way, while its $42 is nonzero; stops at the bottom once it is 0.
 * HOW: The count at $44: 0, nothing; negative, y up a pixel and the count
 * up, at 0 it becomes 5; positive, y down a pixel and the count down, at
 * 0 it becomes -5 when $42 is nonzero (else stays 0).
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR. Exit: M 8-bit; Y
 * = the new count; A = the y moved to (16 bits) when it moved.
 */
void abi_ActorBob(void)
{
    uint8_t count;

    op_rep(P_M);
    y8_load(count = rd8(dpa(0x44)));
    if (count & 0x80) {
        /* rising: a pixel up, and after 5 the other way */
        a16_load((uint16_t)(dp16(0x0F) - 1));
        dp16_set(0x0F, cpu.a);
        y8_load(++count);
        if (!count)
            y8_load(count = 0x05);
    } else if (count) {
        /* falling: a pixel down; at 0, up again while $42 is nonzero */
        a16_load((uint16_t)(dp16(0x0F) + 1));
        dp16_set(0x0F, cpu.a);
        y8_load(--count);
        if (!count) {
            x8_load(rd8(dpa(0x42)));
            if ((uint8_t)cpu.x) {
                count = 0xFB;
                y8_load(count);
            }
        }
    }
    wr8(dpa(0x44), count);
    op_sep(P_M);
}

/*
 * @routine 84:A7A1 ActorBobAnimate
 * WHAT IT DOES: Plays the actor's animation on a frame, and bobs it while
 * its $42 is nonzero.
 * HOW: AnimStep; then ActorBob when $42 is nonzero.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL. Exit:
 * as AnimStep, or as ActorBob when it ran.
 */
void abi_ActorBobAnimate(void)
{
    abi_AnimStep();
    a8_load(rd8(dpa(0x42)));
    if ((uint8_t)cpu.a)
        abi_ActorBob();
}
