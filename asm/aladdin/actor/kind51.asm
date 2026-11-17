; kind51.asm: actor kind $51, a thing that rises and settles (Aladdin can
; land on it once it has): its start and cycle, what it does in its second
; state, its variant with $07 bit 1 set, which plays once and is gone, and
; the jump its code ends by.
; C: src/aladdin/actor/kind51.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND51_FRAMES = $40            ; frames left of the present step
!KIND51_SOLID = $41             ; nonzero: Aladdin can land on it (its wait tests it)
!V_08F9 = $08F9                 ; read (a word) and not used by its second state

; @routine 83:D239 Kind51Start
; @actor
; WHAT IT DOES: Actor kind $51's start: drawn in front and with its own
; frame parity bits; with $07 bit 1 set it is the variant that plays once
; (Kind51Variant), else it cycles (Kind51Cycle).
; HOW: ACTOR_08 bits 0-1 and ACTOR_09 bits $90 set (TSB); $07 AND 2:
; Kind51Variant (JMP), else on into Kind51Cycle.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $51 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83D239)
Kind51Start:
    lda.b #$03
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    and.b #$02
    beq Kind51Cycle
    jmp.w Kind51Variant

; @routine 83:D24A Kind51Cycle
; @actor
; WHAT IT DOES: Kind $51's cycle, for good: down (animation 0, not to be
; landed on) for 64 frames, or 144 with $07 bit 0 set; then up (animation
; 1, Aladdin can land on it) for 128.
; HOW: KIND51_SOLID 0, AnimStart 0; KIND51_FRAMES $40 (or $90) frames of
; BRK; AnimStart 1, KIND51_FRAMES $80, KIND51_SOLID up, $80 frames of
; BRK; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached from
; Kind51Start and by a jump from Kind51Second (83:D32C). Exit: never
; returns.
%at($83D24A)
Kind51Cycle:
    stz.b !KIND51_SOLID
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$40
    lda.b !actor_07
    and.b #$01
    beq .down
    ldx.b #$90
.down:
    txa
    sta.b !KIND51_FRAMES
.down_wait:
    brk.b #$00
    dec.b !KIND51_FRAMES
    bne .down_wait
    lda.b #$01
    jsl.l AnimStart
    lda.b #$80
    sta.b !KIND51_FRAMES
    inc.b !KIND51_SOLID
.up_wait:
    brk.b #$00
    dec.b !KIND51_FRAMES
    bne .up_wait
    bra Kind51Cycle

; @routine 83:D2BE Kind51Second
; @actor
; WHAT IT DOES: Kind $51 in its second state: animation 2 with sound $0F
; for 65 frames, then by a random pick animation 3, or animation 4 with a
; pair of actors started; 96 frames turned away from Aladdin; then back
; to its start state and its cycle.
; HOW: AnimStart 2, SoundQueueAdd $0F; KIND51_FRAMES $20; a BRK and
; AnimStep, the word at V_08F9 read; $20 frames, then $20 more, of BRK
; and AnimStep. RandomFromList list $0C: 0, AnimStart 3; else
; ActorSpawnPair, AnimStart 4. KIND51_FRAMES $60: each frame facing and
; flip = ActorSideOfPlayer EOR 1, BRK, AnimStep. ACTOR_09 bit 7 and state
; bits $0C set; facing and flip 0; Kind51Cycle (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $51 with state bit 7 set (the table at
; $83:8DFD). Exit: never returns.
%at($83D2BE)
Kind51Second:
    lda.b #$02
    jsl.l AnimStart
    lda.b #$0F
    jsl.l SoundQueueAdd
    lda.b #$20
    sta.b !KIND51_FRAMES
    brk.b #$00
    jsl.l AnimStep
    rep.b #$20
    lda.w !V_08F9
    sep.b #$20
.first:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND51_FRAMES
    bne .first
    lda.b #$20
    sta.b !KIND51_FRAMES
.second:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND51_FRAMES
    bne .second
    ldx.b #$0C
    jsl.l RandomFromList
    bne .pair
    lda.b #$03
    bra .pose
.pair:
    jsr.w ActorSpawnPair
    lda.b #$04
.pose:
    jsl.l AnimStart
    lda.b #$60
    sta.b !KIND51_FRAMES
.turned:
    jsl.l ActorSideOfPlayer
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND51_FRAMES
    bne .turned
    lda.b #$80
    tsb.b !actor_09
    lda.b #$0C
    tsb.b !actor_state
    stz.b !actor_facing
    stz.b !actor_flip
    jmp.w Kind51Cycle

; @routine 83:D279 Kind51Variant
; @actor
; WHAT IT DOES: Kind $51 with $07 bit 1 set: after a pause when $07 bit 2
; is set, plays its animation 5 for $58 frames, then is gone for good.
; HOW: KIND51_SOLID cleared; $07 bit 2: KIND51_FRAMES = $0A frames
; (BRK each). AnimStart 5; KIND51_FRAMES = $58 frames of BRK and
; AnimStep; then Kind51End.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
; jump from its start (83:D247). Exit: never returns: as Kind51End.
%at($83D279)
Kind51Variant:
    stz.b !KIND51_SOLID
    lda.b !actor_07
    and.b #$04
    beq .play
    lda.b #$0A
    sta.b !KIND51_FRAMES
.pause:
    brk.b #$00
    dec.b !KIND51_FRAMES
    bne .pause
.play:
    lda.b #$05
    jsl.l AnimStart
    lda.b #$58
    sta.b !KIND51_FRAMES
.step:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND51_FRAMES
    bne .step
    jmp.w Kind51End

; @routine 83:D32F Kind51End
; @actor
; WHAT IT DOES: Where kind $51's code ends: the actor gone for good.
; HOW: JMP to ActorEndPlaced.
; Entry: D = the actor's record; reached by a jump from Kind51Variant
; (83:D29F). Exit: never returns: as ActorEndPlaced.
%at($83D32F)
Kind51End:
    jmp.w ActorEndPlaced
