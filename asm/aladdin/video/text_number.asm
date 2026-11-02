; text_number.asm: a two-digit number written into the text layer's tilemap.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/video/text_number.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/hw.inc"

arch 65816


; @routine 83:8BDE TextNumberTwoDigits
; WHAT IT DOES: Writes the number X (0-99) as two digits into the text
; layer's tilemap at the offset A (high byte) and Y (low byte), with
; attribute $28, and has the NMI upload the tilemap.
; HOW: The offset made in A (XBA, TYA); X divided by 10 by the CPU's
; divider (eight NOPs while it works); tens (the quotient) + $30 at the
; offset, ones (the remainder) + $30 two bytes on, $28 in both attribute
; bytes; TEXT_TILEMAP_UPLOAD up.
; Entry: M 8-bit, X 8-bit; X = the number; A = the offset's high byte; Y =
; its low byte; DB a bank whose $4200-$421F are the CPU's; called by JSR
; (83:8BDA, and by a jump from 83:8BA5). Exit: M 8-bit, X 8-bit; A = $28;
; X = the offset's low byte; C clear.
%at($838BDE)
TextNumberTwoDigits:
    xba
    tya
    stx.w !WRDIVL
    ldx.b #$0A
    stx.w !WRDIVB
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    rep.b #$11
    tax
    lda.w !RDDIVL
    adc.b #$30
    sta.l !text_tilemap,x
    lda.w !RDMPYL
    clc
    adc.b #$30
    sta.l !text_tilemap+2,x
    lda.b #$28
    sta.l !text_tilemap+1,x
    sta.l !text_tilemap+3,x
    sep.b #$10
    inc.w !text_tilemap_upload
    rts
