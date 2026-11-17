; player_ride.asm: the player carried along by an actor's move. Written
; from the bytes; named and described by hand.
; C: src/aladdin/player/player_ride.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!RIDE_KEPT_X = $4C               ; the actor's x pixels where its place was kept (word)
!RIDE_KEPT_Y = $4E               ; its y pixels there (word)

; @routine 81:9D63 PlayerMoveWithActor
; WHAT IT DOES: Moves the player by as much as the actor has moved since
; its place was last kept at $4C and $4E: so he goes along with it.
; HOW: With 16-bit A: the actor's y pixels less its $4E added to the
; player's y pixels, its x pixels less its $4C (kept on the stack
; meanwhile) to his x pixels. No script runs it: written from its bytes.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL. Exit: M 8-bit; A = the player's new x pixels
; (16 bits); N, V, Z, C from its addition.
%at($819D63)
PlayerMoveWithActor:
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.b !RIDE_KEPT_X
    pha
    sec
    lda.b !actor_y+1
    sbc.b !RIDE_KEPT_Y
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    pla
    clc
    adc.w !player+!actor_x+1
    sta.w !player+!actor_x+1
    sep.b #$20
    rtl
