; move_change.asm: changing the player's move only when it differs: from
; move A, or running at the move its speed picks, keeping the animation's
; frame. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/move_change.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Not yet filed: the change of move that keeps the animation's frame
; ($2F), in the player's code at $81:DACE.

; @routine 81:DC9D PlayerMoveChange
; WHAT IT DOES: Puts the player into move A, starting its animation,
; unless it is in that move already.
; HOW: A compared with PLAYER_MOVE: the same, returns at PlayerMoveStart's
; RTS; else on into PlayerMoveStart.
; Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
; $80. Exit: the same move: A unchanged, Z set; else as PlayerMoveStart.
%at($81DC9D)
PlayerMoveChange:
    cmp.w !player_move
    beq PlayerMoveStart_done


; @routine 81:CF1C PlayerRunMove
; WHAT IT DOES: Sets the player's running move by its speed: move 2 below
; 2 pixels a frame, move 3 at or above, keeping the animation's frame.
; HOW: The speed's fraction and pixels (16-bit X) compared with $0200;
; then the change at $81:DC7D: when the move differs, PLAYER_MOVE, the
; move's animation by AnimOfMove and AnimResume, the frame ($2F) kept.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit; the same move: A low = the move, Z set;
; changed: A low = the frame kept, X and Y as AnimResume left them.
%at($81CF1C)
PlayerRunMove:
    rep.b #$10
    lda.b #$02
    ldx.b !actor_speed_x
    cpx.w #$0200
    bcc .lCF29
    lda.b #$03
.lCF29:
    sep.b #$10
    jmp.w PlayerMoveKeepFrame

; @routine 81:DC7D PlayerMoveKeepFrame
; WHAT IT DOES: Puts the player into move A, unless he is in it already,
; keeping his animation's frame: the new move's animation resumes at the
; frame the old one had reached.
; HOW: A against PLAYER_MOVE: the same, out at PlayerMoveStart's RTS
; ($81:DCAC). Else A to PLAYER_MOVE; the frame ($2F) pushed; AnimOfMove,
; AnimResume; the frame pulled back into $2F.
; Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
; $80; jumped to (PlayerRunMove). Exit: the same move: A low = the move,
; Z set; changed: A low = the frame kept, N and Z from it; X and Y as
; AnimResume left them.
%at($81DC7D)
PlayerMoveKeepFrame:
    cmp.w !player_move
    beq PlayerMoveStart_done
    sta.w !player_move
    lda.b !ACTOR_FRAME
    pha
    jsr.w AnimOfMove
    jsl.l AnimResume
    pla
    sta.b !ACTOR_FRAME
    rts
