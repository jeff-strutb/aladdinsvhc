; kind3b.asm: actor kind $3B, by its $07 a switch, a marker of a level
; flag, or a platform that carries the player: across, up and down, or
; along a track drawn in the map. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/kind3b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K3B_LEVEL_FLAGS = $7EF260       ; eight level flags (bytes); bit 7 set by actors
!K3B_PARTS = $C058               ; in bank $83: the platform parts' code (words)
!K3B_FRAMES = $40                ; frames left
!K3B_RUN = $41                   ; frames of a run after the first
!K3B_TOGGLED = $42               ; the switch: nonzero once it toggled; the marker: the flag shown
!K3B_STATE = $44                 ; the switch: 0 up, positive down, negative coming back up
!K3B_QUARTER = $45               ; the track: the quarter's high byte, then the direction after a spot
!K3B_TURN = $46                  ; the track: added to the direction at each step to a spot
!K3B_REST_Y = $4A                ; the switch: its y pixels at rest (word)
!K3B_TARGET_X = $1A              ; the track: the spot's target (words)
!K3B_TARGET_Y = $1E
!K3B_AHEAD = $D222               ; in bank $80: by direction times 4, x and y pixels ahead (words)
!K3B_TURNS = $D232               ; in bank $80: the direction after a turn, by the quarter's bits
!K3B_SPOT_DIRS = $D23A           ; in bank $80: the direction out of a spot
!K3B_SPOT_TURNS = $D242          ; in bank $80: the turn at each step to a spot
!K3B_PROBE_X = $000C             ; the point MapQuarterAt reads (words)
!K3B_PROBE_Y = $000E
!K3B_V_1427 = $1427              ; zero: the switch comes back up
!K3B_SWITCH_CODE = $D9C5         ; where the switch sends the player's code
!K3B_TRACK_CASES = $C103         ; in bank $83: the track's cases by the quarter's low bits (words)

; @routine 83:BF83 Kind3BStart
; @actor
; WHAT IT DOES: Actor kind $3B, by its $07: with bit 4 a switch the player
; works by coming into its zone (his code sent to $D9C5 and he is placed
; under it; the level flag of its $07 low bits toggled when it is down);
; with bit 3 a marker that shows that level flag (pose 2 plus the flag)
; and clears it 16 frames after an actor marks it; else a platform the
; player rides, one of three parts by its $07 low bits: Kind3BPart0,
; Kind3BPart1, Kind3BPart2.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; facing, flip and
; K3B_STATE 0. The switch: AnimStart 0; K3B_REST_Y = its y pixels;
; K3B_FRAMES 0; each frame (BRK first; K3B_FRAMES 0 clears K3B_TOGGLED):
; K3B_STATE 0: on when V_141C is set and ActorZoneHasTestPointLong (Y =
; $70) says C clear: the player's resume address = $D9C5,
; PlayerSpotBelowActor 3, K3B_STATE up. K3B_STATE negative: up by one.
; Positive: V_1427 0 sets it to $FC; else with K3B_TOGGLED 0, K3B_FRAMES
; at least $0F and the flag not negative, the flag'"'"'s bit 0 flipped and
; K3B_TOGGLED up. The marker: K3B_TOGGLED $FF, ACTOR_08 bits 0-1 set; each
; frame (BRK after the first): the flag negative: COP $10 and its high
; bits cleared; else when it differs from K3B_TOGGLED, kept there and
; AnimStart with it + 2. The platforms: AnimStart 4; JMP through
; K3B_PARTS by ($07 times 2) AND 7. No script runs the switch past its
; first frames: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $3B in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83BF83)
Kind3BStart:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !K3B_STATE
    lda.b !actor_07
    bit.b #$10
    bne .switch
    bit.b #$08
    bne .marker
    jmp.w .platform
.switch:
    lda.b #$00
    jsl.l AnimStart
    rep.b #$20
    lda.b !actor_y+1
    sta.b !K3B_REST_Y
    sep.b #$20
    stz.b !K3B_STATE
    stz.b !K3B_FRAMES
.frame:
    lda.b !K3B_FRAMES
    bne .yield
    stz.b !K3B_TOGGLED
.yield:
    brk.b #$00
    lda.b !K3B_STATE
    beq .up
    bmi .rising
    lda.w !K3B_V_1427
    bne .down
    lda.b #$FC
    sta.b !K3B_STATE
    bra .frame
.down:
    lda.b !K3B_TOGGLED
    bne .frame
    lda.b !K3B_FRAMES
    cmp.b #$0F
    bcc .frame
    lda.b !actor_07
    and.b #$07
    tax
    lda.l !K3B_LEVEL_FLAGS,x
    bmi .frame
    eor.b #$01
    sta.l !K3B_LEVEL_FLAGS,x
    inc.b !K3B_TOGGLED
    bra .frame
.up:
    lda.w !v_141c
    beq .frame
    ldy.b #$70
    jsl.l ActorZoneHasTestPointLong
    bcs .frame
    rep.b #$20
    lda.w #!K3B_SWITCH_CODE
    sta.w !player+!actor_03
    sep.b #$20
    lda.b #$03
    jsl.l PlayerSpotBelowActor
.rising:
    inc.b !K3B_STATE
    bra .frame
.marker:
    lda.b #$FF
    sta.b !K3B_TOGGLED
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    bra .show
.markerFrame:
    brk.b #$00
.show:
    lda.b !actor_07
    and.b #$07
    tax
    lda.l !K3B_LEVEL_FLAGS,x
    bmi .marked
    cmp.b !K3B_TOGGLED
    beq .markerFrame
    sta.b !K3B_TOGGLED
    inc a
    inc a
    jsl.l AnimStart
    bra .markerFrame
.marked:
    lda.b #$10
    cop.b #$00
    lda.b !actor_07
    and.b #$07
    tax
    lda.l !K3B_LEVEL_FLAGS,x
    and.b #$0F
    sta.l !K3B_LEVEL_FLAGS,x
    bra .markerFrame
.platform:
    lda.b #$04
    jsl.l AnimStart
    lda.b !actor_07
    asl a
    and.b #$07
    tax
    jmp.w (!K3B_PARTS,x)

; @routine 83:C05E Kind3BPart0
; @actor
; WHAT IT DOES: Kind $3B's platform part 0: after a frame it moves away
; from the player'"'"'s facing for 32 frames carrying him, rests 12 frames,
; turns, and again for 64 frames, back and forth for good.
; HOW: BRK; facing = the player'"'"'s facing EOR 1; K3B_RUN = $40;
; ActorSpeedXSet with entry $0C; K3B_FRAMES = $20; each frame (BRK first)
; ActorMoveX and ActorCarryPlayer until K3B_FRAMES runs out; facing
; flipped; 12 BRKs; K3B_FRAMES = K3B_RUN, again. No script runs past its
; first BRK: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81 (the speed
; table's bank); reached by a jump from Kind3BStart. Exit: never returns.
%at($83C05E)
Kind3BPart0:
    brk.b #$00
    lda.w !player+!actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.b #$40
    sta.b !K3B_RUN
    ldy.b #$0C
    jsl.l ActorSpeedXSet
    lda.b #$20
    sta.b !K3B_FRAMES
.run:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w ActorCarryPlayer
    dec.b !K3B_FRAMES
    bne .run
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.b #$0C
    sta.b !K3B_FRAMES
.rest:
    brk.b #$00
    dec.b !K3B_FRAMES
    bne .rest
    lda.b !K3B_RUN
    sta.b !K3B_FRAMES
    bra .run

; @routine 83:C098 Kind3BPart1
; @actor
; WHAT IT DOES: Kind $3B's platform part 1: after a frame it moves up and
; down, carrying the player: 32 frames one way first, then 64 frames each
; way, resting 12 frames at each end.
; HOW: BRK; K3B_RUN = $40; ActorSpeedYSet with entry $1E; K3B_FRAMES =
; $20; each frame (BRK first) ActorMoveY and ActorCarryPlayer until
; K3B_FRAMES runs out; the speed turned: its low word negated, its top
; byte EOR $FF; 12 BRKs; K3B_FRAMES = K3B_RUN, again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81 (the speed
; table's bank); reached by a jump from Kind3BStart. Exit: never returns.
Kind3BPart1:
    brk.b #$00
    lda.b #$40
    sta.b !K3B_RUN
    ldy.b #$1E
    jsl.l ActorSpeedYSet
    lda.b #$20
    sta.b !K3B_FRAMES
.run:
    brk.b #$00
    jsl.l ActorMoveY
    jsr.w ActorCarryPlayer
    dec.b !K3B_FRAMES
    bne .run
    rep.b #$20
    lda.b !actor_speed_y
    eor.w #$FFFF
    inc a
    sta.b !actor_speed_y
    sep.b #$20
    lda.b !actor_speed_y+2
    eor.b #$FF
    sta.b !actor_speed_y+2
    lda.b #$0C
    sta.b !K3B_FRAMES
.rest:
    brk.b #$00
    dec.b !K3B_FRAMES
    bne .rest
    lda.b !K3B_RUN
    sta.b !K3B_FRAMES
    bra .run

; @routine 83:C0D7 Kind3BPart2
; @actor
; WHAT IT DOES: Kind $3B's platform part 2: follows a track drawn in the
; map, carrying the player: at each step it reads the map quarter ahead
; in its direction and by its low bits goes on 8 steps, goes on and turns
; back, goes on and turns by the quarter'"'"'s high bits, waits a frame, or
; at a flagged spot marks the level flag there and travels to the spot'"'"'s
; target in 5 steps of three frames, turning as it goes.
; HOW: ActorFacePlayer; the direction ($14) doubled. Each step: $000C and
; $000E = its x and y pixels plus the word pair of K3B_AHEAD by the
; direction times 4; MapQuarterAt; its low 3 bits times 2 pick the case
; (the table at $83:C103), its high byte kept in K3B_QUARTER. 0, 1: 8
; frames (BRK first) of ActorStepCarry. 4, 5: the same, the direction EOR
; 2, 8 BRKs. 2: the same 8 steps, then the direction from K3B_TURNS by
; K3B_QUARTER'"'"'s top two bits and the direction'"'"'s bit 0 (shifted out), 8
; BRKs. 3: K3B_TARGET_X and _Y = $000C, $000E until LevelFlagSpotFind
; finds the spot (C clear); the flag'"'"'s bit 7 set (LevelFlagFromMask, ORA
; $80); by its bit 0 (the flag'"'"'s old value'"'"'s bit 1) and the direction:
; K3B_TURN from K3B_SPOT_TURNS, K3B_QUARTER from K3B_SPOT_DIRS, the target
; moved by K3B_AHEAD by that direction times 4; the direction times 4; 5
; times: three frames (BRK first) of ActorStepToTargetCarry, then the
; direction plus K3B_TURN, AND $0F; the x and y fractions cleared; the
; direction = K3B_QUARTER. 6, 7: a BRK. Then the next step. No script
; runs cases 6 and 7: written from their bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
; jump from Kind3BStart. Exit: never returns.
Kind3BPart2:
    jsr.w ActorFacePlayer
    asl.b !actor_facing
.step:
    lda.b !actor_facing
    asl a
    asl a
    tax
    rep.b #$20
    clc
    lda.w !K3B_AHEAD,x
    adc.b !actor_x+1
    sta.w !K3B_PROBE_X
    clc
    lda.w !K3B_AHEAD+2,x
    adc.b !actor_y+1
    sta.w !K3B_PROBE_Y
    jsl.l MapQuarterAt
    and.b #$07
    asl a
    tax
    xba
    sta.b !K3B_QUARTER
    jmp.w (!K3B_TRACK_CASES,x)
; (the case table and cases 6 and 7 lie between, not filed)
%at($83C118)
.on:
    lda.b #$08
    sta.b !K3B_FRAMES
.onFrame:
    brk.b #$00
    jsr.w ActorStepCarry
    dec.b !K3B_FRAMES
    bne .onFrame
    jmp.w .step
.back:
    lda.b #$08
    sta.b !K3B_FRAMES
.backFrame:
    brk.b #$00
    jsr.w ActorStepCarry
    dec.b !K3B_FRAMES
    bne .backFrame
    lda.b !actor_facing
    eor.b #$02
    sta.b !actor_facing
    lda.b #$08
    sta.b !K3B_FRAMES
.backRest:
    brk.b #$00
    dec.b !K3B_FRAMES
    bne .backRest
    jmp.w .step
.turn:
    lda.b #$08
    sta.b !K3B_FRAMES
.turnFrame:
    brk.b #$00
    jsr.w ActorStepCarry
    dec.b !K3B_FRAMES
    bne .turnFrame
    lda.b #$00
    asl.b !K3B_QUARTER
    rol a
    asl.b !K3B_QUARTER
    rol a
    lsr.b !actor_facing
    rol a
    tax
    lda.w !K3B_TURNS,x
    sta.b !actor_facing
    lda.b #$08
    sta.b !K3B_FRAMES
.turnRest:
    brk.b #$00
    dec.b !K3B_FRAMES
    bne .turnRest
    jmp.w .step
.spot:
    rep.b #$20
    lda.w !K3B_PROBE_X
    sta.b !K3B_TARGET_X
    lda.w !K3B_PROBE_Y
    sta.b !K3B_TARGET_Y
    sep.b #$20
    jsr.w LevelFlagSpotFind
    bcs .spot
    lda.l !K3B_LEVEL_FLAGS,x
    pha
    jsr.w LevelFlagFromMask
    ora.b #$80
    sta.l !K3B_LEVEL_FLAGS,x
    pla
    and.b #$0F
    lsr a
    lda.b !actor_facing
    rol a
    tax
    lda.w !K3B_SPOT_TURNS,x
    sta.b !K3B_TURN
    lda.w !K3B_SPOT_DIRS,x
    sta.b !K3B_QUARTER
    asl a
    asl a
    tax
    rep.b #$20
    clc
    lda.w !K3B_AHEAD,x
    adc.b !K3B_TARGET_X
    sta.b !K3B_TARGET_X
    clc
    lda.w !K3B_AHEAD+2,x
    adc.b !K3B_TARGET_Y
    sta.b !K3B_TARGET_Y
    sep.b #$20
    asl.b !actor_facing
    asl.b !actor_facing
    lda.b #$05
    sta.b !K3B_FRAMES
.spotStep:
    brk.b #$00
    jsr.w ActorStepToTargetCarry
    brk.b #$00
    jsr.w ActorStepToTargetCarry
    brk.b #$00
    jsr.w ActorStepToTargetCarry
    clc
    lda.b !actor_facing
    adc.b !K3B_TURN
    and.b #$0F
    sta.b !actor_facing
    dec.b !K3B_FRAMES
    bne .spotStep
    stz.b !actor_x
    stz.b !actor_y
    lda.b !K3B_QUARTER
    sta.b !actor_facing
    jmp.w .step
