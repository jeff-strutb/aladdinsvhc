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
incsrc "../include/actor.inc"

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

; The handlers by !parallax_kind (words, this bank), and the backdrop
; bands' sets: per set its bands' count (times 2, words), where its edges
; start in !sband_edges (words); !sband_edges (its first two words those
; starts) lists each set's lines (y in the level) where a band starts,
; closed by $7FFF; the two HDMA tables BandTable writes in turn (words).
; All in bank $80 but the first.
!parallax_handlers = $81C781
!sband_sets_count = $80C602
!sband_edges = $80C606
!sband_tables = $80C628
!parallax_kind = $083F           ; the level's handler, times 2 (word)
!backdrop_stage = $0841          ; PlayField3Stages's stage, times 2 (word)

; @routine 81:C772 ParallaxUpdate
; WHAT IT DOES: Sets the backdrop's scroll from the play field's in the
; way the level asks: runs the level's parallax handler.
; HOW: JSR through PARALLAX_HANDLERS by PARALLAX_KIND; then 8-bit
; registers.
; Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the handler leaves A and the flags; M and X 8-bit (X and Y's
; high bytes cleared).
%at($81C772)
ParallaxUpdate:
    ldx.w !parallax_kind
    jsr.w (!parallax_handlers,x)
    sep.b #$30
    rts

; @routine 81:C77B ParallaxUpdateLong
; WHAT IT DOES: ParallaxUpdate for callers in other banks.
; HOW: 16-bit registers, ParallaxUpdate, RTL.
; Entry: DB a bank whose $0000-$1FFF is work RAM; called by JSL.
; Exit: as ParallaxUpdate.
ParallaxUpdateLong:
    rep.b #$30
    jsr.w ParallaxUpdate
    rtl

; @routine 81:C7E2 BandTableSet2
; WHAT IT DOES: BandTable for band set 2: four drifting bands over two.
; HOW: Into BandTable past its first load, with A = 2.
; Entry: as BandTable. Exit: as BandTable.
%at($81C7E2)
BandTableSet2:
    lda.w #2
    bra BandTable_set

; @routine 81:C848 BandTable
; WHAT IT DOES: Scrolls a backdrop of horizontal bands, each at its own
; speed (band set 0): writes the bands' HDMA table for this frame.
; HOW: SBAND_SET = the set; the backdrop's scroll Y the play field's;
; SBAND_LINE = the scroll Y + 8, SBAND_COUNT the set's bands; BandScrolls.
; Then in the other of SBAND_TABLES than last frame's (SBAND_BUFFER), after
; a 3-byte head: for each band the screen's line reaches, an entry of its
; lines (to its lower edge, at most the 224 left; two entries past 127)
; and its scroll; a 0 to end. The head: one line at the top band's
; scroll, which BG2's scroll X takes too. The backdrop's scroll X is the
; play field's halved; HDMA_TABLE_AT (channel 4) the table.
; Entry: M 16-bit, X 16-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A = the backdrop's scroll X, C the bit shifted out; N and Z from
; DB (PLB); V from the last add or subtract; X = the last band's edge
; index; Y = the table; DB unchanged.
%at($81C848)
BandTable:
    lda.w #0
.set:
    sta.l !sband_set
    sep.b #$20
    phb
    lda.b #!sband_set>>16
    pha
    plb
    rep.b #$20
    lda.l !layer0+!LAYER_SCROLL_Y
    sta.l !layer1+!LAYER_SCROLL_Y
    clc
    adc.w #8
    sta.w !sband_line
    ldx.w !sband_set
    lda.l !sband_sets_count,x
    lsr a
    sta.w !sband_count
    jsr.w BandScrolls
    lda.l !sband_buffer               ; the other table than last frame's
    eor.w #2
    sta.l !sband_buffer
    tax
    lda.l !sband_tables,x
    tay
    phy
    lda.w #224
    sta.w !sband_lines_left
    ldx.w !sband_set
    lda.l !sband_edges,x
    sta.w !sband_edge
    stz.w !sband_at
.band:
    ldx.w !sband_edge
    lda.w !sband_line
    cmp.l !sband_edges,x
    bcc .next
    cmp.l !sband_edges+2,x
    bcs .next
    sec                             ; the line is in this band: its lines
    lda.l !sband_edges+2,x
    sbc.w !sband_line
    bne .lines
    lda.w #1                        ; none: one (no script meets that)
.lines:
    cmp.w !sband_lines_left
    bcc .fits
    lda.w !sband_lines_left
.fits:
    sta.w !sband_lines
    ldx.w !sband_at
    cmp.w #128
    bcs .two
    sta.w 3,y                       ; an entry: lines, scroll
    lda.w !sband_scrolls,x
    sta.w 4,y
    clc
    tya
    adc.w #3
    tay
    bra .entered
.two:
    sbc.w #127                      ; past 127 lines: two entries
    sta.w 3,y
    lda.w #127
    sta.w 6,y
    lda.w !sband_scrolls,x
    sta.w 4,y
    sta.w 7,y
    clc
    tya
    adc.w #6
    tay
.entered:
    ldx.w !sband_edge
    lda.l !sband_edges+2,x
    sta.w !sband_line
    clc
    lda.w !sband_lines_left
    sbc.w !sband_lines
    bcc .done                       ; the screen is full
    inc a
    sta.w !sband_lines_left
.next:
    inc.w !sband_edge
    inc.w !sband_edge
    inc.w !sband_at
    inc.w !sband_at
    dec.w !sband_count
    bne .again
.done:
    lda.w #0                        ; the end mark
    sta.w 3,y
    ply
    lda.w #1                        ; the head: one line at the top band's scroll
    sta.w 0,y
    lda.w 4,y
    sta.w 1,y
    sta.l !bg2_x
    lda.l !layer0+!LAYER_SCROLL_X
    lsr a
    sta.l !layer1+!LAYER_SCROLL_X
    plb
    sty.w !hdma_table_at
    rts
.again:
    jmp.w .band

; @routine 81:C948 BandScrolls
; WHAT IT DOES: Works out each backdrop band's scroll X for this frame, by
; the band set: set 0 halves the scroll once more per band upward, with
; two drifting bands of cloud on top; set 2 has four drifting bands that
; the play field's moves right hold back, over two at half and three
; quarters of its speed.
; HOW: JMP through its own table by SBAND_SET. Both add $C0 to BAND_DRIFT (24
; bits, from its fraction byte). Set 0: SBAND_SCROLLS from the bottom band
; up, the scroll X shifted right once more each; the top two, BAND_DRIFT's
; pixels plus the scroll X, shifted right once and twice. Set 2: BAND_DRIFT's
; pixels less the scroll X's move right this frame (none if it moved
; left), plus the scroll X, shifted right 0-3 times for the top four;
; then the scroll X halved, and plus its quarter.
; Entry: M 16-bit, X 16-bit; DB = $7E; SBAND_SET 0 or 2; SBAND_COUNT the
; set's bands. Exit: set 0: A = the second band's scroll, N clear, Z from
; it, C the bit shifted out, V from the drift's sum; X = 0, Y = 0. Set
; 2: A = the last band's scroll; N, Z, C, V from its add; X = 2; Y as at
; entry; SCRATCH = the scroll X's quarter.
BandScrolls:
    ldx.w !sband_set
    jmp.w (.sets,x)
.sets:
    dw .halving, .drifting
.halving:
    lda.w !sband_count
    asl a
    tay
    lda.l !layer0+!LAYER_SCROLL_X
.halve:
    lsr a
    sta.w !sband_scrolls-2,y
    dey
    dey
    bne .halve
    clc
    lda.l !band_drift
    adc.w #$00C0
    sta.l !band_drift
    lda.l !band_drift+2
    adc.w #0
    sta.l !band_drift+2
    clc
    lda.l !band_drift+1
    adc.l !layer0+!LAYER_SCROLL_X
    lsr a
    sta.w !sband_scrolls
    lsr a
    sta.w !sband_scrolls+2
    rts
.drifting:
    clc
    lda.l !band_drift
    adc.w #$00C0
    sta.l !band_drift
    lda.l !band_drift+2
    adc.w #0
    sta.l !band_drift+2
    sec                             ; held back by the move right
    lda.l !layer0+!LAYER_SCROLL_X
    sbc.l !scroll_x_before
    bpl .moved
    lda.w #0
.moved:
    sta.l !scratch
    sec
    lda.l !band_drift+1
    sbc.l !scratch
    sta.l !band_drift+1
    clc
    lda.l !band_drift+1
    adc.l !layer0+!LAYER_SCROLL_X
    sta.w !sband_scrolls
    lsr a
    sta.w !sband_scrolls+2
    lsr a
    sta.w !sband_scrolls+4
    lsr a
    sta.w !sband_scrolls+6
    lda.l !layer0+!LAYER_SCROLL_X
    pha
    lsr a
    sta.w !sband_scrolls+8
    lsr a
    sta.l !scratch
    pla
    clc
    adc.l !scratch
    sta.w !sband_scrolls+10
    rts

; @routine 81:C9F2 PlayField3Stages
; WHAT IT DOES: The backdrop of a level that changes as the player goes:
; stage 0 follows the play field with an offset down until the player
; reaches its end (x $0F10) on firm ground, or the scroll passes $0F80;
; stage 1 eases the backdrop's y back to 0, at most 8 a frame; stage 2
; scrolls it across with the play field, at half speed past $1000.
; HOW: JMP through its own table by BACKDROP_STAGE. Stage 0:
; BackdropFollowOffsetY; with the player's x at STAGE1_X or more, the
; scroll X at STAGE1_SCROLL or more, or PlayerOnFirmGround (8-bit) C
; set, BACKDROP_STAGE + 2. Stage 1: with the backdrop's scroll Y 0, the
; next stage; else it less itself clamped to -8..8, then the x as stage
; 2. Stage 2: the backdrop's scroll X the play field's, past WIDE_X half
; of what is past it. Only stage 0 up to its x test is run by a script.
; Entry: M 16-bit, X 16-bit; DB = $80 (work RAM). Exit: M 16-bit, X
; 16-bit; A and flags as the last step left them.
PlayField3Stages:
    ldx.w !backdrop_stage
    jmp.w (.stages,x)
.stages:
    dw .follow, .ease, .wide
.follow:
    jsr.w BackdropFollowOffsetY
    lda.w !player+!actor_x+1
    cmp.w #$0F10
    bcc .stay
    lda.w !layer0+!LAYER_SCROLL_X
    cmp.w #$0F80
    bcs .on
    sep.b #$30
    jsl.l PlayerOnFirmGround
    rep.b #$30
    bcc .stay
.on:
    inc.w !backdrop_stage
    inc.w !backdrop_stage
.stay:
    rts
.ease:
    lda.w !layer1+!LAYER_SCROLL_Y
    beq .eased
    bmi .up
    cmp.w #8
    bcc .step
    lda.w #8
    bra .step
.up:
    cmp.w #-8
    bcs .step
    lda.w #-8
.step:
    eor.w #$FFFF
    inc a
    clc
    adc.w !layer1+!LAYER_SCROLL_Y
    sta.w !layer1+!LAYER_SCROLL_Y
    jsr.w .wide_x
    rts
.eased:
    inc.w !backdrop_stage
    inc.w !backdrop_stage
    rts
.wide:
    jsr.w .wide_x
    rts
.wide_x:
    sec
    lda.w !layer0+!LAYER_SCROLL_X
    cmp.w #$1000
    bcc .narrow
    sbc.w #$1000
    lsr a
    clc
    adc.w #$1000
.narrow:
    sta.w !layer1+!LAYER_SCROLL_X
    rts
