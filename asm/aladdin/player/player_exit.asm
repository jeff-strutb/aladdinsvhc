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
