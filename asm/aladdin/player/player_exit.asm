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
!STAGE_GOES_ON = $C6F2          ; by level: nonzero, the level's end follows; 0 the next part

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
