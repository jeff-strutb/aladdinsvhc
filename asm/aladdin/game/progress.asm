; progress.asm: the state a new game starts with, and the level the game's
; progress leads to.
; C: src/aladdin/game/progress.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; The game's progress: which stage of which level sequence.
!progress_sequence = $035C       ; the sequence (an offset in progress_levels)
!progress_stage = $035D          ; the stage in it
!progress_levels = $81B73E       ; per sequence, the offset of its levels; then the levels
; A new game's counts (purposes not yet established, named by address).
!state_0358 = $0358
!state_035a = $035A
!state_0362 = $0362              ; three bytes set to 3
!state_0365 = $0365              ; three bytes set to 3
!state_0368 = $0368              ; two bytes set to $10
!state_036a = $036A              ; cleared, with $036B-$036E

; @routine 81:8118 NewGameState
; WHAT IT DOES: Sets the counts a new game starts with, and the first
; level.
; HOW: LevelCountersClear; LevelFromProgress; three bytes from $0362 and
; three from $0365 = 3, $0368-$0369 = $10, and $036A-$036E, $0358 and
; $035A cleared.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
; $10; X = the level; N, Z from the last load; Y, C, V as LevelCountersClear
; and LevelFromProgress leave them.
%at($818118)
NewGameState:
    jsl.l LevelCountersClear
    jsr.w LevelFromProgress
    lda.b #$03
    sta.w !state_0362
    sta.w !state_0362+1
    sta.w !state_0362+2
    lda.b #$03
    sta.w !state_0365
    sta.w !state_0365+1
    sta.w !state_0365+2
    lda.b #$10
    sta.w !state_0368+1
    sta.w !state_0368
    stz.w !state_036a+1
    stz.w !state_036a
    stz.w !state_036a+4
    stz.w !state_036a+3
    stz.w !state_0358
    stz.w !state_036a+2
    stz.w !state_035a
    rts


; @routine 81:8157 LevelFromProgress
; WHAT IT DOES: Sets the level from the game's progress: the stage of the
; level sequence it has reached.
; HOW: The sequence's offset in the table at $B73E plus the stage indexes
; the table again; that byte is the level.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte = the
; level; X = its index in the table; N, Z from the level;
; C, V from the ADC; Y unchanged.
%at($818157)
LevelFromProgress:
    ldx.w !level_035c
    clc
    lda.w !progress_levels,x
    adc.w !progress_stage
    tax
    lda.w !progress_levels,x
    sta.w !level
    rts
