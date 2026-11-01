; hud.asm: the play screen's status line in the text layer: the health
; marks and the two counters. Each is drawn when its shown value falls
; behind the game's (the refresh at $81:F43C).
; C: src/aladdin/video/hud.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!HEALTH_SHOWN = $0366           ; the health the marks show
!HUD_COUNT_1_SHOWN = $0368      ; the first counter as shown (its value at $0369)
!HUD_COUNT_2_SHOWN = $036A      ; the second counter as shown (its value at $036B)
!HEALTH_MARK_PLACES = $81F58D   ; by health less one: the mark's tilemap offset
!HEALTH_MARK_TILE = $6A         ; the mark's top tile; its bottom is 16 on
!HUD_TILE_BITS = $2C00          ; palette and priority of the status line's tiles
!DIGIT_TILE_0 = $05             ; the counters' digit tiles start here; bottoms 16 on

; @routine 81:F4D0 HudHealthMarkShow
; WHAT IT DOES: Draws health mark A (1 up) in the status line, and keeps
; A less one as the health shown.
; HOW: The mark's place comes from a table by A less one; its top and
; bottom tiles go in the text tilemap a row apart, and the layer's upload
; is asked for.
; Entry: M 8-bit, X 8-bit; A = the mark; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($81F4D0)
HudHealthMarkShow:
    ldy.b #!HEALTH_MARK_TILE
    dec a
    tax
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
; whose $0000-$1FFF is work RAM. Exit: as the C states.
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
; whose $0000-$1FFF is work RAM. Exit: as the C states.
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
