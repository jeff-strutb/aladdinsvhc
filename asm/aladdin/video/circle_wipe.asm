; circle_wipe.asm: the circle that closes in on a point (the wipe at a
; level's end), drawn as the window's left and right edge on each scanline.
; C: src/aladdin/video/circle_wipe.c.

incsrc "../include/macros.inc"

arch 65816

; The circle, drawn by the midpoint method an octant at a time.
!circle_x = $002C               ; octant's x, from the radius down (word)
!circle_y = $002E               ; octant's y, from 0 up (word)
!circle_cx = $0032              ; centre's screen x (word)
!circle_cy = $0034              ; centre's scanline (word)
; The plot's working bytes.
!span_x = $000C                 ; centre x - x, centre x + x (clamped)
!span_y = $000E                 ; centre x - y, centre x + y
!row_up_x = $0014               ; scanline centre - x, then its table offset (word)
!row_down_x = $0016             ; scanline centre + x ($FF off screen), then its offset
!row_up_y = $0018               ; scanline centre - y, then its offset
!row_down_y = $001A             ; scanline centre + y, then its offset
!window_edges = $7F5000         ; window edges per scanline: left, right

; @routine 81:B2D9 CircleWipePlot
; WHAT IT DOES: Puts one step of the circle into the window table: its
; eight symmetric points as the left and right edges of four scanlines.
; HOW: Clamps centre x -/+ x and -/+ y to 0..$FF, and the scanlines centre
; -/+ x and -/+ y to $FF when past the screen (scanline $FF is outside the
; picture); doubles each scanline into a word offset and stores the -/+ y
; span at scanlines centre -/+ x, the -/+ x span at centre -/+ y.
; Entry: DB a bank whose $0000-$1FFF is work RAM; called by JSR
; (CircleWipeStep). Exit: M 16-bit, X 16-bit; A = the -/+ x span (word),
; X = scanline centre + y's offset; N and Z from the last load; C as the
; last clamp left it.
%at($81B2D9)
CircleWipePlot:
    sep.b #$30
    sec
    lda.w !circle_cx
    sbc.w !circle_x
    bcs .bB2E6
    lda.b #$00
.bB2E6:
    sta.w !span_x
    clc
    lda.w !circle_cx
    adc.w !circle_x
    bcc .bB2F4
    lda.b #$FF
.bB2F4:
    sta.w !span_x+1
    sec
    lda.w !circle_cx
    sbc.w !circle_y
    bcs .bB302
    lda.b #$00
.bB302:
    sta.w !span_y
    clc
    lda.w !circle_cx
    adc.w !circle_y
    bcc .bB310
    lda.b #$FF
.bB310:
    sta.w !span_y+1
    sec
    lda.w !circle_cy
    sbc.w !circle_x
    bcs .bB31E
    lda.b #$FF
.bB31E:
    sta.w !row_up_x
    clc
    lda.w !circle_cy
    adc.w !circle_x
    bcc .bB32C
    lda.b #$FF
.bB32C:
    sta.w !row_down_x
    sec
    lda.w !circle_cy
    sbc.w !circle_y
    bcs .bB33A
    lda.b #$FF
.bB33A:
    sta.w !row_up_y
    clc
    lda.w !circle_cy
    adc.w !circle_y
    bcc .bB348
    lda.b #$FF
.bB348:
    sta.w !row_down_y
    stz.w !row_up_x+1
    stz.w !row_down_x+1
    stz.w !row_up_y+1
    stz.w !row_down_y+1
    rep.b #$30
    asl.w !row_up_x
    asl.w !row_down_x
    asl.w !row_up_y
    asl.w !row_down_y
    ldx.w !row_up_x
    lda.w !span_y
    sta.l !window_edges,x
    ldx.w !row_down_x
    lda.w !span_y
    sta.l !window_edges,x
    ldx.w !row_up_y
    lda.w !span_x
    sta.l !window_edges,x
    ldx.w !row_down_y
    lda.w !span_x
    sta.l !window_edges,x
    rts
