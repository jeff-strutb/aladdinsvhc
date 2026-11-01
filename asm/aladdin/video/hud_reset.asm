; hud_reset.asm: the status line drawn afresh: its text, every health mark
; place, and each value marked as not yet shown so the refresh draws it.
; C: src/aladdin/video/hud_reset.c.

incsrc "../include/macros.inc"

arch 65816

!HEALTH_MARKS = $0365           ; how many health marks the line has
!HEALTH_SHOWN = $0366           ; the health the marks show
!HUD_0363 = $0363               ; a value as shown (its value at $0364), a digit
!HUD_COUNT_1_SHOWN = $0368      ; the first counter as shown (its value at $0369)
!HUD_COUNT_2_SHOWN = $036A      ; the second counter as shown (its value at $036B)
!HUD_036D = $036D               ; a value as shown (its value at $036E)

; Not yet filed
!TEXT_DRAW = $81972D            ; text A drawn in the text layer, its upload asked for
!HUD_DIGIT_REFRESH = $81F47F    ; the digit at $0363 drawn when it differs from $0364
; HudRedrawAll goes on into the refresh at $81:F456 (each value drawn that
; differs from the one shown), the code right after it.

; @routine 81:F433 HudRedrawAll
; WHAT IT DOES: Draws the status line afresh: its text, every health mark
; place, and then each of its values.
; HOW: Text 0; HudHealthMarkShow for each mark from the last down to the
; first; the shown health, $0363, $036D and the two counters zeroed; the
; digit's refresh at $81:F47F; then on into the refresh at $81:F456.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($81F433)
HudRedrawAll:
    lda.b #$00
    jsl.l !TEXT_DRAW
    lda.w !HEALTH_MARKS
.mark:
    pha
    jsr.w HudHealthMarkShow
    pla
    dec a
    bne .mark
    stz.w !HEALTH_SHOWN
    stz.w !HUD_0363
    stz.w !HUD_036D
    stz.w !HUD_COUNT_1_SHOWN
    stz.w !HUD_COUNT_2_SHOWN
    jsr.w !HUD_DIGIT_REFRESH
