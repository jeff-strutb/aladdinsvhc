; kind77.asm: actor kind $77, a platform that waits until Aladdin is on
; it, then speeds up to the right until it reaches x $0630, carrying him
; while he stays on.
; C: src/aladdin/actor/kind77.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K77_ON = $40                   ; nonzero while Aladdin stands on it this frame
!K77_SPEED_CAP = $41            ; word: it speeds up while its speed is below this
!K77_END_X = $43                ; word: it moves while its x pixels are below this
!K77_13 = $13                   ; 4 at its start
!K77_BOX = $28                  ; ActorSolidForPlayer's box offset
!K77_LAST_X = $000C             ; its x pixels word as he got on or last moved with it
!K77_LAST_Y = $000E             ; and its y pixels word
!PLAYER_X = $08E7               ; Aladdin's x pixels word
!PLAYER_Y = $08EA               ; Aladdin's y pixels word

; @routine 83:F494 Kind78Start
; @actor
; WHAT IT DOES: Actor kind $78: the same platform as kind $77, with its
; own course: from speed $4B with acceleration 1, up to $0180, until its
; x reaches $0B90.
; HOW: K77_SPEED_CAP $0180, K77_END_X $0B90; A = 1, Y = $4B; then on in
; Kind77Start's code from its ActorSpeedXSetAccel ($83:F4BC, a BRA).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $78 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83F494)
Kind78Start:
    lda.b #$80
    sta.b !K77_SPEED_CAP
    lda.b #$01
    sta.b !K77_SPEED_CAP+1
    lda.b #$0B
    sta.b !K77_END_X+1
    lda.b #$90
    sta.b !K77_END_X
    lda.b #$01
    ldy.b #$4B
    bra Kind77Start_run

; @routine 83:F4AA Kind77Start
; @actor
; WHAT IT DOES: Actor kind $77: a solid platform (box $28) that Aladdin
; stands on and that holds him as ActorHoldPlayer does; once he is on it,
; each frame it speeds up to the right (acceleration $10, from speed $48,
; up to $00C0) until its x reaches $0630, and while he is on it he moves
; with it.
; HOW: K77_SPEED_CAP $00C0, K77_END_X $0630; ActorSpeedXSetAccel A = $10,
; Y = $48; actor_08 bit 1 and actor_09 bits $90 set; AnimStart 0; K77_13
; 4. Each frame (BRK first) ActorSolidForPlayer Y = $28 until C clear,
; ActorHoldPlayer between. Then each frame: K77_ON = (ActorSolidForPlayer
; C clear), and on: its x, y kept at K77_LAST_X, K77_LAST_Y; x below
; K77_END_X: speed below K77_SPEED_CAP: ActorAccelerateX; ActorMoveRight;
; K77_ON: Aladdin's x and y + its move since; ActorHoldPlayer; BRK.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $77 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83F4AA)
Kind77Start:
    lda.b #$C0
    sta.b !K77_SPEED_CAP
    stz.b !K77_SPEED_CAP+1
    lda.b #$06
    sta.b !K77_END_X+1
    lda.b #$30
    sta.b !K77_END_X
    lda.b #$10
    ldy.b #$48
.run:
    jsl.l ActorSpeedXSetAccel
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    lda.b #$04
    sta.b !K77_13
.wait:
    brk.b #$00
    ldy.b #!K77_BOX
    jsr.w ActorSolidForPlayer
    bcc .on
    jsr.w ActorHoldPlayer
    bra .wait
.frame:
    brk.b #$00
    stz.b !K77_ON
    ldy.b #!K77_BOX
    jsr.w ActorSolidForPlayer
    bcs .move
.on:
    inc.b !K77_ON
    rep.b #$20
    lda.b !actor_x+1
    sta.w !K77_LAST_X
    lda.b !actor_y+1
    sta.w !K77_LAST_Y
    sep.b #$20
.move:
    rep.b #$20
    lda.b !actor_x+1
    cmp.b !K77_END_X
    bcs .moved
    lda.b !actor_speed_x
    cmp.b !K77_SPEED_CAP
    sep.b #$20
    bcs .cruise
    jsl.l ActorAccelerateX
.cruise:
    jsl.l ActorMoveRight
.moved:
    sep.b #$20
    lda.b !K77_ON
    beq .hold
    rep.b #$20
    lda.b !actor_x+1
    sec
    sbc.w !K77_LAST_X
    clc
    adc.w !PLAYER_X
    sta.w !PLAYER_X
    lda.b !actor_y+1
    sec
    sbc.w !K77_LAST_Y
    clc
    adc.w !PLAYER_Y
    sta.w !PLAYER_Y
    sep.b #$20
.hold:
    jsr.w ActorHoldPlayer
    bra .frame
