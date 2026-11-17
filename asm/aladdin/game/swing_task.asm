; swing_task.asm: a task that moves the 24-bit value at $18CC up, then
; down at a slowing speed, over 48 frames. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/game/swing_task.c.

incsrc "../include/macros.inc"

arch 65816

!SWING_VALUE = $18CC             ; the value moved (24 bits)
!SWING_1A10 = $1A10              ; counted up as the task starts
!SWING_1A12 = $1A12              ; nonzero: no camera follow up and down
!SWING_COUNT = $00A5             ; slot 2's own bytes: frames left in a stretch
!SWING_STEP = $00A6              ; the speed's fall each frame
!SWING_SPEED = $00A7             ; the speed (word)

; @routine 81:EE8B SwingTask
; WHAT IT DOES: A task that moves the 24-bit scroll value at $18CC:
; counts up $1A10 and $1A12, sets its middle byte to $3E, waits
; 16 frames, takes $000408 off it each frame for 16 frames, then adds a
; speed to it each frame for 16 frames, the speed starting at $0258 and
; falling by $49 a frame; then ends.
; HOW: SWING_COUNT, SWING_STEP and SWING_SPEED are the task's own bytes in
; slot 2 ($A5-$A8); the 24-bit sums byte by byte (SBC, then ADC, with the
; carry between bytes); TaskWait 1 each frame; TaskExit. No script runs it
; (nothing found creates it): written from its bytes.
; Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
; RAM. Exit: none: the task ends.
%at($81EE8B)
SwingTask:
    inc.w !SWING_1A10
    inc.w !SWING_1A12
    lda.b #$3E
    sta.w !SWING_VALUE+1
    lda.b #$10
    jsl.l TaskWait
    lda.b #$10
    sta.w !SWING_COUNT
.up:
    lda.b #$01
    jsl.l TaskWait
    lda.w !SWING_VALUE
    sec
    sbc.b #$08
    sta.w !SWING_VALUE
    lda.w !SWING_VALUE+1
    sbc.b #$04
    sta.w !SWING_VALUE+1
    lda.w !SWING_VALUE+2
    sbc.b #$00
    sta.w !SWING_VALUE+2
    dec.w !SWING_COUNT
    bne .up
    lda.b #$10
    sta.w !SWING_COUNT
    lda.b #$49
    sta.w !SWING_STEP
    lda.b #$58
    sta.w !SWING_SPEED
    lda.b #$02
    sta.w !SWING_SPEED+1
.down:
    lda.b #$01
    jsl.l TaskWait
    sec
    lda.w !SWING_SPEED
    sbc.w !SWING_STEP
    sta.w !SWING_SPEED
    lda.w !SWING_SPEED+1
    sbc.b #$00
    sta.w !SWING_SPEED+1
    clc
    lda.w !SWING_VALUE
    adc.w !SWING_SPEED
    sta.w !SWING_VALUE
    lda.w !SWING_VALUE+1
    adc.w !SWING_SPEED+1
    sta.w !SWING_VALUE+1
    lda.w !SWING_VALUE+2
    adc.b #$00
    sta.w !SWING_VALUE+2
    dec.w !SWING_COUNT
    bne .down
    jml.l TaskExit
