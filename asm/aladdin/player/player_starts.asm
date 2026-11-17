; player_starts.asm: Aladdin's code as his record starts: by its $06 the
; carpet, else by its $07 the level's own start (a walk in, then the
; level's tasks) or one of the scenes that place him (dropped in, a new
; game, the story, walking on in the ending's scenes).
; C: src/aladdin/player/player_starts.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_player.inc"

arch 65816

!CAMERA_AIM_X = $03F1           ; added to the scroll x for the point the camera keeps him from (word)
!PLAYER_FLAGS = $0347           ; bit 7 cleared as he starts
!V_0360 = $0360                 ; nonzero: no walk in
!APPLES = $0369                 ; apples left, two decimal digits
!LIVES = $0364                  ; lives left
!V_036B = $036B
!HOVER_ON = $036E               ; nonzero: the fourth action makes him hover
!VIEW_EDGE_RULE = $145B         ; the rule the view's edges follow
!V_147B = $147B
!STORY_78 = $0078               ; the story's figures: nonzero once one is in place
!STORY_79 = $0079               ; nonzero at the end of the story's third part
!STORY_7A = $007A               ; set to $80: the story's parts go on
!CUE_CF = $00CF                 ; nonzero while the story's text is busy
!CUE_BE = $00BE                 ; the scene's cues, counted up in turn
!CUE_BF = $00BF
!CUE_C3 = $00C3
!CUE_C4 = $00C4
!TASK_PARAM = $0075             ; the first task slot's parameter
!FIGURE_29 = $29                ; word: his picture list
!WALK_FRAMES = !actor_stream    ; frames of the walk in
!STORY_SCENE_TASK = $81F344     ; the task the story's scenes start in slot 4

; Starts by $07 not yet filed (no script runs them)
!START_DDC1 = $DDC1
!START_DDE0 = $DDE0
!START_DF6F = $DF6F
!START_E0AE = $E0AE
!START_E154 = $E154

; @routine 81:CB3B PlayerStart
; @actor
; WHAT IT DOES: Aladdin's record starts: the camera's aim and his first
; test box; then by $06 the carpet, or by $07 the level's own start (he
; walks in, 32 frames or $68 on level 6, unless V_0360 is set), the
; carpet, or one of the scenes that place him.
; HOW: CAMERA_AIM_X $0080; PLAYER_BOX_WANTED 0, PlayerBoxSet. $06 by
; START_BY_06: 0 goes on by $07 (START_BY_07), 1-3 PlayerCarpetStart. $07
; 0: ACTOR_09 $40D0, FIGURE_29 $8907, ACTOR_REDRAW $FF, PLAYER_FLAGS bit
; 7 cleared; V_0360 set: PlayerStartTasks. Level $14 or more: (level $1A,
; which no script runs: ActorSpawnShared $60) ActorSpeedXClear,
; PlayerMoveStart 0, PlayerStand. Else PlayerMoveStart 2, speed entry 3
; ($6C on level 6) by ActorSpeedXSet, each frame (BRK first) ActorMoveX
; and ActorSnapToGround for WALK_FRAMES, then PlayerStartTasks. $07 1:
; PlayerCarpetStart; 5 PlayerStartDropped; 6 PlayerStartNewGame; 7
; PlayerStory; 9 PlayerSceneLand; 10 PlayerSceneWalkOn; 11
; PlayerSceneWalkRight; 14 PlayerEndingRide; 15 PlayerSceneStand; 2-4, 8,
; 12 and 13, which no script runs, to code not yet filed.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's jump for the player in state $0C (through the stub at
; $83:91F4). Exit: never returns.
%at($81CB3B)
PlayerStart:
    lda.b #$80
    sta.w !CAMERA_AIM_X
    stz.w !CAMERA_AIM_X+1
    ldx.b #$00
    stx.w !player_box_wanted
    jsr.w PlayerBoxSet
    lda.b !actor_06
    asl a
    tax
    jmp.w (.by_06,x)
.by_07:
    lda.b !actor_07
    asl a
    tax
    jmp.w (.by_07_table,x)
.by_06:
    dw .by_07, PlayerCarpetStart, PlayerCarpetStart, PlayerCarpetStart
.by_07_table:
    dw .level, PlayerCarpetStart, !START_DDC1, !START_DDC1
    dw !START_DDE0, PlayerStartDropped, PlayerStartNewGame, PlayerStory
    dw !START_DF6F, PlayerSceneLand, PlayerSceneWalkOn, PlayerSceneWalkRight
    dw !START_E154, !START_E0AE, PlayerEndingRide, PlayerSceneStand
.level:
    rep.b #$20
    lda.w #$40D0
    sta.b !actor_09
    lda.w #$8907
    sta.b !FIGURE_29
    sep.b #$20
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    lda.w !PLAYER_FLAGS
    and.b #$7F
    sta.w !PLAYER_FLAGS
    lda.w !V_0360
    bne PlayerStartTasks
    lda.w !level
    cmp.b #$14
    bcs PlayerStartTasks_late
    lda.b #$02
    jsr.w PlayerMoveStart
    lda.b #$20
    sta.b !WALK_FRAMES
    ldy.b #$03
    lda.w !level
    cmp.b #$06
    bne .speed
    lda.b #$68
    sta.b !WALK_FRAMES
    ldy.b #$6C
.speed:
    jsl.l ActorSpeedXSet
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !WALK_FRAMES
    bne .walk

; @routine 81:CBD1 PlayerStartTasks
; @actor
; WHAT IT DOES: The end of Aladdin's start: below level $14 the text
; blink's task starts; he stands still and on into standing.
; HOW: LEVEL under $14: TaskCreate (slot 2, parameter 0,
; TextBlinkTask). ActorSpeedXClear, PlayerMoveStart 0, PlayerStand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached
; from PlayerStart's walk in and by a jump from PlayerStartNewGame. Exit:
; never returns.
%at($81CBD1)
PlayerStartTasks:
    lda.w !level
    cmp.b #$14
    bcs .stand
    jsl.l TaskCreate
    db 2*24, 0                  ; slot 2, parameter 0
    dl TextBlinkTask
.stand:
    jsl.l ActorSpeedXClear
    lda.b #$00
    jsr.w PlayerMoveStart
    jmp.w PlayerStand
.late:                          ; PlayerStart's level $14 or more
    cmp.b #$1A
    bne .stand
    lda.b #$60                  ; no script runs this
    jsl.l ActorSpawnShared
    bra .stand

; @routine 81:DDFF PlayerStartDropped
; @actor
; WHAT IT DOES: Aladdin is dropped in facing right: he falls with his
; jump's poses until he is in the ground, lands with its sound and pose
; ($19) for 128 frames, and stands.
; HOW: Facing and flip 0; Y = $4D, ActorMotionPreset; ACTOR_40 and
; ACTOR_41 (the pose step) 0. Each frame (BRK first): PlayerJumpPoseStep,
; ActorFly; rising, or ActorOutOfGround Z set: again. SoundQueueAdd $10,
; PlayerMoveStart $19, COP $80, PlayerStand.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 5. Exit: never returns.
%at($81DDFF)
PlayerStartDropped:
    stz.b !actor_facing
    stz.b !actor_flip
    ldy.b #$4D
    jsl.l ActorMotionPreset
    stz.b !actor_40
    stz.b !actor_41
.frame:
    brk.b #$00
    jsr.w PlayerJumpPoseStep
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .frame
    jsl.l ActorOutOfGround
    beq .frame
    lda.b #$10
    jsl.l SoundQueueAdd
    lda.b #$19
    jsr.w PlayerMoveStart
    lda.b #$80
    cop.b #$00
    jmp.w PlayerStand

; @routine 81:DE32 PlayerStartNewGame
; @actor
; WHAT IT DOES: Aladdin placed for a new game at (128, 176) facing
; right: 3 lives, $10 apples, full health; then his start's end.
; HOW: (16-bit) FIGURE_29 $8907, x 128, y 176; VIEW_EDGE_RULE 1; facing
; and flip 0; PlayerMoveStart 0; ACTOR_09 $D0, ACTOR_REDRAW $FF, ACTOR_06
; and TASK_PARAM 0; APPLES $10, LIVES 3, HEALTH from HEALTH_MARKS,
; V_036B and HOVER_ON 0; on at PlayerStartTasks.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 6. Exit: never returns.
%at($81DE32)
PlayerStartNewGame:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    lda.w #$0080
    sta.b !actor_x+1
    lda.w #$00B0
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$01
    sta.w !VIEW_EDGE_RULE
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsr.w PlayerMoveStart
    lda.b #$D0
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    stz.b !actor_06
    stz.w !TASK_PARAM
    lda.b #$10
    sta.w !APPLES
    lda.b #$03
    sta.w !LIVES
    lda.w !health_marks
    sta.w !health
    stz.w !V_036B
    stz.w !HOVER_ON
    jmp.w PlayerStartTasks

; @routine 81:DE79 PlayerStory
; @actor
; WHAT IT DOES: Aladdin in the story's scene: he turns this way and that
; as its figures come and its pages of text go, starts the story's next
; scenes, and at its end is shown no more.
; HOW: FIGURE_29 $8907; PLAYER_BOX_WANTED and V_142D 0; facing and flip
; 1; PlayerMoveStart 0; ACTOR_09 $D0, ACTOR_REDRAW $FF. Frames (BRK)
; until STORY_78; a turn (move 5, COP 8, move 0), flip 0; frames until
; CUE_CF is 0; COP $20, a turn, flip up, COP $20, TaskCreate (slot 4,
; parameter 9, STORY_SCENE_TASK); frames until CUE_CF is 0; COP $20, a
; turn, flip 0, TaskCreate (slot 4, parameter $0B); frames until CUE_CF
; is 0; COP $10, STORY_7A $80; frames until STORY_79; a turn, flip up;
; frames while TASK_PARAM is 4; ACTOR_09 bits 6 and 7 cleared (TRB), x's
; high byte 1; then BRK for good.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 7. Exit: never returns.
%at($81DE79)
PlayerStory:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    sep.b #$20
    stz.w !player_box_wanted
    stz.w !v_142d
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$00
    jsr.w PlayerMoveStart
    lda.b #$D0
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
.figure:
    brk.b #$00
    lda.w !STORY_78
    beq .figure
    lda.b #$05
    jsr.w PlayerMoveStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsr.w PlayerMoveStart
    stz.b !actor_flip
.text1:
    brk.b #$00
    lda.w !CUE_CF
    bne .text1
    lda.b #$20
    cop.b #$00
    lda.b #$05
    jsr.w PlayerMoveStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsr.w PlayerMoveStart
    inc.b !actor_flip
    lda.b #$20
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 9                  ; slot 4, parameter 9
    dl !STORY_SCENE_TASK
.text2:
    brk.b #$00
    lda.w !CUE_CF
    bne .text2
    lda.b #$20
    cop.b #$00
    lda.b #$05
    jsr.w PlayerMoveStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsr.w PlayerMoveStart
    stz.b !actor_flip
    jsl.l TaskCreate
    db 4*24, 11                 ; slot 4, parameter $0B
    dl !STORY_SCENE_TASK
.text3:
    brk.b #$00
    lda.w !CUE_CF
    bne .text3
    lda.b #$10
    cop.b #$00
    lda.b #$80
    sta.w !STORY_7A
.part3:
    brk.b #$00
    lda.w !STORY_79
    beq .part3
    lda.b #$05
    jsr.w PlayerMoveStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsr.w PlayerMoveStart
    inc.b !actor_flip
.end:
    brk.b #$00
    lda.w !TASK_PARAM
    cmp.b #$04
    beq .end
    lda.b #$C0
    trb.b !actor_09
    lda.b #$01
    sta.b !actor_x+2
.stay:
    brk.b #$00
    bra .stay

; @routine 81:E042 PlayerSceneWalkOn
; @actor
; WHAT IT DOES: Aladdin in a scene of level 5's end: he drops to the
; ground if he is above it, faces right and cues the scene, walks right
; to x $70, stops, and cues it on twice with a pose ($1B) between; then
; he stays.
; HOW: ActorSnapToGround Z set: PlayerMoveStart $0E, vertical speed
; $000300, each frame (BRK first) ActorMoveY until ActorOutOfGround Z
; clear. Facing and flip 0, PlayerMoveStart 0; CUE_C3 and V_147B up; COP
; $40; CUE_C4 up; PlayerMoveStart 2, speed $000100; each frame (BRK
; first) ActorMoveX until x pixels' low byte is $70 or more;
; PlayerMoveStart 0, COP $20, CUE_BE up, PlayerMoveStart $1B, COP $A0,
; CUE_BF up; then COP $80 for good.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; PlayerStart's jump for $07 = 10. Exit: never returns.
%at($81E042)
PlayerSceneWalkOn:
    jsl.l ActorSnapToGround
    bne .on_ground
    lda.b #$0E
    jsr.w PlayerMoveStart
    lda.b #$03
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y
    stz.b !actor_speed_y+2
.drop:
    brk.b #$00
    jsl.l ActorMoveY
    jsl.l ActorOutOfGround
    beq .drop
.on_ground:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsr.w PlayerMoveStart
    inc.w !CUE_C3
    inc.w !V_147B
    lda.b #$40
    cop.b #$00
    inc.w !CUE_C4
    lda.b #$02
    jsr.w PlayerMoveStart
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x
    stz.b !actor_speed_x+2
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_x+1
    cmp.b #$70
    bcc .walk
    lda.b #$00
    jsr.w PlayerMoveStart
    lda.b #$20
    cop.b #$00
    inc.w !CUE_BE
    lda.b #$1B
    jsr.w PlayerMoveStart
    lda.b #$A0
    cop.b #$00
    inc.w !CUE_BF
.stay:
    lda.b #$80
    cop.b #$00
    bra .stay

; @routine 81:E126 PlayerSceneWalkRight
; @actor
; WHAT IT DOES: Aladdin walks right in a scene until x $0248, holds the
; landing's pose ($19) for 128 frames, and stands.
; HOW: M and X 8-bit; facing and flip 0; PlayerMoveStart 2; Y = 3,
; ActorSpeedXSet; each frame (BRK first) ActorMoveX while x pixels are
; $0248 or less (16-bit compare); PlayerMoveStart $19, COP $80,
; PlayerStand.
; Entry: D = the player's record; DB = $80; reached by PlayerStart's
; jump for $07 = 11. Exit: never returns.
%at($81E126)
PlayerSceneWalkRight:
    sep.b #$30
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$02
    jsr.w PlayerMoveStart
    ldy.b #$03
    jsl.l ActorSpeedXSet
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    rep.b #$20
    lda.w #$0248
    cmp.b !actor_x+1
    sep.b #$20
    bcs .walk
    lda.b #$19
    jsr.w PlayerMoveStart
    lda.b #$80
    cop.b #$00
    jmp.w PlayerStand
