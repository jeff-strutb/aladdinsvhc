; kind20_dazed.asm: actor kind $20's guard knocked down and dazed, until
; it gets up or is knocked away. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/kind20_dazed.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!DAZE_PLACE = $07                ; its place's byte: bit 4, the palette from ROM
!DAZE_FRAMES = $42               ; frames of the daze left

; @routine 83:9C49 Kind20Dazed
; @actor
; WHAT IT DOES: Actor kind $20's guard knocked down: it can no longer be
; hit as before, falls to the ground, then sits dazed for 96 frames;
; coming down on it or a shot from its front knocks it away (Kind20Hit),
; a shot from behind starts the daze again; then it gets up (Kind20Halt).
; HOW: ActorXTo40, Actor44Set40And80; facing = flip; ACTOR_09 bit 7
; cleared; PLACE bit 4: ActorPaletteFromRom with Y = 5. AnimStart 5,
; ActorPictureSend, ActorMotionPreset with Y = 2. Until ActorOutOfGround
; says Z clear, each frame (BRK first): ActorBouncesPlayer (bounced:
; ACTOR_09 bit 7 cleared, state $8C), ActorHitByShotCC, AnimStep,
; ActorPictureSend, ActorStepY, ActorAccelerateY. Then DAZE_FRAMES = $60;
; each frame (BRK first): ActorBouncesPlayer bounced: ACTOR_09 bit 7
; cleared, state $8C, Kind20Hit; ActorHitByShotCC hit (C clear): facing
; not negative, from the start again, else Kind20Hit; AnimStep,
; ActorPictureSend. At 0: ACTOR_09 bit 7 set (TSB), Kind20Halt. No script
; runs the fall, a bounce, a shot or PLACE bit 4: written from their
; bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $20 through the entry at $83:90E1.
; Exit: never returns.
%at($839C49)
Kind20Dazed:
    jsr.w ActorXTo40
    jsr.w Actor44Set40And80
    lda.b !actor_flip
    sta.b !actor_facing
    lda.b #$80
    trb.b !actor_09
    lda.b !DAZE_PLACE
    and.b #$10
    beq .look
    ldy.b #$05
    jsl.l ActorPaletteFromRom
.look:
    lda.b #$05
    jsl.l AnimStart
    jsl.l ActorPictureSend
    ldy.b #$02
    jsl.l ActorMotionPreset
.fall:
    jsl.l ActorOutOfGround
    bne .landed
    brk.b #$00
    jsr.w ActorBouncesPlayer
    beq .fallShot
    asl.b !actor_09
    lsr.b !actor_09
    lda.b #$8C
    sta.b !actor_state
.fallShot:
    jsr.w ActorHitByShotCC
    jsl.l AnimStep
    jsl.l ActorPictureSend
    jsl.l ActorStepY
    jsl.l ActorAccelerateY
    bra .fall
.landed:
    lda.b #$60
    sta.b !DAZE_FRAMES
.daze:
    brk.b #$00
    jsr.w ActorBouncesPlayer
    beq .shot
    asl.b !actor_09
    lsr.b !actor_09
    lda.b #$8C
    sta.b !actor_state
    jmp.w Kind20Hit
.shot:
    jsr.w ActorHitByShotCC
    bcs .on
    lda.b !actor_facing
    bpl Kind20Dazed
    jmp.w Kind20Hit
.on:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    dec.b !DAZE_FRAMES
    bne .daze
    lda.b #$80
    tsb.b !actor_09
    jmp.w Kind20Halt

; @routine 83:9CD2 Kind22Start
; @actor
; WHAT IT DOES: Actor kind $22's start: it stays where it was placed for
; good, its wait (Kind22Wait: Aladdin touching it is hurt; its end out of
; range) run each frame.
; HOW: ACTOR_08 0; ACTOR_09 bit 7 set (LDA, ORA, STA); each frame BRK, for
; good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $22 (the table at $83:8CAB). Exit: never returns.
Kind22Start:
    stz.b !actor_08
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
.still:
    brk.b #$00
    bra .still

; @routine 83:9CDE Kind22Second
; @actor
; WHAT IT DOES: Where actors of kind $22 go on from the actor loop in a
; state with bit 7 set (the table at $83:8DFD): ActorEndPlaced.
; HOW: JML to ActorEndPlaced ($83:96CA).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as ActorEndPlaced.
Kind22Second:
    jml.l ActorEndPlaced
%ends_at($839CE2)
