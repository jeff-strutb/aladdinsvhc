; kind85_8e.asm: actor kinds $85 (a brief pose, with a sound unless its
; $07 bit 4 is set) and $8E (by its $07: a pose for 32 frames, or a burst of
; copies of itself, each moved off for two frames).
; C: src/aladdin/actor/kind85_8e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K8E_FRAMES = $40               ; frames left
!K8E_COPIES = $41               ; copies left to make (set by its spawner)

; @routine 84:9602 Kind85Start
; @actor
; WHAT IT DOES: Actor kind $85: drawn in front, sound $36 unless its $07
; bit 4 is set ($84:9614, which no script meets), pose 0 for 16 frames,
; then it ends.
; HOW: ACTOR_08 bit 1, ACTOR_09 bit 7 set; $07 bit 4 clear:
; SoundQueueAdd $36; AnimStart 0; COP $10; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $85 in state $0C (the table at $83:8DAB, through
; the stub at $83:92F2). Exit: never returns; ends in ActorEndPlaced.
%at($849602)
Kind85Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
    lda.b !actor_07
    bit.b #$10
    bne .quiet
    lda.b #$36
    jsl.l SoundQueueAdd
.quiet:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    jml.l ActorEndPlaced

; @routine 84:9B6B Kind8EStart
; @actor
; WHAT IT DOES: Actor kind $8E, by its $07: 0 and 1 a pose (0 or 1) for
; 32 frames, then it ends; 2 or more a burst: K8E_COPIES times a copy of
; itself ($07 0) made and the actor moved off by vector set $20 for two
; frames, then it ends. Only $07 = 1 is met by a script ($84:9B72-9B98,
; $84:9BBD-9BE2 written from their bytes).
; HOW: $07 0: ACTOR_08 bit 1, ACTOR_09 bit 4 set, AnimStart 0, K8E_FRAMES
; $20 frames of BRK and AnimStep, ActorEndPlaced; $07 1 the same with
; AnimStart 1. Else (JMP $84:9BBD): $07 and ACTOR_SPAWN_AT 0; each copy:
; ActorCreateChild8 with A = its kind ($06), K8E_FRAMES 2, two frames of
; BRK, ActorMoveByVector with X = $20 and the facing written back;
; K8E_COPIES down; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; the actor loop's jump for kind $8E in state $0C (the table at $83:8DAB,
; through the stub at $83:9351). Exit: never returns; ends in
; ActorEndPlaced.
%at($849B6B)
Kind8EStart:
    lda.b !actor_07
    beq .pose0
    dec a
    beq .pose1
    jmp.w .burst
.pose0:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$10
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    sta.b !K8E_FRAMES
.frame0:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K8E_FRAMES
    bne .frame0
    jml.l ActorEndPlaced
.pose1:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$10
    sta.b !actor_09
    lda.b #$01
    jsl.l AnimStart
    lda.b #$20
    sta.b !K8E_FRAMES
.frame1:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K8E_FRAMES
    bne .frame1
    jml.l ActorEndPlaced
.burst:
    stz.b !actor_07
    stz.b !actor_spawn_at
.copy:
    lda.b !actor_06
    jsl.l ActorCreateChild8
    lda.b #$02
    sta.b !K8E_FRAMES
.move:
    brk.b #$00
    ldx.b #$20
    jsl.l ActorMoveByVector
    lda.b !actor_facing
    sta.b !actor_facing
    dec.b !K8E_FRAMES
    bne .move
    dec.b !K8E_COPIES
    bne .copy
    jml.l ActorEndPlaced
