/*
 * spawn_at.c: a new actor of a kind at a place the caller puts in two
 * work RAM words, started at its next turn.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

#define SPAWN_X 0x002Cu    /* where the actor goes: x, then y (pixels, words) */
#define SPAWN_Y 0x002Eu
#define SPAWN_07 0x0030u   /* the byte it gets at $07, kept during the call */
#define ACTOR_NEW 0x0Cu    /* the state of a record that starts at its next turn */

/* Not yet filed: a free record of the first pool for kind A (its $06): X
   the record (16 bits), C clear; none: C set. */
#define ACTOR_ALLOC 0x82AA2Bu



/*
 * @routine 80:A4DC ActorSpawnAt
 * WHAT IT DOES: Starts a new actor of kind A's high byte at the place in
 * SPAWN_X and SPAWN_Y, with A's low byte at its $07; nothing when no
 * record is free.
 * HOW: The low byte kept at $0030; the record taken by the routine at
 * $82:AA2B (which sets the kind); its state $0C (new), its $07, and the
 * whole pixels of its x and y from the two words.
 * Entry: M 8-bit, X 8-bit; A = kind (high byte) and the byte for $07
 * (low); DB a bank whose $0000-$1FFF is work RAM. Exit: started: X = the
 * record (16 bits), A = the y word, M 16-bit, C clear, N and Z from the
 * y; Y as $82:AA2B leaves it. None free: as $82:AA2B leaves them (C set).
 */
void abi_ActorSpawnAt(void)
{
    sta8(ea_abs(SPAWN_07));
    xba();
    call_routine(ACTOR_ALLOC, 1);
    if (IF_C)
        return;
    lda8_v(ACTOR_NEW);
    sta8(ea_absx(ACTOR_STATE));
    lda8(ea_abs(SPAWN_07));
    sta8(ea_absx(ACTOR_07));
    op_rep(0x20);
    lda16(ea_abs(SPAWN_X));
    sta16(ea_absx(ACTOR_X + 1));
    lda16(ea_abs(SPAWN_Y));
    sta16(ea_absx(ACTOR_Y + 1));
    clc();
}
/*
 * @routine 80:A4D8 ActorSpawnAtLong
 * WHAT IT DOES: ActorSpawnAt for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorSpawnAt; called by JSL. Exit: as ActorSpawnAt.
 */
void abi_ActorSpawnAtLong(void)
{
    abi_ActorSpawnAt();
}
