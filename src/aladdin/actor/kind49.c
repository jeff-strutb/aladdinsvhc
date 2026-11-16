/*
 * kind49.c: actor kind $49, which carries the player along as it moves.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorCarryPlayerLong(void);

/*
 * @routine 84:804D Kind49Carry
 * @actor
 * WHAT IT DOES: Actor kind $49: each frame it carries the player along by
 * however far it moved (its own moves are its kind's wait work).
 * HOW: ACTOR_09 bits $90 set (bit 7: its kind's routine runs each frame
 * it waits); then each frame a yield and ActorCarryPlayerLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $49 in state $0C (the table at $83:8CAB,
 * through the stub at $83:923C). Exit: never returns.
 */
void abi_Kind49Carry(void)
{
    uint8_t flags = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90);

    wr8(dpa(ACTOR_09), flags);
    a8_load(flags);
    for (;;) {
        actor_yield(0x848053u);
        abi_ActorCarryPlayerLong();
    }
}
