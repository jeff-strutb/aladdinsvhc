; tile_changes.asm: changing a tile of the level's map while it is played
; (a door opening, a block broken), in the map and on screen.
; C: src/aladdin/map/tile_changes.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!tile_quarters = $81BA46         ; per quarter (8 bytes apart): its VRAM offset, then its place in the tile (words)
!tile_writes = $0619             ; queued VRAM word writes: address, word (4 bytes each)
!tile_writes_count = $0617       ; how many are queued (word)
!tile_writes_pending = $0616     ; nonzero: the NMI writes the queue
!screen_map_place = $7FF042      ; the screen map from its first place (one row and one screen in)
; Direct page (D = 0).
!change_vram = $32               ; the changed tile's VRAM address (word)
!change_tile = $34               ; the new tile's quarters: a long pointer (word, then bank $36)
!tm_x = $0C                      ; MapTileAddress: x, y on 16-pixel tiles, the place, the screen's tiles
!tm_y = $0E
!tm_place = $10
!tm_screen = $14

; @routine 81:9A64 TileChangeQueueQuarter
; WHAT IT DOES: Queues one 8x8 quarter of a changed map tile for the NMI
; to write to VRAM.
; HOW: The quarter's VRAM offset (from the table at $BA46 + Y) added to
; the tile's address, and its tilemap word (from the new tile's quarters,
; at its place) go to the queue at X; the queue's count steps up.
; Entry: M 16-bit, X 16-bit; D = 0; X = the queue's byte offset; Y = the
; quarter's table offset; $32 = the tile's VRAM address; $34-$36 = the new
; tile's quarters; DB = $81; called by JSR. Exit: X past the record; Y =
; the quarter's place; A = the word queued.
%at($819A64)
TileChangeQueueQuarter:
    clc
    lda.b !change_vram
    adc.w !tile_quarters,y
    sta.w !tile_writes,x
    lda.w !tile_quarters+2,y
    tay
    lda.b [!change_tile],y
    sta.w !tile_writes+2,x
    inc.w !tile_writes_count
    inx
    inx
    inx
    inx
    rts


; @routine 81:9C50 MapTileAddress
; WHAT IT DOES: The place of the map tile under point X, Y in the level's
; tiles.
; HOW: The point rounded to the 16-pixel tile; its screen place (x / 256
; and y / 1024 at 64 a row) gives the screen's tiles from the screen map;
; plus the tile's place in the screen (x / 8 and y * 2).
; Entry: M 16-bit, X 16-bit; D = 0; X = the x; Y = the y; called by JSR.
; Exit: X = the tile's offset in the level's tiles; Y as at entry; $0C,
; $0E = the rounded point; $10 = the place in the screen; $14 = the
; screen's tiles.
%at($819C50)
MapTileAddress:
    txa
    and.w #$FFF0
    sta.b !scratch
    pha
    tya
    and.w #$FFF0
    sta.b !tm_y
    pla
    xba
    asl a
    and.w #$003E
    sta.b !tm_place
    tya
    lsr a
    lsr a
    and.w #$03C0
    ora.b !tm_place
    tax
    lda.l !screen_map_place,x
    sta.b !tm_screen
    pha
    lda.b !scratch
    lsr a
    lsr a
    lsr a
    and.w #$001E
    sta.b !tm_place
    lda.b !tm_y
    asl a
    and.w #$01E0
    ora.b !tm_place
    clc
    adc.b !tm_screen
    tax
    ply
    rts

!tile_changes_count = $0719      ; changes queued (word)
!tile_changes_end = $071B       ; byte offset past the last change queued (word)
!tile_changes = $071D            ; per change, 8 bytes: x, y, new tile (words)
; Direct page (D = 0) for the view box.
!view_w = $1C                    ; the box's width (word)
!view_half_w = $1E
!view_mid_x = $20
!view_h = $22
!view_half_h = $24
!view_mid_y = $26
!view_tmp = $28
!change_x = $2C                  ; the change's x and y (words)
!change_y = $2E
!change_queue_at = $30           ; the VRAM queue's byte offset (word)
!new_tile_step = $5A             ; the new tile's quarters: step, bank (words)
!new_tile_bank = $5C

; @routine 81:999A TileChangesApply
; WHAT IT DOES: Applies the map tile changes queued this frame: each
; change's new tile goes into the level's tiles, and one in view has its
; four quarters queued for the NMI to write to VRAM.
; HOW: With D = 0: the view box (the drawn area with 8 pixels round it);
; for each change, its tile's VRAM address (TilemapWordLong) and place in
; the level's tiles (MapTileAddress); the new tile is stored and stepped
; to its quarters; a change inside the box queues its four quarters
; (TileChangeQueueQuarter). Then the queue empties, and the NMI's write is
; requested when anything was queued.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSL. Exit: M and X 8-bit;
; D as at entry, N and Z from it (PLD); no changes: as at entry, A low
; byte = 0.
%at($819999)
; TileChangesApply's way out with nothing queued (the byte before it).
TileChangesNone:
    rtl
TileChangesApply:
    lda.w !tile_changes_count
    beq TileChangesNone
    rep.b #$30
    phd
    lda.w #$0000
    tcd
    clc
    lda.w !layer0+!LAYER_DRAWN_X
    pha
    adc.w #$FFF8
    sta.b !view_tmp
    pla
    clc
    adc.w #$0118
    sec
    sbc.b !view_tmp
    sta.b !view_w
    lsr a
    sta.b !view_half_w
    clc
    adc.b !view_tmp
    sta.b !view_mid_x
    clc
    lda.w !layer0+!LAYER_DRAWN_Y
    pha
    adc.w #$FFF8
    sta.b !view_tmp
    pla
    clc
    adc.w #$00F0
    sec
    sbc.b !view_tmp
    sta.b !view_h
    lsr a
    sta.b !view_half_h
    clc
    adc.b !view_tmp
    sta.b !view_mid_y
    stz.b !change_queue_at
    stz.w !tile_writes_count
    ldy.w #$0000
.l99E6:
    phy
    lda.w !tile_changes,y
    sta.b !change_x
    lda.w !tile_changes+2,y
    sta.b !change_y
    lda.w !tile_changes+4,y
    sta.b !change_tile
    lda.b !change_x
    pha
    ldy.b !change_y
    phy
    jsl.l TilemapWordLong
    sta.b !change_vram
    ply
    plx
    jsr.w MapTileAddress
    lda.b !change_tile
    sta.l !level_tiles,x
    clc
    adc.b !new_tile_step
    sta.b !change_tile
    lda.b !new_tile_bank
    sta.b !change_tile+2
    sec
    lda.b !change_x
    sbc.b !view_mid_x
    clc
    adc.b !view_half_w
    cmp.b !view_w
    bcs .l9A4A
    sec
    lda.b !change_y
    sbc.b !view_mid_y
    clc
    adc.b !view_half_h
    cmp.b !view_h
    bcs .l9A4A
    ldx.b !change_queue_at
    ldy.w #$0018
    jsr.w TileChangeQueueQuarter
    ldy.w #$0010
    jsr.w TileChangeQueueQuarter
    ldy.w #$0008
    jsr.w TileChangeQueueQuarter
    ldy.w #$0000
    jsr.w TileChangeQueueQuarter
    stx.b !change_queue_at
.l9A4A:
    pla
    clc
    adc.w #$0008
    tay
    dec.w !tile_changes_count
    bne .l99E6
    stz.w !tile_changes_end
    sep.b #$30
    lda.w !tile_writes_count
    beq .l9A62
    inc.w !tile_writes_pending
.l9A62:
    pld
    rtl

; @routine 81:9978 TileChangeAdd
; WHAT IT DOES: Queues a change of the map tile at a point to a new tile,
; for TileChangesApply to make.
; HOW: The record at the queue's end ($071B) gets the point's x and y, each
; with 8 added, and the new tile; the end moves on 8 bytes and the count
; ($0719) steps up.
; Entry: M 16-bit, X 16-bit; A = the new tile; X = the point's x; Y = its
; y; DB = $00-$3F or $80-$BF; called by JSL. Exit: X = the record's
; offset; A = it plus 8; Y unchanged.
%at($819978)
TileChangeAdd:
    phx
    ldx.w !tile_changes_end
    sta.w !tile_changes+4,x
    tya
    ora.w #$0008
    sta.w !tile_changes+2,x
    pla
    ora.w #$0008
    sta.w !tile_changes,x
    clc
    txa
    adc.w #$0008
    sta.w !tile_changes_end
    inc.w !tile_changes_count
    rtl
