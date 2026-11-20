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
!tm_quarter = $12                ; MapQuarterAt: the quarter's offset in the tile (0, 2, 4, 6)
!level_quarters = $5A            ; the level's tile quarters: a long pointer (word, then bank $5C)
!tc_level_tiles = $7F0000        ; the screens' tiles, a word each
; MapTileSourceAt (D = 0): the long pointers it reads the level's tiles
; through (purposes as these routines use them), its scratch.
!block_tiles = $54               ; per block, where its tiles' words start in block_tile_words (words)
!block_tile_words = $57          ; the level's tile words, as loaded
!ms_place = $10                  ; the tile's place in its block
!ms_block = $12                  ; the block's start, from block_sources
; TileRectQueue: the rectangle (work RAM and the direct page at 0), the view.
!rect_x = $2C                    ; the top left point (words)
!rect_y = $2E
!rect_tiles = $30                ; the next tile's words (word)
!rect_size = $32                 ; columns, then rows (bytes)
!rect_rows = $34                 ; rows left (word)
!rect_columns = $36              ; columns left in the row (word)
!play_view_x = $1773             ; the view's left edge (pixels, word)
!play_view_y = $1777             ; the view's top edge (pixels, word)
; PointOffsetAt (the direct page and the data bank): the point, the side
; bit, and the offsets by X (x, then y at +2; words).
!po_x = $0C
!po_y = $0F
!po_flip = $15                   ; bit 0 set: the x offset is taken off
!point_offsets = $BA62

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


; @routine 81:9BAE TileRectQueue
; WHAT IT DOES: Queues a rectangle of tiles for the screen: each tile in
; view (four tilemap words from the data bank's X) as a record of
; tile_blocks at its VRAM address, the queue then marked ready. Nothing in
; the game calls it.
; HOW: A low byte = the columns, A high byte = the rows; X = the tiles'
; words (8 bytes a tile, row by row); $2C, $2E = the top left point. With
; no room (tile_blocks_length over $78) it returns with C clear. Else,
; with the direct page at 0 (its own kept on the stack): for each row
; (16 pixels down each) and column (16 across), a tile in view (x past
; the view's left edge less 16 and up to 256 past it, y up to 224 below
; its top edge and past it less 16) gets its four words and
; TilemapWordLong's address queued; the source steps 8 bytes for every
; tile; then tile_blocks_pending counted up.
; Entry: M 16-bit, X 16-bit; A = rows * 256 + columns (each nonzero); X =
; the tiles' words; $2C, $2E = the point; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL. Exit: C clear with no room (A = $78 less the
; length, X, D unchanged); else C set; D as at entry, N and Z from it
; (PLD); $0C, $0E past the last tile; $30-$37 used.
%at($819BAE)
TileRectQueue:
    sta.w !rect_size
    xba
    and.w #$00FF
    sta.w !rect_rows
    stx.w !rect_tiles
    sec
    lda.w #$0078
    sbc.w !tile_blocks_length
    bcs .room
    rtl
.room:
    phd
    lda.w #$0000
    tcd
    lda.b !rect_y
    sta.b !tm_y
.row:
    lda.b !rect_size
    and.w #$00FF
    sta.b !rect_columns
    lda.b !rect_x
    sta.b !tm_x
.tile:
    clc
    lda.w !play_view_x
    adc.w #$0100
    sec
    sbc.b !tm_x
    cmp.w #$0110
    bcs .next
    clc
    lda.w !play_view_y
    adc.w #$00E0
    sec
    sbc.b !tm_y
    cmp.w #$00F0
    bcs .next
    ldx.w !tile_blocks_length
    ldy.w !rect_tiles
    lda.w 0,y
    sta.w !tile_blocks+2,x
    lda.w 2,y
    sta.w !tile_blocks+4,x
    lda.w 4,y
    sta.w !tile_blocks+6,x
    lda.w 6,y
    sta.w !tile_blocks+8,x
    ldy.b !tm_y
    lda.b !tm_x
    jsl.l TilemapWordLong
    sta.w !tile_blocks,x
    clc
    txa
    adc.w #$000A
    sta.w !tile_blocks_length
.next:
    clc
    lda.w !rect_tiles
    adc.w #$0008
    sta.w !rect_tiles
    clc
    lda.b !tm_x
    adc.w #$0010
    sta.b !tm_x
    dec.b !rect_columns
    bne .tile
    clc
    lda.b !tm_y
    adc.w #$0010
    sta.b !tm_y
    dec.b !rect_rows
    bne .row
    inc.w !tile_blocks_pending
    pld
    sec
    rtl

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

; @routine 81:9C8D PointOffsetAt
; WHAT IT DOES: A point moved by the offsets at the data bank's $BA62+X:
; Y = the direct page's $0F plus the value at $BA64+X; X = its $0C plus
; the value at $BA62+X, or less it when bit 0 of its $15 is set. Nothing
; in the game calls it.
; HOW: CLC, LDA $0F, ADC $BA64,X, TAY; bit 0 of $15 to C (LSR); LDA $0C,
; then ADC or (SEC) SBC $BA62,X; TAX. Bytes or words as M is set (X and Y
; as X is).
; Entry: any widths; D, DB and X as above; called by JSL. Exit: X, Y as
; above; A = X's value; N, V, Z, C from the last ADC or SBC.
%at($819C8D)
PointOffsetAt:
    clc
    lda.b !po_y
    adc.w !point_offsets+2,x
    tay
    lda.b !po_flip
    lsr a
    lda.b !po_x
    bcs .back
    adc.w !point_offsets,x
    tax
    rtl
.back:
    sec
    sbc.w !point_offsets,x
    tax
    rtl

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
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: M and X 8-bit;
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

; @routine 81:9A84 MapTileAt
; WHAT IT DOES: Reads the level's map tile under point X, Y.
; HOW: With the direct page at 0 (its own kept on the stack),
; MapTileAddress gives the tile's offset; the tile word is read from the
; level's tiles at $7F:0000.
; Entry: M 16-bit, X 16-bit; X = the x; Y = the y; called by JSL. Exit: A
; = the tile word; X = its offset; Y as at entry; D as at entry, N and Z
; from it (PLD); scratch as MapTileAddress leaves it.
%at($819A84)
MapTileAt:
    phd
    lda.w #$0000
    tcd
    jsr.w MapTileAddress
    lda.l !tc_level_tiles,x
    pld
    rtl

; @routine 81:9A92 MapTileRestoreAtOffset
; WHAT IT DOES: Puts back the level's own tile (as loaded) at tile offset
; X of the level's map, for the point in scratch $0C, $0E, and queues it
; for the screen. Nothing in the game calls it.
; HOW: With the direct page at 0 (its own kept on the stack),
; MapTileSourceAt finds the tile's word; then on at
; MapTileRestoreShownTail.
; Entry: M 16-bit, X 16-bit; X = the tile's offset in the level's tiles;
; Y = the y; $0C, $0E = the point (rounded to the tile); called by JSL.
; Exit: as MapTileRestoreShownTail.
%at($819A92)
MapTileRestoreAtOffset:
    phd
    lda.w #$0000
    tcd
    jsr.w MapTileSourceAt
    bra MapTileRestoreShownTail

; @routine 81:9A9C MapTileRestore
; WHAT IT DOES: Puts back the level's own tile (as loaded) under point X,
; Y in the level's map. Nothing in the game calls it.
; HOW: With the direct page at 0 (its own kept on the stack),
; MapTileSourceOf finds the tile and its word, which is written to the
; level's tiles.
; Entry: M 16-bit, X 16-bit; X = the x; Y = the y; called by JSL. Exit: A
; = the tile word; X = its offset; Y = its place in block_tile_words; D as
; at entry, N and Z from it (PLD); scratch $0C-$14 as MapTileSourceOf
; leaves it.
%at($819A9C)
MapTileRestore:
    phd
    lda.w #$0000
    tcd
    jsr.w MapTileSourceOf
    lda.b [!block_tile_words],y
    sta.l !tc_level_tiles,x
    pld
    rtl

; @routine 81:9AAC MapTileRestoreShown
; WHAT IT DOES: Puts back the level's own tile (as loaded) under point X,
; Y in the level's map, and queues it for the screen. Nothing in the game
; calls it.
; HOW: With the direct page at 0 (its own kept on the stack),
; MapTileSourceOf; then on into MapTileRestoreShownTail.
; Entry: M 16-bit, X 16-bit; X = the x; Y = the y; called by JSL. Exit:
; as MapTileRestoreShownTail.
%at($819AAC)
MapTileRestoreShown:
    phd
    lda.w #$0000
    tcd
    jsr.w MapTileSourceOf
; @routine 81:9AB4 MapTileRestoreShownTail
; WHAT IT DOES: The end of MapTileRestoreShown and MapTileRestoreAtOffset:
; the tile's word written to the level's map and queued for the screen.
; HOW: The word at block_tile_words + Y to the level's tiles at X;
; MapTileBlockQueue; the caller's direct page pulled; RTL.
; Entry: M 16-bit, X 16-bit; D = 0, the caller's D pushed above its JSL's
; return; X = the tile's offset; Y = its place in block_tile_words; $0C,
; $0E = the point; reached from MapTileRestoreShown and by a branch from
; MapTileRestoreAtOffset. Exit: A = X = the new length of tile_blocks; D
; as before, N and Z from it (PLD); C clear.
%at($819AB4)
MapTileRestoreShownTail:
    lda.b [!block_tile_words],y
    sta.l !tc_level_tiles,x
    jsr.w MapTileBlockQueue
    pld
    rtl

; @routine 81:9ABF MapTileSourceOf
; WHAT IT DOES: MapTileAddress, then MapTileSourceAt: the tile under point
; X, Y and where the level's own word for it is.
; HOW: Calls MapTileAddress, then on into MapTileSourceAt.
; Entry: M 16-bit, X 16-bit; D = 0; X = the x; Y = the y; called by JSR.
; Exit: as MapTileSourceAt, with X = the tile's offset.
%at($819ABF)
MapTileSourceOf:
    jsr.w MapTileAddress
; @routine 81:9AC2 MapTileSourceAt
; WHAT IT DOES: Where the level's own word (as loaded) for the tile at the
; point in scratch $0C, $0E is, in block_tile_words.
; HOW: The block's start from block_sources (Y's two bytes swapped are the
; index), plus the tile's place in its block (x bits 5-7 and y bits 5-7)
; indexes block_tiles; that word, plus 2 when x bit 4 is set and 4 when y
; bit 4 is, is the place.
; Entry: M 16-bit, X 16-bit; D = 0; Y = the y; $0C, $0E = the point;
; called by JSR. Exit: Y = the place; A = $0010; $10 = the tile's place in
; its block; $12 = the block's start; N, V, Z from the last BIT; C from
; the ADC; X unchanged.
%at($819AC2)
MapTileSourceAt:
    tya
    xba
    tay
    lda.w !block_sources,y
    sta.b !ms_block
    lda.b !tm_x
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000E
    sta.b !ms_place
    lda.b !tm_y
    lsr a
    and.w #$0070
    ora.b !ms_place
    clc
    adc.b !ms_block
    tay
    lda.b [!block_tiles],y
    tay
    lda.w #$0010
    bit.b !tm_x
    beq .x_even
    iny
    iny
.x_even:
    bit.b !tm_y
    beq .done
    iny
    iny
    iny
    iny
.done:
    rts

; @routine 81:9AF6 MapTileBlockQueue
; WHAT IT DOES: Queues tile A for the screen at the point in scratch $0C,
; $0E: its four tilemap words and its VRAM address as a record of
; tile_blocks.
; HOW: The four words from the level's tile quarters at A (level_quarters
; + A, + 2, + 4, + 6) to the record at the queue's end; TilemapWordLong
; for the point gives the address; the queue's length up by 10.
; Entry: M 16-bit, X 16-bit; D = 0; A = the tile word; $0C, $0E = the
; point; DB a bank whose $0000-$1FFF is work RAM; called by JSR. Exit: A =
; X = the new length; Y = the y; C clear.
%at($819AF6)
MapTileBlockQueue:
    tay
    ldx.w !tile_blocks_length
    lda.b [!level_quarters],y
    sta.w !tile_blocks+2,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+4,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+6,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+8,x
    iny
    iny
    ldy.b !tm_y
    lda.b !tm_x
    jsl.l TilemapWordLong
    sta.w !tile_blocks,x
    clc
    txa
    adc.w #$000A
    sta.w !tile_blocks_length
    rts

; @routine 81:9B2A MapTileSetShown
; WHAT IT DOES: Puts tile word A into the level's map under point X, Y,
; as MapTileSet does, and queues it for the screen. Nothing in the game
; calls it.
; HOW: With the direct page at 0 (its own kept on the stack, and A),
; MapTileAddress; A written to the level's tiles; then as MapTileBlockQueue,
; written out in place.
; Entry: M 16-bit, X 16-bit; A = the tile word; X = the x; Y = the y;
; called by JSL. Exit: A = X = the new length of tile_blocks; Y = the y;
; D as at entry, N and Z from it (PLD); C clear.
%at($819B2A)
MapTileSetShown:
    phd
    pha
    lda.w #$0000
    tcd
    jsr.w MapTileAddress
    pla
    sta.l !tc_level_tiles,x
    tay
    ldx.w !tile_blocks_length
    lda.b [!level_quarters],y
    sta.w !tile_blocks+2,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+4,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+6,x
    iny
    iny
    lda.b [!level_quarters],y
    sta.w !tile_blocks+8,x
    iny
    iny
    ldy.b !tm_y
    lda.b !tm_x
    jsl.l TilemapWordLong
    sta.w !tile_blocks,x
    clc
    txa
    adc.w #$000A
    sta.w !tile_blocks_length
    pld
    rtl

; @routine 81:9B6D MapTileSet
; WHAT IT DOES: Puts tile word A into the level's map under point X, Y.
; HOW: With the direct page at 0 (its own kept on the stack, and A),
; MapTileAddress gives the tile's offset; A is written to the level's
; tiles there.
; Entry: M 16-bit, X 16-bit; A = the tile word; X = the x; Y = the y;
; called by JSL. Exit: A as at entry; X = the tile's offset; Y as at entry;
; D as at entry, N and Z from it (PLD); scratch as MapTileAddress leaves
; it.
%at($819B6D)
MapTileSet:
    phd
    pha
    lda.w #$0000
    tcd
    jsr.w MapTileAddress
    pla
    sta.l !tc_level_tiles,x
    pld
    rtl

; @routine 81:9B7D MapQuarterAt
; WHAT IT DOES: Reads the tilemap word of the 8x8 quarter of the map tile
; under the point in scratch $0C, $0E.
; HOW: With the direct page at 0 (its own kept on the stack):
; MapTileAddress; the point is put back; the quarter's offset (x bit 3
; gives 2, y bit 3 gives 4) is kept at $12 and added to the tile word,
; which indexes the level's tile quarters through the long pointer at
; $5A.
; Entry: DB any; $0C = the x; $0E = the y (words); $5A-$5C = the level's
; tile quarters; called by JSL. Exit: A = the tilemap word (16 bits); M
; and X 8-bit; Y = the low byte of its offset; X = the low byte of the
; tile's; $12 = the quarter's offset; D as at entry, N and Z from it
; (PLD).
MapQuarterAt:
    rep.b #$30
    phd
    lda.w #$0000
    tcd
    ldx.b !tm_x
    ldy.b !tm_y
    phy
    phx
    jsr.w MapTileAddress
    pla
    sta.b !tm_x
    and.w #$0008
    lsr a
    lsr a
    sta.b !tm_quarter
    pla
    sta.b !tm_y
    and.w #$0008
    lsr a
    tsb.b !tm_quarter
    clc
    lda.l !tc_level_tiles,x
    adc.b !tm_quarter
    tay
    lda.b [!level_quarters],y
    sep.b #$30
    pld
    rtl
