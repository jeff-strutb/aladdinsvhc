; placement_index.asm: the indexes a level's actor placements get when it
; starts: how many fall in each 32-pixel band and where each band's run
; begins, so the placements near the camera are found without a search.
;
; The placements are 6-byte entries after a count byte, in the bank DB
; names; the tables are 256 bytes each from $7E:C100.
; C: src/aladdin/actor/placement_index.c.

incsrc "../include/macros.inc"

arch 65816

!ENTRY_WORD_2 = $8A7D           ; the word at byte 2 of entry Y
!ENTRY_WORD_4 = $8A7F           ; the word at byte 4 of entry Y
!BAND_COUNT = $7EC100           ; per band: placements in it (or per entry: its band)
!BAND_FIRST = $7EC200           ; per band: the first placement's number
!BAND_RUNS = $7EC300            ; per band: placements counted in the second pass
!BAND_STARTS = $7EC400          ; per band: where its run starts in !BAND_ORDER
!BAND_ORDER = $7EC500           ; placement numbers, band by band

; Direct-page words the two passes use.
!DP_LEFT = $0C
!DP_LAST = $0E
!DP_NEXT = $0F
!DP_TOP = $10
!DP_BAND = $11
!DP_START = $12

; @routine 82:A984 PlacementsByColumn
; WHAT IT DOES: Counts A placements from entry Y into their columns: each
; entry's column (the word at its byte 2 over 32, 10 bits) gets one more
; in the count table, and a column met for the first time since the last
; gets the placement's number in the first-of-band table. Columns from
; 255 up are not counted.
; HOW: The direct page's word at $0C counts down the entries; its byte at
; $0E is the last column met and $0F the placement number, stepped for
; each placement counted. 8-bit A for the table steps.
; Entry: M 16-bit, X 16-bit; A = the count, Y = the first entry's offset;
; D a page whose $0C-$0F it may use ($0E and $0F set by the caller); DB
; the entries' bank. Exit: as the C states.
%at($82A984)
PlacementsByColumn:
    sta.b !DP_LEFT
.entry:
    lda.w !ENTRY_WORD_2,y
    jsr.w ShiftRight5
    and.w #$03FF
    cmp.w #$00FF
    bcs .next
    tax
    sep.b #$20
    lda.l !BAND_COUNT,x
    inc a
    sta.l !BAND_COUNT,x
    txa
    cmp.b !DP_LAST
    beq .counted
    sta.b !DP_LAST
    lda.b !DP_NEXT
    sta.l !BAND_FIRST,x
.counted:
    inc.b !DP_NEXT
.next:
    rep.b #$21
    tya
    adc.w #6
    tay
    dec.b !DP_LEFT
    bne .entry
    rts

; @routine 82:A9BB PlacementsByRow
; WHAT IT DOES: Indexes A placements from entry Y by row: each entry's row
; (the word at its byte 4 over 32, 11 bits, kept only below 255) goes in
; a per-entry table; then row by row the placements in it are listed,
; from the last placement to the first, the row's count stepped for each,
; and each row that has any gets its run's start in the list.
; HOW: Two passes. The direct page's word at $0C counts down the
; entries; $0E is the count, $10 the last entry's number, $11 the row
; searched and $0F the list's length, $12 the last start written.
; Entry: M 16-bit, X 16-bit; A = the count (1 up), Y = the first entry's
; offset; D a page whose $0C-$12 it may use ($11 0 for the first row);
; DB the entries' bank. Exit: as the C states.
PlacementsByRow:
    sta.b !DP_LEFT
    sta.b !DP_LAST
    dec a
    sta.b !DP_TOP
    ldx.w #0
.entry:
    lda.w !ENTRY_WORD_4,y
    jsr.w ShiftRight5
    and.w #$07FF
    cmp.w #$00FF
    bcs .next
    sep.b #$20
    sta.l !BAND_COUNT,x
.next:
    rep.b #$21
    tya
    adc.w #6
    tay
    inx
    dec.b !DP_LEFT
    bne .entry
    sep.b #$30
    lda.b #$00
    sta.b !DP_START
.row:
    ldx.b !DP_TOP
.search:
    cmp.l !BAND_COUNT,x
    bne .not_in_row
    txy
    tax
    pha
    lda.l !BAND_RUNS,x
    inc a
    sta.l !BAND_RUNS,x
    ldx.b !DP_NEXT
    tya
    sta.l !BAND_ORDER,x
    inc.b !DP_NEXT
    pla
    tyx
.not_in_row:
    dex
    cpx.b #$FF
    bne .search
    tax
    ldy.b !DP_NEXT
    cpy.b !DP_START
    beq .no_run
    lda.b !DP_START
    sty.b !DP_START
    sta.l !BAND_STARTS,x
.no_run:
    inc.b !DP_BAND
    lda.b !DP_BAND
    ldx.b !DP_NEXT
    cpx.b !DP_LAST
    bne .row
    rep.b #$30
    rts
