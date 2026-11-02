/*
 * hit_box_long.c: ActorHitsTestBoxIfPlayerFree for callers in other
 * banks.
 */
#include "machine.h"
#include "ops65.h"

void abi_ActorHitsTestBoxIfPlayerFree(void);

/*
 * @routine 83:82F4 ActorHitsTestBoxIfPlayerFreeLong
 * WHAT IT DOES: ActorHitsTestBoxIfPlayerFree for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorHitsTestBox; called by JSL. Exit: as
 * ActorHitsTestBoxIfPlayerFree.
 */
void abi_ActorHitsTestBoxIfPlayerFreeLong(void)
{
    abi_ActorHitsTestBoxIfPlayerFree();
}
