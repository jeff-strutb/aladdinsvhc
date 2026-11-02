; player_attach.asm: the player put at an offset from an actor (below or
; above it, and to one side by the player's facing), from a table of
; offsets.
; C: src/aladdin/player/player_attach.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!attach_offsets = $FE44         ; data bank $80: per offset, x then y (bytes)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; its y
!player_flip = !player+!actor_flip

; @routine 82:BB3A PlayerToActorAbove
; WHAT IT DOES: Puts the player at offset A from the actor: above it by
; the offset's y, and by its x to the right when the player faces right,
; to the left otherwise.
; HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
; player's y pixels = the actor's less y (16 bits, borrowing); facing
; right (its flip nonzero) its x pixels = the actor's plus x, else
; less x.
; Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
; $80; called by JSL. Exit: X = A * 2; A low byte = the high byte of
; the player's x.
%at($82BB3A)
PlayerToActorAbove:
    asl a
    tax
    sec
    lda.b !actor_y+1
    sbc.w !attach_offsets+1,x
    sta.w !player_y
    lda.b !actor_y+2
    sbc.b #$00
    sta.w !player_y+1
PlayerToActorAbove_x:
    lda.w !player_flip
    beq .left
    clc
    lda.b !actor_x+1
    adc.w !attach_offsets,x
    sta.w !player_x
    lda.b !actor_x+2
    adc.b #$00
    sta.w !player_x+1
    rtl
.left:
    sec
    lda.b !actor_x+1
    sbc.w !attach_offsets,x
    sta.w !player_x
    lda.b !actor_x+2
    sbc.b #$00
    sta.w !player_x+1
    rtl


; @routine 82:BB26 PlayerToActorBelow
; WHAT IT DOES: Puts the player at offset A from the actor: below it by
; the offset's y, and by its x to the right when the player faces right,
; to the left otherwise.
; HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
; player's y pixels = the actor's plus y (16 bits, carried); then the x
; as PlayerToActorAbove.
; Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
; $80; called by JSL. Exit: as PlayerToActorAbove.
%at($82BB26)
PlayerToActorBelow:
    asl a
    tax
    clc
    lda.b !actor_y+1
    adc.w !attach_offsets+1,x
    sta.w !player_y
    lda.b !actor_y+2
    adc.b #$00
    sta.w !player_y+1
    bra PlayerToActorAbove_x
