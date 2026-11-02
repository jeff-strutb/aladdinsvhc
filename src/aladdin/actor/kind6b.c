/*
 * kind6b.c: actor kind $6B, which the camera trigger task starts with
 * $07 = 1: shown in its first pose; with $07 nonzero it waits for the
 * task's cue $00BF and goes on (code not yet reached by any script).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);

#define KIND6B_16 0x16u       /* word: $D663 */
#define CUE_BF 0x0000BFu      /* the camera trigger task's third cue: cleared as it starts */
/* Not yet filed (no script reaches it): the part after the cue. */
#define KIND6B_CUED 0x83EC64u

/*
 * @routine 83:EC2C Kind6BWait
 * @actor
 * WHAT IT DOES: Actor kind $6B: shown facing right in animation 0. With
 * $07 zero it stays so; else it waits for CUE_BF and goes on.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; ACTOR_2C bits 1-3 = 1
 * (and $F1, or 2); KIND6B_16 = $D663; facing and flip 0. $07 zero:
 * AnimStart 0, then BRK for good. Else AnimStart 0, then each frame
 * (BRK first) until CUE_BF is nonzero; then on at KIND6B_CUED, which no
 * script reaches yet: its C is not written, and it runs as the
 * original's (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $6B in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind6BWait(void)
{
    uint8_t v, look;

    v = rd8(dpa(ACTOR_08));
    wr8(dpa(ACTOR_08), (uint8_t)(v | 0x02));
    v = rd8(dpa(ACTOR_09));
    wr8(dpa(ACTOR_09), (uint8_t)(v | 0x90));
    look = (uint8_t)((rd8(dpa(ACTOR_2C)) & 0xF1) | 0x02);
    wr8(dpa(ACTOR_2C), look);
    dp16_set(KIND6B_16, 0xD663);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    v = rd8(dpa(ACTOR_07));
    cpu.a = 0xD600;
    nz8_of(0x00);
    abi_AnimStart();
    if (!v)
        for (;;)
            actor_yield(0x83EC53u);
    for (;;) {
        actor_yield(0x83EC5Du);
        uint8_t cue = rd8(dba(CUE_BF));
        a8_load(cue);
        if (cue)
            break;
    }
    jump_routine(KIND6B_CUED);
}
