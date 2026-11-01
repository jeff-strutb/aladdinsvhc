/*
 * rise_fall.c: actor helpers of bank $A5: an animation by whether the
 * actor rises or falls, and a placing from a table. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
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
    ldx8(ea_dp(ACTOR_SPEED_Y + 2));
    if (IF_N)
        lda8_v(0x0C);
    else
        lda8_v(0x0E);
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
    op_rep(0x20);
    lda16(ea_absx(ACTOR_SPOTS));
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_absx(ACTOR_SPOTS + 2));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    stz8(ea_dp(ACTOR_X));
    stz8(ea_dp(ACTOR_Y));
}
