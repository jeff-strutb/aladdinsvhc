; player_scene_moves.asm: scripted figures of Aladdin (bank $A5: started at
; his place, their way picked by $07 through the table at $A5:E56D): the
; ways that run and leap, and that hop for good. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/game/player_scene_moves.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!SCENE_CUE = $146C  ; counted up by scripted actors as the scene goes on
!HOP_TIME = $42  ; frames left of its run

; @routine A5:EC45 PlayerSceneLeap
; @actor
; WHAT IT DOES: The scripted figure's way 2: it stands $20 frames, runs
; along the ground until it leaves it, then leaps; in the air until the
; scene's cue, then holds a pose until the next cue, and stays.
; HOW: Animation 0, COP $20; animation 2, ActorSpeedXFromA5 $0F; each
; frame ActorMoveX and ActorSnapToGround while Z is clear; SCENE_CUE
; cleared, animation $0C, gravity $38, ActorSpeedYFromA5 $12; each frame
; ActorFly until SCENE_CUE is set; it is cleared, animation $0D; BRK until
; SCENE_CUE is set; bit 7 of $09 cleared; COP $7F for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 2. Exit: never returns.
%at($A5EC45)
PlayerSceneLeap:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$0F
    jsr.w ActorSpeedXFromA5
.lEC5A:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    bne .lEC5A
    stz.w !SCENE_CUE
    lda.b #$0C
    jsl.l AnimStart
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$12
    jsr.w ActorSpeedYFromA5
.lEC78:
    brk.b #$00
    jsl.l ActorFly
    lda.w !SCENE_CUE
    beq .lEC78
    stz.w !SCENE_CUE
    lda.b #$0D
    jsl.l AnimStart
.lEC8C:
    brk.b #$00
    lda.w !SCENE_CUE
    beq .lEC8C
    lda.b #$80
    trb.b !actor_09
.lEC97:
    lda.b #$7F
    cop.b #$00
    bra .lEC97


; @routine A5:EC9D PlayerSceneHops
; @actor
; WHAT IT DOES: The scripted figure's way 3: it drops to the ground facing
; left, then for good: runs $30 frames along the ground, stands, holds a
; pose, and jumps up and lands again.
; HOW: $4D = 2, facing and flip 1, gravity $38, ActorSpeedYFromA5 6;
; ActorFall each frame until ActorOutOfGround. Then for good:
; ActorSpeedYFromA5 6, ActorSpeedXFromA5 $0C, animation 1, $30 frames of
; ActorMoveX and ActorSnapToGround; animation 0, COP $30; animation $0A,
; COP $10; animation 0, gravity $38, ActorSpeedYFromA5 $0C, ActorFall each
; frame until ActorOutOfGround.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 3. Exit: never returns.
%at($A5EC9D)
PlayerSceneHops:
    lda.b #$02
    sta.b !actor_4d
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
.lECB0:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .lECB0
.lECBC:
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
    ldy.b #$0C
    jsr.w ActorSpeedXFromA5
    lda.b #$01
    jsl.l AnimStart
    lda.b #$30
    sta.b !HOP_TIME
.lECD0:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !HOP_TIME
    bne .lECD0
    lda.b #$00
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    lda.b #$0A
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$0C
    jsr.w ActorSpeedYFromA5
.lED01:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .lED01
    bra .lECBC

!SCENE_START_COUNT = $1475      ; the count A5:E515 kept when the figure started (the cartridge RAM's byte, plus 1)
!SCENE_START_SLOT = $1476       ; its index into the cartridge RAM's bytes
!SCENE_CART_RAM = $700000       ; cartridge RAM (the board has none: a read gives the bus's last byte)
!SCENE_STEP_A = $00BD           ; nonzero: the walk-off figure goes on (set by the scene)
!SCENE_STEP_B = $00BE           ; counted up as the walk-off figure has run past the player
!SCENE_STEP_C = $00BF           ; counted up as the walk-off figure has reached its mark
!SCENE_MARK_X = $0F68           ; the walk-off figure's mark
!SCENE_TILE = $0012             ; PointInGround's tile behaviour: bit 5 an edge

; @routine A5:EB31 PlayerSceneRuns
; @actor
; WHAT IT DOES: The scripted figure's way 1: it stands, then runs along
; the ground (dropping off edges as it comes to them) until it is more
; than 192 pixels right of the player; it is put at spot $1C facing left
; until the player is well on ($0B00), runs back 72 frames, rears and
; jumps, waits for the player to pass, and leaps again; on landing it
; starts over as way 0.
; HOW: Animation 0, COP $40; animation 2, ActorSpeedXFromA5 $15,
; ActorSpeedYFromA5 6. Each frame (BRK): ActorMoveX, ActorSnapToGround;
; off the ground or at an edge (bit 5): gravity $38, ActorSpeedYFromA5 9,
; each frame ActorAnimRiseFall and ActorFly until ActorOutOfGround, then
; the run again from animation 2. Else ActorAnimByTile; the player's x +
; $C0 below its own: ActorPlaceAtSpot $1C, animation 0, facing and flip
; 1; BRK until the player's x high byte is $0B or more; COP $10; facing
; and flip 0; RUN_TIME $48, animation 1, ActorSpeedXFromA5 $0C; each frame
; ActorMoveX, ActorSnapToGround, $48 frames. $49 up; animation $11, COP
; $40; animation $12, gravity 8, ActorSpeedYFromA5 $0F, gravity $18 once
; falling; each frame ActorFall until ActorOutOfGround; animation $13;
; BRK until its x + $20 is below the player's; animation $0C, gravity
; $38, ActorSpeedXFromA5 3, ActorSpeedYFromA5 $0C; each frame
; ActorAnimRiseFall, ActorFlyFallCapped, ActorOutOfWall8 until
; ActorOutOfGround; animation 0, BRK; $07 0 and on at GuardRestart ($A5:E554).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 1. Exit: never returns.
%at($A5EB31)
PlayerSceneRuns:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$40
    cop.b #$00
.run:
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$15
    jsr.w ActorSpeedXFromA5
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
.step:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    beq .drop
    lda.w !SCENE_TILE
    bit.b #$20
    bne .drop
    jsr.w ActorAnimByTile
    rep.b #$20
    clc
    lda.w !player+!actor_x+1
    adc.w #$00C0
    cmp.b !actor_x+1
    sep.b #$20
    bcs .step
    ldx.b #$1C
    jsr.w ActorPlaceAtSpot
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    bra .held
.drop:
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$09
    jsr.w ActorSpeedYFromA5
.air:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .air
    bra .run
.held:
    brk.b #$00
    lda.w !player+!actor_x+2
    cmp.b #$0B
    bcc .held
    lda.b #$10
    cop.b #$00
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$48
    sta.b !HOP_TIME
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$0C
    jsr.w ActorSpeedXFromA5
.back:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !HOP_TIME
    bne .back
    inc.b !actor_49
    lda.b #$11
    jsl.l AnimStart
    lda.b #$40
    cop.b #$00
    lda.b #$12
    jsl.l AnimStart
    lda.b #$08
    sta.b !actor_accel_y
    ldy.b #$0F
    jsr.w ActorSpeedYFromA5
.rise:
    lda.b !actor_speed_y+2
    bmi .up
    lda.b #$18
    sta.b !actor_accel_y
.up:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .rise
    lda.b #$13
    jsl.l AnimStart
.wait:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$0020
    cmp.w !player+!actor_x+1
    sep.b #$20
    bcs .wait
    lda.b #$0C
    jsl.l AnimStart
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$03
    jsr.w ActorSpeedXFromA5
    ldy.b #$0C
    jsr.w ActorSpeedYFromA5
.leap:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsr.w ActorFlyFallCapped
    jsr.w ActorOutOfWall8
    jsl.l ActorOutOfGround
    beq .leap
    lda.b #$00
    jsl.l AnimStart
    brk.b #$00
    stz.b !actor_07
    jmp.w GuardRestart
%ends_by($A5EC45)

; @routine A5:EE29 PlayerSceneWalkOff
; @actor
; WHAT IT DOES: The scripted figure's way 5: unless the cartridge RAM
; still holds the count the start kept, it goes to the escort's spot
; ($18) instead; else it falls to the ground, walks right to its mark,
; poses, runs on until it is 24 pixels past the player, turns to face
; left for a while, and, once the scene says so, walks on for good.
; HOW: SCENE_START_COUNT - 1 against the cartridge RAM's byte at
; SCENE_START_SLOT: different, on at EscortStaySpot18. Each frame
; ActorOutOfGround zero: BRK and ActorFly. Facing and flip 0, animation 1,
; ActorSpeedXFromA5 0; each frame (BRK) ActorMoveX, ActorSnapToGround
; until its x (16-bit) reaches SCENE_MARK_X. SCENE_STEP_C up; animation 6,
; COP $30; animation $1E, COP $20; animation 2, ActorSpeedXFromA5 $12;
; each frame ActorMoveX, ActorSnapToGround while the player's x + $18 is
; at least its own. SCENE_STEP_B up; animation 4, flip 1, COP $80; flip 0,
; animation 0; BRK until SCENE_STEP_A; COP $10; animation 1,
; ActorSpeedXFromA5 0; each frame ActorMoveX and ActorSnapToGround for
; good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 5. Exit: never returns.
%at($A5EE29)
PlayerSceneWalkOff:
    ldx.w !SCENE_START_SLOT
    lda.w !SCENE_START_COUNT
    dec a
    cmp.l !SCENE_CART_RAM,x
    beq .fall
    jmp.w EscortStaySpot18
.fall:
    jsl.l ActorOutOfGround
    bne .landed
    brk.b #$00
    jsl.l ActorFly
    bra .fall
.landed:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$00
    jsr.w ActorSpeedXFromA5
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    rep.b #$20
    lda.b !actor_x+1
    cmp.w #!SCENE_MARK_X
    sep.b #$20
    bcc .walk
    inc.w !SCENE_STEP_C
    lda.b #$06
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    lda.b #$1E
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$12
    jsr.w ActorSpeedXFromA5
.run:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    rep.b #$20
    clc
    lda.w !player+!actor_x+1
    adc.w #$0018
    cmp.b !actor_x+1
    sep.b #$20
    bcs .run
    inc.w !SCENE_STEP_B
    lda.b #$04
    jsl.l AnimStart
    lda.b #$01
    sta.b !actor_flip
    lda.b #$80
    cop.b #$00
    stz.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.cue:
    brk.b #$00
    lda.w !SCENE_STEP_A
    beq .cue
    lda.b #$10
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$00
    jsr.w ActorSpeedXFromA5
.on:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    bra .on
%ends_by($A5EEE1)

; @routine A5:EEE1 PlayerSceneFallPose
; @actor
; WHAT IT DOES: The scripted figure's way 6: it falls in pose $1C to the
; ground, holds there, poses $1D, and after a while starts over as way 0.
; HOW: M and X 8-bit; animation $1C; each frame (BRK) ActorFall until
; ActorOutOfGround; COP $C0; animation $1D; COP $90, COP $80; $07 0 and
; on at GuardRestart ($A5:E554).
; Entry: D = the actor's record; DB = $A5; reached by the jump through
; the table at $A5:E56D for $07 = 6. Exit: never returns.
%at($A5EEE1)
PlayerSceneFallPose:
    sep.b #$30
    lda.b #$1C
    jsl.l AnimStart
.fall:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .fall
    lda.b #$C0
    cop.b #$00
    lda.b #$1D
    jsl.l AnimStart
    lda.b #$90
    cop.b #$00
    lda.b #$80
    cop.b #$00
    stz.b !actor_07
    jmp.w GuardRestart
%ends_by($A5EF0C)
