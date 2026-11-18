; kind6d.asm: actor kind $6D (the pots and the like that the wait routine
; Kind6DWait drives): its start, eight ways of moving by its $07, and what
; it does once downed (its state with bit 7 set): knocked away, a sound,
; and off until it is out of range.
; C: src/aladdin/actor/kind6d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K6D_FRAMES = $40               ; frames left before it flies off
!K6D_DOWNED = $48               ; counted up by Kind6DWait when a shot downs it
!K6D_KNOCKED = $1F              ; the motion preset it is knocked away with
!K6D_POSE = $07                 ; its animation once downed
!K6D_SOUND = $0F                ; the sound of its fall
!K6D_CHECK = $703000            ; the copier check's bytes: work RAM on a copier, none on the cartridge
!K6D_PROBES = $D667             ; its probe list
!ACTOR_PROBES = $16             ; its probe list's address (word)
!K6D_RECORD = $00               ; its record's start: with X $40 down to 1, bytes $01-$40 set to $AA
!K6D_TURNS = $41                ; the homing way's turns left; then its turn step
!K6D_SPAWN_SLOT = $21           ; the slot byte its children are made with
!K6D_STEP = $47                 ; the spawner's own $07 kept while a child is made
!K6D_RANGE_X = $1A44            ; the x of the point actors are kept in range of (word)

; @routine 83:ECD4 Kind6DStart
; @actor
; WHAT IT DOES: Actor kind $6D's start: set up (a copier's cartridge RAM
; makes it wipe its own record), facing by $07 bits 0-1 (or toward the
; player), then one of eight ways by $07 bits 2-4: 0 back and forth
; across, 1 up and down, 2 a run that rises away, 3 and 7 falls and hops
; (presets $55 and $7F), 4 and 6 a homing flight that turns toward the
; player, 5 a spawner of its own kind.
; HOW: The byte at K6D_CHECK + X up and compared: equal, $01-$40 of its
; record filled with $AA (D-relative) and its $06 0. ACTOR_08 2, ACTOR_09
; $90, its probes K6D_PROBES, ActorStopX, ActorStopY. $07 bits 0-1 nonzero:
; facing and flip that shifted right once; else ActorSideOfPlayer's. $48
; 0. JMP through the table at $83:ED23 by $07 bits 2-4: 0 animation 8,
; ActorMotionReset 0; each leg: acceleration 4, $60 frames (BRK) of
; ActorMoveX and ActorAccelerateX (acceleration 0 at speed $C0), flip
; turned; frames of ActorMoveX and ActorDecelerateX until stopped; facing
; turned, COP $18. 1: animation 8, facing and $49 0; each leg
; ActorMotionReset 3, $60 frames of ActorStepYBack or ActorStepY (by the
; facing) and ActorAccelerateY (gravity 0 at speed $C0), gravity 3,
; frames of it with ActorDecelerateY until it turns; facing turned. 2:
; animations 2 then 4, acceleration 2, ActorSpeedXSet $5A, $40 frames of
; ActorMoveX and ActorAccelerateX; animation 8, acceleration 0, gravity
; $30, $10 frames of ActorStepYBack, ActorMoveX, ActorAccelerateY; then
; with ActorDecelerateY until it falls; then ActorMoveX each frame for
; good. 3: ActorMotionPreset $55, vertical speed 0;
; animation 8; each frame ActorFly, ActorOutOfWallProbe4 (a wall: facing
; and flip turned) until ActorOutOfGround; animation 2, COP 8, animation
; 1, COP 8, preset again; 7 the same with preset $7F, no wall test, and
; its gravity cut while it falls at $0100 or more. 4 (and 6): animation
; 8, K6D_TURNS $14, facing
; times 8 (a direction); each 12 frames of ActorMoveByVector $18, a
; turn toward ActorDirectionToPlayer; after $14, turning on one way each
; 12 frames until it points up ($0C), ActorMoveByVector each frame for
; good. 5: a child of its kind ($07 + 4, slot 0, ActorSpawnSameLook; $07
; back); made, $E0 frames (BRK); then BRK while within $90 of K6D_RANGE_X,
; and again once farther.
; Entry: M 8-bit, X 8-bit (twice its kind); D = the actor's record; DB =
; $80; reached by the actor loop's jump for kind $6D in state $0C (the
; table at $83:8CAB). Exit: never returns.
%at($83ECD4)
Kind6DStart:
    lda.l !K6D_CHECK,x
    inc a
    sta.l !K6D_CHECK,x
    cmp.l !K6D_CHECK,x
    bne .set
    ldx.b #$40
    lda.b #$AA
.wipe:
    sta.b !K6D_RECORD,x
    dex
    bne .wipe
    stz.b !actor_06
.set:
    lda.b #$02
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #!K6D_PROBES
    sta.b !ACTOR_PROBES
    sep.b #$20
    jsl.l ActorStopX
    jsl.l ActorStopY
    lda.b !actor_07
    and.b #$03
    beq .toward
    lsr a
    bra .face
.toward:
    jsl.l ActorSideOfPlayer
.face:
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !actor_48
    lda.b !actor_07
    and.b #$1C
    lsr a
    tax
    jmp.w (.ways,x)
.ways:
    dw .across, .updown, .run, .hop, .homing, .spawner, .homing6, .hop7
.across:
    lda.b #$08
    jsl.l AnimStart
    lda.b #$00
    jsl.l ActorMotionReset
.leg:
    lda.b #$04
    sta.b !actor_accel_x
    lda.b #$60
    sta.b !K6D_FRAMES
.speed:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorAccelerateX
    rep.b #$20
    lda.b !actor_speed_x
    cmp.w #$00C0
    sep.b #$20
    bcc .speeding
    stz.b !actor_accel_x
.speeding:
    dec.b !K6D_FRAMES
    bne .speed
    lda.b #$04
    sta.b !actor_accel_x
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
.slow:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorDecelerateX
    bpl .slow
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.b #$18
    cop.b #$00
    bra .leg
.updown:
    lda.b #$08
    jsl.l AnimStart
    stz.b !actor_facing
    stz.b !actor_49
.vleg:
    lda.b #$03
    jsl.l ActorMotionReset
    lda.b #$60
    sta.b !K6D_FRAMES
.vspeed:
    brk.b #$00
    lda.b !actor_facing
    beq .vdown
    jsl.l ActorStepYBack
    bra .vaccel
.vdown:
    jsl.l ActorStepY
.vaccel:
    jsl.l ActorAccelerateY
    rep.b #$20
    lda.b !actor_speed_y
    cmp.w #$00C0
    sep.b #$20
    bcc .vspeeding
    stz.b !actor_accel_y
.vspeeding:
    dec.b !K6D_FRAMES
    bne .vspeed
    lda.b #$03
    sta.b !actor_accel_y
.vslow:
    brk.b #$00
    lda.b !actor_facing
    beq .vslow_down
    jsl.l ActorStepYBack
    bra .vdecel
.vslow_down:
    jsl.l ActorStepY
.vdecel:
    jsl.l ActorDecelerateY
    bpl .vslow
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    bra .vleg
.run:
    lda.b #$02
    jsl.l AnimStart
    lda.b #$04
    jsl.l AnimStart
    lda.b #$02
    sta.b !actor_accel_x
    ldy.b #$5A
    jsl.l ActorSpeedXSet
    lda.b #$40
    sta.b !K6D_FRAMES
.run_on:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorAccelerateX
    dec.b !K6D_FRAMES
    bne .run_on
    lda.b #$08
    jsl.l AnimStart
    stz.b !actor_accel_x
    lda.b #$30
    sta.b !actor_accel_y
    lda.b #$10
    sta.b !K6D_FRAMES
.rise:
    brk.b #$00
    jsl.l ActorStepYBack
    jsl.l ActorMoveX
    jsl.l ActorAccelerateY
    dec.b !K6D_FRAMES
    bne .rise
.rising:
    brk.b #$00
    jsl.l ActorStepYBack
    jsl.l ActorMoveX
    jsl.l ActorDecelerateY
    lda.b !actor_speed_y+2
    bpl .rising
.away:
    brk.b #$00
    jsl.l ActorMoveX
    bra .away
.hop:
    ldy.b #$55
    jsl.l ActorMotionPreset
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    bra .hop_pose
.hop_again:
    ldy.b #$55
    jsl.l ActorMotionPreset
.hop_pose:
    lda.b #$08
    jsl.l AnimStart
.hop_fall:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    bcc .hop_ground
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.hop_ground:
    jsl.l ActorOutOfGround
    beq .hop_fall
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    bra .hop_again
.homing:
    lda.b #$08
    jsl.l AnimStart
    lda.b #$14
    sta.b !K6D_TURNS
    lda.b !actor_facing
    asl a
    asl a
    asl a
    sta.b !actor_facing
.home_step:
    lda.b #$0C
    sta.b !K6D_FRAMES
.home_fly:
    brk.b #$00
    ldx.b #$18
    jsl.l ActorMoveByVector
    dec.b !K6D_FRAMES
    bne .home_fly
    dec.b !K6D_TURNS
    beq .home_up
    jsl.l ActorDirectionToPlayer
    sec
    sbc.b !actor_facing
    and.b #$0F
    beq .home_step
    cmp.b #$08
    ldx.b !actor_facing
    inx
    bcc .home_turn
    dex
    dex
.home_turn:
    txa
    and.b #$0F
    sta.b !actor_facing
    clc
    adc.b #$04
    and.b #$0F
    lsr a
    lsr a
    lsr a
    sta.b !actor_flip
    bra .home_step
.home_up:
    lda.b #$0C
    sta.b !K6D_FRAMES
    lda.b !actor_flip
    bne .turn_way
    lda.b #$FF
.turn_way:
    sta.b !K6D_TURNS
.up_turn:
    lda.b !actor_facing
    and.b #$0F
    cmp.b #$0C
    beq .up_fly
    dec.b !K6D_FRAMES
    bne .up_fly
    clc
    lda.b !actor_facing
    adc.b !K6D_TURNS
    and.b #$0F
    sta.b !actor_facing
    lda.b #$0C
    sta.b !K6D_FRAMES
.up_fly:
    brk.b #$00
    ldx.b #$18
    jsl.l ActorMoveByVector
    bra .up_turn
.spawner:
    lda.b !actor_07
    sta.b !K6D_STEP
.spawn:
    lda.b #$E0
    sta.b !K6D_FRAMES
    clc
    lda.b !actor_07
    adc.b #$04
    sta.b !actor_07
    stz.b !K6D_SPAWN_SLOT
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .near
    lda.b !K6D_STEP
    sta.b !actor_07
.spawn_wait:
    brk.b #$00
    dec.b !K6D_FRAMES
    bne .spawn_wait
.near:
    brk.b #$00
    rep.b #$20
    sec
    lda.w !K6D_RANGE_X
    sbc.b !actor_x+1
    bpl .distance
    eor.w #$FFFF
    inc a
.distance:
    cmp.w #$0090
    sep.b #$20
    bcc .near
    bra .spawn
.homing6:
    jmp.w .homing
.hop7:
    ldy.b #$7F
    jsl.l ActorMotionPreset
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    bra .hop7_pose
.hop7_again:
    ldy.b #$7F
    jsl.l ActorMotionPreset
.hop7_pose:
    lda.b #$08
    jsl.l AnimStart
.hop7_fall:
    brk.b #$00
    jsl.l ActorFly
    rep.b #$20
    lda.b !actor_speed_y
    bmi .hop7_ground
    cmp.w #$0100
    sep.b #$20
    bcc .hop7_ground
    stz.b !actor_accel_y
.hop7_ground:
    sep.b #$20
    jsl.l ActorOutOfGround
    beq .hop7_fall
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    bra .hop7_again
%ends_at($83EF92)

; @routine 83:EFF0 Kind6DSecond
; @actor
; WHAT IT DOES: Actor kind $6D downed (its state with bit 7 set): knocked
; away in pose 7, with sound $0F unless a shot downed it while it faced
; right; it turns to face right, holds 10 frames, then flies each frame
; until it is out of range.
; HOW: ActorMotionPreset $1F, AnimStart 7, K6D_FRAMES $0A. K6D_DOWNED zero
; or its facing with bit 7 set: SoundQueueAdd $0F. Facing bit 7 cleared.
; Each frame (BRK) AnimStep, K6D_FRAMES times; then each frame (BRK)
; AnimStep, ActorFly and ActorEndOutOfRange with X = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $6D in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns; ends when out of range.
%at($83EFF0)
Kind6DSecond:
    ldy.b #!K6D_KNOCKED
    jsl.l ActorMotionPreset
    lda.b #!K6D_POSE
    jsl.l AnimStart
    lda.b #$0A
    sta.b !K6D_FRAMES
    lda.b !K6D_DOWNED
    beq .sound
    lda.b !actor_facing
    bpl .face
.sound:
    lda.b #!K6D_SOUND
    jsl.l SoundQueueAdd
.face:
    lda.b !actor_facing
    bpl .hold
    and.b #$7F
    sta.b !actor_facing
.hold:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K6D_FRAMES
    bne .hold
.fly:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .fly
