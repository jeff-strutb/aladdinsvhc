; hud.asm: the play screen's status line in the text layer: the health
; marks, the two counters, a digit and a 2x2 mark. Each is drawn when
; its shown value falls behind the game's (HudRefresh, each frame).
; C: src/aladdin/video/hud.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"

arch 65816

!HEALTH_SHOWN = $0366           ; the health the marks show
!HUD_COUNT_1_SHOWN = $0368      ; the first counter as shown (its value at $0369)
!HUD_COUNT_2_SHOWN = $036A      ; the second counter as shown (its value at $036B)
!HEALTH_MARK_PLACES = $81F58D   ; by health less one: the mark's tilemap offset
!HEALTH_MARK_TILE = $6A         ; the mark's top tile; its bottom is 16 on
!HUD_TILE_BITS = $2C00          ; palette and priority of the status line's tiles
!DIGIT_TILE_0 = $05             ; the counters' digit tiles start here; bottoms 16 on
!HUD_VALUE = $0364              ; the value the status line's digit shows (less 1)
!HUD_VALUE_SHOWN = $0363        ; that value as shown
!HUD_MARK_VALUE = $036E         ; zero: the 2x2 mark is blank
!HUD_MARK_SHOWN = $036D         ; that value as shown
!HEALTH_MARK_GAINED_TILE = $60  ; the mark drawn as health is gained (HEALTH_MARK_TILE as it is lost)
!HUD_COUNT_1 = $0369            ; the first counter's value
!HUD_COUNT_2 = $036B            ; the second counter's value

; @routine 81:F456 HudRefresh
; WHAT IT DOES: Each frame, brings the status line up to the game: the
; 2x2 mark, the two counters, then either one health mark (when the
; health shown differs from the health) or the digit.
; HOW: Each shown value against its value: the mark (HudMarkRefresh),
; counter 1 (HudCount1Show), counter 2 (HudCount2Show) when they differ.
; Health shown equal to the health: on into HudDigitRefresh. Above or
; equal (C set): HudHealthMarkShow of the shown health (one mark lost).
; Below: one mark gained: the shown health up one, and the mark at the
; old shown health's place drawn with tile $60 (its code, HudHealthChanged,
; lies after HudDigitRefresh and goes on inside HudHealthMarkShow, at its
; store of the shown health).
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR (81:82D8, and on from 81:F433). Exit: as
; HudDigitRefresh, or as HudHealthMarkShow with Y = $60 or $6A.
%at($81F456)
HudRefresh:
    lda.w !HUD_MARK_VALUE
    cmp.w !HUD_MARK_SHOWN
    beq .count_1
    jsr.w HudMarkRefresh
.count_1:
    lda.w !HUD_COUNT_1
    cmp.w !HUD_COUNT_1_SHOWN
    beq .count_2
    jsr.w HudCount1Show
.count_2:
    lda.w !HUD_COUNT_2
    cmp.w !HUD_COUNT_2_SHOWN
    beq .health
    jsr.w HudCount2Show
.health:
    lda.w !HEALTH_SHOWN
    cmp.w !health
    bne HudHealthChanged

; @routine 81:F47F HudDigitRefresh
; WHAT IT DOES: Draws the status line's digit for the value at $0364 when
; it differs from the one shown ($0363): the value less 1, 0 for 0 and 9
; at most; a value above 100 is held at 99; a negative value gets a mark
; beside the digit.
; HOW: The value kept as shown; negative: tile $2E at $7E:D908 and its low
; nibble drawn; else the value less 1: negative, 0; below 9, it; 100 or
; more ($65 up), $63 kept in both; else 9. The digit's tile (low nibble
; plus 5) at $7E:D8C6 and its lower half (OR $10) at $7E:D906; the text
; layer's upload set.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: M 8-bit; unchanged: A low = the value, Z set; drawn: A low = 1.
%at($81F47F)
HudDigitRefresh:
    lda.w !HUD_VALUE
    cmp.w !HUD_VALUE_SHOWN
    bne .lF488
    rts
.lF488:
    sta.w !HUD_VALUE_SHOWN
    lda.w !HUD_VALUE_SHOWN
    bmi .lF4A7
    dec a
    bmi .lF4B1
    cmp.b #$09
    bcc .lF4B3
    cmp.b #$64
    bcc .lF4A3
    lda.b #$63
    sta.w !HUD_VALUE
    sta.w !HUD_VALUE_SHOWN
.lF4A3:
    lda.b #$09
    bra .lF4B3
.lF4A7:
    pha
    lda.b #$2E
    sta.l !text_tilemap+264
    pla
    bra .lF4B3
.lF4B1:
    lda.b #$00
.lF4B3:
    clc
    and.b #$0F
    adc.b #$05
    sta.l !text_tilemap+198
    ora.b #$10
    sta.l !text_tilemap+262
    lda.b #$01
    sta.w !text_tilemap_upload
    rts
; HudRefresh's health step: lost, HudHealthMarkShow; gained, the mark at
; the shown health drawn with HEALTH_MARK_GAINED_TILE.
HudHealthChanged:
    bcs HudHealthMarkShow
    ldy.b #!HEALTH_MARK_GAINED_TILE
    tax
    inc a
    bra HudHealthMarkPlace

; @routine 81:F4D0 HudHealthMarkShow
; WHAT IT DOES: Draws health mark A (1 up) in the status line, and keeps
; A less one as the health shown.
; HOW: The mark's place comes from a table by A less one; its top and
; bottom tiles go in the text tilemap a row apart, and the layer's upload
; is asked for.
; Entry: M 8-bit, X 8-bit; A = the mark; DB a bank whose $0000-$1FFF is
; work RAM. Exit: A low = 1, A high = the bottom tile's high byte; X = the
; place; Y = $6A; N and Z clear; V and C from adding the row; M 8-bit.
; The other flags unchanged.
%at($81F4D0)
HudHealthMarkShow:
    ldy.b #!HEALTH_MARK_TILE
    dec a
    tax
HudHealthMarkPlace:
    sta.w !HEALTH_SHOWN
    lda.l !HEALTH_MARK_PLACES,x
    tax
    rep.b #$20
    tya
    ora.w #!HUD_TILE_BITS
    sta.l !text_tilemap+74,x
    clc
    adc.w #$0010
    sta.l !text_tilemap+138,x
    sep.b #$20
    lda.b #$01
    sta.w !text_tilemap_upload
    rts

; @routine 81:F4F6 HudCount1Show
; WHAT IT DOES: Draws the first counter, A as two digits, in the status
; line, and keeps A as the value shown.
; HOW: Each hex digit's top tile is 5 plus the digit, its bottom tile 16
; on, a row below; the layer's upload is asked for.
; Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
; whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
; from the last addition. A high, X, Y and the other flags unchanged.
HudCount1Show:
    sta.w !HUD_COUNT_1_SHOWN
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #!DIGIT_TILE_0
    sta.l !text_tilemap+178
    ora.b #$10
    sta.l !text_tilemap+242
    clc
    pla
    and.b #$0F
    adc.b #!DIGIT_TILE_0
    sta.l !text_tilemap+180
    ora.b #$10
    sta.l !text_tilemap+244
    lda.b #$01
    sta.w !text_tilemap_upload
    rts

; @routine 81:F521 HudCount2Show
; WHAT IT DOES: Draws the second counter, A as two digits, in the status
; line, and keeps A as the value shown.
; HOW: As HudCount1Show, four columns to the right.
; Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
; whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
; from the last addition. A high, X, Y and the other flags unchanged.
HudCount2Show:
    sta.w !HUD_COUNT_2_SHOWN
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #!DIGIT_TILE_0
    sta.l !text_tilemap+186
    ora.b #$10
    sta.l !text_tilemap+250
    clc
    pla
    and.b #$0F
    adc.b #!DIGIT_TILE_0
    sta.l !text_tilemap+188
    ora.b #$10
    sta.l !text_tilemap+252
    lda.b #$01
    sta.w !text_tilemap_upload
    rts

; @routine 81:F54C HudMarkRefresh
; WHAT IT DOES: Draws the status line's 2x2 mark for the value at $036E,
; keeping it as shown ($036D): for 0 its blank, else its picture.
; HOW: The value to $036D; zero: the four blank words ($7E:D8AA, $D8AC,
; $D8EA, $D8EC); else tiles $2462, $2463, $2472, $2473 there. The text
; layer's upload counted up.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSR (81:F45E). Exit: M 8-bit; A = the last tile word.
%at($81F54C)
HudMarkRefresh:
    rep.b #$20
    ldx.w !HUD_MARK_VALUE
    stx.w !HUD_MARK_SHOWN
    bne .lF56B
    lda.w #$2020
    sta.l !text_tilemap+170
    sta.l !text_tilemap+172
    sta.l !text_tilemap+234
    sta.l !text_tilemap+236
    bra .lF587
.lF56B:
    lda.w #$2462
    sta.l !text_tilemap+170
    lda.w #$2463
    sta.l !text_tilemap+172
    lda.w #$2472
    sta.l !text_tilemap+234
    lda.w #$2473
    sta.l !text_tilemap+236
.lF587:
    sep.b #$20
    inc.w !text_tilemap_upload
    rts
