; mode7_rotate.asm: mode 7's matrix set for a rotation by an angle, from
; the sine table in bank $82.
; C: src/aladdin/video/mode7_rotate.c.

incsrc "../include/macros.inc"

arch 65816

; Mode 7's matrix as the NMI writes it (M7A-M7D, words).
!m7a_shadow = $038D
!m7b_shadow = $038F
!m7c_shadow = $0391
!m7d_shadow = $0393
!rotate_sines = $8287D3          ; a quarter turn of sines (words), 64 entries and the one past

; @routine 81:B1CD Mode7Rotate
; WHAT IT DOES: Sets mode 7's matrix to a rotation by angle A (256 to a
; turn): A and D the cosine, B the sine and C its negative.
; HOW: The angle's place in its quarter gives the table entry of one value
; (X) and of the other (Y, from the quarter's far end); the quarter (A's
; top two bits) picks which is the sine and which signs they take.
; Entry: M 8-bit, X 8-bit; A = the angle; DB = $00-$3F or $80-$BF; called
; by JSL. Exit: M 8-bit; A as the last store left it (16 bits); X, Y the
; two entries.
%at($81B1CD)
Mode7Rotate:
    pha
    and.b #$3F
    asl a
    tax
    eor.b #$7F
    inc a
    tay
    pla
    cmp.b #$C0
    bcs .lB23F
    cmp.b #$80
    bcs .lB21F
    cmp.b #$40
    bcs .lB1FF
    rep.b #$20
    jsr.w RotateSine
    sta.w !m7b_shadow
    eor.w #$FFFF
    inc a
    sta.w !m7c_shadow
    tyx
    jsr.w RotateSine
    sta.w !m7a_shadow
    sta.w !m7d_shadow
    sep.b #$20
    rtl
.lB1FF:
    rep.b #$20
    jsr.w RotateSine
    eor.w #$FFFF
    inc a
    sta.w !m7a_shadow
    sta.w !m7d_shadow
    tyx
    jsr.w RotateSine
    sta.w !m7b_shadow
    eor.w #$FFFF
    inc a
    sta.w !m7c_shadow
    sep.b #$20
    rtl
.lB21F:
    rep.b #$20
    jsr.w RotateSine
    sta.w !m7c_shadow
    eor.w #$FFFF
    inc a
    sta.w !m7b_shadow
    tyx
    jsr.w RotateSine
    eor.w #$FFFF
    inc a
    sta.w !m7a_shadow
    sta.w !m7d_shadow
    sep.b #$20
    rtl
.lB23F:
    rep.b #$20
    jsr.w RotateSine
    sta.w !m7a_shadow
    sta.w !m7d_shadow
    tyx
    jsr.w RotateSine
    sta.w !m7c_shadow
    eor.w #$FFFF
    inc a
    sta.w !m7b_shadow
    sep.b #$20
    rtl


; @routine 81:B25B RotateSine
; WHAT IT DOES: Entry X of the quarter-turn sine table, divided by 16.
; HOW: The word at $82:87D3 + X, shifted right four times.
; Entry: M 16-bit; X = the entry's byte offset; called by JSR. Exit: A the
; value; N clear, Z from it; C the last bit shifted out.
%at($81B25B)
RotateSine:
    lda.l !rotate_sines,x
    lsr a
    lsr a
    lsr a
    lsr a
    rts
