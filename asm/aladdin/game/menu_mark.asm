; menu_mark.asm: the selection mark of a menu drawn in the text layer: taken
; away from the choice left; and the title menu's cursor moved. Drafted
; by cdraft.py; named and described by hand.
; C: src/aladdin/game/menu_mark.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!MENU_CHOICE = $0849            ; the title menu's choice (0-2)
!MENU_CHOICE_OLD = $084A        ; the choice before the last move
!v_0847 = $0847
!v_0848 = $0848
!MENU_PICTURES = $CE25          ; in bank $81: a byte per choice for the call at $81:972D
; Not yet filed: the mark's move ($81:EE04), the menu's step ($81:EE55)
; and the call at $81:972D.
!MENU_MARK_MOVE = $81EE04
!MENU_STEP = $81EE55
!MENU_PICTURE_SHOW = $81972D

; @routine 81:EDA6 TitleMenuCursor
; WHAT IT DOES: Moves the title screen menu's cursor on Up or Down
; (wrapping round the three choices), with its mark, picture and sound;
; anything else, the menu's other step.
; HOW: Pad 1's newly pressed high byte: Up, the choice ($0849) down by 1
; (wrapping to 2); Down, up by 1 (wrapping to 0); neither, the step at
; 81:EE55 only. Moved: the mark moved (81:EE04), the step at 81:EE55, the
; call at 81:972D with the old choice's byte from $CE25, $0847-$0848
; cleared, the old choice kept ($084A), $0343 raised to at least $04EC,
; sound $0C.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR (81:EDA2). Exit: M
; 8-bit; as SoundQueueAdd, or as the step at 81:EE55.
%at($81EDA6)
TitleMenuCursor:
    lda.w !joy_pressed+1
    bit.b #$08
    bne .lEDC4
    bit.b #$04
    bne .lEDB5
    jsr.w !MENU_STEP
    rts
.lEDB5:
    inc.w !MENU_CHOICE
    lda.w !MENU_CHOICE
    cmp.b #$03
    bne .lEDD1
    stz.w !MENU_CHOICE
    bra .lEDD1
.lEDC4:
    dec.w !MENU_CHOICE
    lda.w !MENU_CHOICE
    bpl .lEDD1
    lda.b #$02
    sta.w !MENU_CHOICE
.lEDD1:
    jsl.l !MENU_MARK_MOVE
    jsr.w !MENU_STEP
    ldy.w !MENU_CHOICE_OLD
    lda.w !MENU_PICTURES,y
    jsl.l !MENU_PICTURE_SHOW
    stz.w !v_0847
    stz.w !v_0848
    lda.w !MENU_CHOICE
    sta.w !MENU_CHOICE_OLD
    rep.b #$20
    lda.w #$04EC
    cmp.w !v_0343
    bcc .lEDFB
    sta.w !v_0343
.lEDFB:
    sep.b #$20
    lda.b #$0C
    jsl.l SoundQueueAdd
    rts

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
