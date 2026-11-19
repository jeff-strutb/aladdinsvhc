; backdrop_drift.asm: the actor of kind $67 (its start at $83:E5F1) moves
; the backdrop layer up and down the screen, by its speed a frame, in
; turns.
; C: src/aladdin/actor/backdrop_drift.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; The backdrop's height against the play field's, added to the play
; field's scroll Y for the backdrop's ($81:CA6A): 32-bit, a fraction byte
; first.
!BACK_Y_OFFSET = $1A2B
!DRIFT_SPEED = $42              ; the actor's: fraction pixels a frame (word)
!DRIFT_FRAMES = $40             ; the actor's: frames left of the drift at full speed (word)

; @routine 83:E5F1 Kind67Start
; @actor
; WHAT IT DOES: Actor kind $67: drifts the backdrop down the screen for
; 128 frames at $80 fraction pixels a frame, slows to a stop by 1 a
; frame, rests 48 frames; then the same upwards; for good.
; HOW: $09 = $80. Then each turn, 16-bit: DRIFT_FRAMES = $80, DRIFT_SPEED
; = $80; DRIFT_FRAMES frames (BRK first) of BackdropDriftDown; frames
; (BRK first) of ActorCount42Down and BackdropDriftDown until
; DRIFT_SPEED is 0; 8-bit, COP $30. The same with BackdropDriftUp (the
; speed set first, then the frames); 8-bit, COP $30; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $67
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83E5F1)
Kind67Start:
    lda.b #$80
    sta.b !actor_09
.turn:
    rep.b #$20
    lda.w #$0080
    sta.b !DRIFT_FRAMES
    lda.w #$0080
    sta.b !DRIFT_SPEED
.down:
    brk.b #$00
    jsr.w BackdropDriftDown
    dec.b !DRIFT_FRAMES
    bne .down
.downSlow:
    brk.b #$00
    jsr.w ActorCount42Down
    jsr.w BackdropDriftDown
    lda.b !DRIFT_SPEED
    bne .downSlow
    sep.b #$20
    lda.b #$30
    cop.b #$00
    rep.b #$20
    lda.w #$0080
    sta.b !DRIFT_SPEED
    lda.w #$0080
    sta.b !DRIFT_FRAMES
.up:
    brk.b #$00
    jsr.w BackdropDriftUp
    dec.b !DRIFT_FRAMES
    bne .up
.upSlow:
    brk.b #$00
    jsr.w ActorCount42Down
    jsr.w BackdropDriftUp
    lda.b !DRIFT_SPEED
    bne .upSlow
    sep.b #$20
    lda.b #$30
    cop.b #$00
    bra .turn
%ends_at($83E645)

; @routine 83:E653 BackdropDriftDown
; WHAT IT DOES: Moves the backdrop down the screen by the actor's speed:
; its offset against the play field's scroll falls.
; HOW: A 32-bit subtraction of the speed from the offset, a word at a
; time.
; Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($83E653)
BackdropDriftDown:
    sec
    lda.w !BACK_Y_OFFSET
    sbc.b !DRIFT_SPEED
    sta.w !BACK_Y_OFFSET
    lda.w !BACK_Y_OFFSET+2
    sbc.w #$0000
    sta.w !BACK_Y_OFFSET+2
    rts

; @routine 83:E666 BackdropDriftUp
; WHAT IT DOES: Moves the backdrop up the screen by the actor's speed:
; its offset against the play field's scroll grows.
; HOW: A 32-bit addition of the speed to the offset, a word at a time.
; Entry: M 16-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
BackdropDriftUp:
    clc
    lda.w !BACK_Y_OFFSET
    adc.b !DRIFT_SPEED
    sta.w !BACK_Y_OFFSET
    lda.w !BACK_Y_OFFSET+2
    adc.w #$0000
    sta.w !BACK_Y_OFFSET+2
    rts
