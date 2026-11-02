; player_box_count.asm: counts, for the actor whose code is at $83:C3E4,
; whether the player is within its box on each axis.
; C: src/aladdin/actor/player_box_count.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!PLAYER_X_PIXELS = !player+!actor_x+1
!PLAYER_Y_PIXELS = !player+!actor_y+1
!BOX_HALF_W = $0010             ; the box: 16 pixels each side in x
!BOX_HALF_H = $003C             ; 60 pixels each side in y

; @routine 83:C47E ActorPlayerBoxCount
; WHAT IT DOES: Counts the player's nearness to the actor on each axis:
; the word at $45 steps up when the actor is within 16 pixels of the
; player in x (-16 to 15), the word at $47 when within 60 in y (-60 to
; 59).
; HOW: For each axis, the actor's pixels less the player's, plus the half
; span, compared unsigned with the whole span; INC of the word when
; below.
; Entry: M 16-bit; D = the actor; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($83C47E)
ActorPlayerBoxCount:
    sec
    lda.b !actor_x+1
    sbc.w !PLAYER_X_PIXELS
    clc
    adc.w #!BOX_HALF_W
    cmp.w #2*!BOX_HALF_W
    bcs .x_out
    inc.b !actor_45
.x_out:
    sec
    lda.b !actor_y+1
    sbc.w !PLAYER_Y_PIXELS
    clc
    adc.w #!BOX_HALF_H
    cmp.w #2*!BOX_HALF_H
    bcs .y_out
    inc.b !actor_47
.y_out:
    rts
