/*
 * move_change.c: changing the player's move only when it differs: from
 * move A, or running at the move its speed picks, keeping the animation's
 * frame. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"

void abi_PlayerMoveStart(void);
void abi_AnimOfMove(void);
void abi_AnimResume(void);
void abi_PlayerMoveKeepFrame(void);

/*
 * @routine 81:DC9D PlayerMoveChange
 * WHAT IT DOES: Puts the player into move A, starting its animation,
 * unless it is in that move already.
 * HOW: A compared with PLAYER_MOVE: the same, returns at PlayerMoveStart's
 * RTS; else on into PlayerMoveStart.
 * Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
 * $80. Exit: the same move: A unchanged, Z set; else as PlayerMoveStart.
 */
void abi_PlayerMoveChange(void)
{
    uint8_t move = (uint8_t)cpu.a, now = rd8(dba(PLAYER_MOVE));

    cmp8_of(move, now);
    if (move != now)
        abi_PlayerMoveStart();
}


/*
 * @routine 81:CF1C PlayerRunMove
 * WHAT IT DOES: Sets the player's running move by its speed: move 2 below
 * 2 pixels a frame, move 3 at or above, keeping the animation's frame.
 * HOW: The speed's fraction and pixels (16-bit X) compared with $0200;
 * then the change at $81:DC7D: when the move differs, PLAYER_MOVE, the
 * move's animation by AnimOfMove and AnimResume, the frame ($2F) kept.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; the same move: A low = the move, Z set;
 * changed: A low = the frame kept, X and Y as AnimResume left them.
 */
void abi_PlayerRunMove(void)
{
    uint16_t speed;

    op_rep(P_X);
    a8_load(0x02);                        /* running below 2 pixels a frame */
    speed = dp16(ACTOR_SPEED_X);
    cpu.x = speed;
    cmp16_of(speed, 0x0200);
    if (speed >= 0x0200)
        a8_load(0x03);                    /* at 2 or more */
    op_sep(P_X);
    abi_PlayerMoveKeepFrame();
}
/*
 * @routine 81:DC7D PlayerMoveKeepFrame
 * WHAT IT DOES: Puts the player into move A, unless he is in it already,
 * keeping his animation's frame: the new move's animation resumes at the
 * frame the old one had reached.
 * HOW: A against PLAYER_MOVE: the same, out at PlayerMoveStart's RTS
 * ($81:DCAC). Else A to PLAYER_MOVE; the frame ($2F) pushed; AnimOfMove,
 * AnimResume; the frame pulled back into $2F.
 * Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
 * $80; jumped to (PlayerRunMove). Exit: the same move: A low = the move,
 * Z set; changed: A low = the frame kept, N and Z from it; X and Y as
 * AnimResume left them.
 */
void abi_PlayerMoveKeepFrame(void)
{
    uint8_t move = (uint8_t)cpu.a;

    cmp8_of(move, rd8(dba(PLAYER_MOVE)));
    if (move == rd8(dba(PLAYER_MOVE)))
        return;
    wr8(dba(PLAYER_MOVE), move);
    a8_load(rd8(dpa(ACTOR_FRAME)));
    pha8();                               /* the frame, kept across the change */
    abi_AnimOfMove();
    abi_AnimResume();
    pla8();
    wr8(dpa(ACTOR_FRAME), (uint8_t)cpu.a);
}
