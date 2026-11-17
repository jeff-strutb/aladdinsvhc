; multiply.asm: the CPU's 8x8 multiplier and 16/8 divider.
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

; Helpers no code refers to: each runs only in the unit calls of
; cov_helpers_unused.box.
!MUL_SCRATCH = $000C             ; (data bank) MultiplyWordByte's word, byte and work: $0C-$13

; @routine 81:A7F1 DivideBy
; WHAT IT DOES: Divides A by Y with the CPU's divider: the quotient in A,
; the remainder in Y. No code refers to it.
; HOW: A to WRDIVL and Y to WRDIVB (at their widths: with X 16-bit the
; high byte goes to $4207), eight NOPs while it works, then A from RDDIVL
; and Y from RDMPYL.
; Entry: M 16-bit and X 8-bit for a 16 by 8 division (either width works
; as written); DB a bank whose $4200-$421F are the CPU's; called by JSL.
; Exit: A = the quotient, Y = the remainder (at their widths); N, Z from
; Y; X, C, V unchanged.
%at($81A7F1)
DivideBy:
    sta.w !WRDIVL
    sty.w !WRDIVB
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    lda.w !RDDIVL
    ldy.w !RDMPYL
    rtl

; @routine 81:A806 MultiplyXA
; WHAT IT DOES: Multiplies X's low byte by A: the 16-bit product in A. No
; code refers to it.
; HOW: X to WRMPYA (with X 16-bit its high byte to WRMPYB too, which
; MultiplyBy then writes over), MultiplyBy, then RDMPYH, XBA, and the
; product's low byte from Y.
; Entry: M 8-bit, X 16-bit (as MultiplyBy); A low byte = the second
; factor; DB a bank whose $4200-$421F are the CPU's; called by JSL. Exit:
; A = the product (16 bits); Y = the product; N, Z from its low byte; X,
; C, V unchanged.
MultiplyXA:
    stx.w !WRMPYA
    jsr.w MultiplyBy
    lda.w !RDMPYH
    xba
    tya
    rtl

; @routine 81:A812 MultiplyWordByte
; WHAT IT DOES: Multiplies the word at $0C by the byte at $0E (scratch, in
; the data bank) into three bytes at $0C-$0E. No code refers to it.
; HOW: With X the word at $0E in WRMPYA (and WRMPYB): $0C times it (high
; byte to $0F, the product to $10-$11), then $0D times it (high byte to
; $11, the product to $12-$13); then from $0E down with the carry: $0E =
; $11, $0D = $12 + $0F, $0C = $10.
; Entry: M 8-bit, X 16-bit (as MultiplyBy); DB a bank whose $0000-$1FFF is
; work RAM and $4200-$421F the CPU's; called by JSL. Exit: A low byte =
; the byte at $0C; N, V, Z, C from its ADC; X = the word at $0E as it was;
; Y = the second product; scratch $0F-$13 as above.
MultiplyWordByte:
    lda.w !MUL_SCRATCH
    ldx.w !MUL_SCRATCH+2
    stx.w !WRMPYA
    jsr.w MultiplyBy
    lda.w !RDMPYH
    sta.w !MUL_SCRATCH+3
    sty.w !MUL_SCRATCH+4
    lda.w !MUL_SCRATCH+1
    stx.w !WRMPYA
    jsr.w MultiplyBy
    lda.w !RDMPYH
    sta.w !MUL_SCRATCH+5
    sty.w !MUL_SCRATCH+6
    clc
    lda.w !MUL_SCRATCH+5
    adc.b #$00
    sta.w !MUL_SCRATCH+2
    lda.w !MUL_SCRATCH+6
    adc.w !MUL_SCRATCH+3
    sta.w !MUL_SCRATCH+1
    lda.w !MUL_SCRATCH+4
    adc.b #$00
    sta.w !MUL_SCRATCH
    rtl
