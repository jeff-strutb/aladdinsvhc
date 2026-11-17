; kind40.asm: actor kind $40, a spring Aladdin bounces off. Its wait is
; Kind40Wait (actor_waits_small.asm).
; C: src/aladdin/actor/kind40.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_state.inc"

arch 65816

!KIND40_16 = $16                ; word: $D29B
!PLAYER_SPRUNG = $D8FD          ; where the player's code goes on when it springs him
!V_141C_40 = $141C

; @routine 83:C507 Kind40Start
; @actor
; WHAT IT DOES: Actor kind $40, a spring: while Aladdin falls onto it from
; above it plays its push (animation 1), sends his code on at
; PLAYER_SPRUNG and rests 32 frames; a touch while he has no health turns
; it to state $8C (no script has that).
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND40_16 = $D29B. Then
; AnimStart 0 and each frame (BRK) until V_141C is set, his speed y high
; byte not negative and ActorHitsTestBoxLong clears C. Health 0: state
; $8C, back to AnimStart 0. Else his y plus 8 less its y: no borrow, wait
; on; a borrow (he is above): AnimStart 1, his resume address = $D8FD,
; COP $20, back to AnimStart 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $40 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83C507)
Kind40Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D29B
    sta.b !KIND40_16
    sep.b #$20
.rest:
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    lda.w !V_141C_40
    beq .wait
    lda.w !player+!actor_speed_y+2
    bmi .wait
    jsl.l ActorHitsTestBoxLong
    bcs .wait
    lda.w !health
    bne .above
    lda.b #$8C
    sta.b !actor_state
    bra .rest
.above:
    rep.b #$20
    lda.w !player+!actor_y+1
    clc
    adc.w #$0008
    sec
    sbc.b !actor_y+1
    sep.b #$20
    bcs .wait
    lda.b #$01
    jsl.l AnimStart
    lda.b #!PLAYER_SPRUNG&255
    sta.w !player+!actor_03
    lda.b #!PLAYER_SPRUNG>>8
    sta.w !player+!actor_04
    lda.b #$20
    cop.b #$00
    bra .rest
