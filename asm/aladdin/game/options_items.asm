; options_items.asm: the options screen's sound output item (stereo or
; monaural), its exit item's empty step, and the cursor's clearing. Left
; and right on an item call its step through the table at $83:8B59.
; C: src/aladdin/game/options_items.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"

arch 65816

!SOUND_OUTPUT = !kept_options+1 ; the second kept option: bit 7 set, stereo
!TEXT_SHOW = $81972D            ; shows text string A of the table at $82:B575
!TEXT_STEREO = $68              ; the string "STEREO"
!TEXT_MONAURAL = $66            ; the string "MONAURAL"
!SOUND_CMD_STEREO = $F7         ; the sound command for stereo; the next, monaural
; In the bank DB names: the cursor's place per item (tilemap byte offsets,
; a word per item).
!CURSOR_PLACES_ITEMS = $CE5D

; @routine 83:8BA8 OptionStereoToggle
; WHAT IT DOES: The sound output item's step (left or right): switches
; between stereo and monaural, seals the kept options and shows it.
; HOW: Bit 7 of the second kept option flipped, KeptOptionsSeal, then on
; into OptionStereoShow.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($838BA8)
OptionStereoToggle:
    lda.w !SOUND_OUTPUT
    eor.b #$80
    sta.w !SOUND_OUTPUT
    jsl.l KeptOptionsSeal

; @routine 83:8BB4 OptionStereoShow
; WHAT IT DOES: Shows the sound output on the options screen, STEREO or
; MONAURAL by bit 7 of the second kept option, and sends the sound driver
; the matching command.
; HOW: Text string $68 (STEREO) or $66 (MONAURAL) through the text routine
; at $81:972D; then sound command $F7 (stereo) or $F8 (monaural) to
; SoundQueueAdd.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
OptionStereoShow:
    lda.b #!TEXT_STEREO
    ldy.w !SOUND_OUTPUT
    bmi .stereo
    lda.b #!TEXT_MONAURAL
.stereo:
    jsl.l !TEXT_SHOW
    lda.b #!SOUND_CMD_STEREO
    ldy.w !SOUND_OUTPUT
    bmi .queue
    inc a
.queue:
    jsl.l SoundQueueAdd
    rts

; @routine 83:8BCE OptionExitStep
; WHAT IT DOES: The exit item's step (left or right): nothing.
; HOW: RTS.
; Entry: any. Exit: as the C states.
OptionExitStep:
    rts

; @routine 83:8C7B OptionCursorClear
; WHAT IT DOES: Clears the options screen's cursor beside every item, and
; asks for the text layer's upload.
; HOW: For each of the four items, last first, the seven tilemap words
; OptionCursorDraw writes at its place are zeroed, with 16-bit registers.
; Entry: M 8-bit, X 8-bit; DB the table's bank, whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($838C7B)
OptionCursorClear:
    ldy.b #$06
    rep.b #$30
.item:
    ldx.w !CURSOR_PLACES_ITEMS,y
    lda.w #$0000
    sta.l !text_tilemap-62,x
    sta.l !text_tilemap,x
    sta.l !text_tilemap+2,x
    sta.l !text_tilemap+4,x
    sta.l !text_tilemap+64,x
    sta.l !text_tilemap+66,x
    sta.l !text_tilemap+68,x
    dey
    dey
    bpl .item
    sep.b #$30
    inc.w !text_tilemap_upload
    rts
