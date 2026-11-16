; level_select.asm: the level select (a development mode): L and R held
; with a direction step the level part and stage, and the level text shows
; them.
; C: src/aladdin/game/level_select.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

; LEVEL_035C's two bytes as the level select shows them: the part (1-7 on
; screen) and the stage in it.
!LEVEL_PART = !level_035c
!LEVEL_STAGE = !level_035c+1
!PART_LAST = 6                  ; the last part (its text a message)
!PART_STAGES_LAST = $CE2E       ; by part: its last stage (the DB bank, $80)
!PAD_HELD_LR = $30              ; L or R, in the pad's low byte
!PAD_UP = $08                   ; in the pad's high byte
!PAD_DOWN = $04
!PAD_LEFT = $02
!PAD_RIGHT = $01
; The level's number in the text layer: row 16, from column 13: a space,
; the part, '.', the stage, a space (tile words, attribute $20).
!LEVEL_NUMBER_AT = !text_tilemap+1050
!TILE_DIGIT_1 = $2031           ; '1': the part and stage shown from 1
!TILE_DOT = $202E
!TILE_SPACE = $2020
!LEVEL_TEXT_LAST_PART = $5E     ; text layouts: the last part's title
!LEVEL_TEXT_5_4 = $60           ; part 5 stage 3's (shown "6.4") title

; @routine 80:A0B1 LevelNumberText
; WHAT IT DOES: Writes the level's part and stage into the text layer as
; "P.S" (or a title in place of it for the last part, and for part 5
; stage 3), and has the NMI upload it.
; HOW: LEVEL_PART = PART_LAST: layout LEVEL_TEXT_LAST_PART; part and stage
; $05, $03: layout LEVEL_TEXT_5_4 (by TextLayoutShow). Else each one's low
; four bits + TILE_DIGIT_1 at LEVEL_NUMBER_AT + 2 and + 6, TILE_DOT
; between, TILE_SPACE either side; TEXT_TILEMAP_UPLOAD up.
; Entry: M 8-bit; DB = $80; called by JSR. Exit: M 8-bit; a layout: as
; TextLayoutShow leaves it; else A = TILE_SPACE, N and Z from the upload
; count, C and V clear.
%at($80A0B1)
LevelNumberText:
    lda.w !LEVEL_PART
    cmp.b #!PART_LAST
    beq .last_part
    rep.b #$20
    lda.w !LEVEL_PART
    cmp.w #$0305
    beq .part_5_3
    and.w #$000F
    clc
    adc.w #!TILE_DIGIT_1
    sta.l !LEVEL_NUMBER_AT+2
    lda.w !LEVEL_STAGE
    and.w #$000F
    adc.w #!TILE_DIGIT_1
    sta.l !LEVEL_NUMBER_AT+6
    lda.w #!TILE_DOT
    sta.l !LEVEL_NUMBER_AT+4
    lda.w #!TILE_SPACE
    sta.l !LEVEL_NUMBER_AT
    sta.l !LEVEL_NUMBER_AT+8
    sep.b #$20
    inc.w !text_tilemap_upload
    rts
.part_5_3:
    sep.b #$20
    lda.b #!LEVEL_TEXT_5_4
    jsl.l TextLayoutShow
    rts
.last_part:
    lda.b #!LEVEL_TEXT_LAST_PART
    jsl.l TextLayoutShow
    rts


; @routine 80:A05C LevelSelect
; WHAT IT DOES: With L or R held, steps the level's part (Up, Down) or its
; stage (Right, Left) within their limits and shows the new level: C
; clear. C set when neither L nor R is held or no direction is newly
; pressed.
; HOW: Up: the part up by 1 unless PART_LAST, stage 0; Down: the part down
; by 1 unless 0, stage 0 (both may apply). Right: the stage up by 1
; unless the part's last (PART_STAGES_LAST); else Left: down by 1 unless
; 0. A step refused ends there; else LevelNumberText.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M 8-bit, X
; 8-bit; C as above; a step refused: X = the part or stage, N and Z as its
; test; else as LevelNumberText leaves them.
%at($80A05C)
LevelSelect:
    lda.w !joy_held
    and.b #!PAD_HELD_LR
    beq .no_select
    lda.w !joy_pressed+1
    and.b #$0F
    beq .no_select
    bit.b #!PAD_UP
    beq .not_up
    ldx.w !LEVEL_PART
    cpx.b #!PART_LAST
    beq .refused
    inc.w !LEVEL_PART
    stz.w !LEVEL_STAGE
.not_up:
    bit.b #!PAD_DOWN
    beq .not_down
    ldx.w !LEVEL_PART
    beq .refused
    dec.w !LEVEL_PART
    stz.w !LEVEL_STAGE
.not_down:
    bit.b #!PAD_RIGHT
    beq .not_right
    ldx.w !LEVEL_PART
    lda.w !LEVEL_STAGE
    cmp.w !PART_STAGES_LAST,x
    beq .refused
    inc.w !LEVEL_STAGE
    bra .show
.not_right:
    bit.b #!PAD_LEFT
    beq .show
    ldx.w !LEVEL_STAGE
    beq .refused
    dec.w !LEVEL_STAGE
.show:
    jsr.w LevelNumberText
.refused:
    clc
    rts
.no_select:
    sec
    rts
