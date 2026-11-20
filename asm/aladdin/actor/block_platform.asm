; block_platform.asm: the platform of kind $68 (its start, Kind68Start,
; and its pieces), which goes back and forth along a leg and is drawn as blocks
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
; Kind68Start's fields and tables.
!K68_COPIER = $18               ; set when the copier check found cartridge RAM: the platform stays put
!K68_LEG_BLOCKS = $1C           ; blocks left of its leg
!K68_LAST_X = $49               ; the low byte of its x when it last stepped a block
!K68_CHILD_SLOT = $21           ; the spawner's: the slot byte its children are made with
!K68_WAIT = $40                 ; frames left before it moves (word), or of its pause at a leg's end
!K68_CHECK = $700040            ; the copier check's bytes ($70:0040, $70:0080): none on the cartridge
!K68_CHECK_FIRST = $0000       ; with DB $70 and X $40: K68_CHECK's first byte ($70:0040)
!K68_CHECK_SECOND = $0040      ; and its second ($70:0080)
!K68_16 = $16                  ; cleared (word) at its start; purpose not yet established
!K68_BLOCKS_AT = $D5C7          ; (data bank) by $07: its first block's x, y (words)
!K68_LEG = $D5E7                ; (data bank) by $07: blocks in each leg
!K68_WAYS = $D50D               ; (data bank) by $07: bits 2 up its facing, bit 0 its actor_47
!K68_POSES = $D515              ; (data bank) by actor_47: its animation
!K68_WAITS = $D5EF              ; (data bank) by $07: frames before it first moves (words)
!K68_PAUSES = $D655             ; (data bank) by the leg: frames of the pause at its end

; @routine 83:E6BB Kind68Start
; @actor
; WHAT IT DOES: Actor kind $68 (also kind $67's second state), the block
; platform: with $07 $0F, a spawner that makes eight platforms ($07 7 down
; to 0) and ends; else one platform set up by its $07 that, after its
; wait, moves along its leg a block at a time (written into the level map
; while in bounds), pauses, and goes back. On a copier (cartridge RAM
; found at $70:0040) it never moves.
; HOW: $07 $0F: x, y 0; $07 7; K68_CHILD_SLOT $6A; ActorCreateChild8 $68,
; the slot down 2, $07 down, while $07 is not negative; ActorEndPlaced.
; Else: $AB written to and compared at $70:0040 + 0 and $CD at +$40 (DB
; $70): both equal, K68_COPIER 1. ACTOR_08 3, ACTOR_09 $90; BLOCK_ALT,
; BACK_LEG (word), gravity and $16-$17 0; BLOCK_X, BLOCK_Y from
; K68_BLOCKS_AT by $07; K68_LEG_BLOCKS from K68_LEG; facing (flip the
; opposite) and actor_47 from K68_WAYS; $48 0; K68_LAST_X its x low;
; ActorKeepSpot, AnimStart by K68_POSES; K68_WAIT frames (BRK) from
; K68_WAITS. Each frame (BRK): ActorKeepSpot, BlockPlatformMove; its x low
; 10 or more from K68_LAST_X: K68_LAST_X 16 on toward it,
; BlockPointInBounds C clear: TileActorDraw; BlockPointStep; the leg's
; blocks down: at 0 the next leg (K68_LEG_BLOCKS again, BACK_LEG and the
; facing flipped, BlockPointStep, BLOCK_ALT flipped, K68_PAUSES frames of
; ActorKeepSpot, or BRK for good with K68_COPIER), else BLOCK_ALT flipped.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $68 in state $0C (the table at $83:8CAB)
; and kind $67's second state ($83:8DFD). Exit: never returns.
%at($83E6BB)
Kind68Start:
    lda.b !actor_07
    cmp.b #$0F
    bne .one
    rep.b #$20
    stz.b !actor_x+1
    stz.b !actor_y+1
    sep.b #$20
    lda.b #$07
    sta.b !actor_07
    lda.b #$6A
    sta.b !K68_CHILD_SLOT
.spawn:
    lda.b #$68
    jsl.l ActorCreateChild8
    dec.b !K68_CHILD_SLOT
    dec.b !K68_CHILD_SLOT
    dec.b !actor_07
    bpl .spawn
    jmp.w ActorEndPlaced
.one:
    stz.b !K68_COPIER
    ldx.b #$40
    phb
    sec
    lda.b #$80
    sbc.b #$10
    pha
    plb
    lda.b #$AB
    sta.w !K68_CHECK_FIRST,x
    cmp.w !K68_CHECK_FIRST,x
    bne .cart
    lda.b #$CD
    sta.w !K68_CHECK_SECOND,x
    cmp.w !K68_CHECK_SECOND,x
    bne .cart
    plb
    inc.b !K68_COPIER
    bra .set
.cart:
    plb
.set:
    lda.b #$03
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    stz.b !BLOCK_ALT
    stz.b !BACK_LEG
    stz.b !BACK_LEG+1
    stz.b !actor_accel_y
    stz.b !K68_16
    stz.b !K68_16+1
    rep.b #$20
    lda.b !actor_07
    asl a
    asl a
    tax
    lda.w !K68_BLOCKS_AT,x
    sta.b !BLOCK_X
    lda.w !K68_BLOCKS_AT+2,x
    sta.b !BLOCK_Y
    sep.b #$20
    ldx.b !actor_07
    lda.w !K68_LEG,x
    sta.b !K68_LEG_BLOCKS
    lda.w !K68_WAYS,x
    pha
    lsr a
    lsr a
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    pla
    and.b #$01
    sta.b !actor_47
    stz.b !actor_48
    sep.b #$20
    lda.b !actor_x+1
    sta.b !K68_LAST_X
    jsl.l ActorKeepSpot
    ldx.b !actor_47
    lda.w !K68_POSES,x
    jsl.l AnimStart
    lda.b !actor_07
    asl a
    tax
    rep.b #$20
    lda.w !K68_WAITS,x
    sta.b !K68_WAIT
.wait:
    brk.b #$00
    dec.b !K68_WAIT
    bne .wait
    sep.b #$20
.move:
    brk.b #$00
    jsl.l ActorKeepSpot
    jsr.w BlockPlatformMove
    ldy.b #$10
    sec
    lda.b !actor_x+1
    sbc.b !K68_LAST_X
    bpl .right
    eor.b #$FF
    inc a
    ldy.b #$F0
.right:
    cmp.b #$0A
    bcc .next
    clc
    tya
    adc.b !K68_LAST_X
    sta.b !K68_LAST_X
    jsr.w BlockPointInBounds
    bcs .out
    jsr.w TileActorDraw
.out:
    jsr.w BlockPointStep
    dec.b !K68_LEG_BLOCKS
    beq .leg_end
    lda.b !BLOCK_ALT
    eor.b #$01
    sta.b !BLOCK_ALT
.next:
    bra .move
.leg_end:
    ldx.b !actor_07
    lda.w !K68_LEG,x
    sta.b !K68_LEG_BLOCKS
    lda.b !BACK_LEG
    eor.b #$01
    sta.b !BACK_LEG
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    jsr.w BlockPointStep
    lda.b !BLOCK_ALT
    eor.b #$01
    sta.b !BLOCK_ALT
    ldx.b !BACK_LEG
    lda.w !K68_PAUSES,x
    sta.b !K68_WAIT
    lda.b !K68_COPIER
    beq .pause
.stay:
    brk.b #$00
    bra .stay
.pause:
    brk.b #$00
    jsl.l ActorKeepSpot
    dec.b !K68_WAIT
    bne .pause
    bra .move
%ends_at($83E7DE)

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
