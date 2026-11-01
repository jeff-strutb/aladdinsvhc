; continue_screen.asm: the screen between a lost life and the next try,
; where the continues left are shown and one can be spent.
; C: src/aladdin/game/continue_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Where the continues digit sits in the text buffer: row 26, column 29.
!CONTINUES_DIGIT = !text_buffer+1722
!TILE_DIGIT_0 = $30

; @routine 80:B441 ContinuesShow
; WHAT IT DOES: Shows the number of continues left on the continue screen.
; HOW: The digit's tile is $30 plus the count (decimal when D is set, as
; the CPU adds); it goes into the text buffer and the buffer's upload is
; asked for.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C
; states.
%at($80B441)
ContinuesShow:
    clc
    lda.w !continues
    adc.b #!TILE_DIGIT_0
    sta.l !CONTINUES_DIGIT
    inc.w !text_upload_pending
    rts

; @routine 80:B44F WramC800Clear
; WHAT IT DOES: Clears the 4 KB buffer at $7E:C800 before a level starts.
; HOW: Zeroes $7E:C800-$7E:D7FF a byte at a time with 16-bit X; P comes
; back from the stack.
; Entry: M 8-bit. Exit: as the C states.
WramC800Clear:
    php
    rep.b #$10
    ldx.w #0
    lda.b #0
.clear:
    sta.l !wram_c800,x
    inx
    cpx.w #4096
    bcc .clear
    plp
    rts
