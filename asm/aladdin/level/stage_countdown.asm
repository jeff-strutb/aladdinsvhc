; stage_countdown.asm: the countdown at $0343 set from the stage's entry of
; its table as a stage starts.
; C: src/aladdin/level/stage_countdown.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!stage_countdowns = $DB99       ; data bank $80: per stage, the countdown's start (word)

; @routine 82:B563 StageCountdownSet
; WHAT IT DOES: Sets the countdown at $0343 to the stage's value from the
; table at $80:DB99.
; HOW: The stage number ($033F) doubled indexes the table's words; the
; word is copied a byte at a time.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: X = the stage
; number * 2; A low byte = the word's high byte.
%at($82B563)
StageCountdownSet:
    lda.w !v_033f
    asl a
    tax
    lda.w !stage_countdowns,x
    sta.w !v_0343
    lda.w !stage_countdowns+1,x
    sta.w !v_0343+1
    rts
