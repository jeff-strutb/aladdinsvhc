; oam.asm: the sprite (OAM) image in work RAM.
;
; Each frame the game builds the 544-byte OAM image at $7E:FA80 (128
; sprites of 4 bytes, then the 32-byte high table) and the NMI copies it to
; the PPU. A sprite at y $E0 is off the screen.
; C: src/aladdin/video/oam.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; The frame's high-table accumulator (direct page, D = 0).
!oam_high_bits = $61            ; the bits gathered for the next high-table byte (word)
!oam_high_count = $63           ; sprites left before it is full (word)

!OAM_Y_OFF = $E0                ; a y below the screen

; @routine 81:95D5 OamBufferClear
; WHAT IT DOES: Takes every sprite off the screen.
; HOW: Fills the 512 sprite bytes with $E0 (y below the screen) and the
; high table with 0, last byte first.
; Entry: M 8-bit. Exit: A low byte = 0; X = $FF (X 8-bit) or $FFFF; Y and
; the flags as at entry (P pulled back).
%at($8195D5)
OamBufferClear:
    php
    rep.b #$10
    ldx.w #511
    lda.b #!OAM_Y_OFF
.sprite:
    sta.l !oam_buffer,x
    dex
    bpl .sprite
    ldx.w #31
    lda.b #0
.high:
    sta.l !oam_high,x
    dex
    bpl .high
    plp
    rtl

; @routine 81:9230 OamBuildBegin
; WHAT IT DOES: Starts the frame's sprites: none used, all 128 free, the
; high table cleared.
; HOW: Clears the sprite and high-table offsets and $0857, sets the
; high-table accumulator to empty (8 sprites to go, no bits), clears the
; high table a word at a time and sets 128 free sprites.
; Entry: M 8-bit, X 8-bit; D = 0; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M 8-bit; A = $0080; X = $FE; N, Z clear; Y, C, V unchanged.
%at($819230)
OamBuildBegin:
    rep.b #$20
    stz.w !oam_0857
    stz.w !oam_next
    stz.w !oam_high_next
    lda.w #8
    sta.b !oam_high_count
    stz.b !oam_high_bits
    ldx.b #30
    lda.w #0
.high:
    sta.l !oam_high,x
    dex
    dex
    bpl .high
    lda.w #128
    sta.w !oam_free
    sep.b #$20
    rtl
