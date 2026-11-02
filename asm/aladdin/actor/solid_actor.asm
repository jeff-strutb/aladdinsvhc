; solid_actor.asm: an actor Aladdin cannot pass through: he stands on its
; top, bumps his head on its bottom, or is pushed out of its sides.
; C: src/aladdin/actor/solid_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

!SOLID_BOXES = $BD66            ; by Y: half-width, half-height (words, the DB bank)
!PLAYER_X_PIXELS = !player+!actor_x+1
!PLAYER_Y_PIXELS = !player+!actor_y+1
!PLAYER_SPEED_Y = !player+!actor_speed_y ; fraction, then whole (word)
!PLAYER_X_BEFORE = $143B        ; his x pixels before this frame's move
!PLAYER_ON_ACTOR = $1428        ; nonzero: he stands on an actor
!V_141C = $141C                 ; zero: he does not bump his head
!HEALTH = $0367                 ; the player's health

; ActorSolidForPlayer's "nothing": C set, V clear. It sits before the
; routine's entry.
%at($83865B)
SolidNone:
    clv
    sec
    rts

; @routine 83:865E ActorSolidForPlayer
; WHAT IT DOES: Keeps Aladdin out of a solid actor: coming down onto its
; top he stands on it, rising into its bottom he stops, otherwise he is
; pushed out of the side he came from.
; HOW: Nothing (C set, V clear) when the actor cannot be hit ($09 bit 6
; clear) or he already stands on an actor. The box at $BD66 plus Y gives
; half-width and half-height (scratch $000C-$0013, the height's less 8
; from $0014, doubled at $0016). His x before the move inside the box and
; his y inside it but above its lower part: on top (y one above the box's
; top, unless he is rising; with no health 4 pixels lower), $1428 counted
; up and $141C cleared (C, V clear); below its middle while rising and
; $141C nonzero: under it, his speed cleared (C, V set); otherwise C and
; V set. Else, his x now inside the box and his y inside its lower part:
; his x put just outside the side he is on (C set, V clear).
; Entry: M 8-bit, X 8-bit; Y = the box's offset; D = the actor; DB a bank
; whose $0000-$1FFF is work RAM, the boxes' bank; called by JSR. Exit:
; M and X 8-bit (M 8-bit, X as on entry when nothing is tested); C and V
; as above; A, N, Z from the last test or store.
;
ActorSolidForPlayer:
    bit.b !actor_09
    bvc SolidNone
    lda.w !PLAYER_ON_ACTOR
    bne SolidNone
    rep.b #$20
    lda.w !SOLID_BOXES,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    lda.w !SOLID_BOXES+2,y
    pha
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    pla
    sec
    sbc.w #8
    sta.w !scratch+8
    asl a
    sta.w !scratch+10
    sec
    lda.b !actor_x+1
    sbc.w !PLAYER_X_BEFORE
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .sides
    sec
    lda.b !actor_y+1
    sbc.w !PLAYER_Y_PIXELS
    clc
    adc.w !scratch+8
    cmp.w !scratch+10
    bcc .sides
    sec
    lda.b !actor_y+1
    sbc.w !PLAYER_Y_PIXELS
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    bcs .sides
    lda.b !actor_y+1
    cmp.w !PLAYER_Y_PIXELS
    bcs .top
    ldx.w !V_141C
    beq .blocked
    ldx.w !PLAYER_SPEED_Y+2
    bpl .blocked
    adc.w !scratch+4
    sta.w !PLAYER_Y_PIXELS
    stz.w !PLAYER_SPEED_Y
    sep.b #$71
    stz.w !PLAYER_SPEED_Y+2
    rts
.blocked:
    sep.b #$60
    sec
    rts
.top:
    ldx.w !PLAYER_SPEED_Y+1
    bmi .blocked
    inc a
    sbc.w !scratch+4
    sta.w !PLAYER_Y_PIXELS
    sep.b #$30
    lda.w !HEALTH
    bne .stood
    rep.b #$20
    clc
    lda.w !PLAYER_Y_PIXELS
    adc.w #4
    sta.w !PLAYER_Y_PIXELS
    sep.b #$20
.stood:
    inc.w !PLAYER_ON_ACTOR
    stz.w !V_141C
    clc
    clv
    rts
.sides:
    sec
    lda.b !actor_x+1
    sbc.w !PLAYER_X_PIXELS
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .out
    sec
    lda.b !actor_y+1
    sbc.w !PLAYER_Y_PIXELS
    inc a
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    bcs .out
    lda.b !actor_x+1
    cmp.w !PLAYER_X_PIXELS
    bcc .left
    sbc.w !scratch
    sta.w !PLAYER_X_PIXELS
    bra .out
.left:
    clc
    inc a
    adc.w !scratch
    sta.w !PLAYER_X_PIXELS
.out:
    sep.b #$31
    clv
    rts
