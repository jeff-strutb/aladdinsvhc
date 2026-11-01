; move_anim.asm: the player's moves and their animations.
; C: src/aladdin/player/move_anim.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!anim_of_move = $C9D3           ; per actor kind its row's offset; per move the animation (in the data bank, $80)

; @routine 81:DC98 PlayerMoveStartAlso
; WHAT IT DOES: The same as PlayerMoveStart, from a second entry the code
; before it jumps over.
; HOW: The move goes to PLAYER_MOVE, then on as PlayerMoveStart.
; Entry: as PlayerMoveStart. Exit: as AnimStart.
%at($81DC98)
PlayerMoveStartAlso:
    sta.w !player_move
    bra PlayerMoveStart_anim

; @routine 81:DCA2 PlayerMoveStart
; WHAT IT DOES: Puts the player into move A, starting its animation from
; its first frame.
; HOW: The move goes to PLAYER_MOVE; AnimOfMove, then AnimStart.
; Entry: M 8-bit, X 8-bit; A = the move; D = the player's record; DB =
; $80. Exit: as AnimStart.
%at($81DCA2)
PlayerMoveStart:
    sta.w !player_move
.anim:
    jsr.w AnimOfMove
    jsl.l AnimStart
    rts

; @routine 81:DCB1 AnimOfMove
; WHAT IT DOES: Gives the animation an actor of its kind plays for move A.
; HOW: The kind's row offset from the move table, plus the move, indexes
; the animation in the same table.
; Entry: M 8-bit, X 8-bit; A = the move; D = the actor's record; DB =
; $80. Exit: A low = the animation; X = its offset in the table; N and Z
; from it; V, C from the add; Y unchanged.
%at($81DCB1)
AnimOfMove:
    ldx.b !actor_06
    clc
    adc.w !anim_of_move,x
    tax
    lda.w !anim_of_move,x
    rts
