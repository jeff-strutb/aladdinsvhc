; scroll_layer.asm: keeping each scroll layer's tilemap ahead of its scroll.
;
; A layer's tilemap in VRAM holds a little more than the screen. As the
; layer scrolls, the column or row of tiles about to come into view is
; drawn from the level's map: a map of blocks at $7F:F000, each block 16
; by 16 metatiles, each metatile 2 by 2 tiles. Each layer record keeps four
; edges (left, right, top, bottom), each a place in the tilemap and the
; same place in the map, and steps them a tile at a time as it draws.
; The direct page is the layer's record ($1738 or $188E) throughout.
; C: src/aladdin/video/scroll_layer.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/rom_video.inc"

arch 65816

; An edge: 10 bytes at a layer record's start (left 0, right 10, top 20,
; bottom 30).
!EDGE_TILEMAP = 0               ; tilemap word: column 0-31, row bits 5-9, screen bit 10
!EDGE_MAP = 2                   ; the block's offset in the block map (2 across, $40 down)
!EDGE_BLOCK = 4                 ; the metatile's offset in the block (2 across, $20 down)
!LAYER_SIZE = $0156            ; a layer record's size: the backdrop's follows the play field's
!EDGE_HALF = 8                  ; the tile in the metatile: 2 the right half, 4 the bottom

; @routine 81:C032 LevelScrollLimits
; WHAT IT DOES: Sets how far the play field may scroll, right and down,
; from the level's size.
; HOW: Follows the level table to the level's header, then to its size
; record; its two size bytes, in screens of 256 pixels, become the scroll
; limits X and Y (the byte times 256).
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M and X 8-bit; A = the Y limit; X = the size record's offset; N
; and Z from the Y limit; C from doubling the level number; Y and the
; other flags unchanged.
%at($81C032)
LevelScrollLimits:
    rep.b #$30
    lda.w !level
    asl a
    tax
    lda.l !level_table,x
    tax
    lda.l !level_table,x
    tax
    lda.l !level_table-1,x
    and.w #$FF00
    sta.l !scroll_limit_x
    lda.l !level_table,x
    and.w #$FF00
    sta.l !scroll_limit_y
    sep.b #$30
    rts

; @routine 81:C0B6 LayersSetup
; WHAT IT DOES: Sets both scroll layers up for the level and puts their
; edges where they start.
; HOW: LayerSetup and LayerPlaceEdges for the play field and the
; backdrop. In levels 0, 2, 3, $14 and $15 the backdrop's rows are 68
; tiles long.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: M and X 8-bit; D unchanged, N
; and Z from it; A, X, Y as the last instructions leave them; C from the
; last level compare.
%at($81C0B6)
LayersSetup:
    phd
    rep.b #$20
    lda.w #!layer0
    ldx.b #0
    jsr.w LayerSetup
    lda.w #!layer1
    ldx.b #7
    jsr.w LayerSetup
    lda.w #!layer0
    ldx.b #0
    jsr.w LayerPlaceEdges
    lda.w #!layer1
    ldx.b #0
    jsr.w LayerPlaceEdges
    sep.b #$30
    lda.w !level
    beq .wide
    cmp.b #$15
    beq .wide
    cmp.b #$14
    beq .wide
    cmp.b #$02
    beq .wide
    cmp.b #$03
    bne .done
.wide:
    lda.b #$44
    sta.w !layer1+!LAYER_ROW_LENGTH
.done:
    pld
    rts

; @routine 81:C0F7 LayerSetup
; WHAT IT DOES: Sets a scroll layer up for a level: where its tilemap and
; block map are, how long its columns and rows are, where its metatiles
; come from, and that it draws all four edges.
; HOW: Copies the layer's 7 bytes from the setup table into its record,
; then fixed values: block lists in bank $7F, metatiles at $8000 up in
; the level's metatile bank, edge mask $0F.
; Entry: M 16-bit, X 8-bit; A = the layer's record; X = its offset in the
; setup table (0 or 7); DB = $80. Exit: D = the record; Y = $0F; N and Z
; from it; A, X and the other flags unchanged.
LayerSetup:
    tcd
    ldy.w !layer_setup,x
    sty.b !LAYER_MAP_BASE+1
    ldy.w !layer_setup+1,x
    sty.b !LAYER_TILEMAP_BASE+1
    lda.w !layer_setup+2,x
    sta.b !LAYER_TILEMAP_MASK
    ldy.w !layer_setup+4,x
    sty.b !LAYER_SCREEN_FLIP+1
    ldy.w !layer_setup+5,x
    sty.b !LAYER_COLUMN_LENGTH
    ldy.w !layer_setup+6,x
    sty.b !LAYER_ROW_LENGTH
    ldy.b #$7F
    sty.b !LAYER_BLOCK_PTR+2
    ldy.b #$80
    sty.b !LAYER_METATILE_PTR+1
    ldy.w !level_metatile_bank
    sty.b !LAYER_METATILE_PTR+2
    ldy.b #$0F
    sty.b !LAYER_EDGES
    rts

; @routine 81:C128 LayerPlaceEdges
; WHAT IT DOES: Puts a layer's four edges where they start, from its
; scroll.
; HOW: EdgePlace for the left, right, top and bottom edges.
; Entry: M 16-bit, X 8-bit; A = the layer's record; X = 0; DB = $80.
; Exit: M 16-bit, X 8-bit; D = the record; X = 16; A, Y as the last
; EdgePlace leaves them; N and Z from X.
LayerPlaceEdges:
    rep.b #$30
    tcd
    lda.w #0
    jsr.w EdgePlace
    lda.w #10
    jsr.w EdgePlace
    lda.w #20
    jsr.w EdgePlace
    lda.w #30
    jsr.w EdgePlace
    sep.b #$10
    rts

; @routine 81:C146 EdgePlace
; WHAT IT DOES: Puts one of a layer's edges where it starts, from the
; layer's scroll: just off the screen on its side.
; HOW: The scroll plus the edge's start from the edge place table, then
; EdgePlaceAt.
; Entry: M 16-bit, X 16-bit; A = the edge's offset in the record; X = its
; offset in the edge place table; D = the layer's record; DB = $80.
; Exit: X = the next edge's table offset (4 on); N and Z from it; A, Y
; as EdgePlaceAt leaves them; $001C = the edge's offset.
EdgePlace:
    phx
    sta.w !scratch+16
    clc
    lda.b !LAYER_SCROLL_Y
    adc.w !edge_place+2,x
    tay
    clc
    lda.b !LAYER_SCROLL_X
    adc.w !edge_place,x
    ldx.w !scratch+16
    jsr.w EdgePlaceAt
    plx
    inx
    inx
    inx
    inx
    rts

; @routine 81:C163 EdgePlaceAt
; WHAT IT DOES: Puts a layer's edge at a pixel position: its tilemap word
; and its place in the level's map.
; HOW: TilemapWord, kept to the layer's mask and given its base bits;
; then EdgeMapPlace.
; Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; X = the
; edge's offset in the record; D = the layer's record; DB = $80.
; Exit: as EdgeMapPlace, with N and Z from Y.
EdgePlaceAt:
    phy
    pha
    jsr.w TilemapWord
    and.b !LAYER_TILEMAP_MASK
    ora.b !LAYER_TILEMAP_BASE
    sta.b !EDGE_TILEMAP,x
    pla
    ply
    jmp.w EdgeMapPlace

; @routine 81:C177 TilemapWord
; WHAT IT DOES: Gives the tilemap word, in a layer's 64 by 32 tilemap,
; of the tile at a pixel position.
; HOW: Column from X bits 3-7, row from Y bits 3-7, the second screen
; (bit 10) from X bit 8, over the base $4800.
; Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; DB a bank
; whose $0000-$1FFF is work RAM. Exit: A = the word; N and Z from it; C
; from the last shift; $0014 = the column and row part; X, Y and the
; other flags unchanged.
%at($81C177)
TilemapWord:
    pha
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    sta.w !scratch+8
    tya
    and.w #$00F8
    asl a
    asl a
    ora.w !scratch+8
    sta.w !scratch+8
    pla
    xba
    lsr a
    tya
    xba
    ror a
    ror a
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$0400
    ora.w #$4800
    ora.w !scratch+8
    rts

; @routine 81:C1A2 EdgeMapPlace
; WHAT IT DOES: Puts a layer's edge at a pixel position in the level's
; map: its block, its metatile in the block and its tile in the metatile.
; HOW: The block from the position's high bytes (one block over and one
; down: the map has a border), plus the layer's map base; the metatile
; from bits 4-7; the tile's half from bit 3.
; Entry: M 16-bit, X 16-bit; A = X position; Y = Y position; X = the
; edge's offset in the record; D = the layer's record; DB a bank whose
; $0000-$1FFF is work RAM. Exit: A = the half; $000C, $000E = the
; position; $0010 = the X half; X, Y and P unchanged.
EdgeMapPlace:
    php
    sta.w !scratch
    sty.w !scratch+2
    xba
    inc a
    asl a
    and.w #$00FE
    sta.w !scratch+4
    tya
    clc
    adc.w #$0100
    and.w #$FF00
    lsr a
    lsr a
    clc
    adc.w !scratch+4
    clc
    adc.b !LAYER_MAP_BASE
    sta.b !EDGE_MAP,x
    lda.w !scratch
    lsr a
    lsr a
    lsr a
    and.w #$001E
    sta.w !scratch+4
    lda.w !scratch+2
    asl a
    and.w #$01E0
    ora.w !scratch+4
    sta.b !EDGE_BLOCK,x
    lda.w !scratch
    lsr a
    lsr a
    and.w #$0002
    sta.w !scratch+4
    lda.w !scratch+2
    lsr a
    and.w #$0004
    ora.w !scratch+4
    sta.b !EDGE_HALF,x
    plp
    rts

; @routine 81:C1FA LayersScroll
; WHAT IT DOES: Once a frame, keeps both scroll layers' tilemaps ahead of
; their scroll: queues the columns and rows coming into view, then draws
; them and moves the layers' edges on.
; HOW: For each layer that draws an edge (or has a column to copy):
; LayerQueueColumn and LayerQueueRow. Then each queued column, of a side
; whose edge its layer draws: LayerDrawColumn and LayerStepAcross; then
; each queued row the same way, with LayerDrawRow and LayerStepUpDown.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: P and D as at entry; A, X, Y
; as the last instructions leave them.
%at($81C1FA)
LayersScroll:
    php
    phd
    rep.b #$20
    lda.w #!layer0
    tcd
.queue:
    lda.b !LAYER_EDGES
    beq .next
    jsr.w LayerQueueColumn
    jsr.w LayerQueueRow
.next:
    clc
    tdc
    adc.w #!LAYER_SIZE
    tcd
    cmp.w #!layer1+!LAYER_SIZE
    bne .queue
    sep.b #$20
.columns:
    ldx.w !column_queue_head
    cpx.w !column_queue_tail
    beq .rows
    lda.w !column_queue+1,x
    xba
    lda.w !column_queue,x
    tcd
    ldy.w !column_queue+2,x
    sty.b !LAYER_COLUMN_SIDE
    clc
    txa
    adc.b #4
    and.b #$1F
    sta.w !column_queue_head
    lda.w !side_edge_bit,y
    bit.b !LAYER_EDGES
    beq .next_column
    jsr.w LayerDrawColumn
    jsr.w LayerStepAcross
.next_column:
    bra .columns
.rows:
    ldx.w !row_queue_head
    cpx.w !row_queue_tail
    beq .done
    lda.w !row_queue+1,x
    xba
    lda.w !row_queue,x
    tcd
    ldy.w !row_queue+2,x
    sty.b !LAYER_ROW_SIDE
    clc
    txa
    adc.b #4
    and.b #$1F
    sta.w !row_queue_head
    lda.w !side_edge_bit+2,y
    bit.b !LAYER_EDGES
    beq .next_row
    jsr.w LayerDrawRow
    jsr.w LayerStepUpDown
.next_row:
    bra .rows
.done:
    pld
    plp
    rts

; @routine 81:C276 LayerQueueColumn
; WHAT IT DOES: When the layer has scrolled 6 pixels or more across from
; where its tiles are drawn to, queues the next column of tiles to draw
; (on the side it is scrolling to) and moves the drawn position 8 pixels
; that way.
; HOW: Compares the scroll X with the drawn X. Far enough: the layer
; (its direct page) and the side (0 right, 1 left) go in the column
; queue, a ring of 8, and the drawn X steps by 8.
; Entry: M 16-bit, X 8-bit; D = the layer's record; DB = $80.
; Exit: nothing queued: Y = the side; A = the distance; N, Z, C from
; comparing it with 6; X unchanged. Queued: A = the new drawn X; X = the
; entry's index; Y = the side times 2; N, V, Z, C from the add. The other
; flags unchanged.
%at($81C276)
LayerQueueColumn:
    ldy.b #0
    sec
    lda.b !LAYER_SCROLL_X
    sbc.b !LAYER_DRAWN_X
    bpl .distance
    iny
    eor.w #$FFFF
    inc a
.distance:
    cmp.w #6
    bcc .done
    ldx.w !column_queue_tail
    tdc
    sta.w !column_queue,x
    tya
    sta.w !column_queue+2,x
    clc
    txa
    adc.w #4
    and.w #$001F
    sta.w !column_queue_tail
    tya
    asl a
    tay
    clc
    lda.w !layer_draw_step,y
    adc.b !LAYER_DRAWN_X
    sta.b !LAYER_DRAWN_X
.done:
    rts

; @routine 81:C2AB LayerQueueRow
; WHAT IT DOES: When the layer has scrolled 6 pixels or more up or down
; from where its tiles are drawn to, queues the next row of tiles to draw
; (on the side it is scrolling to) and moves the drawn position 8 pixels
; that way.
; HOW: Compares the scroll Y with the drawn Y. Far enough: the layer
; (its direct page) and the side (0 down, 1 up) go in the row queue, a
; ring of 8, and the drawn Y steps by 8.
; Entry: M 16-bit, X 8-bit; D = the layer's record; DB = $80.
; Exit: nothing queued: Y = the side; A = the distance; N, Z, C from
; comparing it with 6; X unchanged. Queued: A = the new drawn Y; X = the
; entry's index; Y = the side times 2; N, V, Z, C from the add. The other
; flags unchanged.
LayerQueueRow:
    ldy.b #0
    sec
    lda.b !LAYER_SCROLL_Y
    sbc.b !LAYER_DRAWN_Y
    bpl .distance
    iny
    eor.w #$FFFF
    inc a
.distance:
    cmp.w #6
    bcc .done
    ldx.w !row_queue_tail
    tdc
    sta.w !row_queue,x
    tya
    sta.w !row_queue+2,x
    clc
    txa
    adc.w #4
    and.w #$001F
    sta.w !row_queue_tail
    tya
    asl a
    tay
    clc
    lda.w !layer_draw_step,y
    adc.b !LAYER_DRAWN_Y
    sta.b !LAYER_DRAWN_Y
.done:
    rts

; @routine 81:C2E0 LayerDrawColumn
; WHAT IT DOES: Draws the column of tiles coming into view on a side of a
; layer, from the level's map, ready to be copied to the tilemap.
; HOW: From the side's edge (the right or the left), down the map: each
; block's metatile list, each metatile's tile in the edge's column half,
; the top and bottom halves in turn, into the layer's column buffer from
; the edge's tilemap row on, wrapping at 32, for the layer's column
; length. Then the tilemap word to copy it to and its parts (two when
; longer than 32 tiles).
; Entry: M either, X 8-bit; Y = the side (0 right, 1 left); D = the
; layer's record; DB = $80. Exit: M and X 8-bit; A = the parts; X = the
; edge's offset; Y = the column length; N, Z, C from comparing it with
; 33 (N and Z from the parts when 2); $000C-$0015 as the walk leaves them.
%at($81C2E0)
LayerDrawColumn:
    ldx.w !side_edge,y
    rep.b #$30
    phx
    clc
    lda.b !EDGE_MAP,x
    sta.w !scratch
    lda.b !EDGE_BLOCK,x
    sta.w !scratch+2
    lda.b !EDGE_HALF,x
    sta.w !scratch+6
    lda.b !LAYER_COLUMN_LENGTH
    sta.w !scratch+8
    lda.b !EDGE_TILEMAP,x
    and.w #$FBFF
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    bit.w #$0040
    beq .row
    ora.w #$0020
.row:
    asl a
    and.w #$003F
    tay
    ldx.w !scratch
.block:
    lda.l !block_map,x
    bpl .list
    lda.w #0
.list:
    sta.b !LAYER_BLOCK_PTR
    tyx
    ldy.w !scratch+2
.metatile:
    lda.b [!LAYER_BLOCK_PTR],y
    ora.w #$8000
    sta.b !LAYER_METATILE_PTR
    ldy.w !scratch+6
.tile:
    lda.b [!LAYER_METATILE_PTR],y
    sta.b !LAYER_COLUMN_BUF,x
    dec.w !scratch+8
    beq .drawn
    inx
    inx
    txa
    and.w #$003F
    tax
    tya
    eor.w #$0004
    tay
    bit.w #$0004
    bne .tile
    sta.w !scratch+6
    clc
    lda.w !scratch+2
    adc.w #$0020
    and.w #$01FE
    tay
    sta.w !scratch+2
    cmp.w #$0020
    bcs .metatile
    txy
    lda.w !scratch
    adc.w #$0040
    sta.w !scratch
    tax
    bra .block
.drawn:
    plx
    lda.b !EDGE_TILEMAP,x
    and.w #$041F
    ora.b !LAYER_TILEMAP_BASE
    sta.b !LAYER_COLUMN_DEST
    sep.b #$30
    lda.b #1
    ldy.b !LAYER_COLUMN_LENGTH
    cpy.b #33
    bcc .parts
    inc a
.parts:
    sta.b !LAYER_COLUMN_PARTS
    rts

; @routine 81:C383 LayerDrawRow
; WHAT IT DOES: Draws the row of tiles coming into view on a side of a
; layer, from the level's map, ready to be copied to the tilemap.
; HOW: From the side's edge (the bottom or the top), across the map: each
; block's metatile list, each metatile's tile in the edge's row half, the
; left and right halves in turn, into the layer's row buffer from the
; edge's tilemap column on (both screens), wrapping at 64, for the
; layer's row length. Then the tilemap word to copy it to, and 2 parts.
; Entry: M 8-bit, X 8-bit; Y = the side (0 bottom, 1 top); D = the
; layer's record; DB = $80. Exit: M and X 8-bit; A = 2; N and Z from
; it; X = the edge's offset; Y and C as the walk leaves them;
; $000C-$0015 as the walk leaves them.
LayerDrawRow:
    ldx.w !side_edge+2,y
    rep.b #$30
    phx
    clc
    lda.b !EDGE_MAP,x
    sta.w !scratch
    lda.b !EDGE_BLOCK,x
    sta.w !scratch+2
    lda.b !EDGE_HALF,x
    sta.w !scratch+6
    ldy.b !LAYER_ROW_LENGTH
    sty.w !scratch+8
    lda.b !EDGE_TILEMAP,x
    and.w #$041F
    bit.w #$0400
    beq .column
    ora.w #$0020
.column:
    and.w #$007F
    asl a
    tay
    ldx.w !scratch
.block:
    lda.l !block_map,x
    bpl .list
    lda.w #0
.list:
    sta.b !LAYER_BLOCK_PTR
    tyx
    ldy.w !scratch+2
.metatile:
    lda.b [!LAYER_BLOCK_PTR],y
    ora.w #$8000
    sta.b !LAYER_METATILE_PTR
    ldy.w !scratch+6
.tile:
    lda.b [!LAYER_METATILE_PTR],y
    sta.b !LAYER_ROW_BUF,x
    dec.w !scratch+8
    beq .drawn
    inx
    inx
    txa
    and.w #$007F
    tax
    tya
    eor.w #$0002
    tay
    bit.w #$0002
    bne .tile
    sta.w !scratch+6
    inc.w !scratch+2
    inc.w !scratch+2
    lda.w !scratch+2
    tay
    bit.w #$001E
    bne .metatile
    sec
    sbc.w #$0020
    sta.w !scratch+2
    txy
    inc.w !scratch
    inc.w !scratch
    ldx.w !scratch
    bra .block
.drawn:
    plx
    lda.b !EDGE_TILEMAP,x
    and.w #$0BE0
    and.b !LAYER_TILEMAP_MASK
    ora.b !LAYER_TILEMAP_BASE
    sta.b !LAYER_ROW_DEST
    sep.b #$30
    lda.b #2
    sta.b !LAYER_ROW_PARTS
    rts

; @routine 81:C41F LayerStepAcross
; WHAT IT DOES: After a column is drawn, moves the layer's four edges a
; tile the way the layer scrolled: right or left.
; HOW: By the column's side (0 right): EdgeStepRightIfDrawn or
; EdgeStepLeft for the left, right, top and bottom edges in turn.
; Entry: M 8-bit, X 8-bit; D = the layer's record. Exit: as the last
; step leaves them (X = 30, the bottom edge's offset).
%at($81C41F)
LayerStepAcross:
    ldx.b !LAYER_COLUMN_SIDE
    bne .left
    lda.b #2
    ldx.b #0
    jsr.w EdgeStepRightIfDrawn
    lda.b #1
    ldx.b #10
    jsr.w EdgeStepRightIfDrawn
    lda.b #8
    ldx.b #20
    jsr.w EdgeStepRightIfDrawn
    lda.b #4
    ldx.b #30
    jmp.w EdgeStepRightIfDrawn
.left:
    lda.b #2
    ldx.b #0
    jsr.w EdgeStepLeft
    lda.b #1
    ldx.b #10
    jsr.w EdgeStepLeft
    lda.b #8
    ldx.b #20
    jsr.w EdgeStepLeft
    lda.b #4
    ldx.b #30
    jmp.w EdgeStepLeft

; @routine 81:C45C LayerStepUpDown
; WHAT IT DOES: After a row is drawn, moves the layer's four edges a tile
; the way the layer scrolled: down or up.
; HOW: By the row's side (0 down): EdgeStepDown or EdgeStepUp for the
; left, right, top and bottom edges in turn.
; Entry: M 8-bit, X 8-bit; D = the layer's record. Exit: as the last
; step leaves them (X = 30, the bottom edge's offset).
%at($81C45C)
LayerStepUpDown:
    ldx.b !LAYER_ROW_SIDE
    bne .up
    lda.b #2
    ldx.b #0
    jsr.w EdgeStepDown
    lda.b #1
    ldx.b #10
    jsr.w EdgeStepDown
    lda.b #8
    ldx.b #20
    jsr.w EdgeStepDown
    lda.b #4
    ldx.b #30
    jmp.w EdgeStepDown
.up:
    lda.b #2
    ldx.b #0
    jsr.w EdgeStepUp
    lda.b #1
    ldx.b #10
    jsr.w EdgeStepUp
    lda.b #8
    ldx.b #20
    jsr.w EdgeStepUp
    lda.b #4
    ldx.b #30
    jmp.w EdgeStepUp

; @routine 81:C498 EdgeStepLeft
; WHAT IT DOES: Moves a layer's edge one tile left, when the layer draws
; that edge.
; HOW: When A's bit is set in the layer's edge mask, the tilemap column
; steps back, from 0 to 31 of the other screen; the tile in the metatile
; flips between halves, and from the left half on to the right half of
; the metatile to its left: the block offset steps back, from its first
; column to its last of the block to the left.
; Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
; the record; D = the layer's record. Exit: A, N, Z as the last
; instructions leave them; V from the mask's bit 6; C from comparing
; the tilemap column with 31 when the edge is drawn, else unchanged; X
; and Y unchanged.
%at($81C498)
EdgeStepLeft:
    bit.b !LAYER_EDGES
    beq .done
    dec.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP,x
    and.b #$1F
    cmp.b #$1F
    bne .half
    inc.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP,x
    ora.b #$1F
    sta.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP+1,x
    eor.b #$04
    sta.b !EDGE_TILEMAP+1,x
.half:
    lda.b !EDGE_HALF,x
    eor.b #$02
    sta.b !EDGE_HALF,x
    bit.b #$02
    beq .done
    lda.b !EDGE_BLOCK,x
    bit.b #$1E
    bne .in_block
    ora.b #$1E
    sta.b !EDGE_BLOCK,x
    rep.b #$20
    dec.b !EDGE_MAP,x
    dec.b !EDGE_MAP,x
    sep.b #$20
.done:
    rts
.in_block:
    dec a
    dec a
    sta.b !EDGE_BLOCK,x
    rts

; @routine 81:C4D6 EdgeStepRightIfDrawn
; WHAT IT DOES: Moves a layer's edge one tile right, when the layer draws
; that edge.
; HOW: EdgeStepRight, when A's bit is set in the layer's edge mask.
; Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
; the record; D = the layer's record. Exit: not drawn: N and V from the
; mask, Z set, the rest unchanged; else as EdgeStepRight.
EdgeStepRightIfDrawn:
    bit.b !LAYER_EDGES
    beq EdgeStepRight_done

; @routine 81:C4DA EdgeStepRight
; WHAT IT DOES: Moves a layer's edge one tile right.
; HOW: The tilemap column steps on, from 31 to 0 of the other screen; the
; tile in the metatile flips between halves, and from the right half on
; to the left half of the metatile to its right: the block offset steps
; on, from its last column to its first of the block to the right.
; Entry: M 8-bit, X 8-bit; X = the edge's offset in the record; D = the
; layer's record. Exit: A, Y, N, Z as the last instructions leave them;
; C from comparing the block offset's column bits with $1E when it
; reached the left half, else unchanged; X and V unchanged.
EdgeStepRight:
    inc.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP,x
    and.b #$1F
    bne .half
    dec.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP,x
    and.b #$E0
    sta.b !EDGE_TILEMAP,x
    lda.b !EDGE_TILEMAP+1,x
    eor.b #$04
    sta.b !EDGE_TILEMAP+1,x
.half:
    lda.b !EDGE_HALF,x
    eor.b #$02
    sta.b !EDGE_HALF,x
    bit.b #$02
    bne .done
    lda.b !EDGE_BLOCK,x
    tay
    and.b #$1E
    cmp.b #$1E
    bne .in_block
    tya
    and.b #$E0
    sta.b !EDGE_BLOCK,x
    rep.b #$20
    inc.b !EDGE_MAP,x
    inc.b !EDGE_MAP,x
    sep.b #$20
.done:
    rts
.in_block:
    iny
    iny
    sty.b !EDGE_BLOCK,x
    rts

; @routine 81:C516 EdgeStepUp
; WHAT IT DOES: Moves a layer's edge one tile up, when the layer draws
; that edge.
; HOW: When A's bit is set in the layer's edge mask, the tilemap row
; steps back, from 0 to 31 of the other screen (the layer's screen bits
; flipped); the tile in the metatile flips between halves, and on to the
; metatile above: the block offset steps back a row, from its first to
; its last of the block above.
; Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
; the record; D = the layer's record. Exit: M and X 8-bit; A, Y and the
; flags as the last instructions leave them; X unchanged.
EdgeStepUp:
    bit.b !LAYER_EDGES
    beq .done
    rep.b #$30
    sec
    lda.b !EDGE_TILEMAP,x
    tay
    sbc.w #$0020
    sta.b !EDGE_TILEMAP,x
    and.w #$03E0
    cmp.w #$03E0
    bne .half
    tya
    eor.b !LAYER_SCREEN_FLIP
    ora.w #$03E0
    sta.b !EDGE_TILEMAP,x
.half:
    lda.b !EDGE_HALF,x
    eor.w #$0004
    sta.b !EDGE_HALF,x
    bit.w #$0004
    beq .done
    sec
    lda.b !EDGE_BLOCK,x
    sbc.w #$0020
    and.w #$01FE
    sta.b !EDGE_BLOCK,x
    cmp.w #$01E0
    bcc .done
    sec
    lda.b !EDGE_MAP,x
    sbc.w #$0040
    sta.b !EDGE_MAP,x
.done:
    sep.b #$30
    rts

; @routine 81:C55C EdgeStepDown
; WHAT IT DOES: Moves a layer's edge one tile down, when the layer draws
; that edge.
; HOW: When A's bit is set in the layer's edge mask, the tilemap row
; steps on, from 31 to 0 of the other screen (the layer's screen bits
; flipped); the tile in the metatile flips between halves, and on to the
; metatile below: the block offset steps on a row, from its last to its
; first of the block below.
; Entry: M 8-bit, X 8-bit; A = the edge's bit; X = the edge's offset in
; the record; D = the layer's record. Exit: not drawn: N and V from the
; mask, Z set, the rest unchanged. Else M and X 8-bit; A, Y and the flags
; as the last instructions leave them; X unchanged.
EdgeStepDown:
    bit.b !LAYER_EDGES
    beq .done
    rep.b #$30
    clc
    lda.b !EDGE_TILEMAP,x
    tay
    adc.w #$0020
    sta.b !EDGE_TILEMAP,x
    and.w #$03E0
    bne .half
    tya
    eor.b !LAYER_SCREEN_FLIP
    and.w #$FC1F
    sta.b !EDGE_TILEMAP,x
.half:
    lda.b !EDGE_HALF,x
    eor.w #$0004
    sta.b !EDGE_HALF,x
    bit.w #$0004
    bne .widths
    clc
    lda.b !EDGE_BLOCK,x
    adc.w #$0020
    and.w #$01FE
    sta.b !EDGE_BLOCK,x
    cmp.w #$0020
    bcs .widths
    clc
    lda.b !EDGE_MAP,x
    adc.w #$0040
    sta.b !EDGE_MAP,x
.widths:
    sep.b #$30
.done:
    rts
