; level_end.asm: pieces of the screen at a level's end (81:F7B1): 2x2-tile
; blocks queued for VRAM, the four columns its picture grows in step by
; step. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/game/level_end.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!BLANK_TILE = $080E             ; the tilemap word a blanked block gets
!END_STEPS_DONE = $AD           ; in the direct page: counts the pieces placed
; In the data bank: per column the tile's high byte; per step the VRAM
; word address of column 0 (words); per level (level_035c less 1) four
; picture indices, a byte per column; per index the picture's first tile.
!END_COLUMN_ATTR = $FDF7
!END_STEP_VRAM = $FDC5
!END_LEVEL_PICTURES = $FDD5
!END_PICTURE_TILE = $FDF1

; @routine 81:F973 TileBlockBlank
; WHAT IT DOES: Writes a blank 2x2-tile block record at X in the tile-
; block queue: VRAM address A, its four tilemap words $080E.
; HOW: Five word stores in the 10-byte record; X moves on by 10.
; Entry: M 16-bit, X 16-bit; A = the VRAM word address; X = the record's
; offset in TILE_BLOCKS; called by JSR (by 81:F958, for the 16 blocks of a
; table). Exit: M 16-bit, X 16-bit; X = A = the next record's offset; C
; from the add.
%at($81F973)
TileBlockBlank:
    sta.w !tile_blocks,x
    lda.w #!BLANK_TILE
    sta.w !tile_blocks+2,x
    sta.w !tile_blocks+4,x
    sta.w !tile_blocks+6,x
    sta.w !tile_blocks+8,x
    txa
    clc
    adc.w #$000A
    tax
    rts


; @routine 81:F98C LevelEndColumnTile
; WHAT IT DOES: Gives the tilemap word of the picture piece column $000C
; shows for the level, and counts the piece.
; HOW: END_STEPS_DONE steps up; the column's attribute byte as the high
; byte; the level's picture index for the column (LEVEL_035C less 1, times
; 4, plus the column) picks the picture's first tile, doubled, as the low
; bits.
; Entry: M 8-bit, X 8-bit; $000C = the column (0-3); D the direct page of
; END_STEPS_DONE; called by JSR. Exit: M 16-bit, X 16-bit; A = the tilemap
; word; $0010-$0011 = the attribute word; X = the picture index; Y = its
; offset in the level table.
%at($81F98C)
LevelEndColumnTile:
    inc.b !END_STEPS_DONE
    ldx.w !scratch
    lda.w !END_COLUMN_ATTR,x
    sta.w !scratch+5
    stz.w !scratch+4
    lda.w !level_035c
    dec a
    asl a
    asl a
    adc.w !scratch
    tay
    ldx.w !END_LEVEL_PICTURES,y
    lda.w !END_PICTURE_TILE,x
    rep.b #$30
    and.w #$00FF
    asl a
    ora.w !scratch+4
    rts


; @routine 81:F9B4 LevelEndColumnPlace
; WHAT IT DOES: Gives the VRAM address of column X's piece at step Y of
; the level-end picture: the step's address for column 0, less 6 words a
; column.
; HOW: X times 6 (16-bit, by a double and an add) taken from the step's
; word in END_STEP_VRAM; kept at $000E.
; Entry: M 8-bit; X = the column; Y = the step (0, 2, 4, 6); called by
; JSR. Exit: M 8-bit; $000E = A = the VRAM word address; C from the
; subtraction.
%at($81F9B4)
LevelEndColumnPlace:
    txa
    rep.b #$20
    and.w #$00FF
    asl a
    sta.w !scratch+2
    asl a
    adc.w !scratch+2
    sta.w !scratch+2
    lda.w !END_STEP_VRAM,y
    sec
    sbc.w !scratch+2
    sta.w !scratch+2
    sep.b #$20
    rts
