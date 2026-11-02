/*
 * kind79.c: actor kind $79, which starts below the point actors are kept
 * in range of and rises until it is taken up (its $42 set), then carries
 * the player (code not yet reached by any script).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "native.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStepYBack(void);

#define RANGE_Y 0x1A46u      /* the y of the point actors are kept in range of (word) */
#define KIND79_FIRST_Y 0x45u /* the first of a pair: its y as it was placed (word) */
#define KIND79_TAKEN 0x42u   /* nonzero: it is taken up, and goes on */
#define KIND79_43 0x43u
#define KIND79_RISE 0x6F     /* its vertical speed's entry in the speed table */
/* Not yet filed (no script reaches it): the part after it is taken up,
   which carries the player. */
#define KIND79_TAKEN_ON 0x83F593u

/*
 * @routine 83:F541 Kind79Rise
 * @actor
 * WHAT IT DOES: Actor kind $79: starts 32 pixels (an even $07: the first
 * of a pair, which marks its $07 odd) or 112 pixels (an odd $07) below
 * the point actors are kept in range of, and rises each frame until it
 * is taken up.
 * HOW: Even $07: $07 up, its y kept in KIND79_FIRST_Y, its y RANGE_Y plus
 * $20, ACTOR_09 = $90, ACTOR_12 = 8. Odd: its y RANGE_Y plus $70. Then
 * ACTOR_08 = 2, animation 0, ActorSpeedYSet $6F, $41, KIND79_TAKEN,
 * KIND79_43 and ACTOR_49 cleared; each frame (BRK first)
 * ActorStepYBack, until KIND79_TAKEN is set; then on at KIND79_TAKEN_ON,
 * which no script reaches yet: its C is not written, and it runs as the
 * original's (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $79 in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind79Rise(void)
{
    uint8_t pair = rd8(dpa(ACTOR_07));
    uint16_t y;

    if (!(pair & 1)) {
        /* the first of a pair: its $07 made odd, its place kept */
        wr8(dpa(ACTOR_07), (uint8_t)(pair + 1));
        dp16_set(KIND79_FIRST_Y, dp16(ACTOR_Y + 1));
        flag_set(P_C, 0);
        y = op_adc16(rd16(dba(RANGE_Y)), 0x0020, &cpu.p);
        dp16_set(ACTOR_Y + 1, y);
        wr8(dpa(ACTOR_09), 0x90);
        wr8(dpa(ACTOR_12), 0x08);
    } else {
        flag_set(P_C, 0);
        y = op_adc16(rd16(dba(RANGE_Y)), 0x0070, &cpu.p);
        dp16_set(ACTOR_Y + 1, y);
    }
    wr8(dpa(ACTOR_08), 0x02);
    cpu.a = (uint16_t)(y & 0xFF00);       /* animation 0; A's high byte the sum's */
    nz8_of(0);
    abi_AnimStart();
    y8_load(KIND79_RISE);
    abi_ActorSpeedYSet();
    wr8(dpa(ACTOR_41), 0);
    wr8(dpa(KIND79_TAKEN), 0);
    wr8(dpa(KIND79_43), 0);
    wr8(dpa(ACTOR_49), 0);
    for (;;) {
        actor_yield(0x83F589u);
        abi_ActorStepYBack();
        uint8_t taken = rd8(dpa(KIND79_TAKEN));
        a8_load(taken);
        if (taken)
            break;
    }
    jump_routine(KIND79_TAKEN_ON);
}
