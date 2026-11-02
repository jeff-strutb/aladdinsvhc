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
    if (!IF_Z) goto range;
    /* The tail at $83:9738: ended once the camera's x less 31 is past it. */
    op_rep(0x21);
    lda16(ea_abs(0x1773));
    sbc16_v(0x001F);
    if (IF_N) goto kept;
    cmp16_v(rd16(ea_dp(0x0C)));
    if (!IF_C) goto kept;
    op_sep(0x30);           /* $83:96C6 */
    pla8();
    pla8();
    abi_ActorFreePlaced();
    actor_leave(0x8394BFu);
    return;
kept:
    op_sep(0x20);
    return;
range:
    ldx8_v(0x10);
    jump_routine(0x8396BEu);
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

