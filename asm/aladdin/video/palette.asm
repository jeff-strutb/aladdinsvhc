; palette.asm: loading and keeping palettes.
;
; CGRAM's image is kept at palette_buffer (16 palettes of 16 colours, a
; word each); setting palette_upload has the NMI copy it to CGRAM. Colours
; a fade works from are kept at palette_saved.
; C: src/aladdin/video/palette.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Palette loads in bank $84: 6 bytes each (source offset from palette_data,
; word count, byte offset in palette_buffer), and the colours they copy.
!palette_loads = $84C000
!palette_data = $84C000
!PALETTE_BANK = $84

; @routine 81:98C5 PaletteLoad
; WHAT IT DOES: Loads palette set A into the palette buffer and has it
; uploaded.
; HOW: Load A's entry (A * 6) gives where its colours are, how many and
; where they go; they are copied a word at a time, counting in scratch
; $0C, with the data bank at $84; then the upload request is set.
; Entry: A low byte = the set; DB, D any. Exit: M and X 8-bit; A = the
; last colour copied (16 bits); X, Y = the low bytes of the offsets past
; the last colour; DB as at entry, N and Z from it (PLB); C set, V clear
; (the entry's offset).
%at($8198C5)
PaletteLoad:
    rep.b #$30
    and.w #$00FF
    asl a
    sta.w !scratch
    asl a
    asl a
    sec
    sbc.w !scratch
    tax
    sep.b #$20
    phb
    lda.b #!PALETTE_BANK
    pha
    plb
    rep.b #$20
    lda.w !palette_loads+2,x
    sta.w !scratch
    lda.w !palette_loads,x
    tay
    lda.w !palette_loads+4,x
    tax
.colour:
    lda.w !palette_data,y
    sta.l !palette_buffer,x
    inx
    inx
    iny
    iny
    dec.w !scratch
    bne .colour
    sep.b #$30
    inc.w !palette_upload
    plb
    rtl

; @routine 81:9CA6 PaletteSaveReversed
; WHAT IT DOES: Keeps palette A's 16 colours in the first saved palette,
; last colour first.
; HOW: PaletteOffset, then copies the palette's colours from first to
; last into the saved colours' first palette from its end down.
; Entry: M 8-bit, X 8-bit; A low byte = the palette; DB a bank whose
; $0000-$1FFF is work RAM. Exit: M and X 8-bit; A = the last colour (16
; bits); X = the low byte of the offset past it; Y = $FE; N set, Z clear;
; C clear; V unchanged.
%at($819CA6)
PaletteSaveReversed:
    rep.b #$30
    jsr.w PaletteOffset
    ldy.w #30
.colour:
    lda.l !palette_buffer,x
    sta.w !palette_saved,y
    inx
    inx
    dey
    dey
    bpl .colour
    sep.b #$30
    rtl

; @routine 81:9CBE PaletteOffset
; WHAT IT DOES: The byte offset of palette A in the palette buffer.
; HOW: (A & 15) * 32, into A and X.
; Entry: M 16-bit, X 16-bit; called by JSR. Exit: A = X = the offset;
; N clear, Z from it; C clear; Y, V unchanged.
PaletteOffset:
    and.w #$000F
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    rts

; @routine 81:9DAA PaletteSave
; WHAT IT DOES: Keeps Y palettes from palette A on, for a fade to work
; from.
; HOW: Notes where they start, then copies Y * 16 colours from the
; palette buffer to the same places in the saved colours.
; Entry: M 8-bit, X 8-bit; A low byte = the first palette; Y = how many;
; DB a bank whose $0000-$1FFF is work RAM. Exit: M and X 8-bit; A = the
; last colour (16 bits); X = the low byte of the offset past it; Y = 0;
; Z set, N clear; C = bit 12 of the old Y (its last shift); V unchanged.
%at($819DAA)
PaletteSave:
    rep.b #$30
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    stx.w !palette_saved_at
    tya
    asl a
    asl a
    asl a
    asl a
    tay
.colour:
    lda.l !palette_buffer,x
    sta.w !palette_saved,x
    inx
    inx
    dey
    bne .colour
    sep.b #$30
    rtl
