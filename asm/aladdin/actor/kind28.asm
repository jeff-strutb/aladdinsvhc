; kind28.asm: actor kind $28, which waits for Aladdin to come near, rises
; out of hiding, moves up, then drifts across bobbing on a wave; its
; variant with bit 4 of $07 is instead a spawner of its own kind while he
; is near. Its second state (downed) plays out and ends it. Its wait is
; Kind28Wait (actor_waits_small.asm).
; C: src/aladdin/actor/kind28.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!KIND28_RIGHT = $46             ; counted each frame it waits facing right (Kind28Wait)
!KIND28_UP = $45                ; set once it has risen
!KIND28_STEPS = $40             ; frames left of its rise
!KIND28_WAVE = $41              ; its place in WAVE (0 to $23)
!KIND28_FRAMES = $42            ; frames left before it ends
!SPAWN_WAIT = $43               ; the spawner variant: frames before the next (word)
!SPAWN_COUNTS = $7EFF00         ; spawned actors counted by kind
!KIND28_WAVE_STEPS = $D07A      ; in bank $80: its y step each frame of the wave (signed bytes)

; @routine 83:A03E Kind28Start
; @actor
; WHAT IT DOES: Actor kind $28: hidden until Aladdin is near, it shows
; itself facing him, waits 32 frames, comes up 15 pixels and rises for a
; while (1 frame, or $2A with bit 0 of $07), waits 8 frames, then falls
; and drifts across on a wave (or, with bit 1 of $07, drifts with no wave;
; no script has that) for good. With bit 4 of $07 it is a spawner
; instead: while he is near, an actor of its kind every $60 frames, each
; counted in SPAWN_COUNTS (no script has it).
; HOW: KIND28_RIGHT cleared; bit 4 of $07: on into the spawner. ACTOR_08
; bit 1, ACTOR_09 bit 4 set; level $0C: ACTOR_09 bits 4-5 cleared (no script).
; KIND28_UP cleared; bit 3: AnimStart 1 at once (no script); else BRK
; each frame until ActorNearPlayer with Y = $28, then AnimStart 1. Facing
; from ActorSideOfPlayer (bit 2 of $07: from its x's bit 0 instead, no
; script); ACTOR_09 bit 7 set; COP $20; y less 15 (SBC with the carry as
; AnimStart left it); AnimStart 0; KIND28_STEPS = 1 or $2A by bit 0 of
; $07 (shifted out); ActorMotionPreset with Y = $44; each frame (BRK)
; ActorStepY until the steps run out; KIND28_UP up; COP 8. Bit 1 of $07
; (shifted out): ActorFallStart, then each frame ActorMoveX and
; ActorRiseAfterDelay. Else ActorFallStart, KIND28_WAVE = $0A, and each
; frame (BRK) ActorMoveX, its y plus WAVE's step, KIND28_WAVE on (back to
; 0 at $24), ActorRiseAfterDelay. The spawner: $07 bit 4 cleared, bit 3
; set; SPAWN_WAIT and $21 cleared; each frame (BRK) while SPAWN_WAIT
; counts down, or ActorNearPlayer with Y = $28 says he is far; then
; ActorSpawnSameLook with its kind (a record: SPAWN_COUNTS by kind up),
; SPAWN_WAIT = $160, and each frame it counts down (to 0: spawn again)
; while he stays near.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $28 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83A03E)
Kind28Start:
    stz.b !KIND28_RIGHT
    lda.b !actor_07
    and.b #$10
    beq .shown
    jmp.w .spawner
.shown:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$10
    tsb.b !actor_09
    lda.w !level
    cmp.b #$0C
    bne .level
    lda.b #$30
    trb.b !actor_09
.level:
    stz.b !KIND28_UP
    lda.b !actor_07
    and.b #$08
    beq .hidden
    lda.b #$01
    jsl.l AnimStart
    bra .show
.hidden:
    brk.b #$00
    ldy.b #$28
    jsl.l ActorNearPlayer
    bcs .hidden
    lda.b #$01
    jsl.l AnimStart
.show:
    lda.b !actor_07
    and.b #$04
    beq .side
    lda.b !actor_x+1
    and.b #$01
    bra .face
.side:
    jsl.l ActorSideOfPlayer
.face:
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    lda.b #$80
    tsb.b !actor_09
    lda.b #$20
    cop.b #$00
    rep.b #$20
    lda.b !actor_y+1
    sbc.w #$000F
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
    ldy.b #$01
    lsr.b !actor_07
    bcc .steps
    ldy.b #$2A
.steps:
    sty.b !KIND28_STEPS
    ldy.b #$44
    jsl.l ActorMotionPreset
.rise:
    brk.b #$00
    jsl.l ActorStepY
    dec.b !KIND28_STEPS
    bne .rise
    inc.b !KIND28_UP
    lda.b #$08
    cop.b #$00
    lsr.b !actor_07
    bcc .wave
    jsr.w ActorFallStart
.flat:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w ActorRiseAfterDelay
    bra .flat
.wave:
    jsr.w ActorFallStart
    lda.b #$0A
    sta.b !KIND28_WAVE
.drift:
    brk.b #$00
    jsl.l ActorMoveX
    ldx.b !KIND28_WAVE
    ldy.b #$00
    lda.w !KIND28_WAVE_STEPS,x
    bpl .up
    ldy.b #$FF
.up:
    tax
    tya
    xba
    txa
    rep.b #$21
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    inc.b !KIND28_WAVE
    lda.b !KIND28_WAVE
    cmp.b #$24
    bne .on
    stz.b !KIND28_WAVE
.on:
    jsr.w ActorRiseAfterDelay
    bra .drift
.spawner:
    lda.b !actor_07
    and.b #$EF
    ora.b #$08
    sta.b !actor_07
    stz.b !SPAWN_WAIT
    stz.b !SPAWN_WAIT+1
    stz.b !actor_spawn_at
.wait:
    brk.b #$00
    lda.b !SPAWN_WAIT
    bne .count
    lda.b !SPAWN_WAIT+1
    beq .near
.count:
    rep.b #$20
    dec.b !SPAWN_WAIT
    sep.b #$20
    bra .wait
.near:
    ldy.b #$28
    jsl.l ActorNearPlayer
    bcs .wait
.spawn:
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .again
    ldx.b !actor_06
    lda.l !SPAWN_COUNTS,x
    inc a
    sta.l !SPAWN_COUNTS,x
.again:
    lda.b #$60
    sta.b !SPAWN_WAIT
    lda.b #$01
    sta.b !SPAWN_WAIT+1
.next:
    brk.b #$00
    rep.b #$20
    dec.b !SPAWN_WAIT
    sep.b #$20
    beq .spawn
    ldy.b #$28
    jsl.l ActorNearPlayer
    bcs .wait
    bra .next

; @routine 83:A196 Kind28Second
; @actor
; WHAT IT DOES: Actor kind $28 in its second state (downed): a sound ($2C,
; and $29 first when it never waited facing right; no script has that),
; animation 2 for 32 frames, then its end.
; HOW: KIND28_RIGHT zero: SoundQueueAdd $29. SoundQueueAdd $2C;
; AnimStart 2; KIND28_FRAMES = $20; each frame (BRK) AnimStep until they
; run out; on into ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $28 in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83A196)
Kind28Second:
    lda.b !KIND28_RIGHT
    bne .sound
    lda.b #$29
    jsl.l SoundQueueAdd
.sound:
    lda.b #$2C
    jsl.l SoundQueueAdd
    lda.b #$02
    jsl.l AnimStart
    lda.b #$20
    sta.b !KIND28_FRAMES
.end:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND28_FRAMES
    bne .end
    jmp.w ActorEndPlaced
