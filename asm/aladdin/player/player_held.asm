; player_held.asm: Aladdin's code while something holds him or throws
; him: held by an actor (and how he lets go or jumps off), his bounce off
; an actor he lands on, sprung or flung up, and taken to hang. Each is
; actor code (it yields by BRK, docs/aladdin/ACTORS.md) whose first piece
; starts where the actor that set his resume address sends him, and goes
; on to another of his states by a jump.
; C: src/aladdin/player/player_held.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Pad 1 and the game's actions (system/controls.asm)
!PAD1_DIRS = $0380              ; pad 1's high byte: B Y Select Start Up Down Left Right
!PAD_DOWN = $04
!JUMP_HELD = $0383              ; nonzero while the jump's button is held
!HOVER_HELD = $0386             ; nonzero while the fourth action's button is held
!JUMP_PRESSED = $0387           ; nonzero when the jump's button was just pressed

; The player's work RAM
!TILE_BITS = $0012              ; the last probed tile's behaviour (bit 5: it hurts)
!HOVER_ON = $036E               ; nonzero: the fourth action makes him hover
!V_141B = $141B
!V_141C = $141C                 ; zero: no taking hold
!V_141E = $141E                 ; the top speed's high byte (player_top_speed + 1)
!V_1427 = $1427                 ; nonzero: no landing on actors
!V_142E = $142E                 ; counted up while the continue screen shows
!AIR_THROW = $144B              ; frames left of a throw in the air
!PLAYER_LANDED = $144E          ; 1: he has landed on an actor
!BOUNCE_FRAMES = $144F          ; frames left of a part of the bounce
!BOUNCE_ROW = $1450             ; the bounce's row of vertical speeds (0 the lowest)
!AIR_FRAMES = $1451             ; frames in the air, counted up to $80
!HOLD_LET_GO = $1458            ; nonzero: what holds him lets him go
!NO_LANDING = $145A             ; nonzero: frames with no hold on a ledge; negative: none
!COPIER_HELD = $1461            ; nonzero: a copier's RAM was found; he stays held
!V_1A10 = $1A10
!V_1A12 = $1A12
!BOUNCE_SUM = $002C             ; the bounce's row plus 4, kept for its times 3

; Record fields of the player's code
!FRAMES_40 = !actor_40          ; frames left of a pose (and the jump's pose step)
!FRAMES_47 = !actor_47          ; frames left of the bounce's first part

; Tables in the data bank ($80), by the holder's animation frame (ACTOR_FRAME)
!HELD_PRESETS = $C734           ; the motion preset he jumps off with
!HELD_SPAWN_AT = $C748          ; where his throw's apple starts
!HELD_BOXES = $C75C             ; his box wanted while held
!BOUNCE_ROWS = $C770            ; the bounce's row by frames in the air or left

; The check a copier's cartridge RAM answers and a retail cartridge does not
!COPIER_CHECK = $701234
!COPIER_MARK = $5A

; Kinds and states
!HOLD_KIND = $12                ; the kind of the actor started beside him while held
!STATE_START = $0C

; @routine 81:D59A PlayerHeldLetGo
; @actor
; WHAT IT DOES: Aladdin lets go of what held him: he drops from it with
; his air's settings, the hold's sound and pose, and goes on in the air.
; HOW: PLAYER_BOX_WANTED 3, ActorSpeedXClear, ActorSpeedYClear; gravity
; $38; top speed $0200 (16-bit); ACTOR_SPAWN_AT from HELD_SPAWN_AT by
; ACTOR_FRAME; ActorSpawnPlace with X = the player (16-bit X);
; PlayerMoveStart $0E; SoundQueueAdd 1; NO_LANDING 4, V_1427 0, FRAMES_40
; 3; on at PlayerAirTimed.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerHeld (81:D76B). Exit: never returns.
%at($81D59A)
PlayerHeldLetGo:
    lda.b #$03
    sta.w !player_box_wanted
    jsl.l ActorSpeedXClear
    jsl.l ActorSpeedYClear
    lda.b #$38
    sta.b !actor_accel_y
    rep.b #$20
    lda.w #$0200
    sta.w !player_top_speed
    sep.b #$20
    ldx.b !ACTOR_FRAME
    lda.w !HELD_SPAWN_AT,x
    sta.b !actor_spawn_at
    rep.b #$10
    ldx.w #!player
    jsl.l ActorSpawnPlace
    sep.b #$10
    lda.b #$0E
    jsr.w PlayerMoveStart
    lda.b #$01
    jsl.l SoundQueueAdd
    lda.b #$04
    sta.w !NO_LANDING
    stz.w !V_1427
    lda.b #$03
    sta.b !FRAMES_40
    jmp.w PlayerAirTimed

; @routine 81:D5EC PlayerHeldJumpOff
; @actor
; WHAT IT DOES: Aladdin jumps off what held him, at the speed its frame
; gives, then flies in the air a frame at a time as in PlayerAir until he
; lands, catches a ledge or hovers.
; HOW: V_1427 0; ActorMotionPreset with Y = HELD_PRESETS by A (the
; holder's frame); PlayerGroundYSave; NO_LANDING $10; ACTOR_SPAWN_AT from
; HELD_SPAWN_AT by ACTOR_FRAME; top speed $0200 (16-bit X); ActorSpawnPlace
; with X = the player; SoundQueueAdd 1; V_1A08 $0A, PLAYER_BOX_WANTED 3,
; V_141C up, AIR_FRAMES 0, ACTOR_09 bit 0 cleared, FRAMES_40 6. Each frame
; (BRK first from the second): V_1428: TILE_BITS 0, on at PlayerLand.
; AIR_FRAMES up while under $80, 0 with JUMP_PRESSED. AIR_THROW counting
; down: at 0 FRAMES_40 2 and PlayerJumpPoseStep; else PlayerAppleThrow: a
; throw starts move $16 and AIR_THROW $16, else PlayerJumpPoseStep.
; PlayerAirStep, PlayerFlipByPad. HOVER_ON, HOVER_HELD and falling: on at
; PlayerHover. Gravity $38, or $80 rising with the jump not held and not
; PLAYER_LANDED; PlayerRunSpeed; NO_LANDING down. Rising: the next frame.
; Level 5: GroundBehindBit4 at probe $18 sets V_1A10 and V_1A12. Not down
; on the pad and NO_LANDING 0: GroundBehindBit3 nonzero, on at
; PlayerLedgeGrab. NO_LANDING down again; ActorOutOfGround nonzero: on at
; PlayerLand; else the next frame.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; A = the
; holder's frame (4 to $0C); reached by a jump from PlayerHeld
; (81:D779). Exit: never returns.
%at($81D5EC)
PlayerHeldJumpOff:
    stz.w !V_1427
    tax
    ldy.w !HELD_PRESETS,x
    jsl.l ActorMotionPreset
    jsr.w PlayerGroundYSave
    lda.b #$10
    sta.w !NO_LANDING
    ldx.b !ACTOR_FRAME
    lda.w !HELD_SPAWN_AT,x
    sta.b !actor_spawn_at
    rep.b #$10
    ldx.w #$0200
    stx.w !player_top_speed
    ldx.w #!player
    jsl.l ActorSpawnPlace
    sep.b #$10
    lda.b #$01
    jsl.l SoundQueueAdd
    lda.b #$0A
    sta.w !v_1a08
    lda.b #$03
    sta.w !player_box_wanted
    inc.w !V_141C
    stz.w !AIR_FRAMES
    lda.b !actor_09
    and.b #$FE
    sta.b !actor_09
    lda.b #$06
    sta.b !FRAMES_40
    bra .test
.frame:
    brk.b #$00
.test:
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
    bne .move
    lda.b #$02
    sta.b !FRAMES_40
    bra .pose
.throw:
    jsr.w PlayerAppleThrow
    bcc .pose
    lda.b #$16
    jsr.w PlayerMoveStart
    lda.b #$16
    sta.w !AIR_THROW
    bra .move
.pose:
    jsr.w PlayerJumpPoseStep
.move:
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    lda.w !HOVER_ON
    beq .gravity
    lda.w !HOVER_HELD
    beq .gravity
    lda.b !actor_speed_y+2
    bmi .gravity
    jmp.w PlayerHover
.gravity:
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
    lda.w !NO_LANDING
    beq .rising_test
    dec.w !NO_LANDING
.rising_test:
    lda.b !actor_speed_y+2
    bmi .frame
    lda.w !level
    cmp.b #$05
    bne .ledge_test
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq .ledge_test
    lda.b #$01
    sta.w !V_1A10
    sta.w !V_1A12
.ledge_test:
    lda.w !PAD1_DIRS
    and.b #!PAD_DOWN
    bne .ground_test
    lda.w !NO_LANDING
    bne .ground_test
    jsl.l GroundBehindBit3
    beq .ground_test
    jmp.w PlayerLedgeGrab
.ground_test:
    lda.w !NO_LANDING
    beq .out_test
    dec.w !NO_LANDING
.out_test:
    jsl.l ActorOutOfGround
    bne .land
    jmp.w .frame
.land:
    jmp.w PlayerLand

; @routine 81:D6F8 PlayerHeld
; @actor
; WHAT IT DOES: Aladdin held by an actor (it set his code here): his
; box follows its frame until it lets him go or he lets go (down on the
; pad) or jumps off (the jump, while its frame is 4 to $0C). On a copier
; (its cartridge RAM answers the check) he is held for good.
; HOW: ACTOR_FACING from ACTOR_FLIP; PlayerPosRestore; SoundQueueAdd 6;
; V_141C, PLAYER_LANDED, AIR_THROW, HOLD_LET_GO and V_141B 0, V_1427 up;
; PlayerFlagsClear; PlayerMoveStart $10; COPIER_HELD 0; COPIER_MARK
; stored at COPIER_CHECK and compared: equal, COPIER_HELD up.
; ActorAllocBLong: a record (N clear, X 16-bit): its state $0C and kind
; HOLD_KIND, X 8-bit. Each frame (BRK first): PLAYER_BOX_WANTED from
; HELD_BOXES by ACTOR_FRAME; PlayerBoxUpdate; PlayerOutOfWallsHurt.
; COPIER_HELD: the next frame. HOLD_LET_GO or down on the pad: on at
; PlayerHeldLetGo. JUMP_PRESSED with ACTOR_FRAME 4 to $0C: on at
; PlayerHeldJumpOff with A = the frame. Else the next frame.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where a holder (kinds $6C, $79, $7D, $87 and the
; grabber) set his resume address. Exit: never returns.
%at($81D6F8)
PlayerHeld:
    lda.b !actor_flip
    sta.b !actor_facing
    jsr.w PlayerPosRestore
    lda.b #$06
    jsl.l SoundQueueAdd
    stz.w !V_141C
    stz.w !PLAYER_LANDED
    stz.w !AIR_THROW
    stz.w !HOLD_LET_GO
    stz.w !V_141B
    inc.w !V_1427
    jsr.w PlayerFlagsClear
    lda.b #$10
    jsr.w PlayerMoveStart
    stz.w !COPIER_HELD
    lda.b #!COPIER_MARK
    sta.l !COPIER_CHECK
    cmp.l !COPIER_CHECK
    bne .start
    inc.w !COPIER_HELD
.start:
    jsl.l ActorAllocBLong
    bmi .frame
    lda.b #!STATE_START
    sta.w !actor_state,x
    lda.b #!HOLD_KIND
    sta.w !actor_06,x
    sep.b #$10
.frame:
    brk.b #$00
    ldx.b !ACTOR_FRAME
    lda.w !HELD_BOXES,x
    sta.w !player_box_wanted
    jsr.w PlayerBoxUpdate
    jsr.w PlayerOutOfWallsHurt
    lda.w !COPIER_HELD
    bne .frame
    lda.w !HOLD_LET_GO
    bne .let_go
    lda.w !PAD1_DIRS
    and.b #!PAD_DOWN
    bne .let_go
    lda.w !JUMP_PRESSED
    bne .jump_test
    bra .frame
.let_go:
    jmp.w PlayerHeldLetGo
.jump_test:
    lda.b !ACTOR_FRAME
    sec
    cmp.b #$04
    bcc .frame
    cmp.b #$0D
    bcs .frame
    jmp.w PlayerHeldJumpOff

; @routine 81:D77C PlayerBounce
; @actor
; WHAT IT DOES: Aladdin lands on an actor's top and bounces off it: a
; short squat, then up at a speed by how long he fell (with the jump
; held) or by when the jump is pressed in the squat, through the
; bounce's two poses, and on in the air.
; HOW: V_141B 0; PlayerGroundYSave; PLAYER_LANDED 1; V_1427 and AIR_THROW
; 0; PlayerFlipByPad, PlayerTopSpeedCap, PlayerRunRatesSet; V_1A08 0;
; PLAYER_BOX_WANTED $10; PlayerMoveStart 9; BOUNCE_FRAMES 9, BOUNCE_ROW
; 0, player_slowed 0. JUMP_HELD: BOUNCE_ROW from BOUNCE_ROWS by
; AIR_FRAMES (8 at most), FRAMES_47 $0A frames (BRK each). Else 9 frames
; (BRK each; JUMP_PRESSED: BOUNCE_ROW from BOUNCE_ROWS by BOUNCE_FRAMES)
; and PlayerSpeedLeast. Then V_1A08 $0A; ActorSpeedYSetGravity with
; gravity $38 and Y = (BOUNCE_ROW + 4) * 3 (BOUNCE_SUM kept); BOUNCE_ROW
; 0: top speed $0270 (byte by byte). PlayerMoveStart $0A and 7 frames,
; PlayerMoveStart $0B and 7 frames, each frame (BRK first)
; PlayerAirStep, PlayerFlipByPad, PlayerRunSpeed. player_slowed 0,
; FRAMES_40 and ACTOR_41 1, PLAYER_BOX_WANTED 3; on at PlayerAir.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where PlayerLandOnActor and other actors set his
; resume address. Exit: never returns.
%at($81D77C)
PlayerBounce:
    stz.w !V_141B
    jsr.w PlayerGroundYSave
    lda.b #$01
    sta.w !PLAYER_LANDED
    stz.w !V_1427
    stz.w !AIR_THROW
    jsr.w PlayerFlipByPad
    jsr.w PlayerTopSpeedCap
    jsr.w PlayerRunRatesSet
    stz.w !v_1a08
    lda.b #$10
    sta.w !player_box_wanted
    lda.b #$09
    jsr.w PlayerMoveStart
    lda.b #$09
    sta.w !BOUNCE_FRAMES
    stz.w !BOUNCE_ROW
    stz.w !player_slowed
    lda.w !JUMP_HELD
    beq .squat
    lda.w !AIR_FRAMES
    tax
    sec
    sbc.b #$08
    bcc .row
    ldx.b #$08
.row:
    lda.w !BOUNCE_ROWS,x
    sta.w !BOUNCE_ROW
    lda.b #$0A
    sta.b !FRAMES_47
.held_frame:
    brk.b #$00
    dec.b !FRAMES_47
    bne .held_frame
    bra .up
.squat:
    brk.b #$00
    lda.w !JUMP_PRESSED
    beq .squat_count
    ldx.w !BOUNCE_FRAMES
    lda.w !BOUNCE_ROWS,x
    sta.w !BOUNCE_ROW
.squat_count:
    dec.w !BOUNCE_FRAMES
    bne .squat
    jsr.w PlayerSpeedLeast
.up:
    lda.b #$0A
    sta.w !v_1a08
    lda.w !BOUNCE_ROW
    clc
    adc.b #$04
    sta.w !BOUNCE_SUM
    asl
    adc.w !BOUNCE_SUM
    tay
    lda.b #$38
    jsl.l ActorSpeedYSetGravity
    lda.w !BOUNCE_ROW
    bne .rise
    lda.b #$70
    sta.w !player_top_speed
    lda.b #$02
    sta.w !V_141E
.rise:
    lda.b #$0A
    jsr.w PlayerMoveStart
    lda.b #$07
    sta.w !BOUNCE_FRAMES
.rise_frame:
    brk.b #$00
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    jsr.w PlayerRunSpeed
    dec.w !BOUNCE_FRAMES
    bne .rise_frame
    lda.b #$0B
    jsr.w PlayerMoveStart
    lda.b #$07
    sta.w !BOUNCE_FRAMES
.top_frame:
    brk.b #$00
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    jsr.w PlayerRunSpeed
    dec.w !BOUNCE_FRAMES
    bne .top_frame
    stz.w !player_slowed
    lda.b #$01
    sta.b !FRAMES_40
    sta.b !actor_41
    lda.b #$03
    sta.w !player_box_wanted
    jmp.w PlayerAir

; @routine 81:D8FD PlayerSprung
; @actor
; WHAT IT DOES: Aladdin sprung up by a spring (kind $40): he flies up
; against the ceiling's pull until he slows to falling, then falls with
; the air's settings and pose until he lands (or catches a ledge or
; hovers).
; HOW: V_1A08 0; gravity $12; vertical speed $00 $01 $00 (byte by byte,
; its high byte last); player_ground_y 0 (byte by byte); ActorStopX;
; PlayerMoveStart $0F. Each frame (BRK first) ActorFlyAgainst until the
; vertical speed's high byte is negative. Then V_141C 1, PLAYER_LANDED 0;
; ActorSpeedYSetGravity with gravity $38 and Y = $63; PLAYER_BOX_WANTED 3;
; PlayerMoveStart 4; FRAMES_40 7, ACTOR_41 $0E. Each frame (BRK first):
; PlayerAirStep, PlayerFlipByPad, PlayerJumpPoseStep; HOVER_ON,
; HOVER_HELD and falling: on at PlayerHover. Gravity $38, or $80 rising
; with the jump not held and not PLAYER_LANDED; PlayerRunSpeed. Rising:
; the next frame. GroundBehindBit3 nonzero: on at PlayerLedgeGrab;
; ActorOutOfGround nonzero: on at PlayerLand; else the next frame.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where kind $40 set his resume address. Exit: never
; returns.
%at($81D8FD)
PlayerSprung:
    stz.w !v_1a08
    lda.b #$12
    sta.b !actor_accel_y
    lda.b #$01
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y
    stz.b !actor_speed_y+2
    stz.w !player_ground_y
    stz.w !player_ground_y+1
    jsl.l ActorStopX
    lda.b #$0F
    jsr.w PlayerMoveStart
.up:
    brk.b #$00
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .up
    lda.b #$01
    sta.w !V_141C
    stz.w !PLAYER_LANDED
    lda.b #$38
    ldy.b #$63
    jsl.l ActorSpeedYSetGravity
    lda.b #$03
    sta.w !player_box_wanted
    lda.b #$04
    jsr.w PlayerMoveStart
    lda.b #$07
    sta.b !FRAMES_40
    lda.b #$0E
    sta.b !actor_41
.frame:
    brk.b #$00
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    jsr.w PlayerJumpPoseStep
    lda.w !HOVER_ON
    beq .gravity
    lda.w !HOVER_HELD
    beq .gravity
    lda.b !actor_speed_y+2
    bmi .gravity
    jmp.w PlayerHover
.gravity:
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
    bmi .frame
    jsl.l GroundBehindBit3
    beq .out_test
    jmp.w PlayerLedgeGrab
.out_test:
    jsl.l ActorOutOfGround
    beq .frame
    jmp.w PlayerLand

; @routine 81:D992 PlayerCarried
; @actor
; WHAT IT DOES: Aladdin carried by kind $2C (the ride): its pose $1A, and
; he waits for good while it moves him, until it sets his code again.
; HOW: PlayerMoveStart $1A; then a BRK each frame for good.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where Kind2CStart set his resume address
; (K2C_RIDE_CODE). Exit: never returns.
%at($81D992)
PlayerCarried:
    lda.b #$1A
    jsr.w PlayerMoveStart
.frame:
    brk.b #$00
    bra .frame

; @routine 81:D99B PlayerGrabbedHang
; @actor
; WHAT IT DOES: Aladdin taken hold of by an actor he hangs from (the
; grabbable ones): stopped where he was kept, the hang's sound and pose,
; and he hangs from it.
; HOW: PLAYER_LANDED and AIR_THROW 0, V_1427 up; PlayerFlagsClear;
; ActorStopX, ActorStopY; PlayerPosRestore; SoundQueueAdd 6;
; PlayerMoveStart 6; PLAYER_BOX_WANTED 4; on at PlayerHang.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where ActorHoldPlayer set his resume address.
; Exit: never returns.
%at($81D99B)
PlayerGrabbedHang:
    stz.w !PLAYER_LANDED
    stz.w !AIR_THROW
    inc.w !V_1427
    jsr.w PlayerFlagsClear
    jsl.l ActorStopX
    jsl.l ActorStopY
    jsr.w PlayerPosRestore
    lda.b #$06
    jsl.l SoundQueueAdd
    lda.b #$06
    jsr.w PlayerMoveStart
    lda.b #$04
    sta.w !player_box_wanted
    jmp.w PlayerHang

; @routine 81:D9C5 PlayerSwitchHang
; @actor
; WHAT IT DOES: Aladdin hangs from a switch's handle (kind $3B) or is
; held by the continue screen's choice: he hangs where he was kept until
; V_142E is 0 and down is on the pad, then drops 18 pixels and falls.
; HOW: PlayerPosRestore; PLAYER_LANDED and AIR_THROW 0, V_1427 up;
; PlayerFlagsClear; ActorStopX, ActorStopY; SoundQueueAdd 6;
; PlayerMoveStart 6; PLAYER_BOX_WANTED 4. Each frame (BRK first): V_142E
; nonzero or down not on the pad: the next frame. Then (no script runs
; it) PlayerGroundYSave; NO_LANDING $10; his y pixels $12 lower (16-bit);
; V_1427 0; on at PlayerFall.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where kind $3B or ContinueChoice set his resume
; address. Exit: never returns.
%at($81D9C5)
PlayerSwitchHang:
    jsr.w PlayerPosRestore
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
.frame:
    brk.b #$00
    lda.w !V_142E
    bne .frame
    lda.w !PAD1_DIRS
    and.b #!PAD_DOWN
    beq .frame
    jsr.w PlayerGroundYSave
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

; @routine 81:DA14 PlayerFlung
; @actor
; WHAT IT DOES: Aladdin flung up by an actor he landed on (kind $7B): he
; flies up against the ceiling's pull until he slows to falling, rests 8
; frames, then is in the air as in PlayerAir until he lands, catches a
; ledge or hovers.
; HOW: V_1A08 0; gravity $10; vertical speed $00 $01 $00 (byte by byte);
; ActorStopX; PlayerMoveStart $18. Each frame (BRK first) ActorFlyAgainst
; until the vertical speed's high byte is negative. Then V_141C 1,
; PLAYER_LANDED 0; ActorSpeedYSetGravity with gravity $38 and Y = $66;
; COP 8; FRAMES_40 0; PlayerMoveStart 4; V_1A08 $0A; PlayerGroundYSave.
; Each frame (BRK first): AIR_THROW counting down: at 0 FRAMES_40 8 and
; ACTOR_41 1, PlayerJumpPoseStep; else PlayerAppleThrow: a throw starts
; move $16 and AIR_THROW $14, else PlayerJumpPoseStep. PlayerAirStep,
; PlayerFlipByPad; HOVER_ON, HOVER_HELD and falling: on at PlayerHover.
; Gravity $38, or $80 rising with the jump not held and not
; PLAYER_LANDED; PlayerRunSpeed. Rising: the next frame. GroundBehindBit3
; nonzero: on at PlayerLedgeGrab. V_1428, or ActorOutOfGround nonzero: on
; at PlayerLand; else the next frame.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where kind $7B set his resume address. Exit: never
; returns.
%at($81DA14)
PlayerFlung:
    stz.w !v_1a08
    lda.b #$10
    sta.b !actor_accel_y
    lda.b #$01
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y
    stz.b !actor_speed_y+2
    jsl.l ActorStopX
    lda.b #$18
    jsr.w PlayerMoveStart
.up:
    brk.b #$00
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .up
    lda.b #$01
    sta.w !V_141C
    stz.w !PLAYER_LANDED
    lda.b #$38
    ldy.b #$66
    jsl.l ActorSpeedYSetGravity
    lda.b #$08
    cop.b #$00
    stz.b !FRAMES_40
    lda.b #$04
    jsr.w PlayerMoveStart
    lda.b #$0A
    sta.w !v_1a08
    jsr.w PlayerGroundYSave
.frame:
    brk.b #$00
    lda.w !AIR_THROW
    beq .throw
    dec.w !AIR_THROW
    bne .move
    lda.b #$08
    sta.b !FRAMES_40
    lda.b #$01
    sta.b !actor_41
    bra .pose
.throw:
    jsr.w PlayerAppleThrow
    bcc .pose
    lda.b #$16
    jsr.w PlayerMoveStart
    lda.b #$14
    sta.w !AIR_THROW
    bra .move
.pose:
    jsr.w PlayerJumpPoseStep
.move:
    jsr.w PlayerAirStep
    jsr.w PlayerFlipByPad
    lda.w !HOVER_ON
    beq .gravity
    lda.w !HOVER_HELD
    beq .gravity
    lda.b !actor_speed_y+2
    bmi .gravity
    jmp.w PlayerHover
.gravity:
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
    bmi .frame
    jsl.l GroundBehindBit3
    beq .landed_test
    jmp.w PlayerLedgeGrab
.landed_test:
    lda.w !v_1428
    bne .land
    jsl.l ActorOutOfGround
    beq .frame
.land:
    jmp.w PlayerLand
