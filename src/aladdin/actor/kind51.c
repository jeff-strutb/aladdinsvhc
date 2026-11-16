/*
 * kind51.c: actor kind $51, a thing that rises and settles (Aladdin can
 * land on it once it has): its variant with $07 bit 1 set, which plays
 * once and is gone, and the jump its code ends by.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorEndPlaced(void);
void abi_Kind51End(void);

#define KIND51_FRAMES 0x40u     /* frames left of the present step */
#define KIND51_SOLID 0x41u      /* nonzero: Aladdin can land on it (its wait tests it) */

/*
 * @routine 83:D279 Kind51Variant
 * @actor
 * WHAT IT DOES: Kind $51 with $07 bit 1 set: after a pause when $07 bit 2
 * is set, plays its animation 5 for $58 frames, then is gone for good.
 * HOW: KIND51_SOLID cleared; $07 bit 2: KIND51_FRAMES = $0A frames
 * (BRK each). AnimStart 5; KIND51_FRAMES = $58 frames of BRK and
 * AnimStep; then Kind51End.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
 * jump from its start (83:D247). Exit: never returns: as Kind51End.
 */
void abi_Kind51Variant(void)
{
    uint8_t pause;

    wr8(dpa(KIND51_SOLID), 0);
    pause = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x04);
    a8_load(pause);
    if (pause) {
        a8_load(0x0A);
        wr8(dpa(KIND51_FRAMES), 0x0A);
        do
            actor_yield(0x83D285u);
        while (dec8_at(dpa(KIND51_FRAMES)));
    }
    a8_load(0x05);
    abi_AnimStart();
    a8_load(0x58);
    wr8(dpa(KIND51_FRAMES), 0x58);
    do {
        actor_yield(0x83D295u);
        abi_AnimStep();
    } while (dec8_at(dpa(KIND51_FRAMES)));
    abi_Kind51End();
}

/*
 * @routine 83:D32F Kind51End
 * @actor
 * WHAT IT DOES: Where kind $51's code ends: the actor gone for good.
 * HOW: JMP to ActorEndPlaced.
 * Entry: D = the actor's record; reached by a jump from Kind51Variant
 * (83:D29F). Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind51End(void)
{
    abi_ActorEndPlaced();
}
