; ground_push.asm: ground that pushes the player along: tiles whose behaviour has bit 4
; set. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/ground_push.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!GROUND_PUSHED = $1429          ; 2 while the player stands on ground that pushes, else 0

; @routine 81:CE86 PlayerGroundPush
; WHAT IT DOES: On ground that pushes (tile behaviour bit 4), steps the
; player's speed across by $80 one way or the other by its facing and the
; tile's bit 7, up to $0280 and down to 0, and marks it pushed.
; HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, done.
; Else GROUND_PUSHED = 2; bit 7 clear (C clear): facing left adds $80 (at
; most $0280), facing right takes $80 (not below 0); bit 7 set: the other
; way round.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code (81:CD87) in levels with $035C = 4 or level
; $1A. Exit: M 8-bit; A low as the last step left it.
%at($81CE86)
PlayerGroundPush:
    stz.w !GROUND_PUSHED
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq .lCEF3
    bcs .lCEC5
    lda.b #$02
    sta.w !GROUND_PUSHED
    lda.b !actor_facing
    beq .lCEB5
    rep.b #$20
    lda.b !actor_speed_x
    clc
    adc.w #$0080
    sec
    cmp.w #$0280
    bcc .lCEAF
    lda.w #$0280
.lCEAF:
    sta.b !actor_speed_x
    sep.b #$20
    bra .lCEF3
.lCEB5:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    sbc.w #$0080
    bcc .lCEC1
    sta.b !actor_speed_x
.lCEC1:
    sep.b #$20
    bra .lCEF3
.lCEC5:
    lda.b #$02
    sta.w !GROUND_PUSHED
    lda.b !actor_facing
    beq .lCEDE
    rep.b #$20
    lda.b !actor_speed_x
    sec
    sbc.w #$0080
    bcc .lCEDA
    sta.b !actor_speed_x
.lCEDA:
    sep.b #$20
    bra .lCEF3
.lCEDE:
    rep.b #$20
    lda.b !actor_speed_x
    clc
    adc.w #$0080
    sec
    cmp.w #$0280
    bcc .lCEEF
    lda.w #$0280
.lCEEF:
    sta.b !actor_speed_x
    sep.b #$20
.lCEF3:
    rts
