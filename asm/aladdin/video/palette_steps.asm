; palette_steps.asm: one palette set at a time: from the ROM's table of
; palettes, to white, or back to its kept colours.
; C: src/aladdin/video/palette_steps.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Palettes in bank $87: 16 colours (32 bytes) each.
!rom_palettes = $FF40
!ROM_PALETTE_BANK = $87

; @routine 81:9941 PaletteFromRom
; WHAT IT DOES: Sets palette X (counted from palette 4) to palette A of
; the ROM's table, and has the NMI upload the palettes.
; HOW: With DB = $87: 16 words from $87:FF40 + A * 32 to the palette
; buffer at palette 4 + X; then palette_upload steps up.
; Entry: A low byte = the ROM palette; X low byte = the palette; called by
; JSL. Exit: M 8-bit, X 8-bit; DB as at entry; $0C = 0; X, Y past the
; copy.
%at($819941)
PaletteFromRom:
    phb
    ldy.b #!ROM_PALETTE_BANK
    phy
    plb
    rep.b #$30
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    tay
    txa
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda.w #$0010
    sta.w !scratch
.l9961:
    lda.w !rom_palettes,y
    sta.l !palette_buffer+$80,x
    inx
    inx
    iny
    iny
    dec.w !scratch
    bne .l9961
    sep.b #$30
    inc.w !palette_upload
    plb
    rtl


; @routine 81:9CD7 PaletteWhite
; WHAT IT DOES: Sets palette A to white and has the NMI upload the
; palettes.
; HOW: PaletteOffset, then 16 words of $FFFF into the palette buffer there;
; palette_upload steps up.
; Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
; JSL. Exit: M 8-bit, X 8-bit; A low byte = $FF; Y = $FF.
%at($819CD7)
PaletteWhite:
    rep.b #$30
    jsr.w PaletteOffset
    ldy.w #$000F
    lda.w #$FFFF
.l9CE2:
    sta.l !palette_buffer,x
    inx
    inx
    dey
    bpl .l9CE2
    sep.b #$30
    inc.w !palette_upload
    rtl


; @routine 81:9CF1 PaletteRestoreReversed
; WHAT IT DOES: Sets palette A back to the colours PaletteSaveReversed
; kept, and has the NMI upload the palettes.
; HOW: PaletteOffset; the kept colours (the first 16 words of
; palette_saved, last first) go into the palette buffer there;
; palette_upload steps up.
; Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
; JSL. Exit: M 8-bit, X 8-bit; Y = $FE.
%at($819CF1)
PaletteRestoreReversed:
    rep.b #$30
    jsr.w PaletteOffset
    ldy.w #$001E
.l9CF9:
    lda.w !palette_saved,y
    sta.l !palette_buffer,x
    inx
    inx
    dey
    dey
    bpl .l9CF9
    sep.b #$30
    inc.w !palette_upload
    rtl
