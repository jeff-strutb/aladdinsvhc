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


; More places not yet named.
!dp_06 = $06  ; direct page
!dp_07 = $07  ; direct page
!routine_828017 = $828017  ; not yet filed
!t_bca1 = $BCA1  ; data bank
!t_bca9 = $BCA9  ; data bank
!t_bcaa = $BCAA  ; data bank
!t_bcbb = $BCBB  ; data bank
!t_bcbc = $BCBC  ; data bank
!t_bcbd = $BCBD  ; data bank
!v_002c = $002C
!v_002d = $002D
!v_002e = $002E
!v_0030 = $0030
!v_0031 = $0031
!v_0032 = $0032
!v_0033 = $0033
!v_00bd = $00BD
!v_00c1 = $00C1
!v_0351 = $0351
!v_0399 = $0399
!v_03a6 = $03A6
!v_03a7 = $03A7
!v_03a8 = $03A8
!v_03a9 = $03A9
!v_03aa = $03AA
!v_03ab = $03AB
!v_03b5 = $03B5
!v_1705 = $1705
!v_1707 = $1707
!v_1709 = $1709
!v_170b = $170B
!v_1773 = $1773
!v_1777 = $1777
!v_7e0000 = $7E0000
!v_7e0001 = $7E0001
!v_7e0002 = $7E0002
!v_7ef403 = $7EF403
!v_7ef480 = $7EF480
!v_7ef481 = $7EF481
!v_7ef483 = $7EF483
!v_7ef486 = $7EF486
!v_7ef490 = $7EF490
!v_7ef492 = $7EF492
!v_7ef493 = $7EF493
!v_7ef500 = $7EF500
!v_7ef501 = $7EF501
!v_7ef510 = $7EF510
!v_7ef511 = $7EF511
!v_7ef512 = $7EF512
!v_7ef513 = $7EF513
!v_7ef520 = $7EF520
!v_7ef523 = $7EF523
!v_7ef532 = $7EF532
!v_7ef533 = $7EF533
!v_7f5600 = $7F5600
!v_7f5601 = $7F5601
!v_7f5603 = $7F5603

; @routine 82:871B HdmaChannelFree
; WHAT IT DOES: Finds a free HDMA channel among 4-7, from 7 down: C clear
; and X = the channel, or C set when all four are in use.
; HOW: The high nibble of HDMAEN's shadow ORed with $03B5; all set: C set.
; Else shifted left until a clear bit, X stepping down from 7.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSR. Exit: X = the channel (or 3 or 4 when none); A shifted.
%at($82871B)
HdmaChannelFree:
    ldx.b #$07
    lda.w !hdmaen_shadow
    ora.w !v_03b5
    and.b #$F0
    cmp.b #$F0
    beq .l8731
.l8729:
    asl a
    bcc .l8733
    dex
    cpx.b #$04
    bcs .l8729
.l8731:
    sec
    rts
.l8733:
    clc
    rts


; @routine 82:873B HdmaStart
; WHAT IT DOES: Starts HDMA table set X on register set Y in a free
; channel, its table built at $7E:F400 plus $0032: C clear, $002C the
; channel's HDMAEN bit; C set when no channel is free.
; HOW: $0030 cleared; HdmaChannelFree. The channel's bit ($82:BCA1) at
; $002C, its register base * 16 at $002D; the table address ($F400 +
; $0032) into hdma_table_at for it; DMAP and BBAD from the register set
; ($82:BCA9), bank $7E. The set's entries ($82:BCBB: count, then a word)
; copied to the table, $0030 added to each word, until a count of 0.
; Entry: M 8-bit, X 8-bit; X = the table set; Y = the register set; $0032
; = the table's place; DB = $82; called by JSR. Exit: M 8-bit, X 8-bit; C
; as above.
%at($82873B)
HdmaStart:
    stz.w !v_0030
    stz.w !v_0031
    stx.w !v_002e
    jsr.w HdmaChannelFree
    bcc .l874A
    rts
.l874A:
    txa
    asl a
    asl a
    asl a
    asl a
    sta.w !v_002d
    lda.w !t_bca1,x
    sta.w !v_002c
    txa
    sec
    sbc.b #$04
    asl a
    tax
    clc
    lda.w !v_0032
    adc.b #$00
    sta.w !v_0032
    sta.w !hdma_table_at,x
    lda.w !v_0033
    adc.b #$F4
    sta.w !v_0033
    sta.w !v_0351,x
    ldx.w !v_002d
    lda.w !t_bca9,y
    sta.w !DMAP0,x
    lda.w !t_bcaa,y
    sta.w !BBAD0,x
    lda.b #$7E
    sta.w !A1B0,x
    sta.w !DASB0,x
    ldx.w !v_002e
    rep.b #$10
    ldy.w !t_bcbb,x
    ldx.w !v_0032
.l8797:
    lda.w !t_bcbb,y
    sta.l !v_7e0000,x
    beq .l87BD
    clc
    lda.w !t_bcbc,y
    adc.w !v_0030
    sta.l !v_7e0001,x
    lda.w !t_bcbd,y
    adc.w !v_0031
    sta.l !v_7e0002,x
    inx
    inx
    inx
    iny
    iny
    iny
    bra .l8797
.l87BD:
    sep.b #$10
    clc
    rts


; @routine 82:8000 WindowEffectTask
; WHAT IT DOES: Runs window effect $06 of its slot (its parameter) from
; the table at $82:8017, retrying each frame while the effect cannot
; start.
; HOW: D = the slot's page ($6F + Y); JSR through the table by the
; parameter; when it returns (no HDMA channel was free), TaskWait 1 and
; again.
; Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
; never returns (the effects end the task).
%at($828000)
WindowEffectTask:
    rep.b #$20
    clc
    tya
    adc.w #$006F
    tcd
    sep.b #$20
.l800A:
    ldx.b !dp_06
    jsr.w (!routine_828017,x)
    lda.b #$01
    jsl.l TaskWait
    bra .l800A


; @routine 82:80E2 HdmaEffect2
; WHAT IT DOES: Window effect 2: HDMA table set 4 on register set 6 from
; the table area's base, then the task ends.
; HOW: HdmaStartAtBase; no channel: back (to retry); else the channel on
; in HDMAEN's shadow and TaskExit.
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($8280E2)
HdmaEffect2:
    ldx.b #$04
    ldy.b #$06
    jsr.w HdmaStartAtBase
    bcc .l80EC
    rts
.l80EC:
    lda.w !v_002c
    tsb.w !hdmaen_shadow
    jml.l TaskExit


; @routine 82:82DB HdmaEffect4
; WHAT IT DOES: Window effect 4: HDMA table set 6 on register set 8 with
; its table $20 into the area, the value at $7E:F480 7, and the task left
; waiting for good.
; HOW: $0032 = $20; HdmaStart; no channel: back. $7E:F480 = 7, $7E:F481 =
; 0; the channel on; TaskWait 2 forever.
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($8282DB)
HdmaEffect4:
    lda.b #$20
    sta.w !v_0032
    stz.w !v_0033
    ldx.b #$06
    ldy.b #$08
    jsr.w HdmaStart
    bcc .l82ED
    rts
.l82ED:
    lda.b #$07
    sta.l !v_7ef480
    lda.b #$00
    sta.l !v_7ef481
    lda.w !v_002c
    tsb.w !hdmaen_shadow
.l82FF:
    lda.b #$02
    jsl.l TaskWait
    bra .l82FF


; @routine 82:8388 HdmaEffect8
; WHAT IT DOES: Window effect 8: windows round two places in the level
; ($0E40, $00F8 and $0D60, $01D8), rebuilt each frame and swapped between
; two tables.
; HOW: HdmaStartAtBase with set $0E on register set $0E; $00BD up; the
; channel on; its table address slot kept at $06 and the table at $07
; ($F400). Each frame: TaskWait 1; the table address set; WindowBoxAt for
; each place; an entry of 1 line of no window and the end; WH0's shadow
; from the first entry; the 16 bytes moved from $7F:5600 to the table, and
; the table swapped ($10 apart).
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($828388)
HdmaEffect8:
    ldx.b #$0E
    ldy.b #$0E
    jsr.w HdmaStartAtBase
    bcc .l8392
    rts
.l8392:
    inc.w !v_00bd
    lda.w !v_002c
    tsb.w !hdmaen_shadow
    lda.w !v_002d
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b #$FC
    asl a
    sta.b !dp_06
    tax
    rep.b #$20
    lda.w #$F400
    sta.b !dp_07
.l83B0:
    sep.b #$30
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    ldx.b !dp_06
    lda.b !dp_07
    sta.w !hdma_table_at,x
    ldx.b #$00
    lda.w #$0E40
    sta.w !scratch
    lda.w #$00F8
    sta.w !scratch+2
    jsr.w WindowBoxAt
    lda.w #$0D60
    sta.w !scratch
    lda.w #$01D8
    sta.w !scratch+2
    jsr.w WindowBoxAt
    lda.w #$0001
    sta.l !v_7f5600,x
    lda.w #$00FF
    sta.l !v_7f5601,x
    lda.w #$0000
    sta.l !v_7f5603,x
    lda.l !v_7f5601
    sta.w !v_03a6
    rep.b #$10
    phb
    ldx.w #$5600
    ldy.b !dp_07
    lda.w #$000F
    mvn $7E,$7F
    plb
    lda.b !dp_07
    eor.w #$0010
    sta.b !dp_07
    bra .l83B0


; @routine 82:84CB HdmaEffect10
; WHAT IT DOES: Window effect 10: a window and colour math round the place
; $0E78, $0144 in the level while it is on the screen; when $00C1 is set,
; the window closes in from both sides and the task ends.
; HOW: HdmaStart with set $10 on register set $0C at place $100; the
; channel on (its bit kept at $06); the masks cleared, WOBJSEL $C0; the
; table's 32 bytes at $7E:F500 kept at $F520. Each frame: the place less
; the scroll and the layer's shift: on the screen, the window's left and
; right (right clamped to 255) and its first line into the table and
; WH0/WH1's shadows, the fixed colour (8, 8, 16) added (CGADSUB $33), a
; WRAM move queued to restore the table; off, the colour math off. With
; $00C1 set, WH0 up and WH1 down a step a frame until they cross, then the
; colour math off, the channel off and TaskExit.
; Entry: M 8-bit, X 8-bit; D the task's page; DB = $82; called by JSR from
; WindowEffectTask. Exit: no channel: C set; else never returns.
%at($8284CB)
HdmaEffect10:
    lda.b #$01
    sta.w !v_0033
    stz.w !v_0032
    ldx.b #$10
    ldy.b #$0C
    jsr.w HdmaStart
    bcc .l84DD
    rts
.l84DD:
    lda.w !v_002c
    tsb.w !hdmaen_shadow
    sta.b !dp_06
    lda.b #$FF
    sta.l !v_7ef510
    inc a
    sta.l !v_7ef511
    stz.w !tmw_shadow
    stz.w !tsw_shadow
    stz.w !v_03aa
    stz.w !v_03ab
    lda.b #$C0
    sta.w !wobjsel_shadow
    phb
    rep.b #$10
    ldx.w #$001F
    txa
    ldx.w #$F500
    ldy.w #$F520
    mvn $7E,$7E
    sep.b #$10
    plb
.l8514:
    rep.b #$20
    sec
    lda.w #$0144
    sbc.w !v_1777
    sec
    sbc.w !layer_offset_y
    cmp.w #$00E0
    bcs .l85A1
    sta.w !v_002e
    sec
    lda.w #$0E78
    sbc.w !v_1773
    sec
    sbc.w !bg1_x_shift
    cmp.w #$0100
    bcs .l85A1
    sta.w !v_002c
    sep.b #$20
    clc
    adc.b #$10
    bcc .l8545
    lda.b #$FF
.l8545:
    sta.l !v_7ef533
    sta.w !v_03a9
    lda.w !v_002c
    sta.l !v_7ef532
    sta.w !v_03a8
    lda.w !v_002e
    lsr a
    adc.b #$00
    sta.l !v_7ef520
    sta.l !v_7ef523
    lda.b #$08
    sta.w !coldata_red
    sta.w !coldata_green
    asl a
    sta.w !coldata_blue
    lda.b #$20
    sta.w !cgwsel_shadow
    lda.b #$33
    sta.w !cgadsub_shadow
    rep.b #$20
    ldy.w !v_1705
    lda.w #$F520
    sta.w !v_1707,y
    lda.w #$F500
    sta.w !v_1709,y
    lda.w #$001F
    sta.w !v_170b,y
    sep.b #$20
    clc
    tya
    adc.b #$06
    sta.w !v_1705
    lda.w !v_00c1
    bne .l85CE
    bra .l85C5
.l85A1:
    sep.b #$20
    lda.b #$00
    sta.l !v_7ef500
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
    lda.b #$FF
    sta.w !v_03a8
    inc a
    sta.w !v_03a8
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$53
    sta.w !cgadsub_shadow
.l85C5:
    lda.b #$01
    jsl.l TaskWait
    jmp.w .l8514
.l85CE:
    lda.b #$01
    jsl.l TaskWait
    lda.w !v_03a8
    inc a
    sta.w !v_03a8
    sta.l !v_7ef512
    lda.w !v_03a9
    dec a
    sta.w !v_03a9
    sta.l !v_7ef513
    cmp.w !v_03a8
    bcs .l85CE
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$53
    sta.w !cgadsub_shadow
    lda.b !dp_06
    trb.w !hdmaen_shadow
    jml.l TaskExit


; @routine 82:860B HdmaEffect11
; WHAT IT DOES: Window effect 11: a window that narrows over the lines
; above $00F8 in the level, a step a frame, then the task ends.
; HOW: HdmaStart with set $12 on register set $0C at place $80; the
; channel on; WH2 and the table's window $00FF/$FF00; the line count
; ($00F8 less the scroll y, + $30). Each frame: TaskWait 1, the left edge
; up, the right edge down, the count down; at 0 TaskExit.
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($82860B)
HdmaEffect11:
    lda.b #$80
    sta.w !v_0032
    stz.w !v_0033
    ldx.b #$12
    ldy.b #$0C
    jsr.w HdmaStart
    bcc .l861D
    rts
.l861D:
    lda.w !v_002c
    tsb.w !hdmaen_shadow
    rep.b #$20
    lda.w #$00FF
    sta.l !v_7ef490
    sta.w !v_03a8
    lda.w #$FF00
    sta.l !v_7ef492
    sec
    lda.w #$00F8
    sbc.w !v_1777
    sep.b #$20
    clc
    adc.b #$30
    sta.l !v_7ef483
    sta.l !v_7ef486
.l864A:
    lda.b #$01
    jsl.l TaskWait
    lda.l !v_7ef480
    inc a
    sta.l !v_7ef480
    lda.l !v_7ef492
    inc a
    sta.l !v_7ef492
    eor.b #$FF
    sta.l !v_7ef493
    lda.l !v_7ef483
    dec a
    sta.l !v_7ef483
    sta.l !v_7ef486
    bne .l864A
    jml.l TaskExit


; @routine 82:867B HdmaEffect9
; WHAT IT DOES: Window effect 9: a window below the line $0200 in the
; level less the scroll (at most 240 lines down), kept each frame.
; HOW: HdmaStartAtBase with set $16 on register set $10; the table's first
; count $10; the channel on. Each frame: TaskWait 1; $0200 less the scroll
; y: past the screen or 0 lines, the first count 0; else half of it (at
; most 240) as the first two counts and $10 after.
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($82867B)
HdmaEffect9:
    ldx.b #$16
    ldy.b #$10
    jsr.w HdmaStartAtBase
    bcc .l8685
    rts
.l8685:
    lda.b #$10
    sta.l !v_7ef500
    lda.b #$00
    sta.l !v_7ef501
    lda.w !v_002c
    tsb.w !hdmaen_shadow
.l8697:
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    sec
    lda.w #$0200
    sbc.w !v_1777
    bcc .l86C5
    cmp.w #$00F0
    bcc .l86B0
    lda.w #$00F0
.l86B0:
    lsr a
    beq .l86C5
    sep.b #$20
    sta.l !band_hdma
    sta.l !v_7ef403
    lda.b #$10
    sta.l !v_7ef500
    bra .l8697
.l86C5:
    sep.b #$20
    lda.b #$00
    sta.l !v_7ef500
    bra .l8697


; @routine 82:86CF HdmaEffect12
; WHAT IT DOES: Window effect 12: HDMA table set $14 on register set 0
; from the base, with the layers' window masks and the windows set, then
; the task ends.
; HOW: HdmaStartAtBase; the table's first values from TM's shadow; the
; channel on; TMW and TSW $10, WH0 $38, WH1 $C7, WH2 and WH3 0, WOBJSEL 3;
; TaskExit.
; Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
; Exit: no channel: C set; else never returns.
%at($8286CF)
HdmaEffect12:
    ldx.b #$14
    ldy.b #$00
    jsr.w HdmaStartAtBase
    bcc .l86D9
    rts
.l86D9:
    lda.w !v_0399
    sta.l !v_7ef480
    and.b #$0F
    sta.l !v_7ef481
    lda.w !v_002c
    tsb.w !hdmaen_shadow
    lda.b #$10
    sta.w !tmw_shadow
    sta.w !tsw_shadow
    lda.b #$38
    sta.w !v_03a6
    lda.b #$C7
    sta.w !v_03a7
    stz.w !v_03aa
    stz.w !v_03ab
    lda.b #$03
    sta.w !wobjsel_shadow
    jml.l TaskExit
