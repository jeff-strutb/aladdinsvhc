; kind36.asm: actor kind $36, a figure of the menu and story screens
; that stands, follows a cursor, or walks, by its $07.
; C: src/aladdin/actor/kind36.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K36_FRAMES = $40               ; frames left of its walk
!K36_00A5 = $00A5               ; part 2: the index into K36_X_BY_00A5
!K36_00AC = $00AC               ; parts 4-$0A: nonzero, it speeds off
!K36_X_BY_00A5 = $D1CA          ; part 2: its x pixels' low byte, by K36_00A5 (bank DB)

; @routine 83:AF8E Kind36Start
; @actor
; WHAT IT DOES: Actor kind $36, a figure of the menu and story screens
; that plays one of nine parts by its $07. With bit 4 clear, $07 itself
; picks: 0, it stands (pose 5); 2, it stands (pose 5) with its x pixels'
; low byte set each frame from K36_X_BY_00A5 by K36_00A5; 4 and 6 (facing
; left, right), 8 (facing left, pose 2) and $0A (pose 1), it stands until
; K36_00AC is set, then speeds off sideways for 13 frames with sound $42
; and stays. With bit 4 set, it walks for $16 frames after $18 frames of
; waiting, by bits 0-2: 0 (turned round with bit 3) and stays; 1 with
; pose 1, then sound $42, a child (ActorCreateChild8 with A = $85) and
; poses 3 and 4 for 16 frames each, then moves vertically for good; 2
; turned round with pose 2, then ends.
; HOW: ACTOR_09 bits $90 set; ACTOR_FACING and ACTOR_FLIP 0. Bit 4 clear:
; JMP (K36_PARTS,X) by $07. Parts 4-$0A: AnimStart with the part's pose;
; each frame (BRK) until K36_00AC nonzero; speed x 0 (three bytes);
; ActorSpeedXSetAccel with A = $40, Y = $60; K36_FRAMES = $0D frames of
; ActorAccelerateX and ActorMoveX; SoundQueueAdd $42; BRK for good. Bit 4
; set: K36_FRAMES = $16; ActorSpeedXSet with Y = $2D; JMP (K36_MOVES,X)
; by bits 0-2 times 2; each walks: AnimStart, COP $18, then K36_FRAMES
; frames (BRK first) of ActorMoveX. Move 0: ACTOR_08 bits 0-1 set, with
; $07 bit 3 facing and flip up by one, AnimStart 0; after the walk, BRK
; for good. Move 1: AnimStart 1; after the walk SoundQueueAdd $42,
; ACTOR_SPAWN_AT 0, ActorCreateChild8 with A = $85, AnimStart 3, COP $10,
; ActorSpeedYSet with Y = $57, AnimStart 4, COP $10, then each frame
; ActorMoveY. Move 2: AnimStart 2, facing up by one; after the walk, JML
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $36 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83AF8E)
Kind36Start:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_07
    bit.b #$10
    beq .lAFA1
    jmp.w .walks
.lAFA1:
    tax
    jmp.w (.parts,x)
.parts:
    dw .part0, .part2, .part4, .part6, .part8, .part10
.part0:
    lda.b #$05
    jsl.l AnimStart
.lAFB7:
    brk.b #$00
    bra .lAFB7
.part2:
    lda.b #$05
    jsl.l AnimStart
.lAFC1:
    brk.b #$00
    ldx.w !K36_00A5
    lda.w !K36_X_BY_00A5,x
    sta.b !actor_x+1
    bra .lAFC1
.part4:
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    bra .pose0
.part6:
    stz.b !actor_facing
    stz.b !actor_flip
.pose0:
    lda.b #$00
.pose:
    jsl.l AnimStart
.wait_go:
    brk.b #$00
    lda.w !K36_00AC
    beq .wait_go
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$40
    ldy.b #$60
    jsl.l ActorSpeedXSetAccel
    lda.b #$0D
    sta.b !K36_FRAMES
.lAFF8:
    brk.b #$00
    jsl.l ActorAccelerateX
    jsl.l ActorMoveX
    dec.b !K36_FRAMES
    bne .lAFF8
    lda.b #$42
    jsl.l SoundQueueAdd
.lB00C:
    brk.b #$00
    bra .lB00C
.part8:
    lda.b #$01
    sta.b !actor_facing
    lda.b #$02
    bra .pose
.part10:
    lda.b #$01
    bra .pose
.walks:
    lda.b #$16
    sta.b !K36_FRAMES
    ldy.b #$2D
    jsl.l ActorSpeedXSet
    lda.b !actor_07
    and.b #$07
    asl a
    tax
    jmp.w (.moves,x)
.moves:
    dw .move0, .move1, .move2
.move0:
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    lda.b !actor_07
    bit.b #$08
    beq .lB045
    inc.b !actor_facing
    inc.b !actor_flip
.lB045:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
.lB04F:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !K36_FRAMES
    bne .lB04F
.lB059:
    brk.b #$00
    bra .lB059
.move1:
    lda.b #$01
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
.lB067:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !K36_FRAMES
    bne .lB067
    lda.b #$42
    jsl.l SoundQueueAdd
    stz.b !actor_spawn_at
    lda.b #$85
    jsl.l ActorCreateChild8
    lda.b #$03
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    ldy.b #$57
    jsl.l ActorSpeedYSet
    lda.b #$04
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
.lB099:
    brk.b #$00
    jsl.l ActorMoveY
    bra .lB099
.move2:
    lda.b #$02
    jsl.l AnimStart
    inc.b !actor_facing
    lda.b #$18
    cop.b #$00
.lB0AD:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !K36_FRAMES
    bne .lB0AD
    jml.l ActorEndPlaced
