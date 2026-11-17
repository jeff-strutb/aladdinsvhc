/*
 * kind_a8.c: actor kind $A8, a figure of the level-end screen that waits for its cue
 * and, when its $07 is below the count at $0375, poses again after its own
 * delay with a sound; then it keeps animating.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define CUE_AE 0x00AEu          /* its cue */
#define V_0375 0x0375u          /* a count: figures with $07 below it pose again */
#define KA8_DELAYS 0xD9A6u      /* (data bank) by $07: frames before it poses again */

/*
 * @routine 83:FB1A KindA8Start
 * @actor
 * WHAT IT DOES: Actor kind $A8: shown in pose 1; once cue $AE is set,
 * if its $07 is below V_0375 it waits its delay, takes pose 0 and plays
 * sound $28; then it animates for good.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bit 4 set (TSB); AnimStart 1; BRK
 * until CUE_AE. $07 below V_0375 ($83:FB36, which no script meets): COP
 * KA8_DELAYS by $07, AnimStart 0, SoundQueueAdd $28. Then each frame
 * (BRK) AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds KA8_DELAYS (bank $83); reached by
 * the actor loop's jump for kind $A8 in state $0C (the table at $83:8DAB,
 * through the stub at $83:9446). Exit: never returns.
 */
void abi_KindA8Start(void)
{
    uint8_t v07;

    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x10);
    tsb_at(dpa(ACTOR_09), 0x10);
    a8_load(0x01);
    abi_AnimStart();
    do {
        actor_yield(0x83FB28u);
        a8_load(rd8(dba(CUE_AE)));
    } while (!(uint8_t)cpu.a);
    v07 = rd8(dpa(ACTOR_07));
    x8_load(v07);
    cmp8_of(v07, rd8(dba(V_0375)));
    if (v07 < rd8(dba(V_0375))) {
        a8_load(rd8(dba((uint16_t)(KA8_DELAYS + v07))));
        actor_wait(0x83FB39u);
        a8_load(0x00);
        abi_AnimStart();
        a8_load(0x28);
        abi_SoundQueueAdd();
    }
    for (;;) {
        actor_yield(0x83FB47u);
        abi_AnimStep();
    }
}
