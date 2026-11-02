/*
 * priority_clear.c: an actor's sprite priority bits cleared, except in
 * the level that keeps them.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorPriorityClear(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:81B2 ActorPriorityClear
 * WHAT IT DOES: Clears the actor's sprite priority bits, except in level
 * $0E.
 * HOW: Unless the level's low byte is $0E, bits 4-5 of $09 cleared (TRB
 * $30).
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: A low byte = $30 or the level; Z from the TRB or
 * the compare.
 */
void abi_ActorPriorityClear(void)
{
    uint8_t level = rd8(dba(LEVEL));

    a8_load(level);
    cmp8_of(level, 0x0E);
    if (level == 0x0E)
        return;
    a8_load(0x30);
    trb_at(dpa(ACTOR_09), 0x30);
}
