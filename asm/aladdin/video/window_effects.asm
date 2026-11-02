; window_effects.asm: the windows some screens draw shapes with by HDMA:
; a channel started with its table at the table area's base, the window
; and colour math shadows set for a frame, a shape narrowed round its
; middle, a half turn of sines, and a window box round a point in the
; level.
; C: src/aladdin/video/window_effects.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; HdmaStart ($82:873B), not yet filed, takes these: the place of the
; channel's table from $7E:F400 and a value added to the table's values.
!hdma_value_add = $0030         ; added to each entry's value (word)
!hdma_table_place = $0032       ; the table's place from $7E:F400 (word)

; WindowLinesFromShape's inputs and working values.
!window_shift = $0030           ; added to both edges
!window_half = $0032            ; the most either side of a line's middle
!window_lines = $0033           ; lines left to write
!shape_left = $002C             ; the shape's left edge on the line
!shape_right = $002D            ; its right edge
!shape_middle = $002E           ; its middle
!window_shape = $BBAE           ; data bank $82: the offset of its last pair (word), then left, right pairs
!window_edges = $7EF800         ; the window's edges per line, left then right (the RAM band_pos names)

; HalfSineBuild's table, and the sines it takes the high bytes of.
!sine_bytes = $7F5600           ; 128 sine bytes (a half turn), then their complements
!rotate_sine_high = $87D4       ; data bank $82: the high byte of the first of the sine words at $82:87D3

; WindowBoxAt's point, its place on the screen, and the table it adds to.
!box_x = !scratch               ; the box's left in the level (word)
!box_y = !scratch+2             ; its top (word)
!box_across = $002C             ; its left less the scroll (word)
!box_down = $002E               ; its top less the scroll (word)
!box_table = $7F5600            ; the HDMA table being built: count, then left and right
; The box's off exits go to WindowBoxAt-2: SEC, RTS at the end of $82:8388.
!scroll_x = !layer0+!LAYER_SCROLL_X
!scroll_y = !layer0+!LAYER_SCROLL_Y

; @routine 82:8735 HdmaStartAtBase
; WHAT IT DOES: Starts HDMA table set X on register set Y with its table
; at the base of the table area, $7E:F400.
; HOW: The table's place ($0032) cleared, then on into HdmaStart
; ($82:873B), which takes a free channel, points it at the registers,
; and builds the table there from set X.
; Entry: M 8-bit, X 8-bit; X = the table set; Y = the register set; DB =
; $82; called by JSR. Exit: as HdmaStart: C clear; $002C = the channel's
; HDMAEN bit.
%at($828735)
HdmaStartAtBase:
    stz.w !hdma_table_place
    stz.w !hdma_table_place+1


; @routine 82:8226 WindowColoursWait
; WHAT IT DOES: Sets the window and colour math shadows the window effects
; use (BG1 and BG2 masked on the main screen, the fixed colour added to
; the backdrop), then waits a frame.
; HOW: TMW 3, TSW 0, WOBJSEL $30, CGWSEL $20, CGADSUB $20 and the fixed
; colour (red 6, green 3, blue 22) into their shadows; TaskWait 1.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR
; from a task. Exit: as TaskWait, when the task resumes.
%at($828226)
WindowColoursWait:
    lda.b #$03
    sta.w !tmw_shadow
    stz.w !tsw_shadow
    lda.b #$30
    sta.w !wobjsel_shadow
    lda.b #$20
    sta.w !cgwsel_shadow
    lda.b #$20
    sta.w !cgadsub_shadow
    lda.b #$06
    sta.w !coldata_red
    lda.b #$03
    sta.w !coldata_green
    lda.b #$16
    sta.w !coldata_blue
    lda.b #$01
    jsl.l TaskWait
    rts


; @routine 82:8253 WindowLinesFromShape
; WHAT IT DOES: Writes the window's left and right edges for up to $0033
; lines, from the bottom up: the shape in the table at $82:BBAE narrowed
; to at most $0032 pixels either side of each line's middle, moved right
; by $0030.
; HOW: From the shape's last pair (the table's first word is its offset)
; and the edges at X, back two bytes a line: the middle of the shape's
; left and right; the left edge the larger of the shape's left and the
; middle less the half width, the right edge the smaller of its right and
; the middle plus it, each plus the shift, to $7E:F800 + X and + X + 1.
; Ends when X goes negative, the shape's offset reaches 0 or the line
; count runs out.
; Entry: X 16-bit; X = the offset of the bottom line's edges; DB = $82;
; called by JSR. Exit: M 8-bit; $002C, $002D = the last shape pair used,
; $002E its middle; X, Y past it.
%at($828253)
WindowLinesFromShape:
    ldy.w !window_shape
.line:
    sep.b #$20
    lda.w !window_shape,y
    sta.w !shape_left
    lda.w !window_shape+1,y
    sta.w !shape_right
    sec
    sbc.w !shape_left
    lsr a
    clc
    adc.w !shape_left
    sta.w !shape_middle
    sec
    sbc.w !window_half
    cmp.w !shape_left
    bcs .left
    lda.w !shape_left
.left:
    clc
    adc.w !window_shift
    sta.l !window_edges,x
    clc
    lda.w !shape_middle
    adc.w !window_half
    cmp.w !shape_right
    bcc .right
    lda.w !shape_right
.right:
    clc
    adc.w !window_shift
    sta.l !window_edges+1,x
    dex
    dex
    bmi .done
    dey
    dey
    beq .done
    dec.w !window_lines
    bne .line
.done:
    sep.b #$20
    rts


; @routine 82:82AB HalfSineBuild
; WHAT IT DOES: Builds at $7F:5600 a half turn of sines as bytes (64
; rising, then the same falling) and at $7F:5680 their complements.
; HOW: With DB = $82, for each of 64 entries the high byte of the next
; sine word at $82:87D3 goes to entry x and to entry 63 - x of the second
; quarter ($7F:5640), and EORed with $FF to the same entries $80 on.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: DB = $82; X = $40; Y =
; $80; C set, Z set.
%at($8282AB)
HalfSineBuild:
    lda.b #$82
    pha
    plb
    ldx.b #$00
    ldy.b #$00
.entry:
    lda.w !rotate_sine_high,y
    sta.l !sine_bytes,x
    eor.b #$FF
    sta.l !sine_bytes+128,x
    phx
    txa
    eor.b #$3F
    tax
    lda.w !rotate_sine_high,y
    sta.l !sine_bytes+64,x
    eor.b #$FF
    sta.l !sine_bytes+192,x
    plx
    iny
    iny
    inx
    cpx.b #$40
    bcc .entry
    rts


; @routine 82:8417 WindowBoxAt
; WHAT IT DOES: Adds to the HDMA table being built at $7F:5600 the lines
; of a window 128 pixels wide and up to 96 lines tall whose top left is
; the level point in $000C, $000E; C set, nothing added, when the box is
; off the screen.
; HOW: The point less the play field's scroll ($002C across, $002E down);
; off when across is outside -128 to 255 or down outside -96 to 223.
; Entries of 3 bytes from X, a line count then the window's left and
; right: lines above the box (down above 0) as entries of no window (left
; $FF, right 0), two when more than 127; then the box's lines (96, fewer
; at the screen's bottom, or down + 97 when it starts above the screen)
; with the window from across to across + 128, clipped to 0 and 255.
; Entry: M 16-bit, X 8-bit; X = the next entry's offset; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: added: C clear, X past
; the entries; off: C set (at $82:8415).
%at($828417)
WindowBoxAt:
    sec
    lda.w !box_x
    sbc.w !scroll_x
    sta.w !box_across
    cmp.w #$0100
    bcc .across
    clc
    adc.w #$0080
    cmp.w #$0100
    bcs WindowBoxAt-2
.across:
    sec
    lda.w !box_y
    sbc.w !scroll_y
    sta.w !box_down
    cmp.w #$00E0
    bcc .down
    clc
    adc.w #$0060
    cmp.w #$00E0
    bcs WindowBoxAt-2
.down:
    lda.w !box_down
    bmi .from_top
    beq .from_top
    cmp.w #$0080
    bcc .above
    sbc.w #$007F
    sta.l !box_table,x
    lda.w #$00FF
    sta.l !box_table+1,x
    inx
    inx
    inx
    lda.w #$007F
.above:
    sta.l !box_table,x
    lda.w #$00FF
    sta.l !box_table+1,x
    inx
    inx
    inx
    sec
    lda.w #$00E0
    sbc.w !box_down
    cmp.w #$0060
    bcc .lines
    lda.w #$0060
    bra .lines
.from_top:
    clc
    lda.w !box_down
    adc.w #$0061
.lines:
    sta.l !box_table,x
    lda.w !box_across
    bpl .on_left
    clc
    adc.w #$0080
    xba
    and.w #$FF00
    bra .window
.on_left:
    sec
    lda.w #$0100
    sbc.w !box_across
    cmp.w #$0081
    bcc .to_edge
    sep.b #$20
    clc
    lda.w !box_across
    adc.b #$80
    xba
    lda.w !box_across
    rep.b #$20
    bra .window
.to_edge:
    lda.w !box_across
    ora.w #$FF00
.window:
    sta.l !box_table+1,x
    inx
    inx
    inx
    clc
    rts
