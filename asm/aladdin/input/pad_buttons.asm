; pad_buttons.asm: questions about the buttons pressed this frame.
; C: src/aladdin/input/pad_buttons.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_state.inc"

arch 65816

; @routine 80:A04F PadConfirmPressed
; WHAT IT DOES: Tells whether pad 1 pressed A, X, B, Y or START this
; frame, the buttons that move a screen on.
; HOW: A and X are bits 7 and 6 of the pressed word's low byte; B, Y and
; START bits 7, 6 and 4 of its high byte, tested only when neither A nor
; X was pressed.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C
; states.
%at($80A04F)
PadConfirmPressed:
    lda.w !pad_pressed
    and.b #$C0
    bne .done
    lda.w !pad_pressed+1
    and.b #$D0
.done:
    rts
