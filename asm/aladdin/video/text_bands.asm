; text_bands.asm: a screen of text shown band by band (the story text in
; bank $82): a page of lines written to the text tilemap, and the bands'
; window bytes handed to HDMA channel 5 a frame at a time.
; C: src/aladdin/video/text_bands.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!bands_parity = $F9             ; direct page: steps each commit; bit 0 picks the table
!text_line_list = $F5           ; direct page: the next word of the lines' tilemap offsets
!text_lines_left = !scratch     ; lines left on the page
!text_attr = !scratch+1         ; the line's attribute byte (a tilemap word's high byte)
!text_chars_left = !scratch+2   ; characters left on the line
!page = $0000                   ; Y indexes the page from its bank's start

; @routine 82:B270 TextBandsCommit
; WHAT IT DOES: Hands the 16 window bytes of the text bands ($1C8E) to
; HDMA channel 5, through whichever of its two tables is not in use.
; HOW: By bit 0 of $F9: clear, the bytes go to $1C9E and channel 5's
; table address to $CEEF; set, to $1CAE and $CF3B (the addresses into
; $0352, channel 5's in hdma_table_at). $F9 steps up.
; Entry: M 8-bit, X 8-bit; D the page holding $F9; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: X = $FF; Y = $0F or $1F;
; A low byte = the last byte copied.
%at($82B270)
TextBandsCommit:
    ldx.b #$0F
    ldy.b #$1F
    lda.b #$EF
    sta.w !hdma_table_at+2
    lda.b #$CE
    sta.w !hdma_table_at+3
    lda.b !bands_parity
    lsr a
    bcc .copy
    ldy.b #$2F
    lda.b #$3B
    sta.w !hdma_table_at+2
    lda.b #$CF
    sta.w !hdma_table_at+3
.copy:
    lda.w !text_bands,x
    sta.w !text_bands,y
    dey
    dex
    bpl .copy
    inc.b !bands_parity
    rts


; @routine 82:B190 TextPageDraw
; WHAT IT DOES: Writes a page of text into the text tilemap, each line at
; the next place from the list at $F5 with the line's attribute byte,
; then has the NMI upload the tilemap.
; HOW: The page's first byte is its line count ($000C). Per line: the
; list's next word (stepping $F5) is its tilemap offset; then the line's
; attribute byte ($000D) and length ($000E), then its characters, each
; the low byte of a tilemap word whose high byte is the attribute.
; text_tilemap_upload steps up.
; Entry: M 8-bit, X 16-bit; Y = the page's offset; D = 0; DB = the bank of
; the page and the list; called by JSR. Exit: X, Y past the last line;
; $000C, $000E = 0.
%at($82B190)
TextPageDraw:
    lda.w !page,y
    sta.w !text_lines_left
    iny
.line:
    phy
    ldx.b !text_line_list
    ldy.w !page,x
    inx
    inx
    stx.b !text_line_list
    tyx
    ply
    lda.w !page,y
    sta.w !text_attr
    lda.w !page+1,y
    sta.w !text_chars_left
    iny
    iny
.char:
    lda.w !page,y
    sta.l !text_tilemap,x
    lda.w !text_attr
    sta.l !text_tilemap+1,x
    inx
    inx
    iny
    dec.b !text_chars_left
    bne .char
    dec.b !text_lines_left
    bne .line
    inc.w !text_tilemap_upload
    rts


!band_step = $F7                ; direct page: the step (0-3), a pair of bands each
!band_pair_inner = $CF87        ; data bank $80: per step, the first band of its pair (3, 2, 1, 0)
!band_pair_outer = $CF8B        ; per step, the second (4, 5, 6, 7)
!BAND_SHOWN = $00               ; a band's window byte: its text shown
!BAND_MASKED = $08              ; masked by window 2 (BG3's window byte)

; @routine 82:B224 TextBandsReveal
; WHAT IT DOES: Shows the text band by band from the middle out: a pair
; of bands every 4 frames, over 4 steps.
; HOW: For step $F7 = 0 to 3: the window bytes of the step's two bands
; (from $80:CF87 and $80:CF8B) set to 0, TextBandsCommit, TaskWait 4.
; Entry: M 8-bit, X 8-bit; D the page holding $F7 and $F9; DB = $80;
; called by JSR from a task. Exit: as TaskWait; $F7 = 4.
%at($82B224)
TextBandsReveal:
    stz.b !band_step
.step:
    ldx.b !band_step
    lda.b #!BAND_SHOWN
    ldy.w !band_pair_inner,x
    sta.w !text_bands,y
    ldy.w !band_pair_outer,x
    sta.w !text_bands,y
    jsr.w TextBandsCommit
    lda.b #$04
    jsl.l TaskWait
    inc.b !band_step
    lda.b !band_step
    cmp.b #$04
    bne .step
    rts


; @routine 82:B248 TextBandsConceal
; WHAT IT DOES: Masks the text band by band from the outside in, a pair
; of bands every 4 frames, then waits 40 frames.
; HOW: For step $F7 = 3 down to 0: the window bytes of the step's two
; bands set to 8 (masked), TextBandsCommit, TaskWait 4; then TaskWait 40.
; Entry: M 8-bit, X 8-bit; D the page holding $F7 and $F9; DB = $80;
; called by JSR from a task. Exit: as TaskWait; $F7 = $FF.
%at($82B248)
TextBandsConceal:
    lda.b #$03
    sta.b !band_step
.step:
    ldx.b !band_step
    lda.b #!BAND_MASKED
    ldy.w !band_pair_inner,x
    sta.w !text_bands,y
    ldy.w !band_pair_outer,x
    sta.w !text_bands,y
    jsr.w TextBandsCommit
    lda.b #$04
    jsl.l TaskWait
    dec.b !band_step
    bpl .step
    lda.b #$28
    jsl.l TaskWait
    rts
