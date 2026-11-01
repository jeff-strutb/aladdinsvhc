; bg_follow.asm: the BG positions taken from the scroll layers each frame.
; C: src/aladdin/video/bg_follow.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!BG2_FOLLOWS_PLAY_X = $80
!BG2_FOLLOWS_BACK_Y = $40

; @routine 81:8568 BgFollowLayers
; WHAT IT DOES: Takes this frame's BG1 and BG2 positions from the scroll
; layers, as the level sets them to follow.
; HOW: BG1 takes the play field's x and y unless held. BG2 by its follow
; mode: $80, the play field's x; $40, the backdrop's y; 0, the backdrop's
; x and y; anything else, neither.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR. Exit: M and X 8-bit; X = the BG2 mode; A = the last
; position copied, else as the play field's y or the entry's; N, Z from
; the last load or compare; C from the mode compares; Y = the mode when
; neither $80 nor $40, else unchanged; V unchanged.
%at($818568)
BgFollowLayers:
    rep.b #$20
    ldx.w !bg1_held
    bne .bg2
    lda.w !layer0+!LAYER_SCROLL_X
    sta.w !bg1_x
    lda.w !layer0+!LAYER_SCROLL_Y
    sta.w !bg1_y
.bg2:
    ldx.w !bg2_follows
    cpx.b #!BG2_FOLLOWS_PLAY_X
    beq .play_x
    cpx.b #!BG2_FOLLOWS_BACK_Y
    beq .back_y
    txy
    bne .done
    lda.w !layer1+!LAYER_SCROLL_X
    sta.w !bg2_x
.back_y:
    lda.w !layer1+!LAYER_SCROLL_Y
    sta.w !bg2_y
.done:
    sep.b #$30
    rts
.play_x:
    lda.w !layer0+!LAYER_SCROLL_X
    sta.w !bg2_x
    sep.b #$30
    rts
