; scene_figures.asm: scripted figures: actor kind $93, which walks in once
; the scene's cue $00C2 is given, the player started with $07 = 15,
; who stands at (128, 176) for a scene, and the player started with $07
; = 14, who rides the carpet across the ending, the player started with
; $07 = 9, who comes down to the ground and waits for a cue, and actor
; kind $72,
; which rides along at the player's place in his moves until he takes
; move 5, then flies off. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/game/scene_figures.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_video.inc"

arch 65816

!SCENE_CUE_C2 = $C2  ; counted up by a scripted actor when it has waited; this one waits for it
!FIGURE_29 = $29  ; word: its picture list ($891F, $8907)
!V_145B = $145B
!V_1430 = $1430
; The ride's screens: the play field's x screen (its scroll x's high
; byte), which it waits on.
!PLAY_SCREEN = !layer0+!LAYER_SCROLL_X+1
!RIDE_TIMER = !actor_40         ; frames of its first stretch
!RIDE_COUNT = !actor_46         ; frames of its third stretch (word)
!RIDE_41 = !actor_41            ; 1 through the third stretch, else 0
; Not yet filed (no script rides that far): the ride on from screen 12.
!ENDING_RIDE_ON = $81E315

; The player started with $07 = 9.
!CUE_BE = $00BE                 ; the camera trigger task's second cue: cleared as it starts

; Kind $72's own fields and tables.
!ALONG_16 = $16                 ; word: $D6B5
!ALONG_MOVE = $48               ; the player's move it last took up
!ALONG_08_BY_41 = $D6B3         ; in the bank DB names: its $08 by the player's $41
!ALONG_LEAVE_MOVE = $05         ; the player's move at which it flies off
!KIND_7A = $7A                  ; the child it starts as it flies off
!ALONG_FLY_MOTION = $54         ; the motion preset it flies off on

; @routine A5:F3F0 Kind93WalkIn
; @actor
; WHAT IT DOES: Actor kind $93: stands facing left until the scene's cue
; (SCENE_CUE_C2), then walks left until its x is below 162, takes its
; second pose and stays.
; HOW: $09 bits $90 set, $2C = 2, redraw marked, $29 = $891F, facing and
; flip 1, animation 0; BRK until SCENE_CUE_C2; ActorSpeedXSet $72; each
; frame ActorMoveX while its x pixels' low byte is 162 or more; animation
; 1; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the actor loop's jump for kind $93 in state $0C (through the stub at
; $83:9389). Exit: never returns.
%at($A5F3F0)
Kind93WalkIn:
    lda.b #$90
    tsb.b !actor_09
    lda.b #$02
    sta.b !actor_2c
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    rep.b #$20
    lda.w #$891F
    sta.b !FIGURE_29
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.lF411:
    brk.b #$00
    lda.w !SCENE_CUE_C2
    beq .lF411
    ldy.b #$72
    jsl.l ActorSpeedXSet
.lF41E:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_x+1
    cmp.b #$A2
    bcs .lF41E
    lda.b #$01
    jsl.l AnimStart
.lF430:
    brk.b #$00
    bra .lF430


; @routine 81:E23E PlayerSceneStand
; @actor
; WHAT IT DOES: The player started with $07 = 15: Aladdin set at (128,
; 176) facing right for a scene, standing; after $20 frames V_142D is
; counted up, and he stays.
; HOW: $29 = $8907, x 128, y 176; V_145B = 1; facing and flip 0;
; PlayerMoveStart 8; V_1430 up; $09 = $D0, redraw marked, $06 cleared,
; TASK_PARAM cleared; COP $20; V_142D up; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
; the jump through the player's start table by $07 at $81:CB61 (81:CB56).
; Exit: never returns.
%at($81E23E)
PlayerSceneStand:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    lda.w #$0080
    sta.b !actor_x+1
    lda.w #$00B0
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$01
    sta.w !V_145B
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$08
    jsr.w PlayerMoveStart
    inc.w !V_1430
    lda.b #$D0
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    stz.b !actor_06
    stz.w !task_param
    lda.b #$20
    cop.b #$00
    inc.w !v_142d
.lE276:
    brk.b #$00
    bra .lE276

; @routine 81:E27A PlayerEndingRide
; @actor
; WHAT IT DOES: The player started with $07 = 14: Aladdin on the carpet
; across the ending. He flies right at 2 pixels a frame for $57 frames,
; then bobs up and down at 3/4 of a pixel a frame as the play field
; scrolls: in move 0 to screen 5, move 3 for $1B0 frames, move 4 to
; screen 12, and on (the rest is not filed).
; HOW: $09 = $D0, $0A = $40, picture list $8917, redraw marked; flip 0;
; speed $0200; RIDE_41 0; PlayerMoveStart 0; PLAYER_MOVE 0 again. Each
; frame (BRK first) ActorMoveX, $57 times. Speed $00C0, actor_49 1 (the
; bob going up), ActorSpeedYClear, actor_accel_y 1, PlayerMoveChange 0;
; each frame ActorBobY and ActorMoveX until PLAY_SCREEN is 5. RIDE_41 1,
; PlayerMoveChange 3, RIDE_COUNT $1B0: each frame actor_49 cleared,
; ActorBobY and ActorMoveX, until the count is down to 0. RIDE_41 0,
; PlayerMoveChange 4: each frame ActorBobY and ActorMoveX until
; PLAY_SCREEN is 12; then on at ENDING_RIDE_ON, which no script reaches
; yet and is not filed.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
; the jump through the player's start table by $07 at $81:CB61 (81:CB56).
; Exit: never returns.
%at($81E27A)
PlayerEndingRide:
    rep.b #$20
    lda.w #$40D0
    sta.b !actor_09
    lda.w #$8917
    sta.b !FIGURE_29
    sep.b #$20
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    stz.b !actor_flip
    stz.b !actor_speed_x
    lda.b #$02
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    stz.b !RIDE_41
    lda.b #$00
    jsr.w PlayerMoveStart
    stz.w !player_move
    lda.b #$57
    sta.b !RIDE_TIMER
.fly:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !RIDE_TIMER
    bne .fly
    lda.b #$C0
    sta.b !actor_speed_x
    stz.b !actor_speed_x+1
    lda.b #$01
    sta.b !actor_49
    jsl.l ActorSpeedYClear
    lda.b #$01
    sta.b !actor_accel_y
    stz.b !RIDE_41
    lda.b #$00
    jsr.w PlayerMoveChange
.to_screen_5:
    brk.b #$00
    jsl.l ActorBobY
    jsl.l ActorMoveX
    lda.w !PLAY_SCREEN
    cmp.b #$05
    bne .to_screen_5
    lda.b #$01
    sta.b !RIDE_41
    lda.b #$03
    jsr.w PlayerMoveChange
    lda.b #$B0
    sta.b !RIDE_COUNT
    lda.b #$01
    sta.b !RIDE_COUNT+1
.counted:
    brk.b #$00
    stz.b !actor_49
    jsl.l ActorBobY
    jsl.l ActorMoveX
    rep.b #$20
    dec.b !RIDE_COUNT
    sep.b #$20
    bne .counted
    stz.b !RIDE_41
    lda.b #$04
    jsr.w PlayerMoveChange
.to_screen_12:
    brk.b #$00
    jsl.l ActorBobY
    jsl.l ActorMoveX
    lda.w !PLAY_SCREEN
    cmp.b #$0C
    bne .to_screen_12

; @routine 83:F40D Kind72RideAlong
; @actor
; WHAT IT DOES: Actor kind $72: rides along at the player's place, taking
; his move as its animation, until he takes move 5; then it faces right,
; starts a child of kind $7A, and flies off on motion $54 until it is
; off the screen, and ends.
; HOW: actor_08 bit 0 and actor_09 bits $90 set; ALONG_16 = $D6B5; $29 =
; $890D; redraw marked; animation 0; ALONG_MOVE 0. Each frame (BRK
; first): its x and y pixels the player's; with player_move unlike
; ALONG_MOVE: move 5 leaves the loop, else ALONG_MOVE and the animation
; take it, and actor_08 = ALONG_08_BY_41 by the player's $41. Leaving:
; facing, flip and $21 cleared; ActorCreateChild8 $7A; animation 5;
; ActorMotionPreset $54; each frame (BRK first) ActorFly while actor_09
; bit 6 is set; then ActorEndPlaced (that JML, at 83:F483, no script
; reaches yet: it is left in the intake).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $72 in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83F40D)
Kind72RideAlong:
    lda.b !actor_08
    ora.b #$01
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D6B5
    sta.b !ALONG_16
    lda.w #$890D
    sta.b !FIGURE_29
    sep.b #$20
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    lda.b #$00
    jsl.l AnimStart
    stz.b !ALONG_MOVE
.frame:
    brk.b #$00
    rep.b #$20
    lda.w !player+!actor_x+1
    sta.b !actor_x+1
    lda.w !player+!actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    lda.w !player_move
    cmp.b !ALONG_MOVE
    beq .frame
    sec
    cmp.b #!ALONG_LEAVE_MOVE
    beq .leave
    sta.b !ALONG_MOVE
    jsl.l AnimStart
    ldx.w !player+!actor_41
    lda.w !ALONG_08_BY_41,x
    sta.b !actor_08
    bra .frame
.leave:
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !actor_spawn_at
    lda.b #!KIND_7A
    jsl.l ActorCreateChild8
    lda.b #$05
    jsl.l AnimStart
    ldy.b #!ALONG_FLY_MOTION
    jsl.l ActorMotionPreset
.fly:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_09
    bit.b #$40
    bne .fly

; @routine 81:DFBB PlayerSceneLand
; @actor
; WHAT IT DOES: The player started with $07 = 9: Aladdin faces right and,
; unless there is ground just under him, falls 3 pixels a frame in move
; $0E until he is out of the ground; then he stands (move 0) and waits
; for the cue CUE_BE.
; HOW: Facing and flip 0; ActorSnapToGround: on ground (Z clear) on to
; standing. Else PlayerMoveStart $0E, vertical speed $0300 (3 pixels),
; each frame (BRK first) ActorMoveY and ActorOutOfGround until it leaves
; Z clear. Standing: V_145B 4, PlayerMoveStart 0; each frame (BRK first)
; until CUE_BE is nonzero; then on at $81:DFEF, which no script reaches
; yet: those bytes are left in the intake.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
; the jump through the player's start table by $07 at $81:CB61 (81:CB56).
; Exit: never returns.
%at($81DFBB)
PlayerSceneLand:
    stz.b !actor_facing
    stz.b !actor_flip
    jsl.l ActorSnapToGround
    bne .stand
    lda.b #$0E
    jsr.w PlayerMoveStart
    lda.b #$03
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y
    stz.b !actor_speed_y+2
.fall:
    brk.b #$00
    jsl.l ActorMoveY
    jsl.l ActorOutOfGround
    beq .fall
.stand:
    lda.b #$04
    sta.w !V_145B
    lda.b #$00
    jsr.w PlayerMoveStart
.wait:
    brk.b #$00
    lda.w !CUE_BE
    beq .wait
