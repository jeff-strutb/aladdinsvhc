; player_grab.asm: tests of where Aladdin stands against an actor: just
; in front of it, and close enough to take hold of it.
; C: src/aladdin/actor/player_grab.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!PLAYER_X_PIXELS = !player+!actor_x+1
!PLAYER_Y_PIXELS = !player+!actor_y+1
!PLAYER_FLIP = !player+!actor_flip
!PLAYER_SPEED_Y_WHOLE = !player+!actor_speed_y+1 ; negative: he is going up (word)
!V_1427 = $1427                 ; nonzero: he can take hold whatever his state
!PLAYER_STATE_8C_COUNT = $141B  ; counted up on entering state $8C
!V_1448 = $1448                 ; 7 to $0F: no taking hold
!V_141C = $141C                 ; zero: no taking hold

; @routine 83:882E ActorGrabbableByPlayer
; WHAT IT DOES: Tells whether Aladdin can take hold of the actor: he is
; close in front of it and either $1427 is set, or no state $8C is
; counted, $1448 is not 7 to $0F, $141C is nonzero and he is not rising
; faster than 2 pixels a frame.
; HOW: PlayerFacesActorClose; then the tests in that order, C clear for
; yes (CLC) and set for no (SEP #$21).
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 8-bit; C clear for yes; A, X and the
; other flags from the last test.
%at($83882E)
ActorGrabbableByPlayer:
    jsl.l PlayerFacesActorClose
    bcs .no
    lda.w !V_1427
    bne .yes
    lda.w !PLAYER_STATE_8C_COUNT
    bne .no
    ldx.w !V_1448
    cpx.b #$07
    bcc .state
    cpx.b #$10
    bcc .no
.state:
    lda.w !V_141C
    beq .no
    rep.b #$20
    lda.w !PLAYER_SPEED_Y_WHOLE
    bpl .yes
    cmp.w #$FFFE
    bcc .no
.yes:
    clc
    sep.b #$20
    rts
.no:
    sep.b #$21
    rts

; @routine 83:8861 ActorGrabbableByPlayerLong
; WHAT IT DOES: ActorGrabbableByPlayer for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorGrabbableByPlayer; called by JSL. Exit: as
; ActorGrabbableByPlayer.
ActorGrabbableByPlayerLong:
    jsr.w ActorGrabbableByPlayer
    rtl

; @routine 83:8865 PlayerFacesActorNear
; WHAT IT DOES: Tells whether Aladdin stands just in front of the actor:
; his x plus 18 (8 when he faces left) less the actor's below 26, and his
; y plus 2 less the actor's below 31, unsigned.
; HOW: Two subtractions and unsigned compares on the pixel words.
; Entry: D = the actor; DB a bank whose $0000-$1FFF is work RAM; called by
; JSL. Exit: C clear when he does; A = the last difference (16 bits); N,
; Z from the compare that decided, V from the last subtraction; M 8-bit;
; X = his flip byte (low byte, X 8-bit), Y unchanged.
%at($838865)
PlayerFacesActorNear:
    rep.b #$21
    lda.w #$0012
    ldx.w !PLAYER_FLIP
    beq .facing
    lda.w #$0008
.facing:
    adc.w !PLAYER_X_PIXELS
    sec
    sbc.b !actor_x+1
    cmp.w #$001A
    bcs .out
    lda.w !PLAYER_Y_PIXELS
    adc.w #$0002
    sec
    sbc.b !actor_y+1
    cmp.w #$001F
.out:
    sep.b #$20
    rtl

; @routine 83:888C PlayerFacesActorClose
; WHAT IT DOES: Tells whether Aladdin is close in front of the actor: his
; x plus 22 (8 when he faces left) less the actor's below 30, and his y
; plus 3 less the actor's below 26, unsigned.
; HOW: As PlayerFacesActorNear, with the other offsets and sizes.
; Entry: as PlayerFacesActorNear. Exit: as PlayerFacesActorNear.
PlayerFacesActorClose:
    rep.b #$21
    lda.w #$0016
    ldx.w !PLAYER_FLIP
    beq .facing
    lda.w #$0008
.facing:
    adc.w !PLAYER_X_PIXELS
    sec
    sbc.b !actor_x+1
    cmp.w #$001E
    bcs .out
    lda.w !PLAYER_Y_PIXELS
    adc.w #$0003
    sec
    sbc.b !actor_y+1
    cmp.w #$001A
.out:
    sep.b #$20
    rtl
