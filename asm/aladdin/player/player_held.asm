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
