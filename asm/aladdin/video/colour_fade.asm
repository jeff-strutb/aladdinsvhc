; colour_fade.asm: fading the screen by colour math.
;
; A fade task takes a fixed grey from every layer (colour subtraction) and
; steps the grey's level a frame or more apart; at 31 the screen is black.
; The task's parameter byte is the frames between steps.
; C: src/aladdin/video/colour_fade.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

!FADE_CGADSUB = $9F             ; subtract, on BG1-4 and the sprites
!FADE_WOBJSEL = $80             ; colour window 2 on

; @routine 81:9EDD ColourFadeInSprites
; WHAT IT DOES: A task that fades the sprites alone in from black by
; colour math (CGADSUB $90), then ends. No code creates it.
; HOW: A = $90; ColourFadeBeginWith; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819EDD)
ColourFadeInSprites:
    lda.b #$90
.with:
    jsr.w ColourFadeBeginWith
    bra ColourFadeInTask_fade_in

; @routine 81:9EE4 ColourFadeInBg1
; WHAT IT DOES: A task that fades layer 1 alone in from black by colour
; math (CGADSUB $81), then ends. No code creates it.
; HOW: A = $81, on into ColourFadeInSprites past its load (81:9EDF);
; ColourFadeBeginWith; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInBg1:
    lda.b #$81
    bra ColourFadeInSprites_with

; @routine 81:9EE8 ColourFadeInBg2
; WHAT IT DOES: A task that fades layer 2 alone in from black by colour
; math (CGADSUB $82), then ends. No code creates it.
; HOW: A = $82, on into ColourFadeInSprites past its load (81:9EDF);
; ColourFadeBeginWith; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInBg2:
    lda.b #$82
    bra ColourFadeInSprites_with

; @routine 81:9EEC ColourFadeInBg3
; WHAT IT DOES: A task that fades layer 3 alone in from black by colour
; math (CGADSUB $84), then ends. No code creates it.
; HOW: A = $84, on into ColourFadeInSprites past its load (81:9EDF);
; ColourFadeBeginWith; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInBg3:
    lda.b #$84
    bra ColourFadeInSprites_with

; @routine 81:9EF0 ColourFadeInBg1Sprites
; WHAT IT DOES: A task that fades layer 1 and the sprites in from black
; by colour math (CGADSUB $91), then ends. No code creates it.
; HOW: A = $91, on into ColourFadeInSprites past its load (81:9EDF);
; ColourFadeBeginWith; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInBg1Sprites:
    lda.b #$91
    bra ColourFadeInSprites_with

; @routine 81:9EF4 ColourFadeInBg1Lit
; WHAT IT DOES: A task that fades layer 1 in from black by colour math
; (CGADSUB $81), the screen turned on by clearing forced blank (not
; ScreenFullBright), then ends. No code creates it.
; HOW: A = $81; ColourFadeBeginWith; the grey at 31 for one step, a
; frame's wait; INIDISP_SHADOW's forced blank bit cleared (its low four
; bits kept); then as ColourFadeInTask from its steps down (81:9F20).
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInBg1Lit:
    lda.b #$81
    jsr.w ColourFadeBeginWith
    ldx.b #31
    jsr.w ColourFadeStep
    lda.b #1
    jsl.l TaskWait
    lda.w !inidisp_shadow
    and.b #$0F
    sta.w !inidisp_shadow
    bra ColourFadeInTask_steps

; @routine 81:9F0E ColourFadeInTask
; WHAT IT DOES: A task that fades the screen in from black by colour
; math, then ends.
; HOW: ColourFadeBegin; the grey at 31 (black) for one step, one frame's
; wait, then ScreenFullBright; the grey steps from 31 down to 0
; (ColourFadeStep); then ColourFadeOutTask's end: the colour window off
; and TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819F0E)
ColourFadeInTask:
    jsr.w ColourFadeBegin
.fade_in:
    ldx.b #31
    jsr.w ColourFadeStep
    lda.b #1
    jsl.l TaskWait
    jsl.l ScreenFullBright
.steps:
    ldx.b #31
.step:
    jsr.w ColourFadeStep
    dex
    bpl .step
    bra ColourFadeOutTask_end

; @routine 81:9F2A ColourFadeInKeep
; WHAT IT DOES: A task that fades the screen in from black by colour
; math with CGADSUB as it is, then ends. No code creates it.
; HOW: ColourFadeBeginKeep; then as ColourFadeInTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeInKeep:
    jsr.w ColourFadeBeginKeep
    bra ColourFadeInTask_fade_in

; @routine 81:9F2F ColourFadeOutSprites
; WHAT IT DOES: A task that fades the sprites alone out to black by
; colour math (CGADSUB $90), then ends. No code creates it.
; HOW: A = $90; ColourFadeBeginWith; then as ColourFadeOutTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeOutSprites:
    lda.b #$90
    jsr.w ColourFadeBeginWith
    bra ColourFadeOutTask_fade_out

; @routine 81:9F36 ColourFadeOutBg1
; WHAT IT DOES: A task that fades layer 1 alone out to black by colour
; math (CGADSUB $81), then ends. No code creates it.
; HOW: A = $81; ColourFadeBeginWith; then as ColourFadeOutTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeOutBg1:
    lda.b #$81
    jsr.w ColourFadeBeginWith
    bra ColourFadeOutTask_fade_out

; @routine 81:9F3D ColourFadeOutBg2
; WHAT IT DOES: A task that fades layer 2 alone out to black by colour
; math (CGADSUB $82), then ends. No code creates it.
; HOW: A = $82; ColourFadeBeginWith; then as ColourFadeOutTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeOutBg2:
    lda.b #$82
    jsr.w ColourFadeBeginWith
    bra ColourFadeOutTask_fade_out

; @routine 81:9F44 ColourFadeOutBg3
; WHAT IT DOES: A task that fades layer 3 alone out to black by colour
; math (CGADSUB $84), then ends. No code creates it.
; HOW: A = $84; ColourFadeBeginWith; then as ColourFadeOutTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeOutBg3:
    lda.b #$84
    jsr.w ColourFadeBeginWith
    bra ColourFadeOutTask_fade_out

; @routine 81:9F4B ColourFadeOutBg1Sprites
; WHAT IT DOES: A task that fades layer 1 and the sprites out to black
; by colour math (CGADSUB $91), then ends. No code creates it.
; HOW: A = $91; ColourFadeBeginWith; then as ColourFadeOutTask after its
; ColourFadeBegin.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
ColourFadeOutBg1Sprites:
    lda.b #$91
    jsr.w ColourFadeBeginWith
    bra ColourFadeOutTask_fade_out

; @routine 81:9F52 ColourFadeOutTask
; WHAT IT DOES: A task that fades the screen out to black by colour math,
; then ends.
; HOW: ColourFadeBegin; the grey steps from 0 up to 31 (ColourFadeStep);
; then the colour window off (WOBJSEL's shadow 0) and TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819F52)
ColourFadeOutTask:
    jsr.w ColourFadeBegin
.fade_out:
    ldx.b #0
.step:
    jsr.w ColourFadeStep
    inx
    cpx.b #32
    bne .step
.end:
    stz.w !wobjsel_shadow
    jml.l TaskExit

; @routine 81:9F66 ColourFadeBegin
; WHAT IT DOES: Sets colour math up for a fade by subtraction on every
; layer and the sprites, and halves the fade task's frames between steps
; (at least 1).
; HOW: A = $9F, on into ColourFadeBeginWith.
; Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: as ColourFadeBeginKeep.
ColourFadeBegin:
    lda.b #!FADE_CGADSUB

; @routine 81:9F68 ColourFadeBeginWith
; WHAT IT DOES: Sets colour math up for a fade by subtraction on the
; layers A names (CGADSUB's bits), and halves the fade task's frames
; between steps.
; HOW: CGADSUB's shadow = A, then on into ColourFadeBeginKeep.
; Entry: as ColourFadeBegin, A low byte = CGADSUB. Exit: as
; ColourFadeBeginKeep.
ColourFadeBeginWith:
    sta.w !cgadsub_shadow

; @routine 81:9F6B ColourFadeBeginKeep
; WHAT IT DOES: Sets colour math up for a fade with CGADSUB as it is, and
; halves the fade task's frames between steps (at least 1).
; HOW: CGWSEL's shadow = 0, WOBJSEL's = $80; the task's parameter shifted
; right, 0 becoming 1.
; Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: A low byte = the new
; parameter; N clear, Z clear; C = the parameter's old bit 0; X, Y, V
; unchanged.
ColourFadeBeginKeep:
    stz.w !cgwsel_shadow
    lda.b #!FADE_WOBJSEL
    sta.w !wobjsel_shadow
    lda.w !task_param,y
    lsr a
    bne .set
    inc a
.set:
    sta.w !task_param,y
    rts

; @routine 81:9F7E ColourFadeStep
; WHAT IT DOES: Sets the fade's grey to level X and waits the task's
; frames between steps.
; HOW: The fixed colour's red, green and blue shadows each = X with its
; channel bit; then TaskWait for the task's parameter.
; Entry: M 8-bit, X 8-bit; X = the level (0-31); Y = the task's slot; DB a
; bank whose $0000-$1FFF is work RAM; called by JSR from a task.
; Exit: when the task resumes, the registers as the wait restores them.
ColourFadeStep:
    txa
    ora.b #$20
    sta.w !coldata_red
    txa
    ora.b #$40
    sta.w !coldata_green
    txa
    ora.b #$80
    sta.w !coldata_blue
    lda.w !task_param,y
    jsl.l TaskWait
    rts
