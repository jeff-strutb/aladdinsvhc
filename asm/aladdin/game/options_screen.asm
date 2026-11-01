; options_screen.asm: the options screen's text: a setting's words and the
; cursor beside the item.
; C: src/aladdin/game/options_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/hw.inc"

arch 65816

; In the bank DB names: for each of 4 settings five tilemap words, one
; table per line (8 bytes apart); the cursor's place per item.
!SETTING_WORDS = $CE35
!CURSOR_PLACES = $CE5D
!OPTION_SETTING = $1B17         ; the setting the options screen shows (0-3)

; @routine 83:8C16 OptionSettingShow
; WHAT IT DOES: Shows a setting on the options screen: five tilemap words
; down one column, chosen by A, and asks for the text layer's upload.
; HOW: A doubled indexes five tables 8 bytes apart; each word goes to its
; line of the text tilemap.
; Entry: M 8-bit, X 8-bit; A = the setting (0-3); DB the tables' bank,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($838C16)
OptionSettingShow:
    asl a
    tay
    rep.b #$20
    lda.w !SETTING_WORDS,y
    sta.l !text_tilemap+426
    lda.w !SETTING_WORDS+8,y
    sta.l !text_tilemap+618
    lda.w !SETTING_WORDS+16,y
    sta.l !text_tilemap+746
    lda.w !SETTING_WORDS+24,y
    sta.l !text_tilemap+874
    lda.w !SETTING_WORDS+32,y
    sta.l !text_tilemap+1002
    sep.b #$20
    inc.w !text_tilemap_upload
    rts

; @routine 83:8C43 OptionCursorDraw
; WHAT IT DOES: Draws the options screen's cursor at item Y and asks for
; the text layer's upload.
; HOW: The item's place (a tilemap byte offset) gets three tiles on each
; of two lines, and one tile on the line above, one column in, with
; 16-bit registers.
; Entry: M 8-bit, X 8-bit; Y = the item's offset; DB the table's bank,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
OptionCursorDraw:
    rep.b #$30
    ldx.w !CURSOR_PLACES,y
    lda.w #$207E
    sta.l !text_tilemap-62,x
    lda.w #$2802
    sta.l !text_tilemap,x
    lda.w #$2003
    sta.l !text_tilemap+2,x
    inc a
    sta.l !text_tilemap+4,x
    lda.w #$2812
    sta.l !text_tilemap+64,x
    lda.w #$2013
    sta.l !text_tilemap+66,x
    inc a
    sta.l !text_tilemap+68,x
    sep.b #$30
    inc.w !text_tilemap_upload
    rts

; @routine 83:8B61 OptionSettingStep
; WHAT IT DOES: Steps the options screen's setting up (C set) or down,
; wrapping within 0-3; shows it, keeps it as the first kept option, and
; seals the kept options.
; HOW: INC or DEC, AND #3, then OptionSettingShow and KeptOptionsSeal.
; Entry: M 8-bit, X 8-bit; C = the direction; DB the setting tables'
; bank, whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($838B61)
OptionSettingStep:
    bcs .up
    dec.w !OPTION_SETTING
    bra .show
.up:
    inc.w !OPTION_SETTING
.show:
    lda.w !OPTION_SETTING
    and.b #$03
    sta.w !OPTION_SETTING
    sta.w !kept_options
    jsr.w OptionSettingShow
    jsl.l KeptOptionsSeal
    rts

!SOUND_TEST = $1B15             ; the sound test's number, less one (0-34)
!SOUND_TEST_AT = $04A8          ; its place in the text tilemap
!DIGIT_TILE = $30               ; '0'
!DIGIT_BITS = $28               ; the digits' attributes
!DIGITS_SHOW = $838BDE          ; the digit code SoundTestShow goes on into (not yet filed)

; @routine 83:8B9D SoundTestShow
; WHAT IT DOES: Shows the sound test's number (1 up) as two decimal
; digits on the options screen.
; HOW: The hardware divider gives tens and units; each digit's tile is
; '0' plus it, with its attribute byte, at the place A:Y names (on into
; the digit code at $83:8BDE); the layer's upload is asked for.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $4200-$421F are the CPU registers. Exit: as the C states.
%at($838B9D)
SoundTestShow:
    ldx.w !SOUND_TEST
    inx
    lda.b #!SOUND_TEST_AT>>8
    ldy.b #!SOUND_TEST_AT&255
    jmp.w !DIGITS_SHOW&65535
