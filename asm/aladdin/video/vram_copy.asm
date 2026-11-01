; vram_copy.asm: copying a block of words to VRAM by VMDATA writes, from an
; entry of a table of copies.
; C: src/aladdin/video/vram_copy.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; In the bank DB names, 7 bytes an entry: the source's long address, the
; VRAM word address, the count of words. A second table follows $103
; bytes on.
!VRAM_COPIES = $F197
!VRAM_COPIES_2 = $0103

; @routine 81:F5DC VramCopy
; WHAT IT DOES: Copies the block of words entry X of the copy table names
; to VRAM.
; HOW: VMAIN steps the address after each word; with 16-bit registers
; and D set to 0, the source's long pointer goes in direct-page $0C-$0E
; and the words through VMDATA.
; Entry: X 16-bit or 8-bit; X = the entry's offset; DB the table's bank,
; whose $0000-$1FFF is work RAM and whose $2100-$21FF are the PPU
; registers; called by JSL. Exit: as the C states.
%at($81F5DC)
VramCopy:
    php
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
.entry:
    lda.w #$0000
    tcd
    lda.w !VRAM_COPIES,x
    sta.w !scratch
    lda.w !VRAM_COPIES+2,x
    sta.w !scratch+2
    lda.w !VRAM_COPIES+3,x
    sta.w !VMADDL
    lda.w !VRAM_COPIES+5,x
    tax
    ldy.w #$0000
.word:
    lda.b [!scratch],y
    sta.w !VMDATAL
    iny
    iny
    dex
    bne .word
    plp
    rtl

; @routine 81:F60D VramCopy2
; WHAT IT DOES: VramCopy for entry X of the second copy table.
; HOW: X plus $103, then as VramCopy.
; Entry: as VramCopy. Exit: as the C states.
VramCopy2:
    php
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    txa
    clc
    adc.w #!VRAM_COPIES_2
    tax
    bra VramCopy_entry
