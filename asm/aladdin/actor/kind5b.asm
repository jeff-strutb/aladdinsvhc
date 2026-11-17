; kind5b.asm: actor kind $5B, a thing that is thrown up and falls away:
; with $07 bit 7 clear it first shows, waits, leaves a copy of itself
; (counted in a table by its hit box), then is thrown; with bit 7 set it is
; thrown at once and lets its wait know it is falling.
; C: src/aladdin/actor/kind5b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K5B_FRAMES = $40               ; frames left of the fall
!K5B_07 = $41                   ; its $07, kept over the copy's spawn
!K5B_FALLING = $42              ; set while the bit-7 kind falls (its wait reads it)
!K5B_43 = $43                   ; cleared as it starts
!K5B_DELAYS = $D3F8             ; by $07: frames before it is thrown (in the bank DB names)
!SPAWN_COUNTS = $7EFF00         ; by hit box: copies spawned
!COPY_HIT_BOX = $4A             ; the copy's spawn place

; @routine 83:DF14 Kind5BStart
; @actor
; WHAT IT DOES: Actor kind $5B. $07 bit 7 clear: pose 2 for its delay
; (K5B_DELAYS by $07), pose 1 for 32 frames, a copy of itself spawned
; (counted in SPAWN_COUNTS by its hit box), then thrown up: it rises,
; waits 40 frames, falls for 128 and ends. Bit 7 set: pose 0, thrown at
; once with K5B_FALLING set, it falls for 96 frames, K5B_FALLING
; cleared, and it ends.
; HOW: K5B_43, K5B_FALLING 0; ACTOR_09 bits $90 set; K5B_07 = $07.
; Clear: ACTOR_08 bits 0-1 set; AnimStart 2; COP the delay; AnimStart 1;
; COP $20; ACTOR_SPAWN_AT $4A, $07 $FF, ActorSpawnSameLook with its hit
; box: spawned, SPAWN_COUNTS by the hit box up; $07 back. Then (both):
; ActorStopX, ActorSpeedYSet $3C, gravity 4; BRK and ActorFly while the
; vertical speed is negative; COP $28; frames of BRK and ActorFly; then
; ActorEndPlaced. Set: ACTOR_08 bit 1 set; AnimStart 0; K5B_FALLING up
; before, and 0 after.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds K5B_DELAYS (bank $83); reached
; by the actor loop's jump for kind $5B in state $0C (the table at
; $83:8CAB). Exit: never returns; ends in ActorEndPlaced.
%at($83DF14)
Kind5BStart:
    stz.b !K5B_43
    stz.b !K5B_FALLING
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b !actor_07
    sta.b !K5B_07
    bpl .shown
    jmp.w .at_once
.shown:
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    lda.b #$02
    jsl.l AnimStart
    ldx.b !actor_07
    lda.w !K5B_DELAYS,x
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #!COPY_HIT_BOX
    sta.b !actor_spawn_at
    lda.b #$FF
    sta.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .none
    ldx.b !actor_06
    lda.l !SPAWN_COUNTS,x
    inc a
    sta.l !SPAWN_COUNTS,x
.none:
    lda.b !K5B_07
    sta.b !actor_07
    jsl.l ActorStopX
    ldy.b #$3C
    jsl.l ActorSpeedYSet
    lda.b #$04
    sta.b !actor_accel_y
.rise:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .rise
    lda.b #$28
    cop.b #$00
    lda.b #$80
    sta.b !K5B_FRAMES
.fall:
    brk.b #$00
    jsl.l ActorFly
    dec.b !K5B_FRAMES
    bne .fall
    bra .end
.at_once:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b #$00
    jsl.l AnimStart
    jsl.l ActorStopX
    ldy.b #$3C
    jsl.l ActorSpeedYSet
    lda.b #$04
    sta.b !actor_accel_y
    inc.b !K5B_FALLING
.rise_at_once:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .rise_at_once
    lda.b #$28
    cop.b #$00
    lda.b #$60
    sta.b !K5B_FRAMES
.fall_at_once:
    brk.b #$00
    jsl.l ActorFly
    dec.b !K5B_FRAMES
    bne .fall_at_once
    stz.b !K5B_FALLING
.end:
    jml.l ActorEndPlaced
