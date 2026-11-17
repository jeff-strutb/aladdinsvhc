; nmi_uploads.asm: uploads the NMI makes on request flags the retail game
; never sets: character sets from $7F:D000 or $7F:D400, a stepped row
; upload from $7F:B000, two fixed blocks, and a palette upload from a
; table.
; C: src/aladdin/video/nmi_uploads.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

; Places not yet named keep their address as their name.
!t_8558 = $8558  ; data bank
!t_855a = $855A  ; data bank
!t_b608 = $B608  ; data bank
!t_b609 = $B609  ; data bank
!t_b60a = $B60A  ; data bank
!t_b6a2 = $B6A2  ; data bank
!t_b6a3 = $B6A3  ; data bank
!t_b6a4 = $B6A4  ; data bank
!t_b6a5 = $B6A5  ; data bank
!t_b6a7 = $B6A7  ; data bank
!t_b6ef = $B6EF  ; data bank
!t_b6f0 = $B6F0  ; data bank
!t_b6f1 = $B6F1  ; data bank
!t_b6f2 = $B6F2  ; data bank
!t_b6f4 = $B6F4  ; data bank
!t_b6f5 = $B6F5  ; data bank
!v_082b = $082B
!v_082c = $082C
!v_082d = $082D
!ROWS_ASK = $082A                ; nonzero: the stepped upload runs
!ROWS_STEP = $082E               ; the stepped upload's step: 0 start, 1 a row
!ROWS_ROW = $082F                ; the next row, two a step; at 0 again the upload is done
!ROWS_TILE_HIGH = $0830          ; ORed into each nonzero nibble
!ROWS_BUFFER = $7FB000           ; the stepped upload's nibbles, a byte per two tiles
!v_0845 = $0845

; @routine 80:851B UploadCharsets
; WHAT IT DOES: When CHARSET_ASK asks, copies one of two character sets
; from $7F:D000 or $7F:D400 to VRAM from 0, 32 rows of $20 bytes.
; HOW: CHARSET_ASK (3 or 6) indexes CHARSET_SOURCES (word, bank); it is
; cleared; DMA channel 0, mode 0 to VMDATAL, 32 transfers of $20 bytes,
; the VRAM address stepping $80.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; Y = 0 after a copy; X = 1; A (16 bits) = $1000.
%at($80851B)
UploadCharsets:
    ldy.w !v_082b
    beq .l855A
    stz.w !v_082b
    stz.w !VMAIN
    stz.w !DMAP0
    lda.b #$18
    sta.w !BBAD0
    rep.b #$20
    lda.w !t_8558,y
    sta.w !A1T0L
    ldx.w !t_855a,y
    stx.w !A1B0
    ldy.b #$20
    ldx.b #$01
    lda.w #$0000
.l8543:
    sta.w !VMADDL
    pha
    lda.w #$0020
    sta.w !DAS0L
    stx.w !MDMAEN
    pla
    clc
    adc.w #$0080
    dey
    bne .l8543
    sep.b #$20
.l855A:
    rts


; @routine 80:8577 UploadStepped
; WHAT IT DOES: When ROWS_ASK asks, runs step ROWS_STEP of a stepped
; upload: 0 starts it at row 0; 1 writes a row of ROWS_BUFFER's nibbles
; to VRAM as tiles' high bytes (ROWS_TILE_HIGH ORed into each nonzero
; one), two rows a call, and ends the upload once the row wraps to 0.
; HOW: ROWS_STEP * 2 indexes ROWS_STEPS (JMP). Step 0: ROWS_ROW 0,
; ROWS_STEP up. Step 1: VMAIN $80; VRAM address ROWS_ROW * $40, X =
; ROWS_ROW * $20 (16-bit); $40 bytes: the low nibble then the high one,
; each a word (VMDATAL 0, VMDATAH the nibble, ORed with ROWS_TILE_HIGH
; when nonzero); ROWS_ROW + 2, at 0 ROWS_ASK cleared; X 8-bit.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: as
; the step: step 0 N and Z from ROWS_STEP; step 1 N, Z, C and V from the
; row's sum, A its low byte, Y 0.
%at($808577)
UploadStepped:
    ldy.w !ROWS_ASK
    beq .done
    lda.w !ROWS_STEP
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .start, .row
.start:
    stz.w !ROWS_ROW
    inc.w !ROWS_STEP
.done:
    rts
.row:
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    lda.w !ROWS_ROW
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    asl a
    sta.w !VMADDL
    sep.b #$20
    ldy.w #$0040
.tiles:
    lda.l !ROWS_BUFFER,x
    pha
    and.b #$0F
    beq .low
    ora.w !ROWS_TILE_HIGH
.low:
    stz.w !VMDATAL
    sta.w !VMDATAH
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    beq .high
    ora.w !ROWS_TILE_HIGH
.high:
    stz.w !VMDATAL
    sta.w !VMDATAH
    inx
    dey
    bne .tiles
    clc
    lda.w !ROWS_ROW
    adc.b #$02
    sta.w !ROWS_ROW
    bne .more
    stz.w !ROWS_ASK
.more:
    sep.b #$10
    rts


; @routine 80:869A UploadBlockA
; WHAT IT DOES: When BLOCK_A_ASK asks, DMAs one of the blocks listed at
; BLOCKS_A to VRAM.
; HOW: BLOCK_A_ASK is the entry's offset (7 bytes: source address and
; bank, size, VRAM address); cleared; VMAIN $80, DMA channel 0, mode 1 to
; VMDATA.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = the entry; A = 1.
%at($80869A)
UploadBlockA:
    ldx.w !v_082c
    beq .l86D8
    stz.w !v_082c
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !DMAP0
    lda.b #$18
    sta.w !BBAD0
    lda.w !t_b6a2,x
    sta.w !A1T0L
    lda.w !t_b6a3,x
    sta.w !A1T0H
    lda.w !t_b6a4,x
    sta.w !A1B0
    rep.b #$20
    lda.w !t_b6a5,x
    sta.w !DAS0L
    lda.w !t_b6a7,x
    sta.w !VMADDL
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.l86D8:
    rts


; @routine 80:86D9 UploadBlockB
; WHAT IT DOES: When BLOCK_B_ASK asks, reads back one of the VRAM blocks
; listed at BLOCKS_B into work RAM.
; HOW: BLOCK_B_ASK is the entry's offset (7 bytes: destination address
; and bank, size, VRAM address); cleared; VMAIN $80, DMA channel 0, mode
; $81 from RDVRAML, after the dummy read of RDVRAMH.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = the entry; A = 1.
%at($8086D9)
UploadBlockB:
    ldx.w !v_082d
    beq .l8720
    stz.w !v_082d
    lda.b #$80
    sta.w !VMAIN
    lda.b #$81
    sta.w !DMAP0
    lda.b #$39
    sta.w !BBAD0
    lda.w !t_b6ef,x
    sta.w !A1T0L
    lda.w !t_b6f0,x
    sta.w !A1T0H
    lda.w !t_b6f1,x
    sta.w !A1B0
    rep.b #$20
    lda.w !t_b6f2,x
    sta.w !DAS0L
    sep.b #$20
    lda.w !t_b6f4,x
    sta.w !VMADDL
    lda.w !t_b6f5,x
    sta.w !VMADDH
    lda.w !RDVRAMH
    lda.b #$01
    sta.w !MDMAEN
.l8720:
    rts


; @routine 80:874F PaletteUploadFrom
; WHAT IT DOES: When PALETTE_UPLOAD asks, DMAs the palettes to CGRAM from
; the source PALETTE_SOURCE picks in PALETTE_SOURCES: 512 bytes, or 256
; for source 6.
; HOW: CGADD 0, PALETTE_UPLOAD cleared; DMA channel 0, mode 0 to CGDATA,
; the source from the table (3 bytes an entry), the size by the entry;
; MDMAEN.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = PALETTE_SOURCE; Y = the size's high byte; A = 1.
%at($80874F)
PaletteUploadFrom:
    lda.w !palette_upload
    beq .l8792
    stz.w !CGADD
    stz.w !palette_upload
    lda.b #$00
    sta.w !DMAP0
    lda.b #$22
    sta.w !BBAD0
    ldx.w !v_0845
    lda.w !t_b608,x
    sta.w !A1T0L
    lda.w !t_b609,x
    sta.w !A1T0H
    lda.w !t_b60a,x
    sta.w !A1B0
    cpx.b #$06
    bne .l8783
    lda.b #$00
    ldy.b #$01
    bra .l8787
.l8783:
    lda.b #$00
    ldy.b #$02
.l8787:
    sta.w !DAS0L
    sty.w !DAS0H
    lda.b #$01
    sta.w !MDMAEN
.l8792:
    rts
