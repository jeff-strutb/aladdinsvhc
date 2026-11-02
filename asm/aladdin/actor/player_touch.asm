; player_touch.asm: what an actor does when Aladdin touches it: hurts him,
; or, when he comes down on it from above, he bounces off it (and may
; stomp it).
; C: src/aladdin/actor/player_touch.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_11 = $11                 ; taken one from as the player stomps it
!HEALTH = $0367                 ; the player's health
!PLAYER_SPEED_Y_WHOLE = !player+!actor_speed_y+1 ; negative: he is going up (word)
!V_141B = $141B                 ; nonzero: no bounce
!V_141C = $141C                 ; zero: no bounce
!V_145A = $145A                 ; negative: no bounce
!V_1448 = $1448                 ; $3C: the bouncing actor is not touched
!STOMPED = $8C                  ; the state a stomped actor goes into

; Not yet filed
!STOMP_MARK = $8384F4           ; an actor of kind $11 started at the player's place
!TOUCH_8362 = $838362           ; a touch test (ActorHitsTestBoxIfActive first)
!TOUCH_844A = $83844A           ; a touch test (ActorHitsTestBoxIfActive first)

; @routine 83:80A1 ActorHurtsPlayerOnTouchLong
; WHAT IT DOES: ActorHurtsPlayerOnTouch for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorHurtsPlayerOnTouch; called by JSL. Exit: as the C states.
%at($8380A1)
ActorHurtsPlayerOnTouchLong:
    jsr.w ActorHurtsPlayerOnTouch
    rtl

; @routine 83:835E ActorTouch8362Long
; WHAT IT DOES: The touch test at $83:8362 for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as $83:8362: M 8-bit, X 8-bit; D = the actor; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: as the C states.
%at($83835E)
ActorTouch8362Long:
    jsr.w !TOUCH_8362
    rtl

; @routine 83:83C5 ActorStompOrHurtLong
; WHAT IT DOES: ActorStompOrHurt for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorStompOrHurt; called by JSL. Exit: as the C states.
%at($8383C5)
ActorStompOrHurtLong:
    jsr.w ActorStompOrHurt
    rtl

; @routine 83:83C9 ActorStompOrHurt
; WHAT IT DOES: When Aladdin touches the actor: from the side or below he
; is hurt; coming down on it from above (and able to bounce) he bounces
; off and stomps it: it goes into state $8C, loses one from its $11, and
; a small actor marks the stomp.
; HOW: ActorHitsTestBoxIfActive; ScratchBelowBy10 tells above (C clear)
; from not; not above: PlayerEnter8CFacing. Above: the bounce needs
; health, $141B zero, $141C nonzero, $145A and the player's vertical
; speed not negative; then the bounce at $83:84D5, bit 7 of the actor's
; $09 cleared, the state, the DEC and the mark at $83:84F4.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank. Exit: as the C states.
ActorStompOrHurt:
    jsr.w ActorHitsTestBoxIfActive
    bcs .none
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcc .above
    sep.b #$30
    jsr.w PlayerEnter8CFacing
    lda.b #$01
    rts
.above:
    ldx.w !HEALTH
    beq .none
    ldx.w !V_141B
    bne .none
    ldx.w !V_141C
    beq .none
    ldx.w !V_145A
    bmi .none
    lda.w !PLAYER_SPEED_Y_WHOLE
    bmi .none
    jsr.w PlayerStandOnActor
    sep.b #$30
    asl.b !actor_09
    lsr.b !actor_09
    lda.b #!STOMPED
    sta.b !actor_state
    dec.b !ACTOR_11
    jsr.w !STOMP_MARK
.none:
    sep.b #$32
    rts

; @routine 83:840F ActorBouncesPlayer
; WHAT IT DOES: When Aladdin comes down on the actor from above (and can
; bounce), he bounces up off it; the actor is unharmed.
; HOW: Nothing when $1448 is $3C; ActorHitsTestBoxIfActive;
; ScratchBelowBy10 must say above (C clear) and the player's vertical
; speed not be negative; then health, $141B zero, $141C nonzero, $145A
; not negative; the bounce at $83:84D5.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank. Exit: as the C states.
%at($83840F)
ActorBouncesPlayer:
    lda.w !V_1448
    cmp.b #$3C
    beq .none
    jsr.w ActorHitsTestBoxIfActive
    bcs .none
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcs .none
    lda.w !PLAYER_SPEED_Y_WHOLE
    bpl .falling
.none:
    sep.b #$32
    rts
.falling:
    ldx.w !HEALTH
    beq .none
    ldx.w !V_141B
    bne .none
    ldx.w !V_141C
    beq .none
    ldx.w !V_145A
    bmi .none
    jsr.w PlayerStandOnActor
    sep.b #$30
    lda.b #$01
    rts

; @routine 83:8446 ActorTouch844ALong
; WHAT IT DOES: The touch test at $83:844A for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as $83:844A: M 8-bit, X 8-bit; D = the actor; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: as the C states.
ActorTouch844ALong:
    jsr.w !TOUCH_844A
    rtl
