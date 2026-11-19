/*
 * kind5f.c: actor kind $5F, a brief picture: shown by its $07's
 * animation for 31 frames, then gone.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorEndPlaced(void);

#define K5F_LEFT 0x40u          /* frames left of its showing */

/*
 * @routine 83:E465 Kind5FStart
 * @actor
 * WHAT IT DOES: Actor kind $5F: shows animation $07 for 31 frames, then
 * ends for good.
 * HOW: $09 = $10; AnimStart with A = $07; K5F_LEFT $1F; each frame (BRK
 * first) AnimStep and K5F_LEFT down; at 0 JMP ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $5F in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind5FStart(void)
{
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_07));
    jsl_frame(0x83E46Bu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x1F);
    sta8(ea_dp(K5F_LEFT));
    do {
        actor_yield(0x83E473u);
        jsl_frame(0x83E475u);
        abi_AnimStep();
        cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(K5F_LEFT));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}
