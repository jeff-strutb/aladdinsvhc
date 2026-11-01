; camera.asm: where the play field may scroll: empty blocks stop it.
;
; The level's block map marks the blocks the camera may not show with a
; negative entry. After the scroll moves, the screen's leading edge is
; checked against the map; over an empty block the scroll goes back to
; the block's edge.
; C: src/aladdin/video/camera.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_video.inc"

arch 65816

!scroll_x = !layer0+!LAYER_SCROLL_X
!scroll_y = !layer0+!LAYER_SCROLL_Y

; @routine 81:CA84 CameraStopAcross
; WHAT IT DOES: Stops the play field scrolling into an empty block across:
; after a move left or right, when the screen's leading edge is over an
; empty block, the scroll goes back to the block's edge.
; HOW: Moving left, the blocks at the screen's left edge at its top and
; bottom rows; moving right (or not at all), at its right edge (255
; pixels on). Over an empty block the scroll X becomes the next block's
; start (left) or its block's start (right), and the stopped flag is set.
; Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A, X, Y and the flags as the last check or stop leaves them.
%at($81CA84)
CameraStopAcross:
    stz.w !camera_stopped_x
    lda.w !scroll_x
    cmp.w !scroll_x_before
    bcs .right
    ldy.w !scroll_y_before
    jsr.w BlockAt
    bmi .stop_left
    tya
    adc.w #$00FF
    tay
    lda.w !scroll_x
    jsr.w BlockAt
    bpl .done
.stop_left:
    lda.w !scroll_x+1
    inc a
    sta.w !scroll_x+1
    stz.w !scroll_x-1
    inc.w !camera_stopped_x
    rts
.right:
    clc
    lda.w !scroll_x
    adc.w #$00FF
    ldy.w !scroll_y_before
    jsr.w BlockAt
    bmi .stop_right
    tya
    clc
    adc.w #$00FF
    tay
    clc
    lda.w !scroll_x
    adc.w #$00FF
    jsr.w BlockAt
    bpl .done
.stop_right:
    stz.w !scroll_x-1
    inc.w !camera_stopped_x
.done:
    rts

; @routine 81:CADA CameraStopUpDown
; WHAT IT DOES: Stops the play field scrolling into an empty block up or
; down: after the move, when the screen's leading edge is over an empty
; block, the scroll goes back to the block's edge.
; HOW: Moving up, the blocks at the screen's top edge at its left and
; right columns; moving down (or not at all), at its bottom edge (255
; pixels down). Over an empty block the scroll Y becomes the next block's
; start (up) or its block's start (down), and the stopped flag steps on.
; Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A, X, Y and the flags as the last check or stop leaves them.
CameraStopUpDown:
    ldy.w !scroll_y
    cpy.w !scroll_y_before
    bcs .down
    lda.w !scroll_x
    jsr.w BlockAt
    bmi .stop_up
    clc
    lda.w !scroll_x
    adc.w #$00FF
    jsr.w BlockAt
    bpl .done
.stop_up:
    tya
    xba
    inc a
    sta.w !scroll_y+1
    stz.w !scroll_y-1
    inc.w !camera_stopped_y
    rts
.down:
    tya
    clc
    adc.w #$00FF
    tay
    lda.w !scroll_x
    jsr.w BlockAt
    bmi .stop_down
    clc
    lda.w !scroll_x
    adc.w #$00FF
    jsr.w BlockAt
    bpl .done
.stop_down:
    stz.w !scroll_y-1
    inc.w !camera_stopped_y
.done:
    rts

; @routine 81:CB24 BlockAt
; WHAT IT DOES: Gives the block map entry of the 256-pixel block at a
; position in the level (negative: an empty block).
; HOW: The block's column from the X position's high byte, its row from
; the Y position's, into the map inside its border.
; Entry: M 16-bit, X 16-bit; A = X position; Y = Y position. Exit: A =
; the entry; N and Z from it; X = its offset; C = Y bit 1; $0010 = the
; column part; Y and the other flags unchanged.
BlockAt:
    xba
    asl a
    and.w #$003E
    sta.w !scratch_10
    tya
    lsr a
    lsr a
    and.w #$03C0
    ora.w !scratch_10
    tax
    lda.l !block_map+66,x
    rts
