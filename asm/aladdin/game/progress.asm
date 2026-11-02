; progress.asm: the state a new game starts with, and the level the game's
; progress leads to.
; C: src/aladdin/game/progress.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"

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
!V_032C = $032C
!V_032D = $032D
!V_0347 = $0347
!V_0360 = $0360
; Per demo: its pad stream's offset from $84:F700 (words); its progress
; sequence, stage and $0360 (bytes).
!demo_inputs = $B724
!demo_sequences = $B6FD
!demo_stages = $B70A
!demo_0360 = $B717

; @routine 81:8066 DemoStart
; WHAT IT DOES: Sets a demo up to play: demo $033F's level, and its pad
; stream from the first entry.
; HOW: The play mode = 2 (the pad read from the demo's stream) with 1
; frame left on the current entry; the stream's offset from the word table
; at $B724 (the demo * 2); the progress sequence, the stage and $0360
; from the byte tables at $B6FD, $B70A and $B717.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
; $0360's new value; X = the demo; Y = the demo * 2; N, Z from the last
; load; C from the doubling; V unchanged.
%at($818066)
DemoStart:
    lda.b #2
    sta.w !play_mode
    lda.b #1
    sta.w !demo_input_frames
    ldx.w !demo_number
    txa
    asl a
    tay
    lda.w !demo_inputs,y
    sta.w !demo_input_at
    lda.w !demo_inputs+1,y
    sta.w !demo_input_at+1
    lda.w !demo_sequences,x
    sta.w !progress_sequence
    lda.w !demo_stages,x
    sta.w !progress_stage
    lda.w !demo_0360,x
    sta.w !V_0360
    rts

; @routine 81:80E0 PlayStartCounts
; WHAT IT DOES: Sets what play starts with: the random number's seed and
; full health; with the cheat flag at $0009 set, 10 health marks and 99
; at $0369.
; HOW: Random = $C3, $032C = 1, $032D = 0; when $0009 is set, health
; marks = 10, $036E = 1 and $0369 = $99; health = the health marks;
; $0347, $036C and $035A cleared.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
; the health; N, Z from it; X, Y, C, V unchanged.
%at($8180E0)
PlayStartCounts:
    lda.b #$C3
    sta.w !random
    lda.b #1
    sta.w !V_032C
    stz.w !V_032D
    lda.w !cheat_0009
    beq .counts
    lda.b #10
    sta.w !health_marks
    lda.b #1
    sta.w !state_036a+4
    lda.b #$99
    sta.w !state_0368+1
.counts:
    lda.w !health_marks
    sta.w !health
    stz.w !V_0347
    stz.w !state_036a+2
    stz.w !state_035a
    stz.w !state_036a+2
    rts

; @routine 81:8114 NewGameStateLong
; WHAT IT DOES: Sets the counts a new game starts with, and the first
; level, for callers in other banks.
; HOW: NewGameState.
; Entry: as NewGameState; called by JSL. Exit: as NewGameState.
NewGameStateLong:
    jsr.w NewGameState
    rtl

; @routine 81:8118 NewGameState
; WHAT IT DOES: Sets the counts a new game starts with, and the first
; level.
; HOW: LevelCountersClear; LevelFromProgress; three bytes from $0362 and
; three from $0365 = 3, $0368-$0369 = $10, and $036A-$036E, $0358 and
; $035A cleared.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
; $10; X = the level; N, Z from the last load; Y, C, V as LevelCountersClear
; and LevelFromProgress leave them.
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
