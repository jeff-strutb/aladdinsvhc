; step_count.asm: a byte the bank-$84 sequences count a step a frame, up to
; wrap at 0 or down to $F0, while their task waits a frame each time.
; C: src/aladdin/system/step_count.c.

incsrc "../include/macros.inc"

arch 65816

!STEP_COUNT = $1B19             ; the count (cleared at $83:8A64)

; @routine 84:8982 StepCountUp
; WHAT IT DOES: Counts the step byte up by one, for a loop that runs
; until it wraps to 0.
; HOW: INC, then LDA of the byte.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($848982)
StepCountUp:
    inc.w !STEP_COUNT
    lda.w !STEP_COUNT
    rts

; @routine 84:8989 StepCountDown
; WHAT IT DOES: Counts the step byte down by one and tells whether it has
; reached $F0, for a loop that runs 16 steps down from 0.
; HOW: DEC, then the byte compared with $F0.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
StepCountDown:
    dec.w !STEP_COUNT
    lda.w !STEP_COUNT
    cmp.b #$F0
    rts
