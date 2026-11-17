; oam.asm: the sprite (OAM) image in work RAM.
;
; Each frame the game builds the 544-byte OAM image at $7E:FA80 (128
; sprites of 4 bytes, then the 32-byte high table) and the NMI copies it to
; the PPU. A sprite at y $E0 is off the screen.
; C: src/aladdin/video/oam.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

; The frame's high-table accumulator (direct page, D = 0).
!oam_high_bits = $61            ; the bits gathered for the next high-table byte (word)
!oam_high_count = $63           ; sprites left before it is full (word)

!OAM_Y_OFF = $E0                ; a y below the screen

; @routine 81:95D5 OamBufferClear
; WHAT IT DOES: Takes every sprite off the screen.
; HOW: Fills the 512 sprite bytes with $E0 (y below the screen) and the
; high table with 0, last byte first.
; Entry: M 8-bit. Exit: A low byte = 0; X = $FF (X 8-bit) or $FFFF; Y and
; the flags as at entry (P pulled back).
%at($8195D5)
OamBufferClear:
    php
    rep.b #$10
    ldx.w #511
    lda.b #!OAM_Y_OFF
.sprite:
    sta.l !oam_buffer,x
    dex
    bpl .sprite
    ldx.w #31
    lda.b #0
.high:
    sta.l !oam_high,x
    dex
    bpl .high
    plp
    rtl

; @routine 81:9230 OamBuildBegin
; WHAT IT DOES: Starts the frame's sprites: none used, all 128 free, the
; high table cleared.
; HOW: Clears the sprite and high-table offsets and $0857, sets the
; high-table accumulator to empty (8 sprites to go, no bits), clears the
; high table a word at a time and sets 128 free sprites.
; Entry: M 8-bit, X 8-bit; D = 0; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M 8-bit; A = $0080; X = $FE; N, Z clear; Y, C, V unchanged.
%at($819230)
OamBuildBegin:
    rep.b #$20
    stz.w !oam_0857
    stz.w !oam_next
    stz.w !oam_high_next
    lda.w #8
    sta.b !oam_high_count
    stz.b !oam_high_bits
    ldx.b #30
    lda.w #0
.high:
    sta.l !oam_high,x
    dex
    dex
    bpl .high
    lda.w #128
    sta.w !oam_free
    sep.b #$20
    rtl

; The sprites of the frame's actors (OamDrawActors, OamDrawActor)
!sprite_shift_at = $1A16         ; which of the layers' shifts the sprites take (a byte offset)
!sprite_shift_x = $1A18          ; the shifts, X then Y 4 bytes on, by !sprite_shift_at (words)
!sprite_shift_y = $1A1C
!sprite_origin_x = $1A38         ; the level's point at the screen's left, for sprites (word)
!sprite_origin_y = $1A3C         ; and at its top (word)
!draw_lists = $1492              ; four lists of the actors to draw, $40 bytes each (record addresses)
!draw_counts = $1592             ; each list's length (a byte, 2 apart)
!list_left = $0856               ; records left in the list being drawn
!v_032d = $032D
!actor_pieces = $22              ; the actor's sprite frame: its pieces' count, then 5 bytes a piece (word, bank $7E)
; Direct page 0 while the pieces go out
!pieces_left = $0C               ; the frame's pieces left (word)
!screen_x = $0E                  ; the actor's place on the screen (words)
!screen_y = $10
!piece_x = $12                   ; the piece's x: its high byte's bit 0 is x bit 8
!flip_bits = $1E                 ; the actor's flips, as the sprites' attribute bits
!piece_word = $20                ; the piece's tile word, shifted for its size bit
!tile_base = $26                 ; the actor's first tile (word)
!palette_bits = $2A              ; its palette, as attribute bits (word)

; @routine 81:9258 OamDrawActors
; WHAT IT DOES: Puts this frame's actors into the sprites, list by list
; (Aladdin after the first two), then takes the sprites left over off the
; screen.
; HOW: SPRITE_ORIGIN is the scroll plus the layers' shift SPRITE_SHIFT_AT
; names; DB = $7E; from OAM_NEXT, for each of the four DRAW_LISTS with a
; count, OamDrawList; Aladdin's record (state nonzero and ACTOR_08 bit 3)
; between the second and third; OAM_NEXT kept. The high table's last
; bits shifted down into place and written when the word is not full;
; every sprite left gets y $E1.
; Entry: M 8-bit, X 8-bit; D = 0 (the high table's accumulator); DB a
; bank whose $0000-$1FFF is work RAM; called by JSL.
; Exit: M 8-bit, X 8-bit; DB, D unchanged; A low byte = $E1; X = 0; C
; set; N and Z from DB.
%at($819258)
OamDrawActors:
    sep.b #$20
    ldx.w !sprite_shift_at
    rep.b #$30
    clc
    lda.w !layer0+!LAYER_SCROLL_X
    adc.w !sprite_shift_x,x
    sta.w !sprite_origin_x
    clc
    lda.w !layer0+!LAYER_SCROLL_Y
    adc.w !sprite_shift_y,x
    sta.w !sprite_origin_y
    sep.b #$20
    ldx.w !oam_next
    phb
    lda.b #!oam_buffer>>16
    pha
    plb
    phd
    lda.w !draw_counts
    beq .list1
    ldy.w #0
    jsr.w OamDrawList
.list1:
    lda.w !draw_counts+2
    beq .aladdin
    ldy.w #64
    jsr.w OamDrawList
.aladdin:
    lda.w !player
    beq .list2
    lda.w !player+!actor_08
    and.b #$08
    beq .list2
    lda.b #!player>>8
    xba
    lda.b #!player
    tcd
    jsr.w OamDrawActor
.list2:
    lda.w !draw_counts+4
    beq .list3
    ldy.w #128
    jsr.w OamDrawList
.list3:
    lda.w !draw_counts+6
    beq .lists_done
    ldy.w #192
    jsr.w OamDrawList
.lists_done:
    stx.w !oam_next
    pld
    rep.b #$20
    lda.b !oam_high_count          ; the high table's last bits into place
    and.w #$0007
    beq .hide
    tay
    lda.b !oam_high_bits
.shift:
    lsr a
    lsr a
    dey
    bne .shift
    ldy.w !oam_high_next
    sta.w !oam_high,y
.hide:
    sep.b #$20                     ; the sprites left off the screen
    lda.b #$E1
.next:
    sta.w !oam_buffer+1,x
    inx
    inx
    inx
    inx
    cpx.w #512
    bcc .next
    sep.b #$10
    plb
    rtl

; @routine 81:92EE OamDrawList
; WHAT IT DOES: Draws the actors of one of the draw lists.
; HOW: LIST_LEFT = A; per record address from DRAW_LISTS + Y, D = it and
; OamDrawActor.
; Entry: M 8-bit, X 16-bit; A = the count (nonzero); Y = the list's
; offset; DB = $7E; X = the next sprite's offset. Exit: M 8-bit; X past
; the sprites; Y past the list; D = 0 or the last record; Z set.
OamDrawList:
    sta.w !list_left
.record:
    lda.w !draw_lists+1,y
    xba
    lda.w !draw_lists,y
    tcd
    phy
    jsr.w OamDrawActor
    ply
    iny
    iny
    dec.w !list_left
    bne .record
    rts

OamDrawActor_full:
    sep.b #$20                     ; too many pieces: not drawn
OamDrawActor_skip:
    rts

; @routine 81:9309 OamDrawActor
; WHAT IT DOES: Puts one actor's sprite frame into this frame's sprites,
; at its place on the screen: each piece on the screen's height gets a
; sprite; a frame with more pieces than are left is not drawn.
; HOW: With ACTOR_08 bit 4 set and V_032D bit 0, not drawn (flicker).
; Palette bits from ACTOR_LOOK, or FLASH_PALETTE with ACTOR_08 bit 5 and
; V_032D bit 1 (no script runs that); the frame at ACTOR_PIECES: its count
; at most OAM_FREE, which it takes; the screen place, the actor's x and y
; less SPRITE_ORIGIN; tile base, flips (ACTOR_08 bits 12-13, inverted);
; then with D = 0, per piece: x (byte 0, or byte 1 when ACTOR_FLIP bit 0
; is set, flipping it) and y (byte 2) sign-extended plus the place; a
; piece within 32 above to 224 down gets the sprite at X: tile word plus
; the base, the flips and palette; its x bit 8 and size bit (the tile
; word's bit 12) into the high table's bits, a word out each 8 sprites.
; Entry: M 8-bit, X 16-bit; D = the actor's record; DB = $7E; X = the
; next sprite's offset. Exit: M 8-bit; X past its sprites; drawn: D = 0,
; Y past its pieces, A, N, Z, C, V from the last pointer step; skipped:
; as the test left them (D, Y unchanged); too many pieces: M 8-bit, A =
; the count, flags from its compare.
OamDrawActor:
    lda.b !actor_08
    bit.b #$10
    beq .shown
    lda.w !v_032d                  ; flicker
    and.b #$01
    bne OamDrawActor_skip
.shown:
    phx
    clc
    lda.b !actor_08
    bit.b #$20
    beq .palette
    lda.w !v_032d                  ; flash (no script runs it)
    and.b #$02
    beq .palette
    sec
.palette:
    rep.b #$20
    lda.b !actor_look
    and.w #$0E00
    bcc .own
    lda.w #$0A00
.own:
    sta.w !palette_bits
    plx
    lda.b !actor_pieces
    tay
    lda.w 0,y
    and.w #$00FF
    cmp.w !oam_free
    bcs OamDrawActor_full
    sta.w !pieces_left
    eor.w #$FFFF
    sec
    adc.w !oam_free
    sta.w !oam_free
    iny
    sec
    lda.b !actor_x+1
    sbc.w !sprite_origin_x
    sta.w !screen_x
    sec
    lda.b !actor_y+1
    sbc.w !sprite_origin_y
    sta.w !screen_y
    lda.b !actor_look
    and.w #$01FF
    sta.w !tile_base
    lda.b !actor_08
    eor.w #$3000
    and.w #$3000
    sta.w !flip_bits
    lda.b !actor_flip
    lsr a
    lda.w #0
    tcd
    bcs .mirrored
.piece:
    rep.b #$20
    lda.w 0,y
    jsr.w SignExtend
    clc
    adc.b !screen_x
    sta.w !oam_buffer,x
    sta.b !piece_x
    lda.w 2,y
    jsr.w SignExtend
    clc
    adc.b !screen_y
    sta.w !oam_buffer+1,x
    clc
    adc.w #32
    cmp.w #256
    bcs .off
    lda.w 3,y
    sta.b !piece_word
    adc.b !tile_base
    and.w #$CFFF
    ora.b !flip_bits
    ora.b !palette_bits
    sta.w !oam_buffer+2,x
    lsr.b !piece_x+1               ; x bit 8, then the size bit
    ror.b !oam_high_bits
    asl.b !piece_word
    asl.b !piece_word
    asl.b !piece_word
    asl.b !piece_word
    ror.b !oam_high_bits
    dec.b !oam_high_count
    bne .on
    lda.w #8
    sta.b !oam_high_count
    phy
    ldy.w !oam_high_next
    lda.b !oam_high_bits
    sta.w !oam_high,y
    inc.w !oam_high_next
    inc.w !oam_high_next
    ply
.on:
    inx
    inx
    inx
    inx
.off:
    clc
    tya
    adc.w #5
    tay
    dec.b !pieces_left
    bne .piece
    sep.b #$20
    rts
.mirrored:
    rep.b #$20
    lda.w 1,y
    jsr.w SignExtend
    clc
    adc.b !screen_x
    sta.w !oam_buffer,x
    sta.b !piece_x
    clc
    lda.w 2,y
    jsr.w SignExtend
    adc.b !screen_y
    sta.w !oam_buffer+1,x
    clc
    adc.w #32
    cmp.w #256
    bcs .mirrored_off
    lda.w 3,y
    sta.b !piece_word
    adc.b !tile_base
    and.w #$CFFF
    ora.b !flip_bits
    eor.w #$4000
    ora.b !palette_bits
    sta.w !oam_buffer+2,x
    lsr.b !piece_x+1
    ror.b !oam_high_bits
    asl.b !piece_word
    asl.b !piece_word
    asl.b !piece_word
    asl.b !piece_word
    ror.b !oam_high_bits
    dec.b !oam_high_count
    bne .mirrored_on
    lda.w #8
    sta.b !oam_high_count
    phy
    ldy.w !oam_high_next
    lda.b !oam_high_bits
    sta.w !oam_high,y
    inc.w !oam_high_next
    inc.w !oam_high_next
    ply
.mirrored_on:
    inx
    inx
    inx
    inx
.mirrored_off:
    clc
    tya
    adc.w #5
    tay
    dec.b !pieces_left
    bne .mirrored
    sep.b #$20
    rts
