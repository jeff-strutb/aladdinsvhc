; block_platform.asm: pieces of the platform of kind $68 (its start at
; $83:E6BB), which goes back and forth along a leg and is drawn as blocks
; of the level map, written ($81:9978) where it moves to, a block at a
; time.
; C: src/aladdin/actor/block_platform.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; The platform's own fields of its record.
!BLOCK_X = $42                  ; where its next blocks are written: x, y pixels (words)
!BLOCK_Y = $44
!BACK_LEG = $19                 ; 0 on the way out, 1 on the way back (word)
!BLOCK_ALT = $4A                ; bit 0 flips at every block step

; In the bank DB names: by bit 0 of BLOCK_ALT, the offset from here of a
; list of block steps (words), 4 bytes each: x then y (words), picked by
; actor_47 and the facing.
!BLOCK_STEPS = $D5A3
; In the bank DB names: by actor_07, the start of its list in MOVE_PICKS;
; the list's bytes, picked by actor_47, the leg and the facing, are
; offsets from MOVE_PICKS of 6-byte moves: x then y (24-bit, fraction
; first).
!MOVE_LISTS = $D517
!MOVE_PICKS = $D51F

; @routine 83:E7DE BlockPointInBounds
; WHAT IT DOES: Tells whether the platform's block point is in the part
; of the map its blocks may be written to: C clear when its x pixel is
; from $20 to $DF.
; HOW: x less $80, plus $60, compared unsigned with $C0, with 16-bit A.
; Entry: M 8-bit; D = the platform's record. Exit: as the C states.
%at($83E7DE)
BlockPointInBounds:
    rep.b #$20
    sec
    lda.b !BLOCK_X
    sbc.w #$0080
    clc
    adc.w #$0060
    cmp.w #$00C0
    sep.b #$20
    rts

; @routine 83:E837 BlockPointStep
; WHAT IT DOES: Moves the platform's block point on by one block step:
; the step from the list by bit 0 of BLOCK_ALT, picked by ACTOR_47 and the
; facing, its y step turned round on the way back.
; HOW: With 16-bit registers: the list's offset from the table by the
; bit, plus 4 times (ACTOR_47 twice, or the facing word, low byte); x and
; y added from the entry, y negated when the back-leg word is nonzero.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
; bank. Exit: as the C states.
%at($83E837)
BlockPointStep:
    rep.b #$30
    lda.b !BLOCK_ALT
    and.w #$0001
    asl a
    tax
    lda.b !actor_47
    asl a
    ora.b !actor_facing
    and.w #$00FF
    asl a
    asl a
    clc
    adc.w !BLOCK_STEPS,x
    tax
    clc
    lda.b !BLOCK_X
    adc.w !BLOCK_STEPS,x
    sta.b !BLOCK_X
    lda.w !BLOCK_STEPS+2,x
    ldy.b !BACK_LEG
    beq .add
    eor.w #$FFFF
    inc a
.add:
    clc
    adc.b !BLOCK_Y
    sta.b !BLOCK_Y
    sep.b #$30
    rts

; @routine 83:E86A BlockPlatformMove
; WHAT IT DOES: Moves the platform a frame along its leg: x and y by the
; move its kind's list picks by ACTOR_47, the leg and the facing.
; HOW: The pick ((ACTOR_47 * 2 | leg) * 2 | facing) indexes the list
; ACTOR_07 starts; its byte is the move's offset; two 24-bit additions,
; a byte at a time.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB the tables'
; bank. Exit: as the C states.
BlockPlatformMove:
    ldx.b !actor_07
    lda.b !actor_47
    asl a
    ora.b !BACK_LEG
    asl a
    ora.b !actor_facing
    clc
    adc.w !MOVE_LISTS,x
    tay
    ldx.w !MOVE_PICKS,y
    clc
    lda.b !actor_x
    adc.w !MOVE_PICKS,x
    sta.b !actor_x
    lda.b !actor_x+1
    adc.w !MOVE_PICKS+1,x
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.w !MOVE_PICKS+2,x
    sta.b !actor_x+2
    clc
    lda.b !actor_y
    adc.w !MOVE_PICKS+3,x
    sta.b !actor_y
    lda.b !actor_y+1
    adc.w !MOVE_PICKS+4,x
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.w !MOVE_PICKS+5,x
    sta.b !actor_y+2
    rts
