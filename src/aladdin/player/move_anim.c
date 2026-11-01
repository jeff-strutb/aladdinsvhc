/*
 * move_anim.c: the player's moves and their animations. Drafted by
 * cdraft.py; named and described by hand.
 *
 * The player's move (standing, running, jumping ...) is a number; each
 * actor kind has a row in a table that gives the animation for each move.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_AnimStart(void);

/* Per actor kind, the offset of its row in this table; the row gives, per
   move, the animation (in the data bank, $80) */
#define ANIM_OF_MOVE 0xC9D3

/*
 * @routine 81:DCB1 AnimOfMove
 * WHAT IT DOES: Gives the animation an actor of its kind plays for move A.
 * HOW: The kind's row offset from the move table, plus the move, indexes
 * the animation in the same table.
 * Entry: M 8-bit, X 8-bit; A = the move; D = the actor's record; DB =
 * $80. Exit: A low = the animation; X = its offset in the table; N and Z
 * from it; V, C from the add; Y unchanged.
 */
void abi_AnimOfMove(void)
{
    ldx8(ea_dp(ACTOR_06));
    clc();
    adc8_v(rd8(ea_absx(ANIM_OF_MOVE)));
    tax8();
    lda8(ea_absx(ANIM_OF_MOVE));
}

/*
 * @routine 81:DCA2 PlayerMoveStart
 * WHAT IT DOES: Puts the player into move A, starting its animation from
 * its first frame.
 * HOW: The move goes to PLAYER_MOVE; AnimOfMove, then AnimStart.
 * Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
 * $80. Exit: as AnimStart.
 */
void abi_PlayerMoveStart(void)
{
    sta8(ea_abs(PLAYER_MOVE));
    abi_AnimOfMove();
    abi_AnimStart();
}

/*
 * @routine 81:DC98 PlayerMoveStartAlso
 * WHAT IT DOES: The same as PlayerMoveStart, from a second entry the code
 * before it jumps over.
 * HOW: The move goes to PLAYER_MOVE, then on as PlayerMoveStart.
 * Entry: as PlayerMoveStart. Exit: as AnimStart.
 */
void abi_PlayerMoveStartAlso(void)
{
    sta8(ea_abs(PLAYER_MOVE));
    abi_AnimOfMove();
    abi_AnimStart();
}
