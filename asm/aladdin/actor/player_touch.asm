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
!STOMP_SOUND = $0F              ; the sound of a stomp

; Not yet filed
!STOMP_MARK = $8384F4           ; an actor of kind $11 started at the player's place

; @routine 83:80A1 ActorHurtsPlayerOnTouchLong
; WHAT IT DOES: ActorHurtsPlayerOnTouch for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorHurtsPlayerOnTouch; called by JSL. Exit: as the C states.
%at($8380A1)
ActorHurtsPlayerOnTouchLong:
    jsr.w ActorHurtsPlayerOnTouch
    rtl

; @routine 83:835E ActorStompedByPlayerLong
; WHAT IT DOES: ActorStompedByPlayer for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorStompedByPlayer; called by JSL. Exit: as the C states.
%at($83835E)
ActorStompedByPlayerLong:
    jsr.w ActorStompedByPlayer
    rtl

; @routine 83:8362 ActorStompedByPlayer
; WHAT IT DOES: When Aladdin touches the actor: from the side or below he
; is hurt; coming down on it from above (and able to land) he is stood on
; it and it is stomped: its code address kept at $35, state $8C, one taken
; from its $11, sound $0F.
; HOW: ActorHitsTestBoxIfActive; ScratchBelowBy10 tells above (C clear)
; from not; not above: PlayerEnter8CFacing. Above: $145A not negative,
; $141C nonzero, the player's vertical speed not negative and $141B zero;
; then PlayerStandOnActor, its $03 word to its $35, bit 7 of its $09
; cleared and of its $08 set, the state, the DEC and SoundQueueAdd.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit. A
; landing refused: Z and C set. Hurt or stomped, or no touch: as
; PlayerEnter8CFacing, SoundQueueAdd or ActorHitsTestBoxIfActive leaves
; the rest.
ActorStompedByPlayer:
    jsr.w ActorHitsTestBoxIfActive
    bcs .out
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcs .hurt
    ldx.w !V_145A
    bmi .refused
    ldx.w !V_141C
    beq .refused
    lda.w !PLAYER_SPEED_Y_WHOLE
    bmi .refused
    ldx.w !V_141B
    bne .refused
    jsr.w PlayerStandOnActor
    lda.b !actor_03
    sta.b !actor_35
    sep.b #$30
    asl.b !actor_09
    lsr.b !actor_09
    lda.b !actor_08
    ora.b #$80
    sta.b !actor_08
    lda.b #!STOMPED
    sta.b !actor_state
    dec.b !ACTOR_11
    lda.b #!STOMP_SOUND
    jsl.l SoundQueueAdd
.out:
    sep.b #$30
    rts
.hurt:
    sep.b #$30
    jsr.w PlayerEnter8CFacing
    rts
.refused:
    sep.b #$32
    rts

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

; @routine 83:8446 ActorBouncesOrHurtsPlayerLong
; WHAT IT DOES: ActorBouncesOrHurtsPlayer for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorBouncesOrHurtsPlayer; called by JSL. Exit: as the C states.
ActorBouncesOrHurtsPlayerLong:
    jsr.w ActorBouncesOrHurtsPlayer
    rtl

; @routine 83:844A ActorBouncesOrHurtsPlayer
; WHAT IT DOES: When Aladdin touches the actor: from the side or below he
; is hurt; coming down on it from above (and able to bounce) he is stood
; on it; the actor is unharmed either way.
; HOW: Nothing when $1448 is $3C; ActorHitsTestBoxIfActive;
; ScratchBelowBy10 tells above (C clear) from not; not above:
; PlayerEnter8CFacing, A = 2. Above: health, $141B zero, $141C nonzero,
; $145A and the player's vertical speed not negative; then
; PlayerStandOnActor, A = 1.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit. Hurt:
; A low = 2; stood: A low = 1; N and Z clear either way. Otherwise: Z
; set; C set when the $1448 test or the box test said no; A, X, Y and the
; other flags from the last test.
ActorBouncesOrHurtsPlayer:
    lda.w !V_1448
    cmp.b #$3C
    beq .none
    jsr.w ActorHitsTestBoxIfActive
    bcs .none
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcc .above
    sep.b #$30
    jsr.w PlayerEnter8CFacing
    lda.b #2
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
    lda.b #1
    rts
.none:
    sep.b #$32
    rts
