/*
 * rise_fall.c: actor helpers of bank $A5: an animation by whether the
 * actor rises or falls, and a placing from a table. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAnimChange(void);

/* Spots an actor is put at: X, Y (pixels, words), 4 bytes each (in the
   data bank) */
#define ACTOR_SPOTS 0xD02C

/*
 * @routine A5:EA20 ActorAnimRiseFall
 * WHAT IT DOES: Shows an actor rising or falling: animation $0C while it
 * rises, $0E while it falls.
 * HOW: By the sign of its vertical speed's top byte, then
 * ActorAnimChange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record. Exit: as
 * ActorAnimChange; X = the speed's top byte.
 */
void abi_ActorAnimRiseFall(void)
{
    uint8_t top = rd8(dpa(ACTOR_SPEED_Y + 2));

    x8_load(top);
    a8_load((top & 0x80) ? 0x0C : 0x0E);  /* rising, falling */
    abi_ActorAnimChange();
}

/*
 * @routine A5:EB1E ActorPlaceAtSpot
 * WHAT IT DOES: Puts an actor at spot X of the spot table, on whole
 * pixels.
 * HOW: Its X and Y pixels from the spot's two words, their fractions 0.
 * Entry: M 8-bit, X 8-bit; X = the spot's offset; D = the actor's
 * record; DB the table's bank. Exit: M 8-bit; A = the spot's Y; N, Z from
 * it; X, Y unchanged.
 */
void abi_ActorPlaceAtSpot(void)
{
    uint32_t spot = dba(ACTOR_SPOTS) + cpu.x;
    uint16_t y = rd16(spot + 2);

    dp16_set(ACTOR_X + 1, rd16(spot));
    dp16_set(ACTOR_Y + 1, y);
    a16_load(y);
    wr8(dpa(ACTOR_X), 0);
    wr8(dpa(ACTOR_Y), 0);
}
