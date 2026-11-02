/*
 * move_change.c: changing the player's move only when it differs: from
 * move A, or running at the move its speed picks, keeping the animation's
 * frame. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_PlayerMoveStart(void);
void abi_AnimOfMove(void);
void abi_AnimResume(void);

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
    cmp8_v(rd8(ea_abs(PLAYER_MOVE)));  /* 81DC9D cmp */
    if (IF_Z) goto L_81DCAC;  /* 81DCA0 beq */
    abi_PlayerMoveStart(); return;
L_81DCAC:
    return;  /* 81DCAC rts */
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
    op_rep(0x10);  /* 81CF1C rep */
    lda8_v(0x02);  /* 81CF1E lda */
    ldx16(ea_dp(0x19));  /* 81CF20 ldx */
    cpx16_v(0x0200);  /* 81CF22 cpx */
    if (!IF_C) goto L_81CF29;  /* 81CF25 bcc */
    lda8_v(0x03);  /* 81CF27 lda */
L_81CF29:
    op_sep(0x10);  /* 81CF29 sep */
    goto L_81DC7D;  /* 81CF2B jmp */
L_81DC7D:
    cmp8_v(rd8(ea_abs(PLAYER_MOVE)));  /* 81DC7D cmp */
    if (IF_Z) goto L_81DCAC;  /* 81DC80 beq */
    sta8(ea_abs(PLAYER_MOVE));  /* 81DC82 sta */
    lda8(ea_dp(0x2F));  /* 81DC85 lda */
    pha8();  /* 81DC87 pha */
    abi_AnimOfMove();  /* 81DC88 jsr */
    abi_AnimResume();  /* 81DC8B jsl */
    pla8();  /* 81DC8F pla */
    sta8(ea_dp(0x2F));  /* 81DC90 sta */
    return;  /* 81DC92 rts */
L_81DCAC:
    return;  /* 81DCAC rts */
}

