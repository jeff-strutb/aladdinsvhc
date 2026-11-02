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


; The matrix and centre the NMI writes, and each element's scale.
!m7_matrix_shadow = $038D       ; M7A, M7B, M7C, M7D (words)
!m7_scales = $0825              ; per element, A to D: the scale (bytes, $80 = 1)
!nmi_ppu_mode = $0829           ; which per-frame PPU update the NMI makes (2: Mode 7)
!mode7_centre = $0395           ; Mode 7's centre, x then y (words)

; @routine 80:85E2 Mode7MatrixWrite
; WHAT IT DOES: Writes the Mode 7 matrix and centre to the PPU, each
; element scaled by its own scale first when the NMI's mode is 1; nothing
; when it is 0.
; HOW: nmi_ppu_mode 0: returns. 1: Mode7ElementScale for D, C, B and A
; with their scales ($0825-$0828), the results (low, then high byte) to
; M7D, M7C, M7A and M7B. Else the matrix shadow ($038D) as it is, a byte
; pair to each of M7A-M7D. Then the centre ($0395) to M7X and M7Y.
; Entry: M 8-bit, X 8-bit; DB a bank whose $2100-$213F are the PPU's and
; $0000-$1FFF work RAM; called by JSR from the NMI. Exit: M 8-bit, X
; 8-bit; written: A low byte = the centre's last byte.
%at($8085E2)
Mode7MatrixWrite:
    lda.w !nmi_ppu_mode
    beq .none
    dec a
    beq .scaled
    lda.w !m7_matrix_shadow
    sta.w !M7A
    lda.w !m7_matrix_shadow+1
    sta.w !M7A
    lda.w !m7_matrix_shadow+2
    sta.w !M7B
    lda.w !m7_matrix_shadow+3
    sta.w !M7B
    lda.w !m7_matrix_shadow+4
    sta.w !M7C
    lda.w !m7_matrix_shadow+5
    sta.w !M7C
    lda.w !m7_matrix_shadow+6
    sta.w !M7D
    lda.w !m7_matrix_shadow+7
    sta.w !M7D
    bra .centre
.none:
    rts
.scaled:
    rep.b #$20
    lda.w !m7_matrix_shadow+6
    ldx.w !m7_scales+3
    jsr.w Mode7ElementScale
    sty.w !M7D
    stx.w !M7D
    lda.w !m7_matrix_shadow+4
    ldx.w !m7_scales+2
    jsr.w Mode7ElementScale
    sty.w !M7C
    stx.w !M7C
    lda.w !m7_matrix_shadow+2
    ldx.w !m7_scales+1
    jsr.w Mode7ElementScale
    phy
    phx
    lda.w !m7_matrix_shadow
    ldx.w !m7_scales
    jsr.w Mode7ElementScale
    sty.w !M7A
    stx.w !M7A
    plx
    ply
    sty.w !M7B
    stx.w !M7B
.centre:
    sep.b #$20
    lda.w !mode7_centre
    sta.w !M7X
    lda.w !mode7_centre+1
    sta.w !M7X
    lda.w !mode7_centre+2
    sta.w !M7Y
    lda.w !mode7_centre+3
    sta.w !M7Y
    rts
