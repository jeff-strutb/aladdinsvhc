; kind2b.asm: actor kind $2B's start and its partner test.
; C: src/aladdin/actor/kind2b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Its record's fields (the same names as the C's, where it names them).
!K2B_DIR = $40                  ; walker: its direction along its surface (0 or 1)
!K2B_LEG = $41                  ; walker: frames left of its leg (word)
!K2B_LEG_DOUBLE = $43           ; walker: twice its leg (word)
!K2B_LEG_FIRST = $45            ; walker: its leg from K2B_LEGS (word)
!K2B_SURFACE = $47              ; its surface ($07 bits 0-2 / 2): 1 a floor walker
!K2B_HOME_X = $48               ; walker: the x pixels its range is from (word)
!K2B_RANGE_DOUBLE = $4A         ; walker: twice its range across
!K2B_4B = $4B                   ; nonzero: its fall ends (only cleared here)
!K2B_RANGE = $4C                ; walker: its range across, from K2B_RANGES
!K2B_4D = $4D                   ; cleared at its start; purpose not yet established
!K2B_STAYS = $4E                ; nonzero: it is never ended out of range
!K2B_LOOK_TABLE = $16           ; its look table (word)
; With $07 bit 4 (the same bytes put to other use):
!K2B_WIDE_FRAMES = $40          ; frames left of its leg
!K2B_WIDE_LEG = $41             ; its leg's frames
!K2B_PACE_X = $42               ; its x pixels before its step (word)
!K2B_PACE_Y = $44               ; and its y pixels (word)
!K2B_SHAKES = $46               ; shakes left before its fall
!K2B_PARTNER = $49              ; its partner's record (word; its maker sets it)
!K2B_PARTNER_JUMPS = $0013      ; the partner's byte: 3, it jumps
!K2B_PARTNER_ENDS = $0016       ; the partner's byte: nonzero, it ends

; Elsewhere (the same names as the C's).
!PLAYER_X = $08E7               ; Aladdin's x pixels (word)
!K2B_POINT_X = $000C            ; the point TileAtProbeLong reads: x (word)
!K2B_POINT_Y = $000E            ; and y (word)
!K2B_LOOKS = $D116              ; its look table
!K2B_LEGS = $D11A               ; its leg by way (bytes, DB)
!K2B_RANGES = $D11E             ; its range across by way (bytes, DB)
!K2B_AHEAD = $D0D6              ; the point ahead by surface and direction (x, y words, DB)
!K2B_ON = $D0F6                 ; the point ahead on its surface (x, y words, DB)
!K2B_FLIPS = $D0CE              ; its flip by surface and direction (bytes, DB)
!K2B_TURNS = $D0C6              ; its animation as it turns (bytes, DB)
!K2B_DIRS = $D0B6               ; its direction ($14) (bytes, DB)
!K2B_WALKS = $D0BE              ; its walking animation (bytes, DB)
!K2B_PAST = $D2E3               ; Kind2BNearPartner's half-span (word, DB)
!K2B_SPAN = $D2E5               ; and span (word, DB)


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
    stz.b !K2B_STAYS
    lda.b !actor_07
    and.b #$10
    beq .walker
    jmp.w .wide
.walker:
    lda.b #$90
    tsb.b !actor_09
    lda.b #$02
    jsl.l AnimStart
    stz.b !K2B_DIR
    jsl.l RandomNext
    bmi .sides
    inc.b !K2B_DIR
.sides:
    rep.b #$20
    lda.w #!K2B_LOOKS
    sta.b !K2B_LOOK_TABLE
    lda.b !actor_x+1
    sta.b !K2B_HOME_X
    sep.b #$20
    lda.b !actor_07
    and.b #$07
    lsr a
    sta.b !K2B_SURFACE
    lda.b !actor_07
    and.b #$02
    adc.b #$00
    tax
    lda.w !K2B_LEGS,x
    sta.b !K2B_LEG
    stz.b !K2B_LEG+1
    sta.b !K2B_LEG_FIRST
    stz.b !K2B_LEG_FIRST+1
    rep.b #$20
    and.w #$00FF
    asl a
    sta.b !K2B_LEG_DOUBLE
    sep.b #$20
    lda.w !K2B_RANGES,x
    sta.b !K2B_RANGE
    asl a
    sta.b !K2B_RANGE_DOUBLE
    stz.b !K2B_4D
    stz.b !K2B_4B
    bra .turn
.frame:
    brk.b #$00
    ldy.b #$04
    jsl.l ActorNearPlayer
    bcc .frame
    rep.b #$20
    dec.b !K2B_LEG
    sep.b #$20
    beq .turn
    lda.b !K2B_SURFACE
    asl a
    adc.b !K2B_DIR
    asl a
    asl a
    tax
    rep.b #$21
    lda.w !K2B_AHEAD,x
    adc.b !actor_x+1
    sta.w !K2B_POINT_X
    lda.w !K2B_AHEAD+2,x
    adc.b !actor_y+1
    sta.w !K2B_POINT_Y
    jsl.l TileAtProbeLong
    cmp.b #$70
    bne .ahead
    bra .turn
.ahead:
    lda.b !K2B_SURFACE
    asl a
    adc.b !K2B_DIR
    asl a
    asl a
    tax
    rep.b #$21
    lda.w !K2B_ON,x
    adc.b !actor_x+1
    sta.w !K2B_POINT_X
    lda.w !K2B_ON+2,x
    clc
    adc.b !actor_y+1
    sta.w !K2B_POINT_Y
    jsl.l TileAtProbeLong
    beq .turn
    lda.b !actor_07
    and.b #$08
    beq .move
    jmp.w .test
.turn:
    rep.b #$20
    lda.b !K2B_LEG_DOUBLE
    sec
    sbc.b !K2B_LEG
    sta.b !K2B_LEG
    sep.b #$20
    lda.b !K2B_SURFACE
    asl a
    adc.b !K2B_DIR
    tax
    lda.w !K2B_FLIPS,x
    sta.b !actor_flip
    lda.w !K2B_TURNS,x
    jsl.l AnimStart
    lda.b #$14
    cop.b #$00
    lda.b !K2B_DIR
    eor.b #$01
    sta.b !K2B_DIR
    bra .go
.dropped:
    lda.b #$20
    cop.b #$00
.go:
    lda.b !K2B_SURFACE
    asl a
    adc.b !K2B_DIR
    tax
    lda.w !K2B_DIRS,x
    sta.b !actor_facing
    lda.w !K2B_FLIPS,x
    sta.b !actor_flip
    lda.w !K2B_WALKS,x
    jsl.l AnimStart
.move:
    ldx.b #$02
    jsl.l ActorMoveByVector
    lda.b !K2B_SURFACE
    dec a
    bne .next
    jsl.l ActorSnapToGround
.next:
    jmp.w .frame
.test:
    rep.b #$21
    lda.b !K2B_HOME_X
    adc.b !K2B_RANGE
    sec
    sbc.w !PLAYER_X
    cmp.b !K2B_RANGE_DOUBLE
    sep.b #$20
    bcs .on
    lda.b !K2B_SURFACE
    dec a
    bne .above
    ldy.b #$02
    jsl.l ActorNearPlayerY
    sep.b #$10
    bcs .on
    jsl.l ActorSideOfPlayer
    cmp.b !K2B_DIR
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
    sta.b !K2B_SURFACE
    lda.b !K2B_LEG_FIRST
    sta.b !K2B_LEG
    stz.b !K2B_LEG+1
    lda.b !actor_x+1
    sta.b !K2B_HOME_X
    lda.b !actor_x+2
    sta.b !K2B_HOME_X+1
    jsl.l ActorSideOfPlayer
    sta.b !K2B_DIR
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    jmp.w .dropped
.on:
    jmp.w .move
.wide:
    stz.b !K2B_4B
    stz.b !K2B_SURFACE
    lda.b #$80
    tsb.b !actor_09
    lda.b #$04
    jsl.l AnimStart
    stz.b !actor_facing
    lda.b #$01
    sta.b !actor_flip
    ldy.b #$42
    jsl.l ActorSpeedXSetAccel
    lda.b #$18
    sta.b !K2B_WIDE_FRAMES
    asl a
    sta.b !K2B_WIDE_LEG
.wide_frame:
    brk.b #$00
    jsl.l ActorMoveX
    jmp.w .wide_test
.wide_count:
    dec.b !K2B_WIDE_FRAMES
    bne .wide_frame
    lda.b !K2B_WIDE_LEG
    sta.b !K2B_WIDE_FRAMES
    lda.b !actor_facing
    sta.b !actor_flip
    eor.b #$01
    sta.b !actor_facing
    lda.b #$10
    cop.b #$00
    bra .wide_frame
.shake:
    lda.b #$08
    sta.b !K2B_SHAKES
.shake_frame:
    lda.b #$06
    cop.b #$00
    lda.b !actor_flip
    eor.b #$01
    sta.b !actor_flip
    dec.b !K2B_SHAKES
    bne .shake_frame
    lda.b #$03
    jsl.l AnimStart
    lda.b #$20
    sta.b !actor_accel_y
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
.fall:
    brk.b #$00
    lda.b !K2B_4B
    bne .landed
    lda.b !actor_speed_y+1
    cmp.b #$03
    beq .fall_step
    jsl.l ActorAccelerateY
.fall_step:
    jsl.l ActorStepY
    bra .fall
.landed:
    inc.b !K2B_STAYS
    inc.b !K2B_SURFACE
.partner:
    rep.b #$30
    ldx.b !K2B_PARTNER
    ldy.w #$0000
    lda.b !actor_x+1
    cmp.w !actor_x+1,x
    bcs .partner_left
    ldy.w #$0001
.partner_left:
    sep.b #$30
    tya
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    lda.b #$50
    sta.b !actor_speed_x
    lda.b #$01
    jsl.l AnimStart
.pace:
    brk.b #$00
    rep.b #$10
    ldx.b !K2B_PARTNER
    lda.w !K2B_PARTNER_ENDS,x
    bne .end
    lda.w !K2B_PARTNER_JUMPS,x
    cmp.b #$03
    beq .jump
    sep.b #$10
    rep.b #$20
    lda.b !actor_x+1
    sta.b !K2B_PACE_X
    lda.b !actor_y+1
    sta.b !K2B_PACE_Y
    sep.b #$20
    jsl.l ActorMoveX
    jsr.w Kind2BNearPartner
    bcc .pace
    rep.b #$20
    lda.b !K2B_PACE_X
    ldy.b !actor_facing
    beq .back
    adc.w #$0004
.back:
    sta.b !actor_x+1
    lda.b !K2B_PACE_Y
    sta.b !actor_y+1
    sep.b #$20
    lda.b !actor_facing
    sta.b !actor_flip
    eor.b #$01
    sta.b !actor_facing
    bra .pace
.end:
    sep.b #$10
    jmp.w ActorEndPlaced
.jump:
    sep.b #$10
    stz.b !actor_speed_y
    lda.b #$FC
    sta.b !actor_speed_y+1
    lda.b #$FF
    sta.b !actor_speed_y+2
    lda.b #$40
    sta.b !actor_accel_y
.jump_frame:
    brk.b #$00
    jsl.l ActorFall
    ldx.b #$08
    jsr.w ActorEndOutOfRange
    lda.b !K2B_4B
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
    ldx.b !K2B_PARTNER
    sec
    lda.b !actor_x+1
    sbc.w !actor_x+1,x
    clc
    adc.w !K2B_PAST
    cmp.w !K2B_SPAN
    sep.b #$30
    rts

!K2B_SPEED_Y_TOP = $04          ; the vertical speed's middle byte it falls at, at most
!K2B_FALL_RESET = $38           ; ActorMotionReset's speed for its fall off a wall or ceiling
!K2B_FALL_PRESET = $03          ; ActorMotionPreset's preset for its fall off the floor

; @routine 83:A725 Kind2BSecond
; @actor
; WHAT IT DOES: Kind $2B downed (its second state, state $8C, as a shot
; downs it): its hit pose, then it falls away, a floor walker in an arc
; (motion preset 3), one on a wall or ceiling straight off, until it is
; out of range.
; HOW: AnimStart 3. K2B_SURFACE one less: zero (it was 1): ActorMotionPreset
; 3; else ActorMotionReset $38. Each frame (BRK first): its vertical
; speed's middle byte other than 4: ActorAccelerateY; ActorMoveX,
; ActorStepY; ActorEndOutOfRange with box 8.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2B in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83A725)
Kind2BSecond:
    lda.b #$03
    jsl.l AnimStart
    dec.b !K2B_SURFACE
    bne .off
    ldy.b #!K2B_FALL_PRESET
    jsl.l ActorMotionPreset
    bra .fall
.off:
    lda.b #!K2B_FALL_RESET
    jsl.l ActorMotionReset
.fall:
    brk.b #$00
    lda.b !actor_speed_y+1
    cmp.b #!K2B_SPEED_Y_TOP
    beq .top
    jsl.l ActorAccelerateY
.top:
    jsl.l ActorMoveX
    jsl.l ActorStepY
    ldx.b #$08
    jsr.w ActorEndOutOfRange
    bra .fall
%ends_at($83A758)
