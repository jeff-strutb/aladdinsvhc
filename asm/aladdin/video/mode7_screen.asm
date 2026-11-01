; mode7_screen.asm: putting up a Mode 7 screen.
;
; The screen's graphics are kept in VRAM as 4-bit planar tiles; for Mode 7
; they are read back into the staging buffer at $7F:5000, converted to
; Mode 7's one byte a pixel (each tile's colours taking a palette row from a
; table), and written to VRAM's high bytes; the map is read back, cleared
; and written again (video/mode7.asm).
; C: src/aladdin/video/mode7_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!m7_staging = $5000              ; the staging buffer (data bank $7F): planar tiles read back
!m7_pixels = $7F5800             ; the converted graphics, a byte a pixel
!m7_pixels_w = $5800
!m7_tile_rows = $81B407          ; per tile (the high byte of its offset / 4 + the row base + the row flag), its palette row
; Direct page (D = 0).
!m7_vram_at = $0C                ; VRAM word address (word)
!m7_read_at = $0E                ; VRAM read address (word); its high byte also the row base
!m7_tiles_left = $2C             ; tiles left to convert (word)
!m7_pixels_left = $2E            ; pixels left in a tile row
!m7_rows_left = $2F              ; rows left in a tile
!m7_row_flag = $30               ; ORed into the palette row lookup
!m7_palette = $32                ; the tile's palette row (its colours' high nibble)
!m7_planes = $10                 ; the four bit-planes of the row being converted
!nmi_ppu_mode = $0829            ; which per-frame PPU update the NMI makes (2: Mode 7)
!bgmode_shadow = $039D           ; BGMODE's shadow
!mode7_params = $038D            ; four Mode 7 words the NMI uses, cleared for a new screen
!mode7_centre = $0395            ; Mode 7's centre, x then y (words)

; @routine 81:AF3B Mode7ScreenSetup
; WHAT IT DOES: Puts up a Mode 7 screen from the graphics and map in VRAM,
; with the interrupts off while VRAM is rewritten.
; HOW: Builds the sine tables; NMI off and forced blank; four 2 KB parts of
; the graphics read back and converted (Mode7GraphicsConvert); the map read
; back, cleared and written again; then Mode 7 (BGMODE 7), its centre and
; parameters, the screen as its shadow says, M7SEL $80, the NMI on.
; Entry: M 8-bit, X 8-bit; D = 0; A low byte = the row flag for the
; palette lookup; DB a bank whose $2100-$4200 are the PPU's and CPU's;
; called by JSL. Exit: M and X 8-bit; interrupts on; A low byte = NMITIMEN
; written; N set, Z clear.
%at($81AF3B)
Mode7ScreenSetup:
    sta.b !m7_row_flag
    jsl.l SineTableBuild
    sei
    lda.w !nmitimen_shadow
    and.b #$7F
    sta.w !NMITIMEN
    lda.w !inidisp_shadow
    ora.b #$80
    sta.w !INIDISP
    lda.b #$0C
    sta.b !m7_read_at+1
    stz.b !m7_read_at
    lda.b #$30
    sta.b !m7_vram_at+1
    stz.b !scratch
.lAF5E:
    jsr.w VramWordsToBuffer
    jsr.w Mode7GraphicsConvert
    sec
    lda.b !m7_vram_at+1
    sbc.b #$10
    sta.b !m7_vram_at+1
    sec
    lda.b !m7_read_at+1
    sbc.b #$04
    sta.b !m7_read_at+1
    bcs .lAF5E
    lda.b #$40
    sta.b !m7_read_at+1
    stz.b !m7_read_at
    jsr.w Mode7MapToBuffer
    jsr.w Mode7MapClear
    stz.b !m7_vram_at+1
    stz.b !scratch
    jsr.w Mode7MapFromBuffer
    lda.b #$02
    sta.w !nmi_ppu_mode
    lda.b #$07
    sta.w !bgmode_shadow
    rep.b #$20
    lda.w #$0080
    sta.w !mode7_centre
    lda.w #$0040
    sta.w !mode7_centre+2
    stz.w !mode7_params
    stz.w !mode7_params+2
    stz.w !mode7_params+4
    stz.w !mode7_params+6
    sep.b #$30
    lda.w !inidisp_shadow
    sta.w !INIDISP
    lda.b #$80
    sta.w !M7SEL
    ora.w !nmitimen_shadow
    sta.w !NMITIMEN
    cli
    rtl


; @routine 81:B059 Mode7GraphicsConvert
; WHAT IT DOES: Converts the staged graphics to Mode 7's pixels and writes
; them to VRAM's high bytes.
; HOW: Mode7TilesConvert, then 4,096 bytes to VMDATAH from the VRAM word
; address at $0C, stepping after the high byte.
; Entry: D = 0; DB a bank whose $2115-$2119 are the PPU's; called by JSR.
; Exit: M 8-bit, X 8-bit; X = Y = 0; Z set; A low byte = the last pixel.
%at($81B059)
Mode7GraphicsConvert:
    rep.b #$30
    jsr.w Mode7TilesConvert
    sep.b #$20
    lda.b #$80
    sta.w !VMAIN
    ldx.b !scratch
    stx.w !VMADDL
    ldy.w #$1000
    ldx.w #$0000
.lB070:
    lda.l !m7_pixels,x
    sta.w !VMDATAH
    inx
    dey
    bne .lB070
    sep.b #$10
    rts


; @routine 81:B07E Mode7TilesConvert
; WHAT IT DOES: Converts the 64 staged planar tiles to a byte a pixel.
; HOW: With the data bank at $7F, Mode7TileRowsConvert over 64 tiles from
; the staging buffer's start.
; Entry: D = 0; called by JSR. Exit: M 8-bit, X and Y 16-bit; DB as at
; entry, N and Z from it (PLB).
%at($81B07E)
Mode7TilesConvert:
    rep.b #$30
    lda.w #$0040
    sta.b !m7_tiles_left
    sep.b #$20
    phb
    lda.b #$7F
    pha
    plb
    ldx.w #$0000
    ldy.w #$0000
    jsr.w Mode7TileRowsConvert
    plb
    rts


; @routine 81:B097 Mode7TileRowsConvert
; WHAT IT DOES: Converts staged 4-bit planar tiles into a byte a pixel,
; each tile's colours in its own palette row.
; HOW: Per tile: its palette row from the table at $B407 (indexed by its
; place and the row flag); per row of 8 pixels, the four bit-planes
; shifted out a bit at a time into a 4-bit colour, nonzero colours ORed
; with the palette row, each pixel to $5800 + X.
; Entry: M 16-bit, X 16-bit; D = 0; DB = $7F; X = the first pixel's
; offset; Y = the first tile's offset in the staging buffer; $2C = the
; tiles; called by JSR. Exit: M 16-bit; X, Y past the last tile; $2C =
; 0; Z set.
%at($81B097)
Mode7TileRowsConvert:
    sep.b #$20
    phx
    stx.b !m7_palette
    lda.b #$00
    xba
    lda.b !m7_palette+1
    lsr a
    lsr a
    clc
    adc.b !m7_read_at+1
    ora.b !m7_row_flag
    tax
    lda.l !m7_tile_rows,x
    sta.b !m7_palette
    plx
    lda.b #$08
    sta.b !m7_rows_left
.lB0B4:
    lda.b #$08
    sta.b !m7_pixels_left
    lda.w !m7_staging,y
    sta.b !m7_planes
    lda.w !m7_staging+1,y
    sta.b !m7_planes+1
    lda.w !m7_staging+16,y
    sta.b !m7_planes+2
    lda.w !m7_staging+17,y
    sta.b !m7_planes+3
.lB0CC:
    lda.b #$00
    asl.b !m7_planes+3
    rol a
    asl.b !m7_planes+2
    rol a
    asl.b !m7_planes+1
    rol a
    asl.b !m7_planes
    rol a
    and.b #$0F
    beq .lB0E0
    ora.b !m7_palette
.lB0E0:
    sta.w !m7_pixels_w,x
    inx
    dec.b !m7_pixels_left
    bne .lB0CC
    iny
    iny
    dec.b !m7_rows_left
    bne .lB0B4
    rep.b #$20
    tya
    clc
    adc.w #$0010
    tay
    dec.b !m7_tiles_left
    bne Mode7TileRowsConvert
    rts


; @routine 81:A287 TileBlockQueue
; WHAT IT DOES: Queues a block of tiles for the NMI to write to VRAM: four
; tilemap rows of four tiles at VRAM address A, the tiles numbered from Y.
; HOW: Four queue records of 10 bytes (a VRAM address, then four tilemap
; words: tile, tile + 1, tile + 16, tile + 17), the address stepping a
; row ($20 words) at a time; the queue's length steps by 40 and the queue
; is marked ready.
; Entry: M 16-bit, X 16-bit; A = the VRAM address; Y = the first tile's
; tilemap word; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
; Exit: M and X 8-bit; X = the new length's low byte; scratch $0C = 0; N,
; Z from the ready flag's increment.
%at($81A287)
TileBlockQueue:
    ldx.w !tile_blocks_length
    sta.w !tile_blocks,x
    inc a
    inc a
    sta.w !tile_blocks+10,x
    clc
    adc.w #$003E
    sta.w !tile_blocks+20,x
    inc a
    inc a
    sta.w !tile_blocks+30,x
    tya
    ldy.w #$0004
    sty.w !scratch
.lA2A5:
    sta.w !tile_blocks+2,x
    inc a
    sta.w !tile_blocks+4,x
    clc
    adc.w #$000F
    sta.w !tile_blocks+6,x
    inc a
    sta.w !tile_blocks+8,x
    sec
    sbc.w #$000F
    tay
    txa
    clc
    adc.w #$000A
    tax
    tya
    dec.w !scratch
    bne .lA2A5
    stx.w !tile_blocks_length
    sep.b #$30
    inc.w !tile_blocks_pending
    rtl
