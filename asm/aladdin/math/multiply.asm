; multiply.asm: the CPU's 8x8 multiplier.
; C: src/aladdin/math/multiply.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

; @routine 81:A7E6 MultiplyBy
; WHAT IT DOES: Multiplies the factor already in WRMPYA by A.
; HOW: Writes A to WRMPYB, waits the multiplier's time and reads the
; 16-bit product.
; Entry: M 8-bit, X 16-bit; A low byte = the second factor; DB a bank
; whose $4200-$421F are the CPU's; called by JSR. Exit: Y = the product;
; N, Z from it; A, X, C, V unchanged.
%at($81A7E6)
MultiplyBy:
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    ldy.w !RDMPYL
    rts
