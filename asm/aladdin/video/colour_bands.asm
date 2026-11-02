; colour_bands.asm: the colour bands' HDMA table: the fixed colour set
; line by line down the screen, written into one of two buffers.
; C: src/aladdin/video/colour_bands.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; The bands (bank $80): their count, then (lines, value) pairs; each
; buffer's offset from colour_band_buffers.
!colour_band_count = $BA7E
!colour_band_pairs = $BA7F
!colour_band_buffer_at = $BAA1
!colour_band_buffers = $7EF500
!colour_band_hdma_at = $0356     ; the buffer the HDMA reads (offset)
; The colour band task's record (D = its slot + $6F).
!band_task_step = $06
!band_task_buffer = $07
!band_task_left = $08

; @routine 81:A08A ColourBandsWrite
; WHAT IT DOES: Writes the colour bands' HDMA table into the task's
; spare buffer, the bands from the 18th on dimmed by its step, and points
; the HDMA at it.
; HOW: The buffer (the task's $07: 0 or 1) is at $7E:F500 + its byte at
; $BAA1; the band count at $BA7E, then (lines, value) pairs from $BA7F,
; are copied (counting at the task's $08), each value from the 18th pair
; on less the step (the task's $06), not below 0; $0356 = the buffer's
; offset; the buffer flips.
; Entry: M 8-bit, X 8-bit; D = the colour band task's record (its slot +
; $6F); DB = $80; called by JSR. Exit: A low byte = the new buffer; X, Y
; past the table (low bytes); the task's $08 = 0.
%at($81A08A)
ColourBandsWrite:
    ldy.b !band_task_buffer
    phy
    ldx.w !colour_band_buffer_at,y
    phx
    lda.w !colour_band_count
    sta.b !band_task_left
    ldy.b #0
.pair:
    lda.w !colour_band_pairs,y
    sta.l !colour_band_buffers,x
    lda.w !colour_band_pairs+1,y
    cpy.b #34
    bcc .put
    sec
    sbc.b !band_task_step
    bpl .put
    lda.b #0
.put:
    sta.l !colour_band_buffers+1,x
    iny
    iny
    inx
    inx
    dec.b !band_task_left
    bne .pair
    plx
    stx.w !colour_band_hdma_at
    pla
    eor.b #$01
    sta.b !band_task_buffer
    rts
