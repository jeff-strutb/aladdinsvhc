; story_card.asm: the story card shown between acts: a screen loaded and a
; text typed out, by a list of commands for each part of the game.
;
; A part's list (STORY_LISTS, in bank $80) is a run of commands, each a
; byte: 0 loads a screen (sets of graphics, VRAM copies and palettes,
; then its song), 2 types a text (its number follows), 4 ends the card
; (a fade, then the level starts), 6 fades and goes on with the list, 8
; goes on to the final act's scene. A text (STORY_TEXTS) is lines, each a
; tilemap offset (word), an attribute byte, then characters: 0 ends the
; line, $FF clears the layer and ends it, $FE sets the wait before the
; next character (word), $FC and $FD end the text; $BA and $B7 are marks
; put on the row above the next character.
; C: src/aladdin/game/story_card.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_pad.inc"
incsrc "../include/wram_video.inc"

arch 65816

!LOAD_LIST_PTR = $4C             ; direct page: the list's next byte (word, in bank $80)
!TEXT_AT = $4E                   ; direct page: the next character's offset in text_tilemap (word)
!TEXT_TYPE_PTR = $50             ; direct page: the text's next byte (word, in bank $80)
!CARD_COUNT = $52                ; direct page: frames left (a byte, or a word while typing)
!TEXT_ATTR = $54                 ; direct page: the line's attribute byte
!STORY_LISTS = $D9F3             ; bank $80: per part (1-7), its list's offset (word)
!STORY_TEXTS = $F3BB             ; bank $80: per text, its offset (word)
!tm_shadow = $0399               ; TM's shadow: the main screen's layers
!routine_818181 = $818181  ; not yet filed
!routine_80af42 = $80AF42  ; not yet filed

; @routine 81:F10D StoryCardTask
; WHAT IT DOES: The story card between acts: for the part being started
; (LEVEL_035C; part 0 as part 7) it runs that part's list: a screen
; loaded and brightened, a text typed a character every 4 frames (any of
; B, Y, A, X held types at once; Start ends the card), then a fade, and
; the level starts (or the list goes on, or the final act's scene).
; HOW: The list's pointer from STORY_LISTS by the part; TextLayerClearFront,
; a frame's wait; NMI off, screen blanked; the text layer's upload at VRAM
; $5C00, $800 bytes; LEVEL $30, LevelRamSetupLong, graphics set 1 and
; VRAM copy 0; NMI on; 48 frames. Then for each command, after CARD_COUNT
; frames: TM_SHADOW 5 and JMP through STORY_COMMANDS by the list's byte.
; 0: NMI off, blanked, VRAM maps $4000 and $4800 filled with 0, text layer
; cleared; then (set $0E, copy $7E) and the list's set, copy and palette,
; until the next byte is not 0: OamBufferClear, ScrollStateClear, NMI on,
; that byte the song (SongPlay, SoundQueueF5WithA); BrightnessUpAndWait;
; 64 frames. 2: the text by the next byte from STORY_TEXTS, line by line:
; TEXT_AT and TEXT_ATTR from its head; each character after CARD_COUNT
; frames (4) put in TEXT_TILEMAP at TEXT_AT with the attribute, TEXT_AT on
; 2, the upload marked; $BA, $B7 put at TEXT_MARKS with no wait or step;
; the codes above; $FC, $FD: the list's next byte, a frame. 4 (or Start):
; SoundQueueF6With1 (B = 3), 48 frames, BrightnessDownAndWait, 48 frames,
; TaskExec to $81:8181. 6: the list's next byte, SoundQueueF6With1 (B =
; 3), 64 frames, BrightnessDownAndWait, 64 frames. 8: TaskExec to
; $80:AF42.
; Entry: M 8-bit, X 8-bit; D = 0; DB = $80; a task's entry (TaskExec from
; the password screen and other tasks). Exit: never returns.
%at($81F10D)
StoryCardTask:
    lda.w !level_035c
    bne .part
    lda.b #$07
.part:
    asl a
    tax
    lda.w !STORY_LISTS,x
    sta.b !LOAD_LIST_PTR
    lda.w !STORY_LISTS+1,x
    sta.b !LOAD_LIST_PTR+1
    jsl.l TextLayerClearFront
    lda.b #$01
    jsl.l TaskWait
    jsl.l NmiDisable
    jsl.l ScreenBlank
    rep.b #$20
    lda.w #$5C00
    sta.w !text_upload_vram
    lda.w #$0800
    sta.w !text_upload_size
    sep.b #$20
    lda.b #$30
    sta.w !level
    jsl.l LevelRamSetupLong
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    jsl.l NmiEnable
    lda.b #$30
    sta.b !CARD_COUNT
.next:
    dec.b !CARD_COUNT
    bne .wait
    lda.b #$05
    sta.w !tm_shadow
    lda.b (!LOAD_LIST_PTR)
    tax
    jmp.w (.commands,x)
.wait:
    lda.b #$01
    jsl.l TaskWait
    bra .next
.commands:
    dw .screen, .text, .end_card, .fade_on, .final_scene
.screen:
    jsl.l NmiDisable
    jsl.l ScreenBlank
    rep.b #$10
    ldx.w #$4000
    ldy.w #$0000
    jsl.l VramFillMap
    ldx.w #$4800
    ldy.w #$0000
    jsl.l VramFillMap
    sep.b #$10
    jsl.l TextLayerClear
.screen_part:
    lda.b #$0E
    jsl.l UnpackSet
    ldx.b #$7E
    jsl.l VramCopy
    jsr.w LoadListNext
    lda.b (!LOAD_LIST_PTR)
    jsl.l UnpackSet
    jsr.w LoadListNext
    lda.b (!LOAD_LIST_PTR)
    tax
    jsl.l VramCopy
    jsr.w LoadListNext
    lda.b (!LOAD_LIST_PTR)
    jsl.l PaletteLoad
    jsr.w LoadListNext
    lda.b (!LOAD_LIST_PTR)
    beq .screen_part
    jsl.l OamBufferClear
    jsl.l ScrollStateClear
    jsl.l NmiEnable
    lda.b (!LOAD_LIST_PTR)
    jsl.l SongPlay
    jsl.l SoundQueueF5WithA
    jsr.w LoadListNext
    jsr.w BrightnessUpAndWait
    lda.b #$40
    sta.b !CARD_COUNT
    jmp.w .next
.text:
    jsr.w LoadListNext
    lda.b (!LOAD_LIST_PTR)
    asl a
    tax
    rep.b #$20
    lda.w !STORY_TEXTS,x
    sta.b !TEXT_TYPE_PTR
.line:
    lda.b (!TEXT_TYPE_PTR)
    sta.b !TEXT_AT
    inc.b !TEXT_TYPE_PTR
    inc.b !TEXT_TYPE_PTR
    sep.b #$20
    lda.b (!TEXT_TYPE_PTR)
    sta.b !TEXT_ATTR
    rep.b #$20
    lda.w #$0004
    sta.b !CARD_COUNT
    sep.b #$20
.frame:
    lda.l !pad1_held+1
    bit.b #$10
    beq .no_start
    jmp.w .end_card
.no_start:
    lda.l !pad1_held
    ora.l !pad1_held+1
    bit.b #$C0
    bne .type
    rep.b #$20
    dec.b !CARD_COUNT
    sep.b #$20
    bne .wait_frame
.type:
    rep.b #$20
    lda.w #$0004
    sta.b !CARD_COUNT
    sep.b #$20
.character:
    jsr.w TextTypeNext
    lda.b !TEXT_ATTR
    xba
    lda.b (!TEXT_TYPE_PTR)
    beq .line_end
    cmp.b #$FC
    bcs .code
    cmp.b #$BA
    beq .mark
    cmp.b #$B7
    bne .put
.mark:
    rep.b #$30
    ldx.b !TEXT_AT
    sta.l !text_tilemap-64,x
    sep.b #$30
    bra .character
.put:
    rep.b #$30
    ldx.b !TEXT_AT
    sta.l !text_tilemap,x
    inx
    inx
    stx.b !TEXT_AT
    sep.b #$30
    inc.w !text_tilemap_upload
.wait_frame:
    lda.b #$01
    jsl.l TaskWait
    bra .frame
.line_end:
    jsr.w TextTypeNext
    rep.b #$20
    jmp.w .line
.code:
    inc a
    bmi .code_fe
    jsl.l TextLayerClear
    bra .line_end
.code_fe:
    inc a
    bmi .text_end
    jsr.w TextTypeNext
    lda.b (!TEXT_TYPE_PTR)
    sta.b !CARD_COUNT
    jsr.w TextTypeNext
    lda.b (!TEXT_TYPE_PTR)
    sta.b !CARD_COUNT+1
    jmp.w .frame
.text_end:
    jsr.w LoadListNext
    lda.b #$01
    sta.b !CARD_COUNT
    jmp.w .next
.end_card:
    lda.b #$03
    xba
    jsl.l SoundQueueF6With1
    lda.b #$30
    sta.b !CARD_COUNT
.end_wait:
    lda.b #$01
    jsl.l TaskWait
    dec.b !CARD_COUNT
    bne .end_wait
    jsr.w BrightnessDownAndWait
    lda.b #$30
    sta.b !CARD_COUNT
.end_dark:
    lda.b #$01
    jsl.l TaskWait
    dec.b !CARD_COUNT
    bne .end_dark
    jsl.l TaskExec
    dl !routine_818181
.fade_on:
    jsr.w LoadListNext
    lda.b #$03
    xba
    jsl.l SoundQueueF6With1
    lda.b #$40
    sta.b !CARD_COUNT
.fade_wait:
    lda.b #$01
    jsl.l TaskWait
    dec.b !CARD_COUNT
    bne .fade_wait
    jsr.w BrightnessDownAndWait
    lda.b #$40
    sta.b !CARD_COUNT
    jmp.w .next
.final_scene:
    jsl.l TaskExec
    dl !routine_80af42
