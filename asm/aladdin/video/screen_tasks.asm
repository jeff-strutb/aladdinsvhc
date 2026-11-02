; screen_tasks.asm: tasks that change the screen over many frames (fades,
; palette sequences, layer shakes), and runs of text-layer words.
; C: src/aladdin/video/screen_tasks.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

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
; Random lists: per list, a mask, then its entries.
!random_lists = $81DCE7

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
%at($819774)
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
