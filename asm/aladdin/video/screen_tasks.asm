; screen_tasks.asm: tasks that change the screen over many frames (fades,
; palette sequences, layer shakes), and runs of text-layer words.
; C: src/aladdin/video/screen_tasks.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"

arch 65816

!TEXT_BLINKS = $A5             ; TextBlinkTask: blinks left (its direct page)
!TEXT_BLINK_LAYOUT = $14       ; its line, and on level TEXT_BLINK_LEVEL_ALT the other
!TEXT_BLINK_LAYOUT_ALT = $70
!TEXT_BLINK_LEVEL_ALT = $11
!V_03FA = $03FA                ; counted up as a level is set up; cleared after the blink with health left

; The palette sequence: per step (5 down to 0), the set to load and the
; frames to hold it.
!palette_sequence_sets = $81BA6A
!palette_sequence_waits = $81BA70
; Shake presets, 4 bytes each: layer, sways, frames a half-sway, step.
!shake_presets = $81BAA3
; The shake task's record, from its slot (the direct page).
!shake_preset = $75             ; the task's parameter: the preset's offset, then the layer
!shake_sways = $76              ; sways left
!shake_half_frames = $77        ; frames in a half-sway
!shake_step_px = $78            ; pixels a frame
; Text layouts in bank $82: per layout, its offset (words); then runs.
!text_layouts = $B575
!TEXT_LAYOUT_BANK = $82
!layout_blank = !scratch+3       ; TextLayoutDraw: the layout byte, bit 7 set to blank
; TextLayoutDrawBig's scratch: the run's high byte, the tile, the rows left
; in a character, the place of its character.
!big_high = !scratch+4
!big_tile = !scratch+6
!big_rows = !scratch+8
!big_place = !scratch+10
; Random lists: per list, a mask, then its entries.
!random_lists = $81DCE7

; @routine 81:9726 TextLayoutShowThird
; WHAT IT DOES: Draws text layout A (or blanks it, bit 7 set) and has the
; NMI upload the third part of the text buffer at the next frame.
; HOW: TextLayoutDraw, then the third of TEXT_PARTS_PENDING counted up
; (DB as at entry).
; Entry: M 8-bit, X 8-bit; A low byte = the layout; called by JSL. Exit:
; as TextLayoutDraw leaves it, but N and Z from the part's count.
%at($819726)
TextLayoutShowThird:
    jsr.w TextLayoutDraw
    inc.w !text_parts_pending+2
    rtl

; @routine 81:972D TextLayoutShow
; WHAT IT DOES: Draws text layout A (or blanks it, bit 7 set) and has the
; NMI copy the text layer to VRAM at the next frame.
; HOW: TextLayoutDraw, then text_tilemap_upload counted up (DB as at
; entry).
; Entry: M 8-bit, X 8-bit; A low byte = the layout; called by JSL. Exit:
; as TextLayoutDraw leaves it, but N and Z from the upload count.
%at($81972D)
TextLayoutShow:
    jsr.w TextLayoutDraw
    inc.w !text_tilemap_upload
    rtl

; @routine 81:9734 TextLayoutDraw
; WHAT IT DOES: Draws text layout A (low 7 bits) into the text layer's
; tilemap, or blanks the places it covers (bit 7 set).
; HOW: With DB = $82, the layout's offset from the word table at $B575;
; then runs until a 0 byte: a count byte (low 6 bits; bit 6 set: one word
; repeated), the tilemap offset (word), and the words (one when
; repeated), copied by TextLayerCopyWords or blanked by
; TextLayerBlankWords; A is kept at scratch $0F for the blank test.
; Entry: M 8-bit, X 8-bit; A low byte = the layout; called by JSR. Exit:
; M and X 8-bit; DB as at entry, N and Z from it (PLB); X, Y as the last
; run left them (low bytes); scratch $0C = 0.
%at($819734)
TextLayoutDraw:
    sta.w !layout_blank
    asl a
    lsr a
    tax
    phb
    lda.b #!TEXT_LAYOUT_BANK
    pha
    plb
    rep.b #$10
    ldy.w !text_layouts,x
.run:
    sep.b #$20
    lda.w 0,y
    beq .done
    iny
    pha
    rol a
    rol a
    pla
    rep.b #$20
    and.w #$003F
    sta.w !scratch
    ldx.w 0,y
    iny
    iny
    lda.w !layout_blank-1
    bpl .copy
    jsr.w TextLayerBlankWords
    bra .next
.copy:
    jsr.w TextLayerCopyWords
.next:
    bcc .run
    iny
    iny
    bcs .run
.done:
    sep.b #$30
    plb
    rts


; @routine 81:9774 TextLayerCopyWords
; WHAT IT DOES: Copies a run of words into the text layer's tilemap, or
; repeats one word along it.
; HOW: Scratch $0C words: each word at the data bank's Y goes to the
; tilemap at X; X steps a word each time, Y only while C is clear (C set:
; the same word all along).
; Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; Y = the
; source; C set to repeat; scratch $0C = the count (nonzero); called by
; JSR. Exit: X, Y past the run; scratch $0C = 0; Z set, N clear; A = the
; last word; C unchanged.
TextLayerCopyWords:
    lda.w 0,y
    sta.l !text_tilemap,x
    bcs .l977F
    iny
    iny
.l977F:
    inx
    inx
    dec.w !scratch
    bne TextLayerCopyWords
    rts


; @routine 81:9787 TextLayerBlankWords
; WHAT IT DOES: Blanks a run of the text layer's tilemap.
; HOW: Scratch $0C words of $0020 (the blank tile) from X; X steps a word
; each time, Y too while C is clear.
; Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; scratch $0C =
; the count (nonzero); called by JSR. Exit: X past the run; Y stepped as
; X when C is clear; scratch $0C = 0; A = $0020; Z set, N clear; C
; unchanged.
%at($819787)
TextLayerBlankWords:
    lda.w #$0020
    sta.l !text_tilemap,x
    bcs .l9792
    iny
    iny
.l9792:
    inx
    inx
    dec.w !scratch
    bne TextLayerBlankWords
    rts

; @routine 81:979A TextLayoutDrawBig
; WHAT IT DOES: Draws text layout X's first run as large characters, each
; four tiles (two by two), into the text layer's tilemap, and has the NMI
; copy the tilemap to VRAM. Nothing in the game calls it.
; HOW: With DB = $82, the layout's offset from the word table at $B575.
; Its first byte: 0 draws nothing; else the low 6 bits count the
; characters. Then the tilemap offset (word), the high byte of every word
; drawn, and per character its first tile (word): tiles t, t+1 in the row,
; t+2, t+3 in the row below. Each next character two words to the right.
; text_tilemap_upload counted up.
; Entry: M 8-bit; X = the layout's offset in the table (16 bits taken);
; called by JSL. Exit: M and X 8-bit; DB as at entry, N and Z from it
; (PLB); C as the last character left it (the ADC), or as at entry with
; no characters; scratch $0C-$17 used.
%at($81979A)
TextLayoutDrawBig:
    phb
    lda.b #!TEXT_LAYOUT_BANK
    pha
    plb
    rep.b #$10
    ldy.w !text_layouts,x
    sep.b #$20
    lda.w 0,y
    beq .done
    iny
    pha
    rol a
    rol a
    pla
    rep.b #$20
    and.w #$003F
    sta.w !scratch
    lda.w 0,y
    sta.w !big_place
    tax
    iny
    iny
    lda.w 0,y
    and.w #$00FF
    xba
    sta.w !big_high
    dey
.char:
    iny
    iny
    lda.w 0,y
    sta.w !big_tile
    lda.w #$0001
    sta.w !big_rows
.row:
    lda.w !big_tile
    ora.w !big_high
    sta.l !text_tilemap,x
    inx
    inx
    inc.w !big_tile
    lda.w !big_tile
    ora.w !big_high
    sta.l !text_tilemap,x
    dec.w !big_rows
    bmi .next
    txa
    clc
    adc.w #$003E
    tax
    inc.w !big_tile
    bra .row
.next:
    dec.w !scratch
    beq .done
    ldx.w !big_place
    inx
    inx
    inx
    inx
    stx.w !big_place
    bra .char
.done:
    sep.b #$30
    inc.w !text_tilemap_upload
    plb
    rtl






; @routine 81:9E3B TextBlinkTask
; WHAT IT DOES: A task that blinks a line of text in the text buffer's third
; part three times (layout $14; $70 on level $11), then ends; with health
; left it also clears V_03FA.
; HOW: Three times: the layout drawn (TextLayoutShowThird), $1E frames
; (TaskWait), blanked (bit 7), $1E frames; the count in the task's
; direct page at TEXT_BLINKS. Then TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; D = the task's direct page;
; DB a bank whose $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819E3B)
TextBlinkTask:
    lda.b #$03
    sta.b !TEXT_BLINKS
    ldx.b #!TEXT_BLINK_LAYOUT
    lda.w !level
    cmp.b #!TEXT_BLINK_LEVEL_ALT
    bne .blink
    ldx.b #!TEXT_BLINK_LAYOUT_ALT
.blink:
    txa
    phx
    jsl.l TextLayoutShowThird
    plx
    lda.b #$1E
    jsl.l TaskWait
    txa
    ora.b #$80
    phx
    jsl.l TextLayoutShowThird
    plx
    lda.b #$1E
    jsl.l TaskWait
    dec.b !TEXT_BLINKS
    bne .blink
    lda.w !health
    beq .end
    stz.w !V_03FA
.end:
    jmp.w TaskExit

; @routine 81:9E8E BrightnessUpTask
; WHAT IT DOES: A task that raises the screen's brightness a step at a
; time to full, then ends.
; HOW: The brightness shadow's low 4 bits kept; at 15 already it ends;
; else the parameter is halved (at least 1) and, for each step, TaskWait
; for that many frames, then the brightness up by one, until 15; then
; TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819E8E)
BrightnessUpTask:
    lda.w !inidisp_shadow
    and.b #$0F
    sta.w !inidisp_shadow
    cmp.b #$0F
    beq .done
    lda.w !task_param,y
    lsr a
    bne .wait
    inc a
.wait:
    sta.w !task_param,y
.step:
    lda.w !task_param,y
    jsl.l TaskWait
    inc.w !inidisp_shadow
    lda.w !inidisp_shadow
    cmp.b #$0F
    bne .step
.done:
    jml.l TaskExit

; @routine 81:9EB9 BrightnessDownTask
; WHAT IT DOES: A task that lowers the screen's brightness a step at a
; time to 0 (dark), then ends.
; HOW: The brightness shadow's low 4 bits kept; at 0 already it ends;
; else the parameter is halved (at least 1) and, for each step, TaskWait
; for that many frames, then the brightness down by one, until 0; then
; TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the frames between steps (halved); DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($819EB9)
BrightnessDownTask:
    lda.w !inidisp_shadow
    and.b #$0F
    sta.w !inidisp_shadow
    beq .done
    lda.w !task_param,y
    lsr a
    bne .wait
    inc a
.wait:
    sta.w !task_param,y
.step:
    lda.w !task_param,y
    jsl.l TaskWait
    dec.w !inidisp_shadow
    bne .step
.done:
    jml.l TaskExit

; @routine 81:AE8A RandomFromList
; WHAT IT DOES: Picks a random entry from list X.
; HOW: The list (its offset at $DCE7 + X) starts with a mask; RandomNext
; masked by it picks the entry, a byte after the mask.
; Entry: X 8-bit; X = the list's number * 2; DB = $81. Exit: M and X
; 8-bit; Y = A low byte = the entry; A high byte = the byte after it;
; scratch $1C = the masked random (word); N, Z from the entry; C, V from
; the ADC.
%at($81AE8A)
RandomFromList:
    rep.b #$30
    lda.w !random_lists,x
    tay
    sep.b #$20
    lda.w !random_lists,y
    sta.w !scratch+16
    jsl.l RandomNext
    and.w !scratch+16
    rep.b #$20
    and.w #$00FF
    sta.w !scratch+16
    tya
    clc
    adc.w !scratch+16
    tay
    lda.w !random_lists+1,y
    sep.b #$30
    tay
    rtl
