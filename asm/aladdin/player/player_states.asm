; player_states.asm: Aladdin's code: the states he goes between as he
; stands, walks, skids, crouches, jumps and falls, lands, hangs, hovers
; and is knocked back or loses a life. Each state is actor code (it
; yields by BRK, docs/aladdin/ACTORS.md) and goes on to the next by a
; jump; the tests it calls by JSR that end in a jump to another state
; drop their own return address first.
; C: src/aladdin/player/player_states.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Pad 1 and the game's actions (system/controls.asm)
!PAD1_DIRS = $0380              ; pad 1's high byte: B Y Select Start Up Down Left Right
!PAD_UP = $08
!PAD_DOWN = $04
!JUMP_HELD = $0383              ; nonzero while the jump's button is held
!DASH_HELD = $0384              ; nonzero while the dash's button is held
!HOVER_HELD = $0386             ; nonzero while the fourth action's button is held
!JUMP_PRESSED = $0387           ; nonzero when the jump's button was just pressed

; The player's work RAM
!TILE_BITS = $0012              ; the last probed tile's behaviour (bit 5: it hurts)
!TILE_HURTS = $20
!GROUND_UNDER = $0013           ; zero: no ground under him (after PlayerGroundSnapHurt)
!PLAYER_FLAGS = $0347           ; bit 0: no hurt from the ground; bit 1 set in state $8C, cleared here
!LIVES = $0364                  ; lives left (the status line shows it, less 1)
!HOVER_ON = $036E               ; nonzero: the fourth action makes him hover
!SCENE_WAIT = $0087             ; the knockback's end waits for this to be 0
!SCENE_ON = $03FA               ; counted up as a life is lost
!V_141B = $141B
!V_141E = $141E                 ; the top speed's high byte (player_top_speed + 1)
!SKID_STARTED = $1422           ; nonzero once the skid's sound and pose have started
!FALLEN_OUT = $1424             ; nonzero: he fell out of the map
!V_1427 = $1427                 ; nonzero: no landing on actors
!GROUND_PUSHED = $1429          ; 2 while he stands on ground that pushes, else 0
!STAND_HELD = $142B             ; nonzero: standing, he does not turn or walk
!HURT_FRAMES = $142F            ; frames the knockback lasts while he has health
!V_1446 = $1446                 ; nonzero: no jump from a crouch
!AIR_THROW = $144B              ; frames left of a throw in the air
!PLAYER_LANDED = $144E          ; 1: he has landed on an actor
!AIR_FRAMES = $1451             ; frames in the air, counted up to $80
!NO_LANDING = $145A             ; nonzero: frames with no hold on a ledge; negative: none
!VIEW_EDGE_RULE = $145B         ; the rule the view's edges follow
!V_1A10 = $1A10
!V_1A12 = $1A12

; Record fields of the player's code
!FRAMES_40 = !actor_40          ; frames left of a pose (and the jump's pose step)
!FRAMES_47 = !actor_47          ; frames left of a landing's pose
!FRAMES_4A = !actor_stream      ; frames left of the standing throw

; The probe point and tile the ground tests leave (map/collision.asm)
!POINT_X = $000C                ; the probe point's x (word)
!POINT_Y = $000E                ; its y (word)
!TILE_MIRRORED = $001A          ; nonzero: the probed tile is mirrored

; Tables in the data bank ($80)
!JUMP_ROWS = $C730              ; the jump's vertical speed entry by the run speed's low bits

; Tasks the life's loss starts
!NO_LIVES_TASK = $80B2E3        ; the game task's entry with no lives left

; @routine 81:CCB6 PlayerStand
; @actor
; WHAT IT DOES: Aladdin standing: each frame he slows to a stop on the
; ground, may throw, jump, slide, crouch or fall off; when the pad points
; one way he turns to it (4 frames) and walks.
; HOW: PlayerFlagsClear, ACTOR_09 bit 0 cleared, PlayerMoveStart 0,
; PLAYER_BOX_WANTED and PLAYER_DASH 0, ActorSnapToGround. Each frame (BRK
; first): ActorDecelerateX, PlayerGroundSnapHurt, PlayerAppleThrow; a
; throw (C set): on as PlayerThrowStanding. JUMP_PRESSED: on to
; PlayerJump with his speed (X, A). On pushing ground, PlayerGroundSlide;
; PlayerCrouch and PlayerNoGroundFall (each can go on elsewhere),
; PlayerFlipByPad. STAND_HELD set, or PadFacing negative: the next frame.
; Facing other than ACTOR_FLIP: PlayerMoveStart 5 and 4 frames (BRK
; first, JUMP_PRESSED jumps). Then PLAYER_DASH: DASH_HELD set, speed
; entry 0 (ActorSpeedXSet) and PLAYER_DASH 1; else ActorStopX and speed
; fraction $80; on at PlayerWalk.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from the player's other states. Exit: never returns.
%at($81CCB6)
PlayerStand:
    jsr.w PlayerFlagsClear
    lda.b !actor_09
    and.b #$FE
    sta.b !actor_09
    lda.b #$00
    jsr.w PlayerMoveStart
    stz.w !player_box_wanted
    stz.w !player_dash
    jsl.l ActorSnapToGround
.frame:
    brk.b #$00
    jsl.l ActorDecelerateX
    jsr.w PlayerGroundSnapHurt
    jsr.w PlayerAppleThrow
    bcc PlayerThrowStanding_jump_test

; @routine 81:CCDC PlayerThrowStanding
; @actor
; WHAT IT DOES: Aladdin throws standing: the throw's pose ($14) for 20
; frames, then he stands on as PlayerStand does after its throw test.
; HOW: PlayerMoveStart $14, FRAMES_4A $14 frames (BRK each), PlayerMoveStart
; 0; then PlayerStand's frame from its jump test on.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerWalk (81:CDC9), and from PlayerStand's own throw
; test. Exit: never returns.
%at($81CCDC)
PlayerThrowStanding:
    lda.b #$14
    jsr.w PlayerMoveStart
    lda.b #$14
    sta.b !FRAMES_4A
.throw:
    brk.b #$00
    dec.b !FRAMES_4A
    bne .throw
    lda.b #$00
    jsr.w PlayerMoveStart
.jump_test:
    lda.w !JUMP_PRESSED
    beq .slide_test
    ldx.b !actor_speed_x
    lda.b !actor_speed_x+1
    jmp.w PlayerJump
.slide_test:
    lda.w !level_035c
    cmp.b #$04
    beq .slide
    lda.w !level
    cmp.b #$1A
    bne .crouch_test
.slide:
    jsr.w PlayerGroundSlide
.crouch_test:
    jsr.w PlayerCrouch
    jsr.w PlayerNoGroundFall
    jsr.w PlayerFlipByPad
    lda.w !STAND_HELD
    bne PlayerStand_frame
    jsr.w PadFacing
    bmi PlayerStand_frame
    lda.b !actor_facing
    cmp.b !actor_flip
    beq .walk
    lda.b #$05
    jsr.w PlayerMoveStart
    lda.b #$04
    sta.b !FRAMES_40
.turn:
    brk.b #$00
    lda.w !JUMP_PRESSED
    beq .turning
    ldx.b !actor_speed_x
    lda.b !actor_speed_x+1
    jmp.w PlayerJump
.turning:
    dec.b !FRAMES_40
    bne .turn
.walk:
    stz.w !player_dash
    lda.w !DASH_HELD
    beq .run
    ldy.b #$00
    jsl.l ActorSpeedXSet
    lda.b #$01
    sta.w !player_dash
    bra PlayerWalk
.run:
    jsl.l ActorStopX
    lda.b #$80
    sta.b !actor_speed_x

; @routine 81:CD5E PlayerWalk
; @actor
; WHAT IT DOES: Aladdin walking and running: each frame his speed follows
; the pad, he moves across and stays on the ground; he falls when there
; is none, jumps, skids when the pad turns, throws, or crouches.
; HOW: V_1427, V_141C, V_1A08 and PLAYER_BOX_WANTED 0; facing from
; ACTOR_FLIP; X = 0, PlayerMoveStart 2. Each frame (BRK first): on
; pushing ground PlayerGroundPush; PlayerRunMove, ActorStepX,
; PlayerOutOfWallsHurt, PlayerGroundSnapHurt; GROUND_UNDER 0: on at
; PlayerFall. PlayerAirSteer; PLAYER_DASH from DASH_HELD; JUMP_PRESSED:
; PlayerJump at his speed. PadFacing other than his facing: SKID_STARTED
; 0, on at PlayerSkid. PlayerAppleThrow C set: PlayerThrowStanding.
; PlayerCrouch, and the next frame.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerStand, PlayerSkid and PlayerLand. Exit: never
; returns.
%at($81CD5E)
PlayerWalk:
    stz.w !V_1427
    stz.w !v_141c
    stz.w !v_1a08
    lda.b #$00
    sta.w !player_box_wanted
    lda.b !actor_flip
    sta.b !actor_facing
    ldx.b #$00
    lda.b #$02
    jsr.w PlayerMoveStart
.frame:
    brk.b #$00
    lda.w !level_035c
    cmp.b #$04
    beq .push
    lda.w !level
    cmp.b #$1A
    bne .run
.push:
    jsr.w PlayerGroundPush
.run:
    jsr.w PlayerRunMove
    jsr.w ActorStepX
    jsr.w PlayerOutOfWallsHurt
    jsr.w PlayerGroundSnapHurt
    lda.w !GROUND_UNDER
    beq PlayerFall
    jsr.w PlayerAirSteer
    stz.w !player_dash
    lda.w !DASH_HELD
    beq .jump_test
    lda.b #$01
    sta.w !player_dash
.jump_test:
    lda.w !JUMP_PRESSED
    beq .turn_test
    ldx.b !actor_speed_x
    lda.b !actor_speed_x+1
    jmp.w PlayerJump
.turn_test:
    jsr.w PadFacing
    cmp.b !actor_facing
    beq .throw_test
    stz.w !SKID_STARTED
    jmp.w PlayerSkid
.throw_test:
    jsr.w PlayerAppleThrow
    bcc .crouch_test
    jmp.w PlayerThrowStanding
.crouch_test:
    jsr.w PlayerCrouch
    jmp.w .frame

; @routine 81:CDD2 PlayerFall
; @actor
; WHAT IT DOES: Aladdin starts to fall: the fall's speeds and pose, and
; on into the air.
; HOW: Y = 2, ActorMotionPresetY; PlayerMoveStart $0E; FRAMES_40 (the
; pose step) 3; PlayerTopSpeedHold; on at PlayerAirStart.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from the player's states and the tests that find no ground.
; Exit: never returns.
%at($81CDD2)
PlayerFall:
    ldy.b #$02
    jsl.l ActorMotionPresetY
    lda.b #$0E
    jsr.w PlayerMoveStart
    lda.b #$03
    sta.b !FRAMES_40
    jsr.w PlayerTopSpeedHold
    jmp.w PlayerAirStart

; @routine 81:CFE0 PlayerJump
; @actor
; WHAT IT DOES: Aladdin jumps: his top speed in the air is the speed he
; had, the jump's rise is by how fast he ran, with the jump's sound; on
; into the air.
; HOW: PLAYER_LANDED 0; X to PLAYER_TOP_SPEED, A to V_141E;
; PlayerFlipByPad; Y from JUMP_ROWS by the speed's pixels' low two bits,
; ActorMotionPresetY; FRAMES_40 (the pose step) 0; SoundQueueAdd 1;
; NO_LANDING 4; on at PlayerAirStart.
; Entry: M 8-bit, X 8-bit; X = the speed's fraction, A = its pixels; D =
; the player's record; DB = $80; reached by a jump from the player's
; states. Exit: never returns.
%at($81CFE0)
PlayerJump:
    stz.w !PLAYER_LANDED
    stx.w !player_top_speed
    sta.w !V_141E
    jsr.w PlayerFlipByPad
    lda.b !actor_speed_x+1
    and.b #$03
    tax
    ldy.w !JUMP_ROWS,x
    jsl.l ActorMotionPresetY
    stz.b !FRAMES_40
    lda.b #$01
    jsl.l SoundQueueAdd
    lda.b #$04
    sta.w !NO_LANDING

; @routine 81:D005 PlayerAirStart
; @actor
; WHAT IT DOES: Aladdin leaves the ground: his speeds held to the air's,
; and on into the air.
; HOW: GROUND_PUSHED and V_1427 0; PlayerTopSpeedCap, PlayerRunRatesSet;
; on at PlayerAirTimed.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerFall, PlayerKnockback and PlayerHover, and from
; PlayerJump's end. Exit: never returns.
%at($81D005)
PlayerAirStart:
    stz.w !GROUND_PUSHED
    stz.w !V_1427
    jsr.w PlayerTopSpeedCap
    jsr.w PlayerRunRatesSet

; @routine 81:D011 PlayerAirTimed
; @actor
; WHAT IT DOES: Aladdin in the air from the ground: the air's settings,
; and his first frame in the air now.
; HOW: V_1A08 $0A, PLAYER_BOX_WANTED 3, V_141C up, AIR_FRAMES 0, ACTOR_09
; bit 0 cleared, PlayerGroundYSave; then PlayerAir's frame with no BRK
; first.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
; from PlayerAirStart's end, and by a jump from the hang's climb
; (81:D5DE). Exit: never returns.
%at($81D011)
PlayerAirTimed:
    lda.b #$0A
    sta.w !v_1a08
    lda.b #$03
    sta.w !player_box_wanted
    inc.w !v_141c
    stz.w !AIR_FRAMES
    lda.b !actor_09
    and.b #$FE
    sta.b !actor_09
    jsr.w PlayerGroundYSave
    bra PlayerAir_test

; @routine 81:D02C PlayerAir
; @actor
; WHAT IT DOES: Aladdin in the air, a frame at a time: his jump's pose,
; a throw, his fall and its gravity (lighter while the jump is held
; rising), and a ledge to catch, until he lands.
; HOW: Each frame (BRK first). Falling with V_1428 set: TILE_BITS 0, on
; at PlayerLand. AIR_FRAMES up while under $80 (0 with JUMP_PRESSED).
; AIR_THROW counting down: at 0 FRAMES_40 (the pose step) 8 and
; PlayerJumpPoseStep; else PlayerAppleThrow: a throw starts move $16 and
; AIR_THROW $15, else PlayerJumpPoseStep. With HOVER_ON (no script runs
; it): HOVER_HELD while falling, on at PlayerHover. PlayerAirStep,
; PlayerFlipByPad; ACTOR_ACCEL_Y $38, or $80 rising with the jump not
; held and not PLAYER_LANDED; PlayerRunSpeed. Rising: the next frame.
; Level 5: GroundBehindBit4 at probe $18 sets V_1A10 and V_1A12 to 1.
; V_1428: PlayerLand. Not down on the pad and NO_LANDING 0:
; GroundBehindBit3 nonzero catches a ledge (PlayerLedgeGrab); else
; NO_LANDING down. Then the ground test of PlayerAirLand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerAirLand, the hang's climb and PlayerAirTimed's own
; frames. Exit: never returns.
%at($81D02C)
PlayerAir:
    brk.b #$00
.test:
    lda.b !actor_speed_y+2
    bmi .counts
    lda.w !v_1428
    beq .counts
    stz.w !TILE_BITS
    jmp.w PlayerLand
.counts:
    lda.w !AIR_FRAMES
    bmi .jump_test
    inc.w !AIR_FRAMES
.jump_test:
    lda.w !JUMP_PRESSED
    beq .throw_test
    stz.w !AIR_FRAMES
.throw_test:
    lda.w !AIR_THROW
    beq .throw
    dec.w !AIR_THROW
    bne .hover_test
    lda.b #$08
    sta.b !FRAMES_40
    bra .pose
.throw:
    jsr.w PlayerAppleThrow
    bcc .pose
    lda.b #$16
    jsr.w PlayerMoveStart
    lda.b #$15
    sta.w !AIR_THROW
    bra .hover_test
.pose:
    jsr.w PlayerJumpPoseStep
.hover_test:
    lda.w !HOVER_ON
    beq .move
    lda.w !HOVER_HELD
    beq .move
    lda.b !actor_speed_y+2
    bmi .move
    jmp.w PlayerHover
.move:
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    lda.b #$38
    sta.b !actor_accel_y
    lda.w !JUMP_HELD
    bne .speed
    lda.b !actor_speed_y+2
    bpl .speed
    lda.w !PLAYER_LANDED
    bne .speed
    lda.b #$80
    sta.b !actor_accel_y
.speed:
    jsr.w PlayerRunSpeed
    lda.b !actor_speed_y+2
    bmi PlayerAir
    lda.w !level
    cmp.b #$05
    bne .landing_test
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq .landing_test
    lda.b #$01
    sta.w !V_1A10
    sta.w !V_1A12
.landing_test:
    lda.w !v_1428
    bne PlayerLand
    lda.w !PAD1_DIRS
    and.b #!PAD_DOWN
    bne .no_hold
    lda.w !NO_LANDING
    bne .no_hold
    jsl.l GroundBehindBit3
    beq PlayerAirLand
    jmp.w PlayerLedgeGrab
.no_hold:
    lda.w !NO_LANDING
    beq PlayerAirLand
    dec.w !NO_LANDING

; @routine 81:D0E0 PlayerAirLand
; @actor
; WHAT IT DOES: Aladdin in the air: when he is in the ground he lands,
; else his next frame in the air.
; HOW: ActorOutOfGround; Z clear: on at PlayerLand; else at PlayerAir.
; Entry: M 8-bit, X 8-bit (M 16-bit from PlayerLedgeGrab's wrong-way
; ledge path, which rand_pw4_2 runs at frame 2005); D = the player's record; DB = $80; reached at the end of
; PlayerAir's frame and by a jump from PlayerLedgeGrab. Exit: never
; returns.
%at($81D0E0)
PlayerAirLand:
    jsl.l ActorOutOfGround
    bne PlayerLand
    jmp.w PlayerAir

; @routine 81:D0E9 PlayerLand
; @actor
; WHAT IT DOES: Aladdin lands: on ground that hurts him he is knocked
; back; else the landing's sound, and after a fall of more than 128
; pixels a hard landing (18 frames) and he stands; else he walks on at
; his speed (a run's at least $0180, a dash's $0200) or stands.
; HOW: ActorSnapToGround; NO_LANDING, AIR_THROW, V_141C and PLAYER_LANDED
; 0. ground_hurts: PlayerEnter8CLong, on at PlayerKnockback. SoundQueueAdd
; $10. (16-bit) PLAYER_GROUND_Y + $80 below his y: PlayerMoveStart $0F,
; FRAMES_47 $12 frames (BRK each), ActorSpeedXClear, PlayerStand. Else
; PadFacing negative: ActorSpeedXClear, PlayerStand. DASH_HELD: speed
; $0200; else his speed held to $0180 at most; on at PlayerWalk.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from the air's states. Exit: never returns.
%at($81D0E9)
PlayerLand:
    jsl.l ActorSnapToGround
    stz.w !NO_LANDING
    stz.w !AIR_THROW
    stz.w !v_141c
    stz.w !PLAYER_LANDED
    lda.w !TILE_BITS
    bit.b #!TILE_HURTS
    beq .landed
    lda.b !actor_08
    bit.b #$10
    bne .landed
    lda.w !PLAYER_FLAGS
    bit.b #$01
    bne .landed
    jsl.l PlayerEnter8CLong
    jml.l PlayerKnockback
.landed:
    lda.b #$10
    jsl.l SoundQueueAdd
    rep.b #$20
    lda.w !player_ground_y
    clc
    adc.w #$0080
    sec
    cmp.b !actor_y+1
    bcc .hard
    sep.b #$20
    jsr.w PadFacing
    bmi .stand
    lda.w !DASH_HELD
    beq .run
    rep.b #$20
    lda.w #$0200
    bra .speed
.run:
    rep.b #$20
    lda.w #$0180
    cmp.b !actor_speed_x
    bcs .walk
.speed:
    sta.b !actor_speed_x
.walk:
    sep.b #$20
    jmp.w PlayerWalk
.hard:
    sep.b #$20
    lda.b #$0F
    jsr.w PlayerMoveStart
    lda.b #$12
    sta.b !FRAMES_47
.hard_frame:
    brk.b #$00
    dec.b !FRAMES_47
    bne .hard_frame
.stand:
    jsl.l ActorSpeedXClear
    jmp.w PlayerStand

; @routine 81:D27F PlayerSkid
; @actor
; WHAT IT DOES: Aladdin skids as the pad turns him while he runs: the
; skid's sound and pose (a fast run's own, from $0180), slowing until he
; stops and stands, walks on the new way, falls, jumps or throws.
; HOW: ACTOR_ACCEL_X $10. Each turn: PlayerFlipByPad; the pad pointing
; the other way (its facing XOR his, less 1, zero): PlayerSpawnEvery8,
; and once (SKID_STARTED) SoundQueueAdd $0A, ACTOR_ACCEL_X $10,
; PlayerMoveStart $12 (from speed $0180: SoundQueueAdd 4 and $13; from
; there also ACTOR_09 bit 0 and PLAYER_BOX_WANTED cleared). No direction:
; ACTOR_ACCEL_X $20; PlayerAppleThrow's throw: move $14 for $14 frames
; (COP), move 0, ActorStopX. Then a frame (BRK first): ActorStepX;
; ActorOutOfWalls C set: ActorStopX, PlayerStand. Not V_1428 and no
; ground (ActorSnapToGround Z): PlayerFall. JUMP_PRESSED: PlayerJump
; with X $80, A 1. PadFacing his facing: it to ACTOR_FLIP, PlayerWalk.
; PlayerRunMove until SKID_STARTED; ActorDecelerateX negative or the
; speed's word 0: ActorStopX, PlayerStand. PlayerCrouch, and again.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerWalk (81:CDC1). Exit: never returns.
%at($81D27F)
PlayerSkid:
    lda.b #$10
    sta.b !actor_accel_x
.turn:
    jsr.w PlayerFlipByPad
    bmi .no_direction
    eor.b !actor_facing
    dec a
    bne .frame
    jsr.w PlayerSpawnEvery8
    lda.w !SKID_STARTED
    bne .frame
    inc.w !SKID_STARTED
    lda.b #$0A
    jsl.l SoundQueueAdd
    lda.b #$10
    sta.b !actor_accel_x
    lda.b #$12
    rep.b #$10
    ldx.b !actor_speed_x
    cpx.w #$0180
    bcc .pose
    lda.b #$04
    jsl.l SoundQueueAdd
    lda.b #$13
.pose:
    sep.b #$10
    jsr.w PlayerMoveStart
    rep.b #$20
    lda.b !actor_speed_x
    cmp.w #$0180
    sep.b #$20
    bcc .frame
    lda.b !actor_09
    and.b #$FE
    sta.b !actor_09
    lda.b #$00
    sta.w !player_box_wanted
    bra .frame
.no_direction:
    lda.b #$20
    sta.b !actor_accel_x
    jsr.w PlayerAppleThrow
    bcc .frame
    lda.b #$14
    jsr.w PlayerMoveStart
    lda.b #$14
    cop.b #$00
    lda.b #$00
    jsr.w PlayerMoveStart
    jsl.l ActorStopX
.frame:
    brk.b #$00
    jsr.w ActorStepX
    jsl.l ActorOutOfWalls
    bcs .stop
    lda.w !v_1428
    bne .jump_test
    jsl.l ActorSnapToGround
    bne .jump_test
    jmp.w PlayerFall
.jump_test:
    lda.w !JUMP_PRESSED
    beq .facing_test
    ldx.b #$80
    lda.b #$01
    jmp.w PlayerJump
.facing_test:
    jsr.w PadFacing
    bmi .slow
    cmp.b !actor_facing
    bne .slow
    sta.b !actor_flip
    jmp.w PlayerWalk
.slow:
    lda.w !SKID_STARTED
    bne .decelerate
    jsr.w PlayerRunMove
.decelerate:
    jsl.l ActorDecelerateX
    bpl .moving
.stop:
    sep.b #$20
    jsl.l ActorStopX
    jmp.w PlayerStand
.moving:
    rep.b #$20
    lda.b !actor_speed_x
    ora.b !actor_speed_x+1
    beq .stop
    sep.b #$20
    jsr.w PlayerCrouch
    jmp.w .turn

; @routine 81:D347 PlayerNoGroundFall
; WHAT IT DOES: When there is no ground to either side of Aladdin's feet,
; he falls (unless V_1428 is set).
; HOW: V_1428 set: back. ACTOR_18 0; GroundLeftLong C set, or
; GroundRightLong C set: back. Else its return address dropped (PLA
; PLA) and on at PlayerFall.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's states at their level (81:CD10, 81:D3D0). Exit:
; A, N, Z and C as the last test left them. No ground: never returns.
%at($81D347)
PlayerNoGroundFall:
    lda.w !v_1428
    bne .back
    stz.b !actor_18
    jsl.l GroundLeftLong
    bcs .back
    jsl.l GroundRightLong
    bcs .back
    pla
    pla
    jmp.w PlayerFall
.back:
    rts
.not_down:                      ; PlayerCrouch's return when down is not held
    rts

; @routine 81:D361 PlayerCrouch
; WHAT IT DOES: When down is held, Aladdin crouches: he slides to a stop
; crouched, may throw (move $15, 24 frames), jump, or fall, and stands
; again when down is let go. Ground that hurts knocks him back.
; HOW: PAD1_DIRS bit 2 clear: back. Else its return address dropped (PLA
; PLA): PLAYER_BOX_WANTED 1, PlayerMoveStart 1, ACTOR_09 bit 0 set,
; ACTOR_ACCEL_X $28, facing from ACTOR_FLIP. Each frame (BRK first):
; ActorSnapToGround; ground_hurts: PlayerEnter8CLong, on at
; PlayerKnockback. PlayerFlipByPad; speed 0: ACTOR_ACCEL_X 0; else
; ActorDecelerateX and PlayerCrouchStep. PlayerAppleThrow's throw: move
; $15, COP $18, move 1. PlayerCrouchJump, PlayerNoGroundFall. Down still
; held: again; else facing from ACTOR_FLIP and on at PlayerStand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's states at their level (81:CD0D, 81:CDCC,
; 81:D341). Exit: down not held: A low = PAD1_DIRS, Z clear of bit 2.
; Else never returns.
%at($81D361)
PlayerCrouch:
    lda.w !PAD1_DIRS
    bit.b #!PAD_DOWN
    beq PlayerNoGroundFall_not_down
    pla
    pla
    lda.b #$01
    sta.w !player_box_wanted
    lda.b #$01
    jsr.w PlayerMoveStart
    lda.b #$01
    ora.b !actor_09
    sta.b !actor_09
    lda.b #$28
    sta.b !actor_accel_x
    lda.b !actor_flip
    sta.b !actor_facing
.frame:
    brk.b #$00
    jsl.l ActorSnapToGround
    lda.w !TILE_BITS
    bit.b #!TILE_HURTS
    beq .flip
    lda.b !actor_08
    bit.b #$10
    bne .flip
    lda.w !PLAYER_FLAGS
    bit.b #$01
    bne .flip
    jsl.l PlayerEnter8CLong
    jml.l PlayerKnockback
.flip:
    jsr.w PlayerFlipByPad
    lda.b !actor_speed_x
    ora.b !actor_speed_x+1
    ora.b !actor_speed_x+2
    bne .slide
    stz.b !actor_accel_x
    bra .throw_test
.slide:
    jsl.l ActorDecelerateX
    jsr.w PlayerCrouchStep
.throw_test:
    jsr.w PlayerAppleThrow
    bcc .tests
    lda.b #$15
    jsr.w PlayerMoveStart
    lda.b #$18
    cop.b #$00
    lda.b #$01
    jsr.w PlayerMoveStart
.tests:
    jsr.w PlayerCrouchJump
    jsr.w PlayerNoGroundFall
    lda.w !PAD1_DIRS
    bit.b #!PAD_DOWN
    bne .frame
    lda.b !actor_flip
    sta.b !actor_facing
    jmp.w PlayerStand

; @routine 81:D3E1 PlayerCrouchStep
; WHAT IT DOES: Moves Aladdin across by his speed as he slides crouched,
; out of walls, and down onto the ground.
; HOW: ActorStepX, PlayerOutOfWallsHurt; V_1428 set: back. Else
; ActorSnapToGround and GROUND_UNDER loaded.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from PlayerCrouch (81:D3B7). Exit: A low = V_1428 or GROUND_UNDER,
; N and Z from it; X and Y as the calls left them.
%at($81D3E1)
PlayerCrouchStep:
    jsr.w ActorStepX
    jsr.w PlayerOutOfWallsHurt
    lda.w !v_1428
    bne .back
    jsl.l ActorSnapToGround
    lda.w !GROUND_UNDER
.back:
    rts

; @routine 81:D3F4 PlayerCrouchJump
; WHAT IT DOES: When the jump is pressed while Aladdin crouches (and
; V_1446 allows it), he jumps.
; HOW: JUMP_PRESSED 0 or V_1446 set: back (by the RTS that ends
; PlayerCrouchStep). Else its return address dropped (PLA PLA), X =
; $80, A = 1, on at PlayerJump.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from PlayerCrouch at the actor's level (81:D3CD). Exit: A low as
; loaded, N and Z from it. Jump: never returns.
%at($81D3F4)
PlayerCrouchJump:
    lda.w !JUMP_PRESSED
    beq PlayerCrouchStep_back
    lda.w !V_1446
    bne PlayerCrouchStep_back
    pla
    pla
    ldx.b #$80
    lda.b #$01
    jmp.w PlayerJump

; @routine 81:D407 PlayerAirStep
; WHAT IT DOES: Moves Aladdin through the air for a frame: gravity, his
; speeds, landing on a floor, out of walls, and ground that hurts.
; HOW: ActorGravity, ActorStepX, ActorMoveY, PlayerLandOrHurt,
; ActorOutOfWalls, PlayerGroundHurtTest (which can drop this call's
; return address with its own).
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's air states at their level (81:D082 and others).
; Exit: as PlayerGroundHurtTest. Hurt: never returns.
%at($81D407)
PlayerAirStep:
    jsr.w ActorGravity
    jsr.w ActorStepX
    jsl.l ActorMoveY
    jsr.w PlayerLandOrHurt
    jsl.l ActorOutOfWalls
    jsr.w PlayerGroundHurtTest
    rts

; @routine 81:D470 PlayerLedgeGrab
; @actor
; WHAT IT DOES: Aladdin catches a ledge as he falls past it: set at the
; ledge's corner, his speeds stopped, the grab's sound and pose; on to
; hang from it.
; HOW: (16-bit A) the ledge tile's mirror flag (TILE_MIRRORED, X) and his
; ACTOR_FLIP must differ (else, paths no script runs, on at
; PlayerAirLand); his x pixels the probe's x to its 16 plus 1 (or plus
; $0F, mirrored), his y pixels the probe's y to its 16 plus 1. PLAYER_LANDED
; and AIR_THROW 0, V_1427 up, PlayerFlagsClear, ActorStopX, ActorStopY,
; SoundQueueAdd 6, PlayerMoveStart 6, PLAYER_BOX_WANTED 4; on at
; PlayerHang.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerAir (81:D0D5) and PlayerHover. Exit: never returns.
%at($81D470)
PlayerLedgeGrab:
    rep.b #$20
    ldx.w !TILE_MIRRORED
    bne .mirrored
    ldx.b !actor_flip
    beq .left
    jmp.w PlayerAirLand
.left:
    lda.w !POINT_X
    and.w #$FFF0
    clc
    adc.w #$0001
    sta.b !actor_x+1
    bra .y
.mirrored:
    ldx.b !actor_flip
    bne .right
    jmp.w PlayerAirLand
.right:
    lda.w !POINT_X
    and.w #$FFF0
    clc
    adc.w #$000F
    sta.b !actor_x+1
.y:
    lda.w !POINT_Y
    and.w #$FFF0
    clc
    adc.w #$0001
    sta.b !actor_y+1
    sep.b #$20
    stz.w !PLAYER_LANDED
    stz.w !AIR_THROW
    inc.w !V_1427
    jsr.w PlayerFlagsClear
    jsl.l ActorStopX
    jsl.l ActorStopY
    lda.b #$06
    jsl.l SoundQueueAdd
    lda.b #$06
    jsr.w PlayerMoveStart
    lda.b #$04
    sta.w !player_box_wanted

; @routine 81:D4D1 PlayerHang
; @actor
; WHAT IT DOES: Aladdin hangs from a ledge: up climbs onto it (move 7,
; 32 frames, then up 24 pixels and across 8, standing); down lets go (18
; pixels lower, falling, with no hold for 16 frames).
; HOW: Each frame (BRK first): PAD1_DIRS bit 3 (up): PlayerMoveStart 7,
; PLAYER_BOX_WANTED 5, COP $20; V_1427 0, facing from ACTOR_FLIP; speed
; $000800 across and $FFE800 down, ActorMove once, both speeds cleared;
; on at PlayerStand. Bit 2 (down): facing from ACTOR_FLIP, NO_LANDING
; $10, y $12 lower (16-bit), V_1427 0, on at PlayerFall.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
; from PlayerLedgeGrab's end and by a jump from 81:D9C2. Exit: never
; returns.
%at($81D4D1)
PlayerHang:
    brk.b #$00
    lda.w !PAD1_DIRS
    and.b #!PAD_UP
    beq .down_test
    bra .climb
.down_test:
    lda.w !PAD1_DIRS
    and.b #!PAD_DOWN
    beq PlayerHang
    lda.b !actor_flip
    sta.b !actor_facing
    lda.b #$10
    sta.w !NO_LANDING
    rep.b #$20
    lda.b !actor_y+1
    clc
    adc.w #$0012
    sta.b !actor_y+1
    sep.b #$20
    stz.w !V_1427
    jmp.w PlayerFall
.climb:
    lda.b #$07
    jsr.w PlayerMoveStart
    lda.b #$05
    sta.w !player_box_wanted
    lda.b #$20
    cop.b #$00
    stz.w !V_1427
    lda.b !actor_flip
    sta.b !actor_facing
    lda.b #$00
    sta.b !actor_speed_x+2
    lda.b #$FF
    sta.b !actor_speed_y+2
    rep.b #$20
    lda.w #$0800
    sta.b !actor_speed_x
    lda.w #$E800
    sta.b !actor_speed_y
    sep.b #$20
    jsl.l ActorMove
    jsl.l ActorSpeedXClear
    jsl.l ActorSpeedYClear
    jmp.w PlayerStand

; @routine 81:D86A PlayerHover
; @actor
; WHAT IT DOES: Aladdin hovers while the fourth action is held (with
; HOVER_ON): his own pose and a slow fall at speed $0200 across; let go,
; he falls; he can land, catch a ledge or meet ground that hurts. No
; script runs it.
; HOW: PLAYER_BOX_WANTED $12, V_141B 0, PlayerMoveStart $0C; Y = $18,
; ActorSpeedYSet; top speed $0200. Each frame (from the second, BRK
; first): ActorStepX, ActorMoveY; his y pixels' high byte not negative:
; ActorOutOfWalls and PlayerGroundHurtJump. PlayerFlipByPad. V_1428:
; the landing (below). HOVER_HELD 0: gravity $36, the pose step 1 with 1
; frame, on at PlayerAirStart. Level 5: GroundBehindBit4 at probe $18
; sets V_1A10 and V_1A12. PlayerRunSpeed, PlayerOutOfWallsHurt;
; ActorOutOfGround Z clear: the landing. NO_LANDING: down by 1. Else
; GroundBehindBit3 Z clear: PlayerLedgeGrab. The landing:
; PLAYER_BOX_WANTED 3, y fraction and the vertical speed cleared, on at
; PlayerLand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from the player's air states with HOVER_ON (81:D07F and three
; more). Exit: never returns.
%at($81D86A)
PlayerHover:
    lda.b #$12
    sta.w !player_box_wanted
    stz.w !V_141B
    lda.b #$0C
    jsr.w PlayerMoveStart
    ldy.b #$18
    jsl.l ActorSpeedYSet
    lda.b #$02
    sta.w !V_141E
    stz.w !player_top_speed
    bra .move
.frame:
    brk.b #$00
.move:
    jsr.w ActorStepX
    jsl.l ActorMoveY
    lda.b !actor_y+2
    bmi .flip
    jsl.l ActorOutOfWalls
    jsr.w PlayerGroundHurtJump
.flip:
    jsr.w PlayerFlipByPad
    lda.w !v_1428
    bne .land
    lda.w !HOVER_HELD
    bne .level_test
    lda.b #$36
    sta.b !actor_accel_y
    lda.b #$01
    sta.b !FRAMES_40
    sta.b !actor_41
    jmp.w PlayerAirStart
.level_test:
    lda.w !level
    cmp.b #$05
    bne .speed
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq .speed
    lda.b #$01
    sta.w !V_1A10
    sta.w !V_1A12
.speed:
    jsr.w PlayerRunSpeed
    jsr.w PlayerOutOfWallsHurt
    jsl.l ActorOutOfGround
    bne .land
    lda.w !NO_LANDING
    bne .no_hold
    jsl.l GroundBehindBit3
    beq .frame
    jmp.w PlayerLedgeGrab
.no_hold:
    dec.w !NO_LANDING
    bra .frame
.land:
    lda.b #$03
    sta.w !player_box_wanted
    stz.b !actor_y
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    jmp.w PlayerLand

; @routine 81:DADD PlayerKnockback
; @actor
; WHAT IT DOES: Aladdin is hit: knocked back the way he faced, hurt's
; pose and sound, for $18 frames or until he lands, then he stands
; (blinking) or falls on. With no health left, or fallen out of the map,
; he loses a life: the song, and once the scene is over the stage starts
; again (or ends with no lives).
; HOW: HEALTH 0: NO_LANDING $FF. Y = 5 ($56 on level 6), unless
; FALLEN_OUT (then straight to the life's loss); GROUND_PUSHED and V_1427
; 0; ActorMotionPreset; SoundQueueAdd 2; PLAYER_BOX_WANTED 6,
; PlayerBoxSet; PlayerMoveStartAlso $17; gravity $30; facing turned
; round for the tests; HURT_FRAMES $18, V_141C 0. Each frame (BRK first):
; ActorFallCap, ActorFly, PlayerLandOrHurt, ActorOutOfWalls on both sides;
; with health HURT_FRAMES down, negative: the end. Rising: again.
; PlayerFallenTest C clear: the life's loss. Level 5: GroundBehindBit4 at
; probe $18 sets V_1A10, V_1A12 (no script runs it). V_1428, or
; ActorOutOfGround Z clear: the end. The end: PLAYER_FLAGS bit 1 off and
; bit 2 on; no health: the life's loss with move 8. Else (16-bit) y less
; 13 unless V_1428, PLAYER_HURT_TIME $7E, ACTOR_08 bit 4 and ACTOR_09
; bit 7 set, V_141B 0, PlayerFlagsClear; HURT_FRAMES not negative:
; PlayerStand; else the pose step 1 with 1 frame, PlayerAirStart. Fallen
; out: HEALTH 0, ACTOR_09 bit 7 off, ACTOR_08 bit 7 on and bit 3 off,
; FALLEN_OUT 0, V_1A10, V_1A12 and SCENE_ON up, V_1A08 $FF. The life's
; loss: a frame, SongPlay $2A, VIEW_EDGE_RULE 4, NO_LANDING $FF, V_1A10
; 1, Y = 2 ActorMotionPresetY, $B0 frames (out of the ground, flying);
; TaskCreate; frames until SCENE_WAIT is 0; V_141B 0, Level035CSet, LIVES
; down; TaskExec to one of two entries by whether any are left.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's jump (through the stub at $83:91FD), and
; by a jump from the player's states when ground hurts him. Exit: never
; returns.
%at($81DADD)
PlayerKnockback:
    lda.w !health
    bne .preset
    lda.b #$FF
    sta.w !NO_LANDING
.preset:
    ldy.b #$05
    bra PlayerKnockbackFromLedge_start
; @routine 81:DAEB PlayerKnockbackFromLedge
; @actor
; WHAT IT DOES: PlayerKnockback with its motion preset 2 (as from a
; ledge). No input reaches it: only the JMP at 81:DCFD names it, in code
; ($81:DCE4-DCFF) nothing reaches; cov_knockback_entries.box runs it from
; his record with its resume address set to it.
; HOW: Y = 2; then as PlayerKnockback from its start.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI at a resume address set to it. Exit: never
; returns.
%at($81DAEB)
PlayerKnockbackFromLedge:
    ldy.b #$02
.start:
    stz.w !GROUND_PUSHED
    stz.w !V_1427
    lda.w !FALLEN_OUT
    beq .knocked
    jmp.w PlayerKnockbackNoHealth_fallen
.knocked:
    lda.w !level
    sec
    cmp.b #$06
    bne .motion
    ldy.b #$56
.motion:
    jsl.l ActorMotionPreset
    lda.b #$02
    jsl.l SoundQueueAdd
    ldx.b #$06
    stx.w !player_box_wanted
    jsr.w PlayerBoxSet
    lda.b #$17
    jsr.w PlayerMoveStartAlso
    lda.b #$30
    sta.b !actor_accel_y
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.b #$18
    sta.w !HURT_FRAMES
    stz.w !v_141c
.frame:
    brk.b #$00
    jsr.w ActorFallCap
    jsl.l ActorFly
    jsr.w PlayerLandOrHurt
    jsl.l ActorOutOfWalls
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    jsl.l ActorOutOfWalls
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.w !health
    beq .rising_test
    dec.w !HURT_FRAMES
    bmi .end
.rising_test:
    lda.b !actor_speed_y+2
    bmi .frame
    jsr.w PlayerFallenTest
    bcc PlayerKnockbackNoHealth_fallen
    lda.w !level
    cmp.b #$05
    bne .ground_test
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq .ground_test
    lda.b #$01
    sta.w !V_1A10
    sta.w !V_1A12
.ground_test:
    lda.w !v_1428
    bne .end
    jsl.l ActorOutOfGround
    beq .frame
.end:
    sep.b #$20
    lda.w !PLAYER_FLAGS
    and.b #$FD
    ora.b #$04
    sta.w !PLAYER_FLAGS
    lda.w !health
    beq PlayerKnockbackNoHealth_no_health
    lda.w !v_1428
    bne .blink
    rep.b #$20
    lda.b !actor_y+1
    sec
    sbc.w #$000D
    sta.b !actor_y+1
    sep.b #$20
.blink:
    lda.b #$7E
    sta.w !player_hurt_time
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    stz.w !V_141B
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
    jsr.w PlayerFlagsClear
    lda.w !HURT_FRAMES
    bmi .air
    jmp.w PlayerStand
.air:
    lda.b #$01
    sta.b !FRAMES_40
    sta.b !actor_41
    jmp.w PlayerAirStart
; @routine 81:DBCF PlayerKnockbackNoHealth
; @actor
; WHAT IT DOES: Aladdin knocked back with no health left: his life is
; lost (PlayerKnockback's own path there). No input reaches this entry:
; it is a JMP right after PlayerKnockback's JMP to PlayerAirStart, and
; nothing in the ROM names $81:DBCF. cov_knockback_entries.box runs it
; from his record with its resume address set to it.
; HOW: JMP to the life's loss as PlayerKnockback takes it with no health.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI at a resume address set to it. Exit: never
; returns.
%at($81DBCF)
PlayerKnockbackNoHealth:
    jmp.w .no_health
.fallen:
    sep.b #$30
    stz.w !health
    lda.b !actor_09
    and.b #$7F
    sta.b !actor_09
    lda.b #$80
    tsb.b !actor_08
    stz.w !FALLEN_OUT
    inc.w !V_1A10
    inc.w !V_1A12
    inc.w !SCENE_ON
    lda.b #$FF
    sta.w !v_1a08
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    bra .life_lost
.no_health:
    inc.w !SCENE_ON
    ldx.b #$06
    stx.w !player_box_wanted
    lda.b #$80
    tsb.b !actor_08
    lda.b #$08
    jsr.w PlayerMoveStartAlso
.life_lost:
    brk.b #$00
    lda.b #$2A
    jsl.l SongPlay
    lda.b #$04
    sta.w !VIEW_EDGE_RULE
    lda.b #$FF
    sta.w !NO_LANDING
    lda.b #$01
    sta.w !V_1A10
    ldy.b #$02
    jsl.l ActorMotionPresetY
    lda.b #$B0
    sta.b !FRAMES_40
.lost_frame:
    brk.b #$00
    jsl.l ActorOutOfGround
    bne .lost_count
    lda.w !v_1428
    bne .lost_count
    jsl.l ActorFly
.lost_count:
    dec.b !FRAMES_40
    bne .lost_frame
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessDownTask
.scene_frame:
    brk.b #$00
    jsl.l ActorOutOfGround
    bne .scene_test
    lda.w !v_1428
    bne .scene_test
    jsl.l ActorFly
.scene_test:
    lda.w !SCENE_WAIT
    bne .scene_frame
    stz.w !V_141B
    jsl.l Level035CSet
    dec.w !LIVES
    lda.w !LIVES
    beq .no_lives
    jsl.l TaskExec
    dl LevelPlayStartTask
.no_lives:
    jsl.l TaskExec
    dl !NO_LIVES_TASK
