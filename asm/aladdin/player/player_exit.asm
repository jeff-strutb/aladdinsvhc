; player_exit.asm: Aladdin's code at a stage's end: the exit walk of an
; escort level (LevelExitCheck sets it) and the wait after a level's end
; scene; each fades the screen out, waits for the fade's task and starts
; the running task again at what follows the stage. Actor code (it yields
; by BRK, docs/aladdin/ACTORS.md) reached where his resume address was set.
; C: src/aladdin/player/player_exit.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!SCENE_ON = $03FA               ; counted up as a stage ends
!V_1454 = $1454                 ; counted up as a stage ends
!VIEW_EDGE_RULE = $145B         ; the rule the view's edges follow
!EXIT_VIEW_RULE = $04           ; the rule while he walks out
!TASK2_STATE = $009F            ; task slot 2's state: 0 when free
!EXIT_X_MAX = $2060             ; the escort walk's x pixels go no further
!FRAMES_40 = !actor_40          ; frames left of the walk's first part

; Tables in the data bank ($80)
!FIGURE_KIND = $27             ; kind $27, the figure that crosses the screen
!FIGURE_07 = $08
!STATE_START = $0C
!GENIE_SCREEN_TASK = $80A63B   ; the Genie screen's task (no C yet)
!STAGE_GOES_ON = $C6F2          ; by level: nonzero, the level's end follows; 0 the next part
!EXIT_SONG = $28                ; the song at an exit spot
!EXIT_POSE = $19                ; his pose there
!STAGE_BASE = $C710             ; by act: what the stage's digit adds to LEVEL_035C+1
!V_146B = $146B
!V_147B = $147B
!V_1448 = $1448
!V_036C = $036C                 ; nonzero: kind $27 figures were touched (on at PlayerSceneFigure)

; @routine 81:E730 PlayerEscortExit
; @actor
; WHAT IT DOES: Aladdin walks out of an escort level's exit (the scroll
; passed its spot): facing right at a walk's speed, 32 frames, then the
; screen fades out while he walks on (never past x $2060), and the
; running task goes on at the level's end or at the story card.
; HOW: ACTOR_FACING and ACTOR_FLIP 0; SecondPoolFreeAll; SCENE_ON and
; V_1454 up; ActorSpeedXSetAccel with A $28, Y $57 when LEVEL_035C is 1,
; else A $16, Y $45; VIEW_EDGE_RULE 4; PlayerMoveStart 0; FRAMES_40 $20
; frames (BRK first): ActorAccelerateX, ActorMoveX. SoundQueueF6With1;
; TaskCreate BrightnessDownTask in slot 2 with parameter 4. Each frame
; (BRK first): ActorAccelerateX, ActorMoveX; his x pixels $2060 at most
; (16-bit); until TASK2_STATE is 0. STAGE_GOES_ON by LEVEL (X 8-bit):
; nonzero, TaskExec LevelEndTask; else TaskExec StoryCardTask.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where LevelExitCheck set his code (escort levels,
; the table at $80:B96B). Exit: never returns.
%at($81E730)
PlayerEscortExit:
    stz.b !actor_facing
    stz.b !actor_flip
    jsl.l SecondPoolFreeAll
    inc.w !SCENE_ON
    inc.w !V_1454
    lda.w !level_035c
    dec
    bne .slow
    lda.b #$28
    ldy.b #$57
    jsl.l ActorSpeedXSetAccel
    bra .walk
.slow:
    lda.b #$16
    ldy.b #$45
    jsl.l ActorSpeedXSetAccel
.walk:
    lda.b #!EXIT_VIEW_RULE
    sta.w !VIEW_EDGE_RULE
    lda.b #$00
    jsr.w PlayerMoveStart
    lda.b #$20
    sta.b !FRAMES_40
.walk_frame:
    brk.b #$00
    jsl.l ActorAccelerateX
    jsl.l ActorMoveX
    dec.b !FRAMES_40
    bne .walk_frame
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 2*24, 4                  ; slot 2, parameter 4
    dl BrightnessDownTask
.fade_frame:
    brk.b #$00
    jsl.l ActorAccelerateX
    jsl.l ActorMoveX
    rep.b #$20
    lda.w #!EXIT_X_MAX
    cmp.b !actor_x+1
    bcs .fade_test
    sta.b !actor_x+1
.fade_test:
    sep.b #$20
    lda.w !TASK2_STATE
    bne .fade_frame
    ldx.w !level
    lda.w !STAGE_GOES_ON,x
    beq .story
    jsl.l TaskExec
    dl LevelEndTask
.story:
    jsl.l TaskExec
    dl StoryCardTask

; @routine 81:E7B1 PlayerStageEnd
; @actor
; WHAT IT DOES: Aladdin at a level's end once its end scene is over: the
; screen fades out, and the running task goes on at the level's end (or
; at the next part, for a level the table marks so).
; HOW: ACTOR_FACING and ACTOR_FLIP 0; SecondPoolFreeAll; SCENE_ON and
; V_1454 up; SoundQueueF6With1; TaskCreate BrightnessDownTask in slot 2
; with parameter 4. Each frame (BRK first) until TASK2_STATE is 0.
; STAGE_GOES_ON by LEVEL (X 8-bit): nonzero, TaskExec LevelEndTask; else
; (no script's level) TaskExec NextPartTask.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where the end scene's task (80:A4C1) set his code.
; Exit: never returns.
%at($81E7B1)
PlayerStageEnd:
    stz.b !actor_facing
    stz.b !actor_flip
    jsl.l SecondPoolFreeAll
    inc.w !SCENE_ON
    inc.w !V_1454
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 2*24, 4                  ; slot 2, parameter 4
    dl BrightnessDownTask
.frame:
    brk.b #$00
    lda.w !TASK2_STATE
    bne .frame
    ldx.w !level
    lda.w !STAGE_GOES_ON,x
    beq .next_part
    jsl.l TaskExec
    dl LevelEndTask
.next_part:
    jsl.l TaskExec
    dl NextPartTask

; @routine 81:CBF9 PlayerSpotExit
; @actor
; WHAT IT DOES: Aladdin at a level's exit spot: the stage's end song, he
; is put on the ground in pose $19 for 128 frames; then the running task
; goes on at the Genie screen's (kind $27 figures touched) or the screen
; fades out (PlayerSpotExitFade).
; HOW: SecondPoolFreeAll; BRK; SongPlay $28; V_146B $80; SCENE_ON, V_1454
; and V_147B up; LEVEL_035C compared with 3 (its BEQ goes to the next
; instruction either way); V_1448 0; ActorSnapToGround; PlayerMoveStart
; $19; COP $80. V_036C 0: on at PlayerSpotExitFade; else on into
; PlayerSceneFigure (81:CC31).
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
; the actor loop's RTI where LevelExitCheck set his code (EXIT_WALK, the
; levels not marked at $80:B96B). Exit: never returns.
%at($81CBF9)
PlayerSpotExit:
    jsl.l SecondPoolFreeAll
    brk.b #$00
    lda.b #!EXIT_SONG
    jsl.l SongPlay
    lda.b #$80
    sta.w !V_146B
    inc.w !SCENE_ON
    inc.w !V_1454
    inc.w !V_147B
    lda.w !level_035c
    cmp.b #$03
    beq .act3
.act3:
    lda.b #$00
    sta.w !V_1448
    jsl.l ActorSnapToGround
    lda.b #!EXIT_POSE
    jsr.w PlayerMoveStart
    lda.b #$80
    cop.b #$00
    lda.w !V_036C
    beq PlayerSpotExitFade

; @routine 81:CC31 PlayerSceneFigure
; @actor
; WHAT IT DOES: The end of a level scene when one of kind $27's figures was
; touched ($036C): Aladdin starts a kind $27 figure ($07 8, the crosser),
; waits 224 frames, and the running task goes on at the Genie screen's
; task (80:A63B).
; HOW: ActorAllocBLong; the record (X 16-bit; no test of N) gets state
; $0C, kind $27, $07 8; X 8-bit; COP $E0; TaskExec GENIE_SCREEN_TASK.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
; PlayerSceneLand's jump (81:E024) with $036C nonzero. Exit: never
; returns.
%at($81CC31)
PlayerSceneFigure:
    jsl.l ActorAllocBLong
    lda.b #!STATE_START
    sta.w !actor_state,x
    lda.b #!FIGURE_KIND
    sta.w !actor_06,x
    lda.b #!FIGURE_07
    sta.w !actor_07,x
    sep.b #$10
    lda.b #$E0
    cop.b #$00
    jsl.l TaskExec
    dl !GENIE_SCREEN_TASK

; @routine 81:CC51 PlayerSpotExitFade
; @actor
; WHAT IT DOES: The exit spot's fade: the screen fades out, and the running
; task goes on at the level's end, the story card or the next part.
; HOW: TaskCreate BrightnessDownTask in slot 2 with parameter 4. Each frame
; (BRK first) until TASK2_STATE is 0. STAGE_GOES_ON by LEVEL (X 8-bit):
; positive, TaskExec LevelEndTask; negative (levels $3A on, which the game
; has not), TaskExec StoryCardTask; 0, TaskExec NextPartTask.
; Entry: as PlayerSpotExit; reached by its branch (81:CC2F) with V_036C 0.
; Exit: never returns.
%at($81CC51)
PlayerSpotExitFade:
    jsl.l TaskCreate
    db 2*24, 4                  ; slot 2, parameter 4
    dl BrightnessDownTask
.frame:
    brk.b #$00
    lda.w !TASK2_STATE
    bne .frame
    ldx.w !level
    lda.w !STAGE_GOES_ON,x
    beq .next_part
    bmi .story
    jsl.l TaskExec
    dl LevelEndTask
.story:
    jsl.l TaskExec
    dl StoryCardTask
.next_part:
    jsl.l TaskExec
    dl NextPartTask

; @routine 81:CC80 StageNumberText
; WHAT IT DOES: Puts the act and stage numbers ("act-stage") in the text
; layer with text layout 8, for the NMI to upload. Nothing in the game
; calls it: no JSR, JMP, JSL or JML names 81:CC80 and no word in bank $81
; does; a unit call at its entry runs it.
; HOW: TextLayoutShow 8; X = LEVEL_035C (the act); the act's digit (the
; act plus 1, unless it is 6) at TEXT_TILEMAP+$2DC, tile $1E at +$2DE, the
; stage's digit (LEVEL_035C+1 plus STAGE_BASE by the act) at +$2E0, each
; with attribute $20; TEXT_TILEMAP_UPLOAD up.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
; A = $20; X = the act.
%at($81CC80)
StageNumberText:
    lda.b #$08
    jsl.l TextLayoutShow
    lda.w !level_035c
    tax
    cmp.b #$06
    beq .act
    inc a
.act:
    sta.l !text_tilemap+732
    lda.b #$1E
    sta.l !text_tilemap+734
    clc
    lda.w !level_035c+1
    adc.w !STAGE_BASE,x
    sta.l !text_tilemap+736
    lda.b #$20
    sta.l !text_tilemap+733
    sta.l !text_tilemap+735
    sta.l !text_tilemap+737
    inc.w !text_tilemap_upload
    rts
