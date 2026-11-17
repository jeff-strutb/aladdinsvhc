/*
 * player_ride.c: the player carried along by an actor's move. Written
 * from the bytes; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

#define RIDE_X_PIXELS (PLAYER + ACTOR_X + 1)  /* the player's x pixels (word) */
#define RIDE_Y_PIXELS (PLAYER + ACTOR_Y + 1)  /* his y pixels (word) */
#define RIDE_KEPT_X 0x4Cu         /* the actor's x pixels where its place was kept (word) */
#define RIDE_KEPT_Y 0x4Eu         /* its y pixels there (word) */

/*
 * @routine 81:9D63 PlayerMoveWithActor
 * WHAT IT DOES: Moves the player by as much as the actor has moved since
 * its place was last kept at $4C and $4E: so he goes along with it.
 * HOW: With 16-bit A: the actor's y pixels less its $4E added to the
 * player's y pixels, its x pixels less its $4C (kept on the stack
 * meanwhile) to his x pixels. No script runs it: written from its bytes.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: M 8-bit; A = the player's new x pixels
 * (16 bits); N, V, Z, C from its addition.
 */
void abi_PlayerMoveWithActor(void)
{
    op_rep(P_M);
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_dp(RIDE_KEPT_X)));
    pha16();
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_dp(RIDE_KEPT_Y)));
    clc();
    adc16_v(rd16(ea_abs(RIDE_Y_PIXELS)));
    sta16(ea_abs(RIDE_Y_PIXELS));
    pla16();
    clc();
    adc16_v(rd16(ea_abs(RIDE_X_PIXELS)));
    sta16(ea_abs(RIDE_X_PIXELS));
    op_sep(P_M);
}
