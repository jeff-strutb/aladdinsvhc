/*
 * flag_drops.c: the cases of ActorFlagOnce (actor/actor_steps.c): an
 * actor that drops children once, its flag in V_0359 marking that it has.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorCreateChild8(void);
void abi_ActorSpawn(void);
void abi_FlagDropChild(void);
void abi_RandomNext(void);

#define FLAG_V_0359 0x0359u       /* eight flags, set by the actors that use these */
#define FLAG_MASK 0x41u           /* the actor's flag's mask */
#define FLAG_DROP_SORTS_A 0xD31Eu /* in bank $80: by chance (0-31), a child's sort, 0 none */
#define FLAG_DROP_SORTS_B 0xD33Eu /* in bank $80: by chance (0-31), a child's sort */
#define FLAG_DROP_LEFT 0x40u      /* FlagDropThree: children left, less one */
#define FLAG_DROP_BOX 0x30        /* the children's hit box */
#define CHILD_21 0x0021u          /* a child's $21 (absolute, by its record) */

/* ActorFlagOnce's mark ($83:CF61): the flag set. */
static void flag_mark(void)
{
    lda8(ea_dp(FLAG_MASK));
    tsb8(ea_abs(FLAG_V_0359));
}

/* RandomNext, then the sort at `sorts` by RANDOM's low 5 bits. */
static void drop_sort(uint16_t sorts)
{
    abi_RandomNext();
    lda8(ea_abs(RANDOM));
    and8_v(0x1F);
    tax8();
    lda8(ea_absx(sorts));
}

/*
 * @routine 83:CF31 FlagDropMaybe
 * WHAT IT DOES: ActorFlagOnce's case 0: by chance drops a child (hit box
 * $30) of one of two sorts, and marks the actor's flag when it does or
 * when none falls.
 * HOW: RandomNext; RANDOM's low 5 bits index FLAG_DROP_SORTS_A (bank $80):
 * nonzero, on into FlagDropChild with it; 0, on at ActorFlagOnce's mark
 * ($83:CF40): the flag set.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; $41 = its flag's mask;
 * DB = $80; reached by a jump from ActorFlagOnce. Exit: as FlagDropChild,
 * or as the mark leaves them (A low = the mask).
 */
void abi_FlagDropMaybe(void)
{
    drop_sort(FLAG_DROP_SORTS_A);
    if (!IF_Z) {
        abi_FlagDropChild();
        return;
    }
    flag_mark();
}

/*
 * @routine 83:CF42 FlagDropAlways
 * WHAT IT DOES: ActorFlagOnce's case 1: drops a child (hit box $30) of a
 * sort picked by chance, and marks the actor's flag.
 * HOW: RandomNext; RANDOM's low 5 bits index FLAG_DROP_SORTS_B (bank $80);
 * on into FlagDropChild with it. No script runs it: written from its
 * bytes.
 * Entry: as FlagDropMaybe. Exit: as FlagDropChild.
 */
void abi_FlagDropAlways(void)
{
    drop_sort(FLAG_DROP_SORTS_B);
    abi_FlagDropChild();
}

/*
 * @routine 83:CF51 FlagDropFour
 * WHAT IT DOES: ActorFlagOnce's case 2: drops a child of sort 4 (hit box
 * $30), and marks the actor's flag.
 * HOW: A = 4, on into FlagDropChild. No script runs it: written from its
 * bytes.
 * Entry: as FlagDropMaybe. Exit: as FlagDropChild.
 */
void abi_FlagDropFour(void)
{
    lda8_v(0x04);
    abi_FlagDropChild();
}

/*
 * @routine 83:CF53 FlagDropChild
 * WHAT IT DOES: Drops a child of sort A (its $07) with hit box $30 at the
 * actor's spawn point $3C; when there was a record for it, marks the
 * actor's flag so it drops no more.
 * HOW: $07 = A, ACTOR_SPAWN_AT = $3C, ActorCreateChild8 with A = $30;
 * created (C clear): on at ActorFlagOnce's mark ($83:CF61), the flag set
 * in V_0359 (TSB); else RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; A = the sort; $41 =
 * its flag's mask; DB = $80; reached from FlagDropMaybe, FlagDropAlways
 * and FlagDropFour. Exit: created: A low = the mask, Z from its test with
 * V_0359; else as ActorCreateChild8 leaves them (C set).
 */
void abi_FlagDropChild(void)
{
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x3C);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(FLAG_DROP_BOX);
    jsl_frame(0x83CF5Bu); abi_ActorCreateChild8(); cpu.s = (uint16_t)(cpu.s + 3);
    if (IF_C)
        return;
    flag_mark();
}

/*
 * @routine 83:CF67 FlagDropThree
 * WHAT IT DOES: ActorFlagOnce's case 3: drops three children (hit box $30,
 * sort 7) at the actor's spawn point $92, numbered 2, 1 and 0 in their
 * $21, and marks the actor's flag; when the records run out it stops
 * without the mark.
 * HOW: FLAG_DROP_LEFT = 2, $07 = 7, ACTOR_SPAWN_AT = $92; each: ActorSpawn
 * with A = $30, none free (N): RTS; the child's $21 = FLAG_DROP_LEFT, X
 * 8-bit, FLAG_DROP_LEFT down while not negative. Then the flag set in
 * V_0359 (TSB).
 * Entry: as FlagDropMaybe. Exit: M 8-bit, X 8-bit; all three: A low = the
 * mask, Z from its test with V_0359; else as ActorSpawn leaves them.
 */
void abi_FlagDropThree(void)
{
    lda8_v(0x02);
    sta8(ea_dp(FLAG_DROP_LEFT));
    lda8_v(0x07);
    sta8(ea_dp(ACTOR_07));
    lda8_v(0x92);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    do {
        lda8_v(FLAG_DROP_BOX);
        jsl_frame(0x83CF75u); abi_ActorSpawn(); cpu.s = (uint16_t)(cpu.s + 3);
        if (IF_N)
            return;
        lda8(ea_dp(FLAG_DROP_LEFT));
        sta8(ea_absx(CHILD_21));
        op_sep(P_X);
        MEM8(dec, ea_dp(FLAG_DROP_LEFT));
    } while (!IF_N);
    flag_mark();
}
