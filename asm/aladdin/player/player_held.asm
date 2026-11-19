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
