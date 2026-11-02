; move_change.asm: changing the player's move only when it differs: from
; move A, or running at the move its speed picks, keeping the animation's
; frame. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/move_change.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Not yet filed: the change of move that keeps the animation's frame
; ($2F), in the player's code at $81:DACE.
!PLAYER_MOVE_KEEP_FRAME = $81DC7D

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
    jmp.w !PLAYER_MOVE_KEEP_FRAME
