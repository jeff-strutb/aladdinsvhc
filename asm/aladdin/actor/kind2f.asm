; kind2f.asm: actor kind $2F, which lies still until it can be hit, rises,
; then walks the ground and hops where it ends; and its downed state. Its
; wait is ActorWaitHitsRange0 ($83:B630).
; C: src/aladdin/actor/kind2f.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND2F_FRAMES = $40            ; frames left of a step (a word for its walk)
!KIND2F_HITS = $42              ; shots that hit it without downing it (its wait counts them)
!KIND2F_16 = $16                ; word: $D16E
!COPY_TEST_2F = $701000         ; save RAM, which the retail board lacks: a write there reads back on a copier

; @routine 83:B4BE Kind2FStart
; @actor
; WHAT IT DOES: Actor kind $2F: it lies still until it can be hit (bit 6
; of $09), rises with sound $16 and falls back to the ground, then walks
; along it for up to 256 frames, turning at walls, and hops when the
; ground ahead ends, again and again. When the walk runs out it stands
; and waits $B4 frames before rising again (no script has that). On a
; cartridge whose save RAM reads back (a copier), it puts the player at
; x 0 as it starts.
; HOW: The byte at COPY_TEST,X counted up and read back equal: the
; player's x pixels = 0. ACTOR_08 = 2, ACTOR_09 = $90, $16 = $D16E;
; facing and flip = bits 0-1 of $07 shifted right once, or (zero)
; ActorSideOfPlayer; KIND2F_HITS = 0; AnimStart 0. Each frame (BRK) until
; ACTOR_09 bit 6; KIND2F_FRAMES 8 frames, AnimStart 1, $10 frames;
; ActorMotionPresetY with Y = $1E; SoundQueueAdd $16; each frame ActorFall,
; ActorFallSpeedCap, ActorOutOfGround until it is in the ground. Landed:
; AnimStart 6 for 5 frames, AnimStart 2 for $10; AnimStart 3 (4 with bit
; 2 of $07); KIND2F_FRAMES = $100 (word); ActorSpeedXSet with Y = 6 (9).
; Each frame ActorMoveX, ActorSnapToGround; ActorMapAhead5Down12 zero:
; the hop (AnimStart 2 for 8 frames, AnimStart 5, ActorMotionPreset with
; Y = $1D, each frame ActorFly, ActorFallSpeedCap, ActorOutOfWallProbe4,
; ActorOutOfGround until in the ground, then landed again). A wall
; (ActorOutOfWallProbe4, C set): facing and flip turned (no script).
; KIND2F_FRAMES down: at 0 (no script) AnimStart 2 for $10 frames,
; AnimStart 0, KIND2F_FRAMES = $B4 and back to the 8-frame wait before
; the rise (BRK only between the 2's frames, as the code has it).
; Entry: M 8-bit, X 8-bit; X = the kind's table offset; D = the actor's
; record; DB = $80; reached by the actor loop's jump for kind $2F in its
; start state (the table at $83:8CAB). Exit: never returns.
%at($83B4BE)
Kind2FStart:
    lda.l !COPY_TEST_2F,x
    inc a
    sta.l !COPY_TEST_2F,x
    cmp.l !COPY_TEST_2F,x
    bne .lB4D3
    stz.w !player+!actor_x+1
    stz.w !player+!actor_x+2
.lB4D3:
    lda.b #$02
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D16E
    sta.b !KIND2F_16
    sep.b #$20
    lda.b !actor_07
    and.b #$03
    beq .lB4ED
    lsr a
    bra .lB4F1
.lB4ED:
    jsl.l ActorSideOfPlayer
.lB4F1:
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !KIND2F_HITS
    lda.b #$00
    jsl.l AnimStart
.lB4FD:
    brk.b #$00
    lda.b !actor_09
    and.b #$40
    beq .lB4FD
    lda.b #$08
.lB507:
    sta.b !KIND2F_FRAMES
.lB509:
    brk.b #$00
    dec.b !KIND2F_FRAMES
    bne .lB509
    lda.b #$10
    sta.b !KIND2F_FRAMES
    lda.b #$01
    jsl.l AnimStart
.lB519:
    brk.b #$00
    dec.b !KIND2F_FRAMES
    bne .lB519
    ldy.b #$1E
    jsl.l ActorMotionPresetY
    lda.b #$16
    jsl.l SoundQueueAdd
.lB52B:
    brk.b #$00
    jsl.l ActorFall
    jsr.w ActorFallSpeedCap
    jsl.l ActorOutOfGround
    beq .lB52B
.lB53A:
    lda.b #$05
    sta.b !KIND2F_FRAMES
    lda.b #$06
    jsl.l AnimStart
.lB544:
    brk.b #$00
    dec.b !KIND2F_FRAMES
    bne .lB544
    lda.b #$02
    jsl.l AnimStart
    lda.b #$10
    sta.b !KIND2F_FRAMES
.lB554:
    brk.b #$00
    dec.b !KIND2F_FRAMES
    bne .lB554
    ldx.b #$03
    lda.b !actor_07
    and.b #$04
    beq .lB564
    ldx.b #$04
.lB564:
    txa
    jsl.l AnimStart
    rep.b #$20
    lda.w #$0100
    sta.b !KIND2F_FRAMES
    sep.b #$20
    ldy.b #$06
    lda.b !actor_07
    and.b #$04
    beq .lB57C
    ldy.b #$09
.lB57C:
    jsl.l ActorSpeedXSet
.lB580:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    jsr.w ActorMapAhead5Down12
    beq .lB5C4
    jsl.l ActorOutOfWallProbe4
    bcc .lB59D
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.lB59D:
    rep.b #$20
    dec.b !KIND2F_FRAMES
    sep.b #$20
    beq .lB5A7
    bra .lB580
.lB5A7:
    lda.b #$10
    sta.b !KIND2F_FRAMES
    lda.b #$02
    jsl.l AnimStart
.lB5B1:
    dec.b !KIND2F_FRAMES
    beq .lB5B9
    brk.b #$00
    bra .lB5B1
.lB5B9:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$B4
    jmp.w .lB507
.lB5C4:
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    sta.b !KIND2F_FRAMES
.lB5CE:
    brk.b #$00
    dec.b !KIND2F_FRAMES
    bne .lB5CE
    lda.b #$05
    jsl.l AnimStart
    ldy.b #$1D
    jsl.l ActorMotionPreset
.lB5E0:
    brk.b #$00
    jsl.l ActorFly
    jsr.w ActorFallSpeedCap
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq .lB5E0
    jmp.w .lB53A

; @routine 83:B643 Kind2FSecond
; @actor
; WHAT IT DOES: Actor kind $2F in its second state (downed): it is knocked
; up with sound $0F (no sound when shots have hit it and it faces right),
; plays its downed animation for 10 frames, then flies off until it is
; out of range.
; HOW: ActorMotionPreset with Y = $1F; AnimStart 7; KIND2F_FRAMES = $0A.
; KIND2F_HITS nonzero and facing not negative: no sound (no script);
; else SoundQueueAdd $0F. Facing negative: its bit 7 cleared (no script).
; 10 frames (BRK) of AnimStep; then each frame AnimStep, ActorFly and
; ActorEndOutOfRange with X = 0 (by JSR from its own level: out of range
; it ends there).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2F in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83B643)
Kind2FSecond:
    ldy.b #$1F
    jsl.l ActorMotionPreset
    lda.b #$07
    jsl.l AnimStart
    lda.b #$0A
    sta.b !KIND2F_FRAMES
    lda.b !KIND2F_HITS
    beq .lB65B
    lda.b !actor_facing
    bpl .lB661
.lB65B:
    lda.b #$0F
    jsl.l SoundQueueAdd
.lB661:
    lda.b !actor_facing
    bpl .lB669
    and.b #$7F
    sta.b !actor_facing
.lB669:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND2F_FRAMES
    bne .lB669
.lB673:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .lB673

