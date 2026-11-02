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
!PLAYER_CODE = !player+!actor_03 ; where his code goes on (word, his bank)
!PLAYER_LANDED_CODE = $D77C     ; his code once landed on an actor
!PLAYER_LANDED = $144E          ; 1: he has landed on an actor
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
    jsr.w PlayerStandOnActor
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
    jsl.l ActorAllocBLong
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

; @routine 83:84D5 PlayerStandOnActor
; WHAT IT DOES: Stands Aladdin on the actor he has come down on: his code
; goes on at his landing ($D77C of his bank) and his y is the actor's top.
; HOW: Unless a state $8C is counted ($141B): $D77C as his code address,
; his y pixels = the actor's less scratch $0010 (the boxes' half-heights
; less 4, from ScratchBelowBy10), and $144E = 1. With $141B nonzero it
; pulls its own return address and returns from its caller (M and X
; 8-bit), code the retail game never runs: each of its five callers
; ($83:8382, $83F6, $843E, $847E, $84BC) has just found $141B zero.
; Entry: M 16-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 16-bit, X 8-bit; A = his y pixels; X =
; 1; N, Z from X; C, V from the subtraction; Y unchanged.
%at($8384D5)
PlayerStandOnActor:
    lda.w #!PLAYER_LANDED_CODE
    ldx.w !PLAYER_STATE_8C_COUNT
    bne .counted
    sta.w !PLAYER_CODE
    sec
    lda.b !actor_y+1
    sbc.w !scratch+4
    sta.w !PLAYER_Y_PIXELS
    ldx.b #1
    stx.w !PLAYER_LANDED
    rts
.counted:
    sep.b #$30
    pla
    pla
    rts
