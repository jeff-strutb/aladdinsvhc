; carpet_ride.asm: Aladdin's code on the magic carpet: the ride's start
; (it flies in from the left), its frames (the pad steers it through
; carpet.asm's routines), a wall that knocks him off, and the crash that
; loses a life. Two rides: the one whose record has $06 = 1 and another
; (carpet.asm's "Alt" routines).
; C: src/aladdin/player/carpet_ride.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_player.inc"

arch 65816

!PAD1_DIRS = $0380              ; pad 1's high byte: its low 4 bits Up Down Left Right
!TILE_BITS = $0012              ; the last probed tile's behaviour (bit 5: it hurts)
!PLAYER_FLAGS = $0347           ; bit 7 cleared as the ride starts
!SUB_LAYERS = $039A             ; the layers on the sub screen: bit 4 (sprites) set
!LIVES = $0364                  ; lives left
!SCENE_WAIT = $0087             ; the crash's end waits for this to be 0
!SCENE_ON = $03FA               ; counted up as a life is lost
!SCROLL_KIND = $083D            ; the level's scroll handler, times 2
!VIEW_EDGE_RULE = $145B         ; the rule the view's edges follow
!V_1A10 = $1A10
!V_1A12 = $1A12
!NO_LIVES_TASK = $80B2E3        ; the game task's entry with no lives left

!FIGURE_29 = $29                ; word: his picture list
!CARPET_DIRECTION = $14         ; the steering direction (carpet.asm)
!CARPET_CLIMBING = $42          ; nonzero: tilted to climb
!CARPET_TILTED = $43            ; CarpetTilt's answer: nonzero when it tilted
!CARPET_SPED_UP = $44
!CARPET_SLOWED = $45
!FLY_IN_FRAMES = !actor_stream  ; frames of the flight in

!CARPET_DIRECTIONS = $C7AA      ; (bank $80) the steering direction by the pad's low 4 bits; negative: none

; @routine 81:E3F5 PlayerCarpetStart
; @actor
; WHAT IT DOES: The carpet ride's start (with $07 = 2 the ending's ride
; instead): the rider's look and test box, the text blink's task on
; level 7; he flies in from the left (127 frames, or $35 on the other
; ride) and, but on levels 7 and $17, drifts to cruising speed until
; the view holds still and starts the text blink's task; then the ride.
; HOW: $07 = 2: PlayerEndingRide. ACTOR_09 $40D0, FIGURE_29 $8917,
; CARPET_CLIMBING word 0, ACTOR_REDRAW $FF, PLAYER_BOX_WANTED $11,
; PlayerBoxSet. Level 7 (and under $14): TaskCreate (slot 2, parameter 0,
; TextBlinkTask). PLAYER_FLAGS bit 7 off, SUB_LAYERS bit 4 on;
; CARPET_SPED_UP, CARPET_SLOWED, facing and flip 0; ActorStopY. Speed
; $0003E0 and FLY_IN_FRAMES $7F ($06 = 1), else $000300 and $35;
; PlayerMoveStart 0; each frame (BRK first) ActorMoveX for them. Speed
; fraction 0. Not level 7 or $17: CarpetCruiseSetAlt, each frame (BRK
; first) ActorMoveX while V_1A12 is set, TaskCreate (TextBlinkTask). On
; at PlayerCarpet.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $06 = 1-3 or $07 = 1. Exit: never returns.
%at($81E3F5)
PlayerCarpetStart:
    lda.b !actor_07
    cmp.b #$02
    bne .carpet
    jmp.w PlayerEndingRide
.carpet:
    rep.b #$20
    lda.w #$40D0
    sta.b !actor_09
    lda.w #$8917
    sta.b !FIGURE_29
    stz.b !CARPET_CLIMBING
    sep.b #$20
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    lda.b #$11
    sta.w !player_box_wanted
    jsr.w PlayerBoxSet
    lda.w !level
    cmp.b #$07
    bne .flags
    cmp.b #$14
    bcs .flags
    jsl.l TaskCreate
    db 2*24, 0                  ; slot 2, parameter 0
    dl TextBlinkTask
.flags:
    lda.w !PLAYER_FLAGS
    and.b #$7F
    sta.w !PLAYER_FLAGS
    lda.w !SUB_LAYERS
    ora.b #$10
    sta.w !SUB_LAYERS
    stz.b !CARPET_SPED_UP
    stz.b !CARPET_SLOWED
    stz.b !actor_facing
    stz.b !actor_flip
    jsl.l ActorStopY
    lda.b !actor_06
    dec a
    bne .other_ride
    lda.b #$E0
    sta.b !actor_speed_x
    lda.b #$03
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$7F
    bra .fly_in
.other_ride:
    stz.b !actor_speed_x
    lda.b #$03
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$35
.fly_in:
    sta.b !FLY_IN_FRAMES
    lda.b #$00
    jsr.w PlayerMoveStart
.fly_in_frame:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !FLY_IN_FRAMES
    bne .fly_in_frame
    stz.b !actor_speed_x
    lda.w !level
    cmp.b #$07
    beq PlayerCarpet
    cmp.b #$17
    beq PlayerCarpet
    jsr.w CarpetCruiseSetAlt
.drift:
    brk.b #$00
    jsl.l ActorMoveX
    lda.w !V_1A12
    bne .drift
    jsl.l TaskCreate
    db 2*24, 0                  ; slot 2, parameter 0
    dl TextBlinkTask

; @routine 81:E49C PlayerCarpet
; @actor
; WHAT IT DOES: A frame of the carpet ride: the rider's animation, a
; wall that hurts (on the first ride, at x under $1E00), and the pad: a
; direction steers (PlayerCarpetSteer); none levels the carpet off and
; drifts it back to cruising.
; HOW: BRK; AnimStep. First ride and x's high byte under $1E:
; PlayerCarpetWallHurt. CARPET_DIRECTIONS by PAD1_DIRS's low 4 bits: not
; negative, it to CARPET_DIRECTION and on at PlayerCarpetSteer. Else
; CarpetLevelOut; CarpetCruiseX (Alt), then direction 10 CarpetCruiseSet
; (on the first ride a path no script runs), else CarpetSteerX (Alt);
; CarpetSteadyY, CarpetSteerY (Alt); CARPET_DIRECTION 0, ActorMove; again.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
; from PlayerCarpetStart's end and by a jump from its own frame's end
; and PlayerCarpetSteer. Exit: never returns.
%at($81E49C)
PlayerCarpet:
    brk.b #$00
    jsl.l AnimStep
    lda.b !actor_06
    dec a
    bne .pad
    lda.b !actor_x+2
    cmp.b #$1E
    bcs .pad
    jsr.w PlayerCarpetWallHurt
.pad:
    lda.w !PAD1_DIRS
    and.b #$0F
    tay
    lda.w !CARPET_DIRECTIONS,y
    bmi .no_direction
    sta.b !CARPET_DIRECTION
    jmp.w PlayerCarpetSteer
.no_direction:
    jsr.w CarpetLevelOut
    lda.b !actor_06
    dec a
    bne .other_ride
    jsr.w CarpetCruiseX
    lda.b !CARPET_DIRECTION
    sec
    cmp.b #$0A
    bne .steer
    jsr.w CarpetCruiseSet
    bra .steady
.steer:
    jsr.w CarpetSteerX
.steady:
    jsr.w CarpetSteadyY
    jsr.w CarpetSteerY
    bra .move
.other_ride:
    jsr.w CarpetCruiseXAlt
    lda.b !CARPET_DIRECTION
    sec
    cmp.b #$0A
    bne .steer_alt
    jsr.w CarpetCruiseSet
    bra .steady_alt
.steer_alt:
    jsr.w CarpetSteerXAlt
.steady_alt:
    jsr.w CarpetSteadyY
    jsr.w CarpetSteerYAlt
.move:
    stz.b !CARPET_DIRECTION
    jsl.l ActorMove
    jmp.w PlayerCarpet

; @routine 81:E51C PlayerCarpetSteer
; @actor
; WHAT IT DOES: A frame of the carpet ride steered: the carpet tilts,
; its speed across follows the direction within its limits, and when it
; tilted its speed up and down too; then the next frame.
; HOW: CARPET_SPED_UP and CARPET_SLOWED 0; CarpetTilt to CARPET_TILTED.
; CarpetSteerX and CarpetSpeedXLimit (Alt on the other ride); not tilted:
; CarpetSteadyY, CarpetSteerY (Alt); tilted: CarpetSteerY (Alt) and
; CarpetSpeedYLimit. CARPET_DIRECTION 0, ActorMove; on at PlayerCarpet.
; Entry: M 8-bit, X 8-bit; D = the player's record; CARPET_DIRECTION the
; pad's; DB = $80; reached by a jump from PlayerCarpet. Exit: never
; returns.
%at($81E51C)
PlayerCarpetSteer:
    stz.b !CARPET_SPED_UP
    stz.b !CARPET_SLOWED
    jsr.w CarpetTilt
    sta.b !CARPET_TILTED
    lda.b !actor_06
    dec a
    bne .other_ride
    jsr.w CarpetSteerX
    jsr.w CarpetSpeedXLimit
    lda.b !CARPET_TILTED
    beq PlayerCarpet_steady
    jsr.w CarpetSteerY
    jsr.w CarpetSpeedYLimit
    bra .move
.other_ride:
    jsr.w CarpetSteerXAlt
    jsr.w CarpetSpeedXLimitAlt
    lda.b !CARPET_TILTED
    beq PlayerCarpet_steady_alt
    jsr.w CarpetSteerYAlt
    jsr.w CarpetSpeedYLimit
.move:
    stz.b !CARPET_DIRECTION
    jsl.l ActorMove
    jmp.w PlayerCarpet

; @routine 81:E713 PlayerCarpetWallHurt
; WHAT IT DOES: The carpet out of a wall in front; when the ground last
; probed hurts, the rider is put into state $8C and the carpet crashes.
; HOW: ActorOutOfWalls; ACTOR_08 bit 7 or bit 4 set, or TILE_BITS bit 5
; clear: back. Else PlayerEnter8CLong, the call's return address dropped
; (PLA PLA) and on at PlayerCarpetCrash.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from PlayerCarpet at the actor's level (81:E4AD). Exit: A low, N
; and Z as the last test left them. Hurt: never returns.
%at($81E713)
PlayerCarpetWallHurt:
    jsl.l ActorOutOfWalls
    lda.b !actor_08
    bmi .back
    bit.b #$10
    bne .back
    lda.w !TILE_BITS
    bit.b #$20
    beq .back
    jsl.l PlayerEnter8CLong
    pla
    pla
    jmp.w PlayerCarpetCrash
.back:
    rts

; @routine 81:E7E9 PlayerCarpetCrash
; @actor
; WHAT IT DOES: The carpet crashes: the view holds still, Aladdin loses
; his health with the song and the hurt sound and falls away (move 5)
; until he is out of the map; then the stage starts again once the scene
; is over, or ends with no lives.
; HOW: SCROLL_KIND 0, VIEW_EDGE_RULE 4, V_1A10 and V_1A12 1; facing and
; flip 0; HEALTH 0, SCENE_ON up; SongPlay $2A, SoundQueueAdd 2; Y = $52,
; ActorMotionPreset; PlayerMoveStart 5. Each frame (BRK first) ActorFly
; while PlayerFallenTest's C is set. TaskCreate (slot 1, parameter 4,
; BrightnessDownTask); frames until SCENE_WAIT is 0; LIVES down; TaskExec
; to LevelPlayStartTask, or with none left to the task's entry for no
; lives.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerCarpetWallHurt. Exit: never returns.
%at($81E7E9)
PlayerCarpetCrash:
    lda.b #$00
    sta.w !SCROLL_KIND
    lda.b #$04
    sta.w !VIEW_EDGE_RULE
    lda.b #$01
    sta.w !V_1A10
    sta.w !V_1A12
    stz.b !actor_facing
    stz.b !actor_flip
    stz.w !health
    inc.w !SCENE_ON
    lda.b #$2A
    jsl.l SongPlay
    lda.b #$02
    jsl.l SoundQueueAdd
    ldy.b #$52
    jsl.l ActorMotionPreset
    lda.b #$05
    jsr.w PlayerMoveStart
.fall:
    brk.b #$00
    jsl.l ActorFly
    jsr.w PlayerFallenTest
    bcs .fall
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessDownTask
.scene:
    brk.b #$00
    lda.w !SCENE_WAIT
    bne .scene
    dec.w !LIVES
    lda.w !LIVES
    beq .no_lives
    jsl.l TaskExec
    dl LevelPlayStartTask
.no_lives:
    jsl.l TaskExec
    dl !NO_LIVES_TASK
