; mode7.asm: setting up a Mode 7 screen: its map and a sine table in work
; RAM.
;
; Mode 7's map is VRAM's low bytes (128x128 tiles), its graphics the high
; bytes. A screen is staged in the 2 KB buffer at $7F:5000: the buffer is
; read back from VRAM and written into the map.
; C: src/aladdin/video/mode7.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

!mode7_buffer = $7F5000         ; staging buffer
; The direct-page words the callers pass VRAM addresses in.
!mode7_map_at = $0C             ; where in the map the buffer goes
!mode7_read_at = $0E            ; where VRAM is read from
; A quarter turn of sine, 17 words from 0 to $100 (1.0).
!sine_quarter = $81B3E5
; The sine table: 64 words a turn, then the cosine table, in bank $7F.
!sine_table = $4F00
!cosine_table = $4F80
!WORK_BANK_HIGH = $7F

; @routine 81:AFC0 Mode7MapClear
; WHAT IT DOES: Clears the whole Mode 7 map to tile 0.
; HOW: Steps VRAM after each low byte from address 0 and writes 0 to the
; low byte 16,384 times.
; Entry: M 8-bit, X 8-bit; DB a bank whose $2115-$2119 are the PPU's;
; called by JSR. Exit: A low byte = 0; X = Y = 0; Z set, N clear; C, V
; unchanged.
%at($81AFC0)
Mode7MapClear:
    lda.b #0
    sta.w !VMAIN
    rep.b #$10
    ldx.w #0
    stx.w !VMADDL
    ldy.w #16384
    ldx.w #0
    lda.b #0
.byte:
    sta.w !VMDATAL
    inx
    dey
    bne .byte
    sep.b #$10
    rts

; @routine 81:AFDF Mode7MapFromBuffer
; WHAT IT DOES: Writes the 32x32 tiles in the staging buffer into the Mode
; 7 map from the address the caller gives.
; HOW: Steps VRAM after each low byte and writes the buffer's 1,024 bytes
; to the low bytes; after every 32 the address moves down a map row
; (+$80, with the carry as it stands) and is set again.
; Entry: M 8-bit, X 8-bit; D's $0C (word) = the map address; DB a bank
; whose $2115-$2119 are the PPU's; called by JSR. Exit: $0C = the address
; after the last row; A = it (16 bits); X = Y = 0; Z set, N clear; C, V
; from its last ADC.
Mode7MapFromBuffer:
    lda.b #0
    sta.w !VMAIN
    rep.b #$10
    ldx.b !mode7_map_at
    stx.w !VMADDL
    ldy.w #1024
    ldx.w #0
.byte:
    lda.l !mode7_buffer,x
    sta.w !VMDATAL
    inx
    txa
    and.b #31
    bne .next
    rep.b #$20
    lda.b !mode7_map_at
    adc.w #128
    sta.b !mode7_map_at
    sta.w !VMADDL
    sep.b #$20
.next:
    dey
    bne .byte
    sep.b #$10
    rts

; @routine 81:B012 Mode7MapToBuffer
; WHAT IT DOES: Reads 2 KB of the Mode 7 map from the address the caller
; gives into the staging buffer.
; HOW: Steps VRAM after each low byte, makes the dummy read the PPU's read
; latch needs, then reads 2,048 low bytes.
; Entry: M 8-bit, X 8-bit; D's $0E (word) = the VRAM address; DB a bank
; whose $2115-$213A are the PPU's; called by JSR. Exit: A low byte = the
; last byte read; X = Y = 0; Z set, N clear; C, V unchanged.
Mode7MapToBuffer:
    lda.b #0
    sta.w !VMAIN
    rep.b #$10
    ldx.b !mode7_read_at
    stx.w !VMADDL
    ldy.w #2048
    ldx.w #0
    lda.w !RDVRAML
.byte:
    lda.w !RDVRAML
    sta.l !mode7_buffer,x
    inx
    dey
    bne .byte
    sep.b #$10
    rts

; @routine 81:B035 VramWordsToBuffer
; WHAT IT DOES: Reads 1,024 words of VRAM from the address the caller
; gives into the staging buffer: Mode 7 graphics, a pixel per high byte.
; HOW: Steps VRAM after each high byte, makes the dummy read (a word read
; of $213A, which reads $213B too), then reads 1,024 words.
; Entry: M 8-bit, X 8-bit; D's $0E (word) = the VRAM address; DB a bank
; whose $2115-$213B are the PPU's; called by JSR. Exit: M and X 8-bit;
; A = the last word read; X = Y = 0; Z set, N clear; C, V unchanged.
VramWordsToBuffer:
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    lda.b !mode7_read_at
    sta.w !VMADDL
    ldy.w #1024
    ldx.w #0
    lda.w !RDVRAMH
.word:
    lda.w !RDVRAML
    sta.l !mode7_buffer,x
    inx
    inx
    dey
    bne .word
    sep.b #$30
    rts

; @routine 81:B120 SineTableBuild
; WHAT IT DOES: Builds the sine and cosine tables (64 steps a turn, $100
; = 1) in bank $7F from a quarter turn in ROM.
; HOW: Each of the quarter's 17 words goes to its four places in the sine
; table and its four in the cosine table, mirrored and negated as the
; quadrant needs.
; Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; X = $22; Y = $FE; A = the
; last value negated (16 bits); DB as at entry, N and Z from it (PLB);
; C, V unchanged.
%at($81B120)
SineTableBuild:
    phb
    lda.b #!WORK_BANK_HIGH
    pha
    plb
    rep.b #$30
    ldx.w #0
    ldy.w #32
.step:
    lda.l !sine_quarter,x
    sta.w !sine_table,x
    sta.w !cosine_table+96,x
    sta.w !sine_table+32,y
    sta.w !cosine_table,y
    eor.w #$FFFF
    inc a
    sta.w !sine_table+64,x
    sta.w !cosine_table+32,x
    sta.w !sine_table+96,y
    sta.w !cosine_table+64,y
    inx
    inx
    dey
    dey
    bpl .step
    sep.b #$30
    plb
    rtl
