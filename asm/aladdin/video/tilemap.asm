; tilemap.asm: the text layer's tilemap buffer and plain VRAM fills.
;
; The text layer's 32x32 tilemap is kept in work RAM at $7E:D800 (2 KB);
; setting the upload request ($0835) has the NMI copy it to VRAM.
; C: src/aladdin/video/tilemap.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; The text layer's blank tile, and the same in front of the other layers.
!TEXT_BLANK = $0020
!TEXT_BLANK_FRONT = $2020

; @routine 81:95B8 TextLayerFill
; WHAT IT DOES: Fills the whole text layer's tilemap buffer with one
; tilemap entry.
; HOW: Stores A as a word in each of the buffer's 1,024 entries, last to
; first.
; Entry: A = the entry (16 bits; M either width). Exit: X = $FFFE, or $FE
; with X 8-bit; A, Y and the flags as at entry (P pulled back); the buffer
; filled.
%at($8195B8)
TextLayerFill:
    php
    rep.b #$30
    ldx.w #2*1023
.entry:
    sta.l !text_tilemap,x
    dex
    dex
    bpl .entry
    plp
    rtl

; @routine 81:95C8 TextLayerClear
; WHAT IT DOES: Blanks the text layer and has it uploaded to VRAM.
; HOW: TextLayerFill with entry $0020 (tile $20, the blank), then sets the
; upload request.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A = $0020; X = $FE (X 8-bit) or $FFFE; N, Z from the request's
; increment; Y, C, V unchanged.
TextLayerClear:
    lda.b #!TEXT_BLANK>>8
    xba
    lda.b #!TEXT_BLANK&255
    jsl.l TextLayerFill
    inc.w !text_tilemap_upload
    rtl

; @routine 81:95F5 VramFillMap
; WHAT IT DOES: Fills a 32x32 tilemap in VRAM with one entry.
; HOW: Word access stepping after the high byte, address X, then Y written
; 1,024 times.
; Entry: M 8-bit, X 16-bit; X = the VRAM word address; Y = the entry; DB a
; bank whose $2115-$2119 are the PPU's. Exit: A low byte = $80; X = 0;
; N clear, Z set; Y, C, V unchanged.
%at($8195F5)
VramFillMap:
    lda.b #$80
    sta.w !VMAIN
    stx.w !VMADDL
    ldx.w #1024
.entry:
    sty.w !VMDATAL
    dex
    bne .entry
    rtl

; @routine 81:9607 TextLayerClearFront
; WHAT IT DOES: Blanks the text layer with blank tiles in front of the
; other layers, and has it uploaded to VRAM.
; HOW: Stores $2020 (tile $20, priority set) in each of the buffer's 1,024
; entries, first to last, counting in $000C; then sets the upload request.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M and X 8-bit; A = $2020; X = $0800; Y unchanged; $000C = 0; N, Z
; from the request's increment; C, V unchanged.
TextLayerClearFront:
    rep.b #$30
    ldx.w #0
    lda.w #1024
    sta.w !scratch
    lda.w #!TEXT_BLANK_FRONT
.entry:
    sta.l !text_tilemap,x
    inx
    inx
    dec.w !scratch
    bne .entry
    sep.b #$30
    inc.w !text_tilemap_upload
    rtl
