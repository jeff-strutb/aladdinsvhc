; brightness_wait.asm: the screen brightened or darkened by a task of its
; own in slot 1, the caller's task waiting until it has ended. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/video/brightness_wait.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!FADE_SLOT_STATE = !task_state+24 ; slot 1's state: 0 once its task has ended

; @routine 81:F311 BrightnessUpAndWait
; WHAT IT DOES: Brings the screen up to full brightness, a step every 2
; frames, and waits until it is done.
; HOW: TaskCreate (slot 1, parameter 4, BrightnessUpTask); then TaskWait
; 1 until slot 1's state is 0, at BrightnessDownAndWait's loop.
; Entry: M 8-bit; called by JSR from a task (the task at
; $81:F10D); DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit; A
; low = 0, Z set; the rest as the last wait restores them.
%at($81F311)
BrightnessUpAndWait:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessUpTask
    bra BrightnessDownAndWait_wait

; @routine 81:F31C BrightnessDownAndWait
; WHAT IT DOES: Takes the screen down to dark, a step every 2 frames, and
; waits until it is done.
; HOW: TaskCreate (slot 1, parameter 4, BrightnessDownTask); then TaskWait
; 1 until slot 1's state is 0.
; Entry: M 8-bit; called by JSR from a task (the task at
; $81:F10D); DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit; A
; low = 0, Z set; the rest as the last wait restores them.
BrightnessDownAndWait:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessDownTask
.wait:
    lda.b #$01
    jsl.l TaskWait
    lda.w !FADE_SLOT_STATE
    bne .wait
    rts
