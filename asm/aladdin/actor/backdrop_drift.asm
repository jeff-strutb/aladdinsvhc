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
