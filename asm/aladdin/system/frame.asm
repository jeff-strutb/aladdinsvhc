; frame.asm: a task's steps from one frame to the next: the play screen's
; frame step, and fixed waits.
; C: src/aladdin/system/frame.c.

incsrc "../include/macros.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 80:AE9A FrameStep
; WHAT IT DOES: Runs one frame's update of the play screen, then waits for
; the next frame, letting the other tasks run.
; HOW: The frame update, then a wait of one frame (TaskWait).
; Entry: as the frame update; M 8-bit at its return; called by JSR from a
; task. Exit: when the task resumes, the registers as the wait restores
; them (as they were after the frame update, A low byte = 1).
%at($80AE9A)
FrameStep:
    jsl.l !frame_update
    lda.b #1
    jsl.l TaskWait
    rts

; @routine 80:AEB9 Wait64Frames
; WHAT IT DOES: Makes the running task wait 64 frames (a little over a
; second), letting the other tasks run.
; HOW: TaskWait with A = 64.
; Entry: M 8-bit; called by JSR from a task. Exit: when the task resumes,
; the registers as the wait restores them (A low byte = 64).
%at($80AEB9)
Wait64Frames:
    lda.b #64
    jsl.l TaskWait
    rts
