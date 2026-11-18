; kind2b.asm: actor kind $2B's start and its partner test.
; C: src/aladdin/actor/kind2b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 83:A467 Kind2BStart
; @actor
; WHAT IT DOES: Actor kind $2B: a walker along a floor, a wall or a ceiling
; (ways 0-7 of $07: its surface and first direction), turning at each end
; of its leg, at a wall and where its surface ends, while Aladdin is near.
; With bit 3, when he is within its range across: a ceiling or wall walker
; drops onto him when he is level with or below it, then walks the floor
; facing him; a floor walker turns to face him when he is within its
; height. With bit 4 ($10): it walks to and fro on its own, and when he is
; below and within its span across it shakes and falls, until its $4B is
; set; then (never ended out of range) it walks to and fro within $1E of
; its partner's x ($49, a record its maker sets), jumps while the
; partner's $13 is 3 and ends when the partner's $16 is set.
; HOW: $4E 0; $07 bit 4: on at $83:A5E8. Else $09 bits $90 set; AnimStart
; 2; $40 (its direction) 0, 1 when RandomNext is not negative; its look
; table ($16) $D116; $48 = its x; $47 = $07 bits 0-2 / 2 (its surface);
; $41 and $45 = its leg from $D11A, $43 its double; $4C its range from
; $D11E, $4A its double; $4B, $4D 0; then its turn ($83:A524: $41 = $43 -
; $41, $15 from $D0CE and AnimStart from $D0C6 by $47 * 2 + $40, $14
; frames (COP), $40 flipped; then $14, $15, its animation from $D0B6,
; $D0CE, $D0BE). Each frame (BRK, $83:A4C4) until ActorNearPlayer box 4:
; then its leg ($41, 16-bit) down, at 0 a turn; the tile at the point
; ahead ($D0D6 offsets) $70, a turn; the tile at the point ahead on its
; surface ($D0F6 offsets) none, a turn; else with bit 3 its test of him
; ($83:A57A). Then ActorMoveByVector X = 2, on the floor ($47 1)
; ActorSnapToGround, and the next frame. Its test: x ($48) + range less
; his x not below the double, nothing; a floor walker: ActorNearPlayerY 2
; C clear and ActorSideOfPlayer other than $40: a turn; else
; ActorPlayerBelow: its drop: ActorMotionReset $28, AnimStart 3, each
; frame ActorFall, ActorOutOfGround until on ground; $47 1, $41 = $45,
; $48 = its x, $40 and $15 ActorSideOfPlayer, AnimStart 0, $20 frames.
; Bit 4 ($83:A5E8): $4B, $47 0; $09 bit 7; AnimStart 4; $14 0, $15 1;
; ActorSpeedXSetAccel $42; $40 $18, $41 $30; each frame ActorMoveX, then
; ActorPlayerBelow and ActorNearPlayerX 4 C clear: its fall; else $40
; down, at 0 from $41, its direction turned and $10 frames. Its fall
; ($83:A628): 8 times 6 frames, $15 flipped; AnimStart 3; $20 $20, its
; vertical speed 0; each frame until $4B: ActorAccelerateY unless $1E is
; 3, ActorStepY. Then $4E and $47 up; $14 1 when the partner is right of
; it, $15 the other; $19 $50; AnimStart 1; each frame: the partner's $16
; set, JMP ActorEndPlaced; its $13 3, a jump (speed $FFFC00, $20 $40;
; each frame ActorFall, ActorEndOutOfRange X = 8, until $4B, then as
; from $83:A664); else ActorMoveX and Kind2BNearPartner: past, its x
; back (+5 with $14 set: ADC #4 with C set) and y back and its direction
; turned.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's jump for kind $2B in state $0C. Exit: never returns.
%at($83A467)
Kind2BStart:
    stz.b $4E
    lda.b $07
    and.b #$10
    beq .walker
    jmp.w .wide
.walker:
    lda.b #$90
    tsb.b $09
    lda.b #$02
    jsl.l AnimStart
    stz.b $40
    jsl.l RandomNext
    bmi .sides
    inc.b $40
.sides:
    rep.b #$20
    lda.w #$D116
    sta.b $16
    lda.b $0C
    sta.b $48
    sep.b #$20
    lda.b $07
    and.b #$07
    lsr a
    sta.b $47
    lda.b $07
    and.b #$02
    adc.b #$00
    tax
    lda.w $D11A,x
    sta.b $41
    stz.b $42
    sta.b $45
    stz.b $46
    rep.b #$20
    and.w #$00FF
    asl a
    sta.b $43
    sep.b #$20
    lda.w $D11E,x
    sta.b $4C
    asl a
    sta.b $4A
    stz.b $4D
    stz.b $4B
    bra .turn
.frame:
    brk.b #$00
    ldy.b #$04
    jsl.l ActorNearPlayer
    bcc .frame
    rep.b #$20
    dec.b $41
    sep.b #$20
    beq .turn
    lda.b $47
    asl a
    adc.b $40
    asl a
    asl a
    tax
    rep.b #$21
    lda.w $D0D6,x
    adc.b $0C
    sta.w $000C
    lda.w $D0D8,x
    adc.b $0F
    sta.w $000E
    jsl.l TileAtProbeLong
    cmp.b #$70
    bne .ahead
    bra .turn
.ahead:
    lda.b $47
    asl a
    adc.b $40
    asl a
    asl a
    tax
    rep.b #$21
    lda.w $D0F6,x
    adc.b $0C
    sta.w $000C
    lda.w $D0F8,x
    clc
    adc.b $0F
    sta.w $000E
    jsl.l TileAtProbeLong
    beq .turn
    lda.b $07
    and.b #$08
    beq .move
    jmp.w .test
.turn:
    rep.b #$20
    lda.b $43
    sec
    sbc.b $41
    sta.b $41
    sep.b #$20
    lda.b $47
    asl a
    adc.b $40
    tax
    lda.w $D0CE,x
    sta.b $15
    lda.w $D0C6,x
    jsl.l AnimStart
    lda.b #$14
    cop.b #$00
    lda.b $40
    eor.b #$01
    sta.b $40
    bra .go
.dropped:
    lda.b #$20
    cop.b #$00
.go:
    lda.b $47
    asl a
    adc.b $40
    tax
    lda.w $D0B6,x
    sta.b $14
    lda.w $D0CE,x
    sta.b $15
    lda.w $D0BE,x
    jsl.l AnimStart
.move:
    ldx.b #$02
    jsl.l ActorMoveByVector
    lda.b $47
    dec a
    bne .next
    jsl.l ActorSnapToGround
.next:
    jmp.w .frame
.test:
    rep.b #$21
    lda.b $48
    adc.b $4C
    sec
    sbc.w $08E7
    cmp.b $4A
    sep.b #$20
    bcs .on
    lda.b $47
    dec a
    bne .above
    ldy.b #$02
    jsl.l ActorNearPlayerY
    sep.b #$10
    bcs .on
    jsl.l ActorSideOfPlayer
    cmp.b $40
    beq .on
    jmp.w .turn
.above:
    jsl.l ActorPlayerBelow
    beq .on
    lda.b #$28
    jsl.l ActorMotionReset
    lda.b #$03
    jsl.l AnimStart
.drop:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .drop
    lda.b #$01
    sta.b $47
    lda.b $45
    sta.b $41
    stz.b $42
    lda.b $0C
    sta.b $48
    lda.b $0D
    sta.b $49
    jsl.l ActorSideOfPlayer
    sta.b $40
    sta.b $15
    lda.b #$00
    jsl.l AnimStart
    jmp.w .dropped
.on:
    jmp.w .move
.wide:
    stz.b $4B
    stz.b $47
    lda.b #$80
    tsb.b $09
    lda.b #$04
    jsl.l AnimStart
    stz.b $14
    lda.b #$01
    sta.b $15
    ldy.b #$42
    jsl.l ActorSpeedXSetAccel
    lda.b #$18
    sta.b $40
    asl a
    sta.b $41
.wide_frame:
    brk.b #$00
    jsl.l ActorMoveX
    jmp.w .wide_test
.wide_count:
    dec.b $40
    bne .wide_frame
    lda.b $41
    sta.b $40
    lda.b $14
    sta.b $15
    eor.b #$01
    sta.b $14
    lda.b #$10
    cop.b #$00
    bra .wide_frame
.shake:
    lda.b #$08
    sta.b $46
.shake_frame:
    lda.b #$06
    cop.b #$00
    lda.b $15
    eor.b #$01
    sta.b $15
    dec.b $46
    bne .shake_frame
    lda.b #$03
    jsl.l AnimStart
    lda.b #$20
    sta.b $20
    stz.b $1D
    stz.b $1E
    stz.b $1F
.fall:
    brk.b #$00
    lda.b $4B
    bne .landed
    lda.b $1E
    cmp.b #$03
    beq .fall_step
    jsl.l ActorAccelerateY
.fall_step:
    jsl.l ActorStepY
    bra .fall
.landed:
    inc.b $4E
    inc.b $47
.partner:
    rep.b #$30
    ldx.b $49
    ldy.w #$0000
    lda.b $0C
    cmp.w $000C,x
    bcs .partner_left
    ldy.w #$0001
.partner_left:
    sep.b #$30
    tya
    sta.b $14
    eor.b #$01
    sta.b $15
    lda.b #$50
    sta.b $19
    lda.b #$01
    jsl.l AnimStart
.pace:
    brk.b #$00
    rep.b #$10
    ldx.b $49
    lda.w $0016,x
    bne .end
    lda.w $0013,x
    cmp.b #$03
    beq .jump
    sep.b #$10
    rep.b #$20
    lda.b $0C
    sta.b $42
    lda.b $0F
    sta.b $44
    sep.b #$20
    jsl.l ActorMoveX
    jsr.w Kind2BNearPartner
    bcc .pace
    rep.b #$20
    lda.b $42
    ldy.b $14
    beq .back
    adc.w #$0004
.back:
    sta.b $0C
    lda.b $44
    sta.b $0F
    sep.b #$20
    lda.b $14
    sta.b $15
    eor.b #$01
    sta.b $14
    bra .pace
.end:
    sep.b #$10
    jmp.w ActorEndPlaced
.jump:
    sep.b #$10
    stz.b $1D
    lda.b #$FC
    sta.b $1E
    lda.b #$FF
    sta.b $1F
    lda.b #$40
    sta.b $20
.jump_frame:
    brk.b #$00
    jsl.l ActorFall
    ldx.b #$08
    jsr.w ActorEndOutOfRange
    lda.b $4B
    beq .jump_frame
    jmp.w .partner
.wide_test:
    jsl.l ActorPlayerBelow
    beq .wide_on
    ldy.b #$04
    jsl.l ActorNearPlayerX
    bcs .wide_on
    jmp.w .shake
.wide_on:
    jmp.w .wide_count

; @routine 83:CD47 Kind2BNearPartner
; WHAT IT DOES: Says whether kind $2B's actor is past $1E of its
; partner's x either way.
; HOW: With 16-bit A and X: its x less the partner's ($49, a record) plus
; the word at $D2E3 ($1E), against the word at $D2E5 ($3C).
; Entry: D = the actor; DB = $80; called by JSR (83:A6AC). Exit: C set when
; past; N, Z, C from the compare; A = the sum; X = the partner; M and X
; 8-bit.
%at($83CD47)
Kind2BNearPartner:
    rep.b #$30
    ldx.b $49
    sec
    lda.b $0C
    sbc.w $000C,x
    clc
    adc.w $D2E3
    cmp.w $D2E5
    sep.b #$30
    rts
