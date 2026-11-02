; menu_mark.asm: the selection mark of a menu drawn in the text layer: taken
; away from the choice left. Drafted by cdraft.py; named and described by
; hand.
; C: src/aladdin/game/menu_mark.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 81:EF15 MenuMarkErase
; WHAT IT DOES: Takes the selection mark away from a menu choice: blanks
; its seven tiles in the text layer's tilemap.
; HOW: Tile word $0020 (a blank) stored at the choice's offset plus 2 (row
; 0), $40, $42, $44 (row 1) and $80, $82, $84 (row 2), with 16-bit A; P
; kept.
; Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
; (from the menu's cursor move, 81:EE04, which draws the mark at the new
; choice by 81:EF39). Exit: P as at entry; A = $0020.
%at($81EF15)
MenuMarkErase:
    php
    rep.b #$20
    lda.w #$0020
    sta.l !text_tilemap+2,x
    sta.l !text_tilemap+64,x
    sta.l !text_tilemap+66,x
    sta.l !text_tilemap+68,x
    sta.l !text_tilemap+128,x
    sta.l !text_tilemap+130,x
    sta.l !text_tilemap+132,x
    plp
    rtl
