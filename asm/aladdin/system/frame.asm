; frame.asm: a task's steps from one frame to the next: fixed waits.
; C: src/aladdin/system/frame.c.

incsrc "../include/macros.inc"

arch 65816

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
