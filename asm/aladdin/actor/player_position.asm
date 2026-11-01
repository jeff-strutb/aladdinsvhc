; player_position.asm: Aladdin's position as actors set and test it.
; C: src/aladdin/actor/player_position.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; A point an actor sets.
!point_145d = $145D
!point_145f = $145F

; @routine 83:8865 ActorInReachOfPlayer
; WHAT IT DOES: Tells whether Aladdin is within reach of the actor: a
; box 26 pixels wide and 31 high placed by his position.
; HOW: Unsigned compares, so a difference below 0 counts as far: player
; X plus 18 (8 when $08F0 is set) less the actor's X must be below 26,
; then player Y plus 2 less the actor's Y below 31.
; Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL. Exit: as the C states.
%at($838865)
ActorInReachOfPlayer:
    rep.b #$21
    lda.w #18
    ldx.w !player+!actor_flip
    beq .reach
    lda.w #8
.reach:
    adc.w !player+!actor_x+1
    sec
    sbc.b !actor_x+1
    cmp.w #26
    bcs .done
    lda.w !player+!actor_y+1
    adc.w #2
    sec
    sbc.b !actor_y+1
    cmp.w #31
.done:
    sep.b #$20
    rtl

; @routine 83:C8DD PlayerToActor
; WHAT IT DOES: Puts Aladdin at the actor's position, 12 pixels below
; its Y, as an actor that carries him does.
; HOW: The actor's x and y pixels to the player's, 12 added to Y.
; Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($83C8DD)
PlayerToActor:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    clc
    adc.w #12
    sta.w !player+!actor_y+1
    sep.b #$20
    rts

; @routine 83:C8F0 Point145DToActor
; WHAT IT DOES: Sets the point at $145D to the actor's position, 12
; pixels below its Y.
; HOW: The actor's x and y pixels to the point's, 12 added to Y.
; Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
Point145DToActor:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !point_145d
    lda.b !actor_y+1
    clc
    adc.w #12
    sta.w !point_145f
    sep.b #$20
    rts
