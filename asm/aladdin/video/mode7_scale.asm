; mode7_scale.asm: a Mode 7 matrix element scaled, through the PPU's
; multiplier, for the matrix the set-up at $80:85E2 writes.
; C: src/aladdin/video/mode7_scale.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

; @routine 80:867A Mode7ElementScale
; WHAT IT DOES: Scales a Mode 7 matrix element: A times X / 256, for the
; caller to write to a matrix register (low byte Y, then high byte X).
; HOW: The PPU's signed multiplier: the element negated goes to M7A (low
; byte, then high), X negated to M7B; the product's middle and high bytes
; are read back into Y and X (both factors negated, so X = $80 counts as
; +128). The high byte is written to M7B once more.
; Entry: M 16-bit, X 8-bit; A = the element; X = the scale; DB a bank
; whose $2100-$213F are the PPU's. Exit: as the C states.
%at($80867A)
Mode7ElementScale:
    eor.w #$FFFF
    inc a
    tay
    sty.w !M7A
    xba
    tay
    sty.w !M7A
    txa
    eor.w #$FFFF
    inc a
    tax
    stx.w !M7B
    ldy.w !MPYM
    ldx.w !MPYH
    stx.w !M7B
    rts
