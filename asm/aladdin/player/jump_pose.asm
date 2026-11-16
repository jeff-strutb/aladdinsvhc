; jump_pose.asm: the poses of Aladdin's jump, stepped once a frame while
; he is in the air: up, at the top, falling, and a roll that starts over.
; C: src/aladdin/player/jump_pose.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!POSE_STEP = !actor_40          ; the step the jump's pose is at (0-8)
!POSE_TIMER = !actor_41         ; frames left in a timed step
!TOP_Y = !actor_42              ; y (pixels, word) where he was as the rise ended

; @routine 81:D1F5 PlayerJumpPoseStep
; WHAT IT DOES: Steps the pose of Aladdin's jump by POSE_STEP: step 0
; starts move 4 for 14 frames; step 1 then starts move $0D and keeps the
; height reached; step 2 starts move $0E once he is lower than that; step
; 3 holds. Step 6 starts move $11 and goes back to step 1 for $12 frames;
; step 7 starts move $0D after them; step 8 starts move $0E once he falls
; and goes on at step 3.
; HOW: Through .steps by POSE_STEP. 0: PlayerMoveStart 4, step on,
; POSE_TIMER $0E. 1: POSE_TIMER down; at 0 PlayerMoveStart $0D, y pixels
; to TOP_Y, step on. 2: (16-bit A) TOP_Y not negative and not below y:
; nothing; else PlayerMoveStart $0E, step on. 3: nothing. 4 and 5, which
; no script runs: SoundQueueAdd 2, PlayerMoveStart 9, POSE_TIMER $10, step
; on. 6: PlayerMoveStart $11, POSE_STEP 1, POSE_TIMER $12. 7: POSE_TIMER
; down; at 0 PlayerMoveStart $0D, step on. 8: rising (the vertical speed
; negative): nothing; else PlayerMoveStart $0E, POSE_STEP 3.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code (81:D06E). Exit: M and X 8-bit; A, X and the
; flags as the step's last instruction left them (X = POSE_STEP * 2
; where the step calls nothing).
%at($81D1F5)
PlayerJumpPoseStep:
    lda.b !POSE_STEP
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .step0, .step1, .step2, .step3, .step4, .step4, .step6, .step7, .step8
.step0:
    lda.b #$04
    jsr.w PlayerMoveStart
    inc.b !POSE_STEP
    lda.b #$0E
    sta.b !POSE_TIMER
    rts
.step1:
    dec.b !POSE_TIMER
    bne .step1_back
    lda.b #$0D
    jsr.w PlayerMoveStart
    lda.b !actor_y+1
    sta.b !TOP_Y
    lda.b !actor_y+2
    sta.b !TOP_Y+1
    inc.b !POSE_STEP
.step1_back:
    rts
.step2:
    rep.b #$20
    lda.b !TOP_Y
    bmi .step2_lower
    cmp.b !actor_y+1
    bcs .step2_back
.step2_lower:
    sep.b #$20
    lda.b #$0E
    jsr.w PlayerMoveStart
    inc.b !POSE_STEP
.step2_back:
    sep.b #$20
    rts
.step3:
    rts
.step4:
    lda.b #$02
    jsl.l SoundQueueAdd
    lda.b #$09
    jsr.w PlayerMoveStart
    lda.b #$10
    sta.b !POSE_TIMER
    inc.b !POSE_STEP
    rts
.step6:
    lda.b #$11
    jsr.w PlayerMoveStart
    lda.b #$01
    sta.b !POSE_STEP
    lda.b #$12
    sta.b !POSE_TIMER
    rts
.step7:
    dec.b !POSE_TIMER
    bne .step7_back
    lda.b #$0D
    jsr.w PlayerMoveStart
    inc.b !POSE_STEP
.step7_back:
    rts
.step8:
    lda.b !actor_speed_y+2
    bmi .step8_back
    lda.b #$0E
    jsr.w PlayerMoveStart
    lda.b #$03
    sta.b !POSE_STEP
.step8_back:
    rts
