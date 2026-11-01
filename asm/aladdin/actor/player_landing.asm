; player_landing.asm: Aladdin coming down on top of an actor: he is stood
; on it, his code goes on at his landing, and an actor of kind $11 starts
; at his feet.
; C: src/aladdin/actor/player_landing.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!V_1448 = $1448                 ; $3C: no landing on actors
!V_0367 = $0367                 ; counted down on entering state $8C; 0: no landing
!PLAYER_STATE_8C_COUNT = $141B   ; counted up on entering state $8C
!V_141C = $141C                 ; 0: no landing
!V_145A = $145A                 ; bit 7 set: no landing
!PLAYER_SPEED_Y_PIXELS = !player+!actor_speed_y+1 ; his vertical speed's whole part (word)
!PLAYER_X_PIXELS = !player+!actor_x+1
!PLAYER_Y_PIXELS = !player+!actor_y+1
; Not yet filed: Aladdin's code to go on at $D77C of his bank, and him
; stood on the actor's top (its Y less the boxes' half-heights).
!PLAYER_STAND_ON_ACTOR = $8384D5
!ACTOR_TAKE_B = $81EA3D         ; a record from the second free list: X, N set when none
!KIND_11 = $11

; @routine 83:848D PlayerLandsOnActor
; WHAT IT DOES: Tells whether Aladdin has come down on top of the actor,
; and if so stands him on it, his code going on at his landing, with an
; actor of kind $11 at his feet.
; HOW: No when $1448 is $3C, when the actor misses the test box
; (ActorHitsTestBoxIfActive), when the boxes overlap by 10 or more
; pixels of height (ScratchBelowBy10), when he is rising, or when $0367
; is 0, a state $8C is counted, $141C is 0 or bit 7 of $145A is set. Yes:
; $83:84D5 stands him on it, then ActorSpawnKind11AtPlayer.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank. Exit: as the C states.
%at($83848D)
PlayerLandsOnActor:
    lda.w !V_1448
    cmp.b #$3C
    beq .no
    jsr.w ActorHitsTestBoxIfActive
    bcs .no
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcs .no
    lda.w !PLAYER_SPEED_Y_PIXELS
    bpl .down
.no:
    sep.b #$32
    rts
.down:
    ldx.w !V_0367
    beq .no
    ldx.w !PLAYER_STATE_8C_COUNT
    bne .no
    ldx.w !V_141C
    beq .no
    ldx.w !V_145A
    bmi .no
    jsr.w !PLAYER_STAND_ON_ACTOR
    sep.b #$30
    jsr.w ActorSpawnKind11AtPlayer
    lda.b #$01
    rts

; @routine 83:84F4 ActorSpawnKind11AtPlayer
; WHAT IT DOES: Starts an actor of kind $11 at Aladdin's feet: at his x,
; 16 pixels below his y, in a record of the second free list.
; HOW: The record taken through $81:EA3D; none free: nothing. Its state
; $0C (to start), kind $11, byte $07 zero, byte $08 $80, and its x and y
; pixels from his.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($8384F4)
ActorSpawnKind11AtPlayer:
    jsl.l !ACTOR_TAKE_B
    bmi .done
    lda.b #$0C
    sta.w !actor_state,x
    lda.b #!KIND_11
    sta.w !actor_06,x
    stz.w !actor_07,x
    lda.b #$80
    sta.w !actor_08,x
    rep.b #$20
    lda.w !PLAYER_X_PIXELS
    sta.w !actor_x+1,x
    lda.w !PLAYER_Y_PIXELS
    clc
    adc.w #$0010
    sta.w !actor_y+1,x
.done:
    sep.b #$30
    rts
