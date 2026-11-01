; parallax.asm: the backdrop's scroll, from the play field's.
;
; Each frame the scroll update ends in one of a table of handlers, chosen
; by the level ($083F), that set the backdrop layer's scroll from the play
; field's: at the same speed, at half speed, or not at all, so the
; backdrop looks farther away.
; C: src/aladdin/video/parallax.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; @routine 81:C797 ParallaxHalf
; WHAT IT DOES: Scrolls the backdrop at half the play field's speed, both
; ways, so it looks far away.
; HOW: The backdrop's scroll X and Y are the play field's, halved.
; Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
; play field's scroll Y halved; N clear, Z from it, C the bit shifted out;
; X, Y and the other flags unchanged.
%at($81C797)
ParallaxHalf:
    lda.w !layer0+!LAYER_SCROLL_X
    lsr a
    sta.w !layer1+!LAYER_SCROLL_X
    lda.w !layer0+!LAYER_SCROLL_Y
    lsr a
    sta.w !layer1+!LAYER_SCROLL_Y

; @routine 81:C7A5 ParallaxNone
; WHAT IT DOES: Leaves the backdrop where it is: for levels whose backdrop
; does not scroll with the play field.
; HOW: Returns at once (the shared return of ParallaxHalf).
; Entry: any. Exit: everything unchanged.
ParallaxNone:
    rts

; @routine 81:C7A6 ParallaxFull
; WHAT IT DOES: Scrolls the backdrop with the play field, at its speed.
; HOW: The backdrop's scroll X and Y are the play field's.
; Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
; play field's scroll Y; N and Z from it; X, Y and the other flags
; unchanged.
ParallaxFull:
    lda.w !layer0+!LAYER_SCROLL_X
    sta.w !layer1+!LAYER_SCROLL_X
    lda.w !layer0+!LAYER_SCROLL_Y
    sta.w !layer1+!LAYER_SCROLL_Y
    rts

; @routine 81:C7D4 ParallaxHalfX
; WHAT IT DOES: Scrolls the backdrop at half the play field's speed across
; and at its full speed up and down.
; HOW: The backdrop's scroll X is the play field's halved, its scroll Y
; the play field's.
; Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
; play field's scroll Y; N and Z from it; C the bit shifted out of scroll
; X; X, Y and the other flags unchanged.
%at($81C7D4)
ParallaxHalfX:
    lda.w !layer0+!LAYER_SCROLL_X
    lsr a
    sta.w !layer1+!LAYER_SCROLL_X
    lda.w !layer0+!LAYER_SCROLL_Y
    sta.w !layer1+!LAYER_SCROLL_Y
    rts

; @routine 81:C7E7 ParallaxDrift
; WHAT IT DOES: Scrolls a backdrop of 11 horizontal bands, each drifting
; at its own speed, one band (the tenth) fixed to the play field.
; HOW: Each band's position steps by its speed; the tenth's becomes the
; play field's scroll X. The positions go into the bands' HDMA table, and
; the first band's into the backdrop's scroll.
; Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M 16-bit; A = the first band's position; X = 44, Y = 33; $000C =
; 0; N and Z from DB; DB unchanged; C from the last step's high word.
%at($81C7E7)
ParallaxDrift:
    ldx.w #0
    ldy.w #11
.step:
    clc
    lda.l !band_pos,x
    adc.l !band_speed,x
    sta.l !band_pos,x
    lda.l !band_pos+2,x
    adc.w #0
    sta.l !band_pos+2,x
    inx
    inx
    inx
    inx
    dey
    bne .step
    lda.w !layer0+!LAYER_SCROLL_X
    sta.l !band_pos+(9*4)+1
    ldx.w #0
    txy
    sep.b #$20
    phb
    lda.b #!band_hdma>>16
    pha
    plb
    rep.b #$20
    lda.w #11
    sta.l !scratch
.table:
    lda.w !band_pos+1,x
    sta.w !band_hdma+1,y
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    lda.l !scratch
    dec a
    sta.l !scratch
    bne .table
    lda.l !band_pos+1
    sta.w !bg2_x
    plb
    rts
