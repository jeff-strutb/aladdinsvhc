/*
 * kind63.c: actor kind $63's per-frame work while it waits.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorShotHitFace(void);
void abi_ActorStompOrHurtLong(void);
void abi_AnimStep(void);
void abi_ActorPictureSend(void);
void abi_ActorFreeOutOfRangeLong(void);

#define KIND63_HIT 0x43u     /* nonzero while it shows being hit: no shot test then */
#define KIND63_GONE 0x47u    /* counted up each frame it is out of range */

/*
 * @routine 84:81FC Kind63Work
 * WHAT IT DOES: What actors of kind $63 do each frame they wait: unless
 * already hit, a shot from the second pool turns it to face Aladdin and
 * hits it; Aladdin is hurt by it or stomps it; it animates and shows its
 * own picture; and out of range by box $28 it ends, its $47 counted up.
 * HOW: KIND63_HIT zero: ActorShotHitFace. Then ActorStompOrHurtLong,
 * AnimStep, ActorPictureSend, ActorFreeOutOfRangeLong with X = $28; C
 * set (out of range): KIND63_GONE up by one.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
 * stub ($83:924D) through the actor loop's wait table at $83:8F4F. Exit:
 * M and X 8-bit; C as ActorFreeOutOfRangeLong left it; in range the
 * registers it left, else N and Z from the count.
 */
void abi_Kind63Work(void)
{
    uint8_t hit = rd8(dpa(KIND63_HIT));

    a8_load(hit);
    if (!hit)
        abi_ActorShotHitFace();
    abi_ActorStompOrHurtLong();
    abi_AnimStep();
    abi_ActorPictureSend();
    x8_load(0x28);
    abi_ActorFreeOutOfRangeLong();
    if (cpu.p & P_C)
        (void)inc8_at(dpa(KIND63_GONE));
}
