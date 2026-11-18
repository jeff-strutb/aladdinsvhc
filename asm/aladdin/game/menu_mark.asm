; menu_mark.asm: the selection mark of a menu drawn in the text layer: taken
; away from the choice left; and the title menu's cursor moved and its
; chosen line blinked. Drafted
; by cdraft.py; named and described by hand.
; C: src/aladdin/game/menu_mark.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!MENU_CHOICE = $0849            ; the title menu's choice (0-2)
!MENU_CHOICE_WAS = $084A        ; the choice before the last move
!MENU_BLINK_HIDDEN = $0847      ; nonzero: the chosen line is blanked
!MENU_BLINK_FRAMES = $0848      ; frames the line has been shown, or frames left blanked
!MENU_LINES = $CE25             ; by choice: its text layout (the DB bank, $81)
!MENU_BLINK_SHOWN = $20         ; frames a line is shown before it blinks off
!PAD_UP = $08                   ; in the pad's high byte
!PAD_DOWN = $04
!TITLE_IDLE_LEAST = $04EC       ; frames the title waits idle, at least, after a move
!SOUND_MENU_MOVE = $0C

; @routine 81:EDA2 TitleMenuCursorLong
; WHAT IT DOES: TitleMenuCursor for a caller in another bank.
; HOW: JSR to it, then RTL.
; Entry: as TitleMenuCursor; called by JSL. Exit: as TitleMenuCursor.
%at($81EDA2)
TitleMenuCursorLong:
    jsr.w TitleMenuCursor
    rtl

; @routine 81:EDA6 TitleMenuCursor
; WHAT IT DOES: Moves the title screen menu's cursor on Up or Down
; (wrapping round the three choices), with its mark, line and sound;
; otherwise blinks the chosen line.
; HOW: Pad 1's newly pressed high byte: neither Up nor Down, TitleMenuBlink
; only. Up, MENU_CHOICE down by 1 (0 wraps to 2); Down, up by 1 (3 wraps
; to 0). Then MenuMarkMove, TitleMenuBlink, the old choice's line drawn
; again (TextLayoutShow; it may have been blinked off), the blink
; restarted, MENU_CHOICE_WAS = the choice, v_0343 (the title's idle
; countdown) raised to at least TITLE_IDLE_LEAST, and sound
; SOUND_MENU_MOVE.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR (81:EDA2). Exit: M
; 8-bit, X 8-bit; as TitleMenuBlink leaves it, or as SoundQueueAdd (A's
; high byte $04).
%at($81EDA6)
TitleMenuCursor:
    lda.w !joy_pressed+1
    bit.b #!PAD_UP
    bne .up
    bit.b #!PAD_DOWN
    bne .down
    jsr.w TitleMenuBlink
    rts
.down:
    inc.w !MENU_CHOICE
    lda.w !MENU_CHOICE
    cmp.b #$03
    bne .moved
    stz.w !MENU_CHOICE
    bra .moved
.up:
    dec.w !MENU_CHOICE
    lda.w !MENU_CHOICE
    bpl .moved
    lda.b #$02
    sta.w !MENU_CHOICE
.moved:
    jsl.l MenuMarkMove
    jsr.w TitleMenuBlink
    ldy.w !MENU_CHOICE_WAS
    lda.w !MENU_LINES,y
    jsl.l TextLayoutShow
    stz.w !MENU_BLINK_HIDDEN
    stz.w !MENU_BLINK_FRAMES
    lda.w !MENU_CHOICE
    sta.w !MENU_CHOICE_WAS
    rep.b #$20
    lda.w #!TITLE_IDLE_LEAST
    cmp.w !v_0343
    bcc .idle_kept
    sta.w !v_0343
.idle_kept:
    sep.b #$20
    lda.b #!SOUND_MENU_MOVE
    jsl.l SoundQueueAdd
    rts

; @routine 81:EE55 TitleMenuBlink
; WHAT IT DOES: Blinks the title menu's chosen line: shown for
; MENU_BLINK_SHOWN frames, then blanked until its count runs out.
; HOW: Shown (MENU_BLINK_HIDDEN zero): MENU_BLINK_FRAMES counted up; at
; MENU_BLINK_SHOWN, MENU_BLINK_HIDDEN set and the line of MENU_CHOICE
; blanked (its layout with bit 7, TextLayoutShow). Blanked:
; MENU_BLINK_FRAMES counted down; at 0, MENU_BLINK_HIDDEN cleared and the
; line of MENU_CHOICE_WAS drawn.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M 8-bit, X
; 8-bit; a line drawn or blanked: as TextLayoutShow leaves it, Y = the
; choice. Else Y = MENU_BLINK_HIDDEN; shown: A = the count, N, Z and C as
; its compare with MENU_BLINK_SHOWN; blanked: A = the count before, N
; and Z from the count after.
%at($81EE55)
TitleMenuBlink:
    lda.w !MENU_BLINK_FRAMES
    ldy.w !MENU_BLINK_HIDDEN
    beq .shown
    dec.w !MENU_BLINK_FRAMES
    bne .done
    stz.w !MENU_BLINK_HIDDEN
    ldy.w !MENU_CHOICE_WAS
    lda.w !MENU_LINES,y
    jsl.l TextLayoutShow
    bra .done
.shown:
    inc a
    sta.w !MENU_BLINK_FRAMES
    cmp.b #!MENU_BLINK_SHOWN
    bne .done
    inc.w !MENU_BLINK_HIDDEN
    ldy.w !MENU_CHOICE
    lda.w !MENU_LINES,y
    ora.b #$80
    jsl.l TextLayoutShow
    bra .done
.done:
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
