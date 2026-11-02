; backdrop_offset.asm: the backdrop layer's scroll taken from the play
; field's, with an offset down. Drafted by cdraft.py; named and described
; by hand.
; C: src/aladdin/video/backdrop_offset.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_video.inc"

arch 65816

!BACKDROP_OFFSET_Y = $1A2B   ; added to the play field's scroll Y for the backdrop's: 4 bytes, from the fraction byte

; @routine 81:CA6A BackdropFollowOffsetY
; WHAT IT DOES: Moves the backdrop with the play field: the same scroll X,
; and its scroll Y the play field's plus an offset.
; HOW: The scroll X word copied; the scroll Y, from its fraction byte,
; plus BACKDROP_OFFSET_Y as two 16-bit adds with carry (4 bytes).
; Entry: M 16-bit, X 16-bit; DB = $80; called by JSR from the scroll
; handler the level picks ($0841). Exit: M 16-bit, X 16-bit; A = the
; result's high word; C, N, V, Z from its add.
%at($81CA6A)
BackdropFollowOffsetY:
    lda.w !layer0+!LAYER_SCROLL_X
    sta.w !layer1+!LAYER_SCROLL_X
    clc
    lda.w !layer0+!LAYER_SCROLL_Y-1
    adc.w !BACKDROP_OFFSET_Y
    sta.w !layer1+!LAYER_SCROLL_Y-1
    lda.w !layer0+!LAYER_SCROLL_Y+1
    adc.w !BACKDROP_OFFSET_Y+2
    sta.w !layer1+!LAYER_SCROLL_Y+1
    rts
