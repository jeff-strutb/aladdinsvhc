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

; @routine 81:9F66 ColourFadeBegin
; WHAT IT DOES: Sets colour math up for a fade by subtraction, and halves
; the fade task's frames between steps (at least 1).
; HOW: CGADSUB's shadow = $9F, CGWSEL's = 0, WOBJSEL's = $80; the task's
; parameter shifted right, 0 becoming 1.
; Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: A low byte = the new
; parameter; N clear, Z clear; C = the parameter's old bit 0; X, Y, V
; unchanged.
%at($819F66)
ColourFadeBegin:
    lda.b #!FADE_CGADSUB
    sta.w !cgadsub_shadow
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
