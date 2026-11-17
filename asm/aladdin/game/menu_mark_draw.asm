; menu_mark_draw.asm: the selection mark of a menu drawn at a choice in the
; text layer (MenuMarkErase, menu_mark.c, takes it away). Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/game/menu_mark_draw.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!MENU_CHOICE = $0849             ; the menu's choice
!MENU_CHOICE_WAS = $084A         ; the choice the mark was drawn at
!MENU_CHOICE_PLACES = $CE28      ; by choice: its tilemap offset (words, the DB bank)
!MENU_CURSOR_PLACES = $CE1F      ; by choice: the one-tile cursor's tilemap offset (words, the DB bank)
!MENU_CURSOR_TILE = $248E        ; the cursor's tile word
!MENU_CURSOR_MOVES = $084B       ; counted up at each move of the cursor
!MENU_CURSOR_BLANK = $0020       ; a blank tile word


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

; @routine 81:EE04 MenuMarkMove
; WHAT IT DOES: Moves a menu's selection mark from the choice it was at
; to the one now chosen.
; HOW: MenuMarkErase at the tilemap offset of choice MENU_CHOICE_WAS
; (from the table MENU_CHOICE_PLACES, by twice the choice), MenuMarkDraw
; at that of MENU_CHOICE, with 16-bit X.
; Entry: M 8-bit, X 8-bit; DB = $81, the table's bank; called by JSL
; (81:EDD1, 80:9F42). Exit: M 8-bit, X 8-bit; P otherwise as MenuMarkDraw
; leaves it; A = $2014; Y = twice the choice.
%at($81EE04)
MenuMarkMove:
    lda.w !MENU_CHOICE_WAS
    asl a
    tay
    rep.b #$10
    ldx.w !MENU_CHOICE_PLACES,y
    jsl.l MenuMarkErase
    lda.w !MENU_CHOICE
    asl a
    tay
    ldx.w !MENU_CHOICE_PLACES,y
    jsl.l MenuMarkDraw
    sep.b #$10
    rtl

; @routine 81:EE21 MenuCursorMoveLong
; WHAT IT DOES: MenuCursorMove for callers in other banks.
; HOW: JSR to it, then RTL. No script runs it: written from its bytes.
; Entry: as MenuCursorMove; called by JSL. Exit: as MenuCursorMove.
MenuCursorMoveLong:
    jsr.w MenuCursorMove
    rtl

; @routine 81:EE25 MenuCursorMove
; WHAT IT DOES: Moves a menu's one-tile cursor from the choice it was at
; to the one now chosen, and counts the move.
; HOW: Tile word $0020 (a blank) at the tilemap offset of choice
; MENU_CHOICE_WAS, MENU_CURSOR_TILE at that of MENU_CHOICE (both from
; MENU_CURSOR_PLACES, by twice the choice; 16-bit A, then X); the upload
; marked, MENU_CURSOR_MOVES counted up. No script runs it: written from
; its bytes.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $CE1F holds MENU_CURSOR_PLACES (bank $80: $0454, $04D4, $0554);
; called by JSR. Exit: M 8-bit, X 8-bit; A low = $8E; X = the new
; choice's offset's low byte; N and Z from MENU_CURSOR_MOVES.
MenuCursorMove:
    lda.w !MENU_CHOICE_WAS
    asl a
    tax
    rep.b #$20
    lda.w !MENU_CURSOR_PLACES,x
    rep.b #$10
    tax
    lda.w #!MENU_CURSOR_BLANK
    sta.l !text_tilemap,x
    lda.w !MENU_CHOICE
    and.w #$00FF
    asl a
    tax
    lda.w !MENU_CURSOR_PLACES,x
    tax
    lda.w #!MENU_CURSOR_TILE
    sta.l !text_tilemap,x
    sep.b #$30
    inc.w !text_tilemap_upload
    inc.w !MENU_CURSOR_MOVES
    rts
