; story_text.asm: the tasks that type the story's text onto the text
; layer, a character every few frames: one text of set 0 (the opening
; story's scenes) or of set 1 (the story's later scenes), picked by the
; task's parameter. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/game/story_text.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; In bank $81 (the data bank): per set (0, 1) a byte, the offset from
; here of the set's list; each list is per text its offset in STORY_TEXT
; (words).
!STORY_TEXT_SETS = $FD8D
; The texts: characters (tiles below $FB), and the codes: $FB place (the
; tilemap byte offset of the text's first row and column, a word), $FC
; palette (a byte, its low 3 bits), $FD wait (a byte, frames), $FE page
; (wait 160 frames, clear the layer, back to the first row and column),
; $FF next row; 0 ends.
!STORY_TEXT = $A50000
!STORY_TEXT_PLACE = $FB
!STORY_TEXT_UP_MARK_1 = $00BA   ; marks put on the row above, at the column the next character takes
!STORY_TEXT_UP_MARK_2 = $00B7
!STORY_TEXT_PAGE_WAIT = $A0
!STORY_TILE = !scratch+32       ; $002C: the tilemap word being put (first the parameter times 2)
!STORY_TILE_AT = !scratch+34    ; $002E: the row's start plus the column
; The task's own bytes in its slot (the direct page).
!STORY_TEXT_FRAMES = $06        ; frames between characters (on entry, the parameter)
!STORY_TEXT_PALETTE = $07       ; the palette bits of the tilemap word's high byte
!STORY_TEXT_COLUMN = $08
!STORY_TEXT_ROW = $09           ; each row two of the layer's rows
!STORY_TEXT_ORIGIN = $0A        ; the tilemap byte offset of the first row and column (word)
!STORY_TEXT_CHAR_FRAMES = $04
!STORY_TEXT_PRIORITY = $20      ; the high byte's priority bit

; @routine 81:F332 StoryTextTask
; WHAT IT DOES: A task that types text of set 0 (the opening story's) onto
; the text layer, a character every 4 frames, then ends.
; HOW: D = its slot; the text picked by its parameter from STORY_TEXT_SETS
; set 0; the layer cleared (TextLayerClear); then each byte of the text:
; a character (below $FB) put at the row and column with the palette and
; priority bits, the upload marked, the column stepped, TaskWait 4
; (STORY_TEXT_UP_MARK_1 and _2 go a row up with no step and no wait); $FB
; sets the place of the first row and column, $FC the palette, $FD waits
; its byte's frames, $FE waits 160 frames and clears the layer back to
; the first row and column, $FF goes to the next row; at 0, TaskExit. No
; script types a mark ($BA, $B7): that path is written from its bytes.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the text; DB a bank whose $0000-$1FFF is work RAM and whose
; $FD8D holds STORY_TEXT_SETS (bank $81). Exit: none: the task ends
; (TaskExit with M and X 8-bit).
%at($81F332)
StoryTextTask:
    rep.b #$20
    clc
    tya
    adc.w #!task_state
    tcd
    sep.b #$20
    ldx.b #0
    ldy.b #!STORY_TEXT_CHAR_FRAMES
    lda.b !STORY_TEXT_FRAMES
    bra StoryTextTask1_type

; @routine 81:F344 StoryTextTask1
; WHAT IT DOES: The text-typing task of StoryTextTask for set 1 (the
; story's scenes, kind $9C's).
; HOW: As StoryTextTask, with set 1.
; Entry: as StoryTextTask. Exit: none: the task ends (TaskExit with M and
; X 8-bit).
StoryTextTask1:
    rep.b #$20
    clc
    tya
    adc.w #!task_state
    tcd
    sep.b #$20
    ldx.b #1
    ldy.b #!STORY_TEXT_CHAR_FRAMES
    lda.b !STORY_TEXT_FRAMES
.type:
    asl a
    sta.w !STORY_TILE
    sty.b !STORY_TEXT_FRAMES
    stz.b !STORY_TEXT_PALETTE
    stz.b !STORY_TEXT_COLUMN
    stz.b !STORY_TEXT_ROW
    lda.w !STORY_TEXT_SETS,x
    clc
    adc.w !STORY_TILE
    tay
    rep.b #$10
    jsl.l TextLayerClear
    ldx.w !STORY_TEXT_SETS,y
.next:
    lda.l !STORY_TEXT,x
    bne .code
    sep.b #$30
    jml.l TaskExit
.code:
    cmp.b #!STORY_TEXT_PLACE
    bcc .char
    inc a
    beq .row
    inc a
    beq .page
    inc a
    beq .wait
    inc a
    beq .palette
    lda.l !STORY_TEXT+1,x
    sta.b !STORY_TEXT_ORIGIN
    lda.l !STORY_TEXT+2,x
    sta.b !STORY_TEXT_ORIGIN+1
    inx
    inx
    inx
    bra .next
.row:
    stz.b !STORY_TEXT_COLUMN
    inc.b !STORY_TEXT_ROW
    inx
    bra .next
.page:
    stz.b !STORY_TEXT_COLUMN
    stz.b !STORY_TEXT_ROW
    lda.b #!STORY_TEXT_PAGE_WAIT
    jsl.l TaskWait
    phx
    jsl.l TextLayerClear
    plx
    inx
    bra .next
.wait:
    lda.l !STORY_TEXT+1,x
    jsl.l TaskWait
    inx
    inx
    bra .next
.palette:
    lda.l !STORY_TEXT+1,x
    and.b #$07
    asl a
    asl a
    sta.b !STORY_TEXT_PALETTE
    inx
    inx
    bra .next
.char:
    phx
    sta.w !STORY_TILE
    lda.b !STORY_TEXT_PALETTE
    ora.b #!STORY_TEXT_PRIORITY
    sta.w !STORY_TILE+1
    rep.b #$20
    lda.b !STORY_TEXT_COLUMN
    and.w #$00FF
    asl a
    clc
    adc.b !STORY_TEXT_ORIGIN
    sta.w !STORY_TILE_AT
    lda.b !STORY_TEXT_ROW
    and.w #$00FF
    xba
    lsr a
    clc
    adc.w !STORY_TILE_AT
    tax
    lda.w !STORY_TILE
    and.w #$00FF
    cmp.w #!STORY_TEXT_UP_MARK_1
    beq .mark
    cmp.w #!STORY_TEXT_UP_MARK_2
    beq .mark
    lda.w !STORY_TILE
    sta.l !text_tilemap,x
    bra .typed
.mark:
    txa
    clc
    adc.w #$FFC0                ; a row up (-64)
    tax
    lda.w !STORY_TILE
    sta.l !text_tilemap,x
    sep.b #$20
    bra .done
.typed:
    sep.b #$20
    inc.w !text_tilemap_upload
    inc.b !STORY_TEXT_COLUMN
    lda.b !STORY_TEXT_FRAMES
    jsl.l TaskWait
.done:
    plx
    inx
    jmp.w .next
