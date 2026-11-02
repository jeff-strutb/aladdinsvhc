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
