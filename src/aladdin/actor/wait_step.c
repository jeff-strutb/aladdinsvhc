/*
 * wait_step.c: what some kinds of actor do on each frame they wait. The
 * actor loop calls a kind's routine (by JSR through the kind's table at
 * $83:8F4F or $83:904F) on every frame an actor of it waits, when bit 7
 * of its $09 is set: most play its animation on and end the actor once it
 * is out of the camera's range.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorFreePlaced(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

/* ActorEndOutOfRange (asm/aladdin/actor/actor_range.asm; its C is held
   back): ends the actor and goes on with the loop's next record when it
   is out of range by box X, else returns. */
#define ACTOR_END_OUT_OF_RANGE 0x8396BEu
#define ACTOR_LOOP_NEXT 0x8394BFu  /* the loop's next record */
#define SUB_81949E 0x81949Eu
#define SUB_8383C9 0x8383C9u
#define SUB_838135 0x838135u
#define SUB_83840F 0x83840Fu
#define SUB_8381E8 0x8381E8u
#define SUB_83865E 0x83865Eu
#define SUB_83848D 0x83848Du
#define SUB_83FBDA 0x83FBDAu
#define SUB_83FB89 0x83FB89u
#define SUB_82BB26 0x82BB26u
#define V_1428 0x001428u
#define V_141C 0x00141Cu
#define V_032D 0x00032Du
/* The word the tail at $83:9738 keeps actors right of: an actor whose x
   is not past it less 31 is ended. */
#define V_1773 0x001773u
/* In bank $80 (DB): sound commands by the actor's x pixel, low 4 bits. */
#define STEP_SOUNDS 0xD3FCu
#define FLOOR_Y 0xB0               /* the lowest the player is held at */

/*
 * @routine 83:9C06 ActorWaitTurnAnim08
 * WHAT IT DOES: Each frame it waits: the tests at $83:83C9 and $83:8135,
 * its animation on, the update at $81:949E, and it ends once out of range
 * (box 8).
 * HOW: Calls them in turn, then on into ActorEndOutOfRange with X = 8.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = 8, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitTurnAnim08(void)
{
    call_routine(SUB_8383C9, 0);
    call_routine(SUB_838135, 0);
    abi_AnimStep();
    call_routine(SUB_81949E, 1);
    ldx8_v(0x08);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:9E0A ActorWaitHoldPlayer
 * WHAT IT DOES: Each frame it waits: its animation on; then, with $42 set,
 * the player falling to or below y $B0 is held there ($1428 = 1, $141C =
 * 0); else, with $44 set, the call at $82:BB26 with A = 5.
 * HOW: The player's y pixel (word) is compared unsigned with $B0 by its
 * low byte, and his vertical speed's sign tested, before y is set to $B0.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: M 8-bit; held: A = 1, N and Z clear, C set; X
 * and Y as AnimStep leaves them; else as AnimStep, the compares or the
 * call at $82:BB26 leave them.
 */
void abi_ActorWaitHoldPlayer(void)
{
    abi_AnimStep();
    lda8(ea_dp(ACTOR_42));
    if (IF_Z) goto not_holding;
    lda8(ea_abs(PLAYER + ACTOR_Y + 1));
    cmp8_v(FLOOR_Y);
    if (!IF_C) return;
    lda8(ea_abs(PLAYER + ACTOR_SPEED_Y + 2));
    if (IF_N) return;
    lda8_v(FLOOR_Y);
    sta8(ea_abs(PLAYER + ACTOR_Y + 1));
    stz8(ea_abs(PLAYER + ACTOR_Y + 2));
    lda8_v(0x01);
    sta8(ea_abs(V_1428));
    stz8(ea_abs(V_141C));
    return;
not_holding:
    lda8(ea_dp(ACTOR_44));
    if (IF_Z) return;
    lda8_v(0x05);
    call_routine(SUB_82BB26, 1);
}

/*
 * @routine 83:B0BB ActorWaitAnimIf07Bit4
 * WHAT IT DOES: Each frame it waits, its animation plays on when bit 4 of
 * its $07 is set.
 * HOW: BIT #$10 on $07, then AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
 * loop. Exit: bit clear: A = $07, Z set, N from it, the rest unchanged.
 * Set: as AnimStep.
 */
void abi_ActorWaitAnimIf07Bit4(void)
{
    lda8(ea_dp(ACTOR_07));
    bit8_i(0x10);
    if (IF_Z) return;
    abi_AnimStep();
}

/*
 * @routine 83:B4B9 ActorWaitAnim
 * WHAT IT DOES: Each frame it waits, its animation plays on.
 * HOW: AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
 * loop. Exit: as AnimStep.
 */
void abi_ActorWaitAnim(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:E4E7 ActorWaitAnimB
 * WHAT IT DOES: Each frame it waits, its animation plays on (a second
 * copy of ActorWaitAnim, for other kinds).
 * HOW: AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
 * loop. Exit: as AnimStep.
 */
void abi_ActorWaitAnimB(void)
{
    abi_AnimStep();
}

/* The tail at $83:9738, which three of these jump into: the actor is
   ended (its return dropped, ActorFreePlaced, on to the loop's next
   record) when the word at $1773 less 31 is not negative and not below
   its x pixel. Else back with M 8-bit. */
static void end_if_left_behind(void)
{
    op_rep(0x21);
    lda16(ea_abs(V_1773));
    sbc16_v(0x001F);
    if (IF_N) goto keep;
    cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
    if (IF_C) {
        op_sep(0x30);
        pla8();
        pla8();
        /* JSR ActorFreePlaced at $83:96CA: its return address on the
           stack while it runs, pulled by its RTS */
        wr8(cpu.s, 0x96); cpu.s--;
        wr8(cpu.s, 0xCC); cpu.s--;
        abi_ActorFreePlaced();
        cpu.s += 2;
        actor_leave(ACTOR_LOOP_NEXT);
        return;
    }
keep:
    op_sep(0x20);
}

/*
 * @routine 83:BD0B ActorWaitAnimEnd10
 * WHAT IT DOES: Each frame it waits: its animation on; then, where
 * LEVEL_035C is 6, it is ended once left behind (the tail at $83:9738);
 * elsewhere once out of range (box $10).
 * HOW: AnimStep, the compare, then a jump to either.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: kept: M 8-bit; as the tail at $83:9738 (A = the
 * word less 31, M 8-bit) or ActorEndOutOfRange (M and X 8-bit, X = $10)
 * returns. Ended: the loop goes on at $83:94BF, no return.
 */
void abi_ActorWaitAnimEnd10(void)
{
    abi_AnimStep();
    lda8(ea_abs(LEVEL_035C));
    cmp8_v(0x06);
    if (IF_Z) {
        end_if_left_behind();
        return;
    }
    ldx8_v(0x10);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:CF8C ActorWaitTest42Anim
 * WHAT IT DOES: Each frame it waits: while $42 is 0, the tests at
 * $83:840F (true: $42 up by 1) and $83:81E8 (C clear: up by 2); then its
 * animation on, and it ends once out of range (box $10).
 * HOW: The tests, AnimStep, then on into ActorEndOutOfRange with X = $10.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = $10, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitTest42Anim(void)
{
    lda8(ea_dp(ACTOR_42));
    if (!IF_Z) goto anim;
    call_routine(SUB_83840F, 0);
    if (IF_Z) goto second;
    MEM8(inc, ea_dp(ACTOR_42));
second:
    call_routine(SUB_8381E8, 0);
    if (IF_C) goto anim;
    MEM8(inc, ea_dp(ACTOR_42));
    MEM8(inc, ea_dp(ACTOR_42));
anim:
    abi_AnimStep();
    ldx8_v(0x10);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:D0EF ActorWaitTest42Anim2
 * WHAT IT DOES: Each frame it waits: while $42 is 0, the test at $83:840F
 * (true: $42 up by 1); the call at $83:865E with Y = $20; its animation
 * on, and it ends once out of range (box $10).
 * HOW: In turn, then on into ActorEndOutOfRange with X = $10.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = $10, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitTest42Anim2(void)
{
    lda8(ea_dp(ACTOR_42));
    if (!IF_Z) goto call;
    call_routine(SUB_83840F, 0);
    if (IF_Z) goto call;
    MEM8(inc, ea_dp(ACTOR_42));
call:
    ldy8_v(0x20);
    call_routine(SUB_83865E, 0);
    abi_AnimStep();
    ldx8_v(0x10);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:D2A2 ActorWaitWake
 * WHAT IT DOES: Each frame it waits: when bit 1 of its $07 is clear, $41
 * is set and the test at $83:848D is true, it wakes: state bits $8C set,
 * bit 7 of $09 (the wait routine) cleared. It ends once out of range (box
 * $10).
 * HOW: TSB/TRB on its fields, then on into ActorEndOutOfRange with X =
 * $10.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = $10, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitWake(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x02);
    if (!IF_Z) goto range;
    lda8(ea_dp(ACTOR_41));
    if (IF_Z) goto range;
    call_routine(SUB_83848D, 0);
    if (IF_Z) goto range;
    lda8_v(0x8C);
    tsb8(ea_dp(ACTOR_STATE));
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
range:
    ldx8_v(0x10);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:D8CE ActorWaitHurtOnTouch
 * WHAT IT DOES: Each frame it waits: its animation on; unless bit 3 of
 * its $07 is set, it hurts the player on touch; it is ended once left
 * behind.
 * HOW: AnimStep, ActorHurtsPlayerOnTouch, then on into the tail at
 * $83:9738.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: kept: M 8-bit; A = the word at $1773 less 31; N,
 * Z and C from its test. Ended: the loop goes on at $83:94BF, no return.
 */
void abi_ActorWaitHurtOnTouch(void)
{
    abi_AnimStep();
    lda8(ea_dp(ACTOR_07));
    and8_v(0x08);
    if (IF_Z)
        abi_ActorHurtsPlayerOnTouch();
    end_if_left_behind();
}

/*
 * @routine 83:DFCD ActorWaitStepSounds
 * WHAT IT DOES: Each frame it waits: its animation on. With bit 7 of $07
 * set: while $42 is set, every 8th count of $032D a sound by its x (the
 * table at $80:D3FC); it hurts the player on touch. Bit 7 clear: the
 * calls at $83:FBDA and $83:FB89 (X = 2, $4C = $3C). It is ended once left
 * behind.
 * HOW: AnimStep, the cases, then on into the tail at $83:9738.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: kept: M 8-bit; A = the word at $1773 less 31; N,
 * Z and C from its test. Ended: the loop goes on at $83:94BF, no return.
 */
void abi_ActorWaitStepSounds(void)
{
    abi_AnimStep();
    lda8(ea_dp(ACTOR_07));
    if (!IF_N) goto other;
    lda8(ea_dp(ACTOR_42));
    if (IF_Z) goto touch;
    lda8(ea_abs(V_032D));
    and8_v(0x07);
    if (!IF_Z) goto touch;
    lda8(ea_dp(ACTOR_X + 1));
    and8_v(0x0F);
    tax8();
    lda8(ea_absx(STEP_SOUNDS));
    abi_SoundQueueAdd();
touch:
    abi_ActorHurtsPlayerOnTouch();
    goto tail;
other:
    call_routine(SUB_83FBDA, 0);
    ldy8_v(0x3C);
    sty8(ea_dp(ACTOR_4C));
    ldx8_v(0x02);
    call_routine(SUB_83FB89, 0);
tail:
    end_if_left_behind();
}

/*
 * @routine 83:E458 ActorWaitAnimRange10
 * WHAT IT DOES: Each frame it waits: its animation on, and it ends once
 * out of range (box $10).
 * HOW: AnimStep, then on into ActorEndOutOfRange with X = $10.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = $10, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitAnimRange10(void)
{
    abi_AnimStep();
    ldx8_v(0x10);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:E5B6 ActorWaitRange00
 * WHAT IT DOES: Each frame it waits, it ends once out of range (box 0).
 * HOW: X = 0, then on into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = 0, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitRange00(void)
{
    ldx8_v(0x00);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}

/*
 * @routine 83:F487 ActorWaitAnimRange18
 * WHAT IT DOES: Each frame it waits: its animation on, the update at
 * $81:949E, and it ends once out of range (box $18).
 * HOW: AnimStep, the call, then on into ActorEndOutOfRange with X = $18.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
 * the actor loop. Exit: in range: as ActorEndOutOfRange returns (M and X
 * 8-bit, X = $18, C clear). Out of range: the actor is ended and the loop
 * goes on at $83:94BF, no return.
 */
void abi_ActorWaitAnimRange18(void)
{
    abi_AnimStep();
    call_routine(SUB_81949E, 1);
    ldx8_v(0x18);
    jump_routine(ACTOR_END_OUT_OF_RANGE);
}
