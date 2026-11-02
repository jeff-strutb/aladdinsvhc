; menu_mark_draw.asm: the selection mark of a menu drawn at a choice in the
; text layer (MenuMarkErase, menu_mark.c, takes it away). Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/game/menu_mark_draw.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816


; @routine 81:EF39 MenuMarkDraw
; WHAT IT DOES: Draws the selection mark at a menu choice: its seven tiles
; in the text layer's tilemap (one in row 0, three in rows 1 and 2).
; HOW: With 16-bit A, tile words at the choice's offset: +2 $207E; +$40,
; +$42, +$44 $2802, $2003, $2004; +$80, +$82, +$84 $2812, $2013, $2014; P
; kept.
; Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
; (from the menu's cursor move, 81:EE1A). Exit: P as at entry; A = $2014.
%at($81EF39)
MenuMarkDraw:
    php
    rep.b #$20
    lda.w #$207E
    sta.l !text_tilemap+2,x
    lda.w #$2802
    sta.l !text_tilemap+64,x
    lda.w #$2003
    sta.l !text_tilemap+66,x
    lda.w #$2004
    sta.l !text_tilemap+68,x
    lda.w #$2812
    sta.l !text_tilemap+128,x
    lda.w #$2013
    sta.l !text_tilemap+130,x
    lda.w #$2014
    sta.l !text_tilemap+132,x
    plp
    rtl
