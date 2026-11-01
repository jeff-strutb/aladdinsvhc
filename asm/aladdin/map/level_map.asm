; level_map.asm: building a level's map in work RAM.
;
; A level's map is made of screens of 32x16 tiles (level_tiles, $200 bytes
; a screen), built from 8x8 blocks of 2x2 tiles, and a screen map that
; places them (screen_map, 32 screens across). The level's map header, at
; $80:E3DC (a word per level, pointing to it in that table), gives the
; screens to build, the banks of their block and tile data, and the layout.
; C: src/aladdin/map/level_map.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/rom.inc"

arch 65816

!map_headers = $80E3DC           ; per level a word, the offset of its header from here
!map_layouts = $80E460           ; layouts: width - 1, height - 1, then the screens (offsets from here)
!screen_map_words = $7FF000      ; the screen map, a word per place (the high byte the screen * 2)
!level_tiles_w = $7F0000
; Direct page (D = 0) and work RAM the build keeps its state in.
!map_tiles_at = $0C              ; where the next screen's tiles go (word)
!map_screens_left = $0E          ; screens still to build
!map_count_a = $10
!map_rows_left = $12             ; block rows left in the screen (word)
!map_cols_left = $14             ; blocks left in the row (word)
!map_layout_at = $10             ; ScreenMapLayout: the screen map place (word)
!map_layout_w = $0C              ; ScreenMapLayout: the width - 1 (columns left)
!map_layout_h = $0D              ; ScreenMapLayout: the height - 1 (rows left)
!map_layout_col = $0E            ; ScreenMapLayout: columns left in this row
!map_block_bank_ptr = $0055      ; long pointer (low $80) to a screen's block list: its bank at $0056
!map_tile_bank_ptr = $0058       ; long pointer (low $80) to the blocks' tiles: its bank at $0059
!map_meta_ptr = $005B            ; long pointer (low $80) to the metatiles: its bank at $005C
!map_block_list = $1A94          ; a screen's 64 blocks (word each), copied for the build
!tm_shadow = $0399               ; TM's shadow: the main screen's layers
!ts_shadow = $039A               ; TS's shadow: the sub screen's layers

; @routine 81:BEEB LevelMapBuildLong
; WHAT IT DOES: LevelMapBuild for callers in other banks.
; HOW: LevelMapBuild.
; Entry: as LevelMapBuild; called by JSL. Exit: as LevelMapBuild.
%at($81BEEB)
LevelMapBuildLong:
    jsr.w LevelMapBuild
    rtl


; @routine 81:BEEF LevelMapBuild
; WHAT IT DOES: Builds the level's map: its screens' tiles and the screen
; map that places them.
; HOW: Clears the screen map; from the level's map header takes the
; screen count, the banks of the block lists, block tiles and metatiles,
; and the main and sub screen layers; for each screen adds its block
; source (BlockSourceAdd), copies its 64-block list and writes each
; block's 2x2 tiles into the screen's 32x16 tiles; then lays out the
; screen map from the level's layouts (ScreenMapLayout), a second layout
; too when the level has one.
; Entry: M 8-bit; D = 0; DB any; called by JSR. Exit: P and DB as at
; entry (pulled); X, Y and A as the layouts left them (16-bit X, Y).
%at($81BEEF)
LevelMapBuild:
    php
    phb
    rep.b #$30
    ldx.w #$07FE
    lda.w #$0000
.lBEF9:
    sta.l !anim_set_of_type,x
    dex
    dex
    bpl .lBEF9
    stz.b !scratch
    stz.b !map_tiles_at+10
    lda.w !level
    asl a
    tax
    phx
    lda.l !map_headers,x
    tax
    sep.b #$20
    lda.l !map_headers,x
    sta.b !map_screens_left
    lda.l !map_headers+1,x
    sta.w !map_block_bank_ptr+1
    lda.b #$80
    sta.w !map_block_bank_ptr
    lda.l !map_headers+2,x
    sta.w !map_tile_bank_ptr+1
    lda.b #$80
    sta.w !map_tile_bank_ptr
    lda.l !map_headers+3,x
    sta.w !level_metatile_bank
    lda.b #$80
    sta.w !map_meta_ptr
    lda.l !map_headers+4,x
    sta.w !tm_shadow
    sta.w !ts_shadow
.lBF46:
    lda.l !map_headers+5,x
    inx
    phx
    xba
    lda.b #$00
    rep.b #$20
    lsr a
    tay
    jsr.w BlockSourceAdd
    sep.b #$20
    lda.w !map_block_bank_ptr+1
    pha
    plb
    ldx.w #$0000
    lda.b #$84
    sta.b !map_count_a
.lBF64:
    lda.w 32768,y
    sta.w !map_block_list,x
    iny
    inx
    dec.b !map_count_a
    bne .lBF64
    lda.w !map_tile_bank_ptr+1
    pha
    plb
    rep.b #$20
    ldx.b !scratch
    ldy.w #$0000
    lda.w #$0008
    sta.b !map_rows_left
.lBF81:
    lda.w #$0008
    sta.b !map_cols_left
.lBF86:
    lda.w !map_block_list,y
    iny
    iny
    phy
    tay
    lda.w 32768,y
    sta.l !level_tiles_w,x
    lda.w 32770,y
    sta.l !level_tiles_w+2,x
    lda.w 32772,y
    sta.l !level_tiles_w+32,x
    lda.w 32774,y
    sta.l !level_tiles_w+34,x
    inx
    inx
    inx
    inx
    ply
    dec.b !map_cols_left
    bne .lBF86
    clc
    txa
    adc.w #$0020
    tax
    dec.b !map_rows_left
    bne .lBF81
    stx.b !scratch
    plx
    sep.b #$20
    dec.b !map_screens_left
    bne .lBF46
    lda.b #$7F
    pha
    plb
    rep.b #$20
    plx
    lda.l !map_layouts,x
    tax
    phx
    lda.l !map_layouts,x
    tax
    ldy.w #$0042
    jsr.w ScreenMapLayout
    plx
    lda.l !map_layouts+2,x
    beq .lBFEB
    tax
    ldy.w #$0442
    jsr.w ScreenMapLayout
.lBFEB:
    plb
    plp
    rts


; @routine 81:BFEE ScreenMapLayout
; WHAT IT DOES: Writes a layout into the screen map: which screen goes in
; each place.
; HOW: The layout's width and height (each less one), then its screen
; numbers row by row; each number * 2 goes to the high byte of the place's
; word (Y on, a row 64 bytes on).
; Entry: M 16-bit, X 16-bit; X = the layout's offset in the layouts; Y =
; the first place's byte offset; DB = $7F; D = 0; called by JSR. Exit: P
; as at entry (pulled); X past the layout; Y = the last row's start + $40.
%at($81BFEE)
ScreenMapLayout:
    php
    sep.b #$20
    sty.b !map_count_a
    lda.l !map_layouts,x
    sta.b !scratch
    lda.l !map_layouts+1,x
    sta.b !map_layout_h
.lBFFF:
    lda.b !scratch
    sta.b !map_screens_left
.lC003:
    lda.l !map_layouts+2,x
    inx
    asl a
    sta.w !screen_map_words+1,y
    iny
    iny
    dec.b !map_screens_left
    bpl .lC003
    rep.b #$20
    clc
    lda.b !map_count_a
    adc.w #$0040
    sta.b !map_count_a
    tay
    sep.b #$20
    dec.b !map_layout_h
    bpl .lBFFF
    plp
    rts
