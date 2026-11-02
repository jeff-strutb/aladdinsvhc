/*
 * at_player.c: an actor kept at Aladdin's place.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorMotionPreset(void);
void abi_ActorMove(void);

#define PLAYER_2F (PLAYER + 0x2Fu)  /* picks the motion of an actor kept at his place */
#define AT_PLAYER_MOTIONS 0xCA03u   /* the motion presets, by his $2F (bytes, the DB bank) */

/*
 * @routine 81:EC02 ActorAtPlayer
 * WHAT IT DOES: Puts the actor at Aladdin's place and moves it by the
 * motion his $2F picks.
 * HOW: His x and y pixels copied into the actor's; the motion preset from
 * the table at $CA03 by his $2F; then ActorMotionPreset and ActorMove.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the table's bank; called by JSR. Exit: as ActorMove.
 */
void abi_ActorAtPlayer(void)
{
    op_rep(0x20);
    lda16(ea_abs(PLAYER + ACTOR_X + 1));
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_abs(PLAYER + ACTOR_Y + 1));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    ldx8(ea_abs(PLAYER_2F));
    ldy8(ea_absx(AT_PLAYER_MOTIONS));
    abi_ActorMotionPreset();
    abi_ActorMove();
}
