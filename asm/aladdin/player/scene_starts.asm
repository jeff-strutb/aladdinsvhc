; scene_starts.asm: Aladdin's code for the scene starts in PlayerStart's
; table by $07 that no level plays: placed and waiting, then walking right
; ($07 2-4); walking in and jumping down ($07 8); running to the middle
; and on through the scene's cues ($07 12); walking out to the next act
; ($07 13).
; C: src/aladdin/player/scene_starts.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!FIGURE_29 = $29                ; word: his picture list
!ACTOR_28 = $28
!PFW_SCENE = $1430              ; nonzero: a scene moves the player
!TASK_PARAM = $0075             ; the first task slot's parameter
!TASK2_STATE = $009F            ; task slot 2's state (0 once it has ended)
!CUE_BE = $00BE                 ; the scene's cue (bit 7: on)
!VIEW_EDGE_RULE = $145B         ; the rule the view's edges follow
!V_147B = $147B
!V_0978 = $0978
!V_0360 = $0360                 ; the restart point
!PART = $035C                   ; the story's part
!V_035D = $035D
!CUE_1B31 = $1B31               ; the scene's cues for $07 = 12
!CUE_1B32 = $1B32
!V_141B = $141B
!V_141C = $141C
!LANDED = $1428                 ; nonzero once he is on the ground
!V_03FB = $03FB

; @routine 81:DDC1 PlayerScenePlaceHigh
; @actor
; WHAT IT DOES: The player started with $07 = 2 or 3: placed at x $0030,
; y $0048, facing right and standing; on at PlayerSceneWaitWalk.
; HOW: (16-bit A) FIGURE_29 $8907, x pixels $0030, y pixels $0048;
; facing and flip 0; PlayerMoveStart 0; on at PlayerSceneWaitWalk.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 2 or 3. Exit: never returns.
%at($81DDC1)
PlayerScenePlaceHigh:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    lda.w #$0030
    sta.b !actor_x+1
    lda.w #$0048
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsr.w PlayerMoveStart
    jmp.w PlayerSceneWaitWalk

; @routine 81:DDE0 PlayerScenePlaceLow
; @actor
; WHAT IT DOES: The player started with $07 = 4: placed at x $0020, y
; $00A8, facing right and standing; on at PlayerSceneWaitWalk.
; HOW: As PlayerScenePlaceHigh, with x $0020 and y $00A8.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 4. Exit: never returns.
%at($81DDE0)
PlayerScenePlaceLow:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    lda.w #$0020
    sta.b !actor_x+1
    lda.w #$00A8
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsr.w PlayerMoveStart
    jmp.w PlayerSceneWaitWalk

; @routine 81:DF3A PlayerSceneWaitWalk
; @actor
; WHAT IT DOES: Aladdin placed in a scene waits until the first task's
; parameter is set, stands 16 frames more, then walks right for ever.
; HOW: PFW_SCENE up; ACTOR_09 $F0, ACTOR_28 $FF, ACTOR_06 0, TASK_PARAM
; 0; each frame (BRK first) until TASK_PARAM is nonzero; PlayerMoveStart
; 0, COP $10; PlayerMoveStart 3; flip 0, speed $0200 (its fraction 0);
; each frame (BRK first) ActorMoveX.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; a jump from PlayerScenePlaceHigh and PlayerScenePlaceLow. Exit: never
; returns.
%at($81DF3A)
PlayerSceneWaitWalk:
    inc.w !PFW_SCENE
    lda.b #$F0
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_28
    stz.b !actor_06
    stz.w !TASK_PARAM
.wait:
    brk.b #$00
    lda.w !TASK_PARAM
    beq .wait
    lda.b #$00
    jsr.w PlayerMoveStart
    lda.b #$10
    cop.b #$00
    lda.b #$03
    jsr.w PlayerMoveStart
    stz.b !actor_flip
    stz.b !actor_speed_x
    lda.b #$02
    sta.b !actor_speed_x+1
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    bra .walk

; @routine 81:DF6F PlayerSceneJumpIn
; @actor
; WHAT IT DOES: The player started with $07 = 8: Aladdin walks right for
; 24 frames, jumps, lands, and after his landing's pose stands.
; HOW: Facing and flip 0; PlayerMoveStart 2; Y = 3, ActorSpeedXSet; 24
; frames (BRK first) of ActorMoveX; Y = $4D, ActorMotionPreset; the pose
; step and timer 0; each frame (BRK first) PlayerJumpPoseStep, ActorFly,
; until he falls (vertical speed not negative) and ActorOutOfGround leaves
; Z clear; SoundQueueAdd $10; PlayerMoveStart $19, COP $80; on at
; PlayerStand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 8. Exit: never returns.
%at($81DF6F)
PlayerSceneJumpIn:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$02
    jsr.w PlayerMoveStart
    ldy.b #$03
    jsl.l ActorSpeedXSet
    lda.b #$18
    sta.b !actor_stream
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !actor_stream
    bne .walk
    ldy.b #$4D
    jsl.l ActorMotionPreset
    stz.b !actor_40
    stz.b !actor_41
.air:
    brk.b #$00
    jsr.w PlayerJumpPoseStep
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .air
    jsl.l ActorOutOfGround
    beq .air
    lda.b #$10
    jsl.l SoundQueueAdd
    lda.b #$19
    jsr.w PlayerMoveStart
    lda.b #$80
    cop.b #$00
    jmp.w PlayerStand

; @routine 81:E0AE PlayerSceneWalkOut
; @actor
; WHAT IT DOES: The player started with $07 = 13: Aladdin walks right to
; the scene's cue point, waits for the cue, walks on as the screen fades,
; and the story goes on to its next part (the task at $81:F61D).
; HOW: Facing and flip 0; VIEW_EDGE_RULE 4, V_147B up; PlayerMoveStart 2,
; speed $0100; each frame (BRK first) ActorMoveX until x pixels' high
; byte is 6 or more and its low byte $50 or more; PlayerMoveStart 0,
; CUE_BE up; frames (BRK first) until CUE_BE bit 7; PlayerMoveStart 2,
; ACTOR_40 0, V_0978 3; 256 frames (BRK first) of ActorMoveX;
; SoundQueueF6With1; TaskCreate (slot 2, parameter 4,
; BrightnessDownTask); frames (BRK first) of ActorMoveX while slot 2's
; state is nonzero; V_0360 and V_035D 0, PART up; TaskExec $81:F61D.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 13. Exit: never returns (TaskExec).
%at($81E0AE)
PlayerSceneWalkOut:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$04
    sta.w !VIEW_EDGE_RULE
    inc.w !V_147B
    lda.b #$02
    jsr.w PlayerMoveStart
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x
    stz.b !actor_speed_x+2
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_x+2
    cmp.b #$06
    bcc .walk
    lda.b !actor_x+1
    cmp.b #$50
    bcc .walk
    lda.b #$00
    jsr.w PlayerMoveStart
    inc.w !CUE_BE
.cue:
    brk.b #$00
    lda.w !CUE_BE
    bpl .cue
    lda.b #$02
    jsr.w PlayerMoveStart
    stz.b !actor_40
    lda.b #$03
    sta.w !V_0978
.walk_on:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !actor_40
    bne .walk_on
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 2*24, 4                  ; slot 2, parameter 4
    dl BrightnessDownTask
.fade:
    brk.b #$00
    jsl.l ActorMoveX
    lda.w !TASK2_STATE
    bne .fade
    stz.w !V_0360
    stz.w !V_035D
    inc.w !PART
    jsl.l TaskExec
    dl $81F61D

; @routine 81:E154 PlayerSceneCued
; @actor
; WHAT IT DOES: The player started with $07 = 12: Aladdin runs to x $80 in
; a scene, then goes through its cues: a pose, a run to the right, a jump
; with the hurt sound, and a last pose once he lands, for ever.
; HOW: PlayerSceneAnimStart 0; Y = 3, ActorSpeedXSet; V_141C, V_141B 0.
; Left of x $80 (its low byte): facing right, frames (BRK first) of
; PlayerSceneMoveStep until it is $80 or more; else facing left, until it
; is below. x low byte $80, flip 0, PlayerSceneAnimStart $13; frames (BRK
; first) until CUE_1B31; PlayerSceneAnimStart $1C, 8 frames (BRK first) of
; PlayerSceneAnimStep; V_147B up, COP $70; PlayerSceneAnimStart 1, facing
; left; Y = $6F, ActorSpeedXSet; frames (BRK first) of ActorMoveX and
; PlayerSceneAnimStep until CUE_1B32, then 6 more; SoundQueueAdd 2, V_03FB
; 8; PlayerSceneAnimStart $0E; facing left, flip 0, V_141C 1, LANDED 0; Y =
; 5, ActorMotionPreset; frames (BRK first) of PlayerSceneAnimStep and
; ActorFly until LANDED; PlayerSceneAnimStart $0F; each frame (BRK first)
; PlayerSceneAnimStep.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 12. Exit: never returns.
%at($81E154)
PlayerSceneCued:
    lda.b #$00
    jsr.w PlayerSceneAnimStart
    ldy.b #$03
    jsl.l ActorSpeedXSet
    stz.w !V_141C
    stz.w !V_141B
    lda.b !actor_x+1
    cmp.b #$80
    bcs .from_right
    stz.b !actor_flip
    stz.b !actor_facing
.run_right:
    brk.b #$00
    jsr.w PlayerSceneMoveStep
    lda.b !actor_x+1
    cmp.b #$80
    bcc .run_right
    bra .middle
.from_right:
    lda.b #$01
    sta.b !actor_flip
    sta.b !actor_facing
.run_left:
    brk.b #$00
    jsr.w PlayerSceneMoveStep
    lda.b !actor_x+1
    cmp.b #$80
    bcs .run_left
.middle:
    lda.b #$80
    sta.b !actor_x+1
    stz.b !actor_flip
    lda.b #$13
    jsr.w PlayerSceneAnimStart
.cue1:
    brk.b #$00
    lda.w !CUE_1B31
    beq .cue1
    lda.b #$1C
    jsr.w PlayerSceneAnimStart
    lda.b #$08
    sta.b !actor_40
.pose:
    brk.b #$00
    jsr.w PlayerSceneAnimStep
    dec.b !actor_40
    bne .pose
    inc.w !V_147B
    lda.b #$70
    cop.b #$00
    lda.b #$01
    jsr.w PlayerSceneAnimStart
    lda.b #$01
    sta.b !actor_flip
    sta.b !actor_facing
    ldy.b #$6F
    jsl.l ActorSpeedXSet
.run:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w PlayerSceneAnimStep
    lda.w !CUE_1B32
    beq .run
    lda.b #$06
    sta.b !actor_40
.run_on:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w PlayerSceneAnimStep
    dec.b !actor_40
    bne .run_on
    lda.b #$02
    jsl.l SoundQueueAdd
    lda.b #$08
    sta.w !V_03FB
    lda.b #$0E
    jsr.w PlayerSceneAnimStart
    lda.b #$01
    sta.b !actor_facing
    stz.b !actor_flip
    sta.w !V_141C
    stz.w !LANDED
    ldy.b #$05
    jsl.l ActorMotionPreset
.fly:
    brk.b #$00
    jsr.w PlayerSceneAnimStep
    jsl.l ActorFly
    lda.w !LANDED
    beq .fly
    lda.b #$0F
    jsr.w PlayerSceneAnimStart
.last:
    brk.b #$00
    jsr.w PlayerSceneAnimStep
    bra .last
