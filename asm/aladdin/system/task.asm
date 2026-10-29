; task.asm: the game's cooperative tasks.
;
; The game runs its screens and modes as up to 8 tasks, each with a slot
; record and a stack of its own. A task runs until it waits for a number of
; frames, exits, or starts again elsewhere; then the scheduler runs the next
; task that is ready, and the NMI counts the waits down
; (docs/aladdin/TASKS.md).
; C: src/aladdin/system/task.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

!TASK_FREE = 0
!TASK_WAITING = 1

; @routine 81:BD20 TaskExit
; WHAT IT DOES: Ends the running task for good and hands over to the
; scheduler.
; HOW: Frees the running task's slot, then moves onto the scheduler's own
; stack and into its loop. Its stack is left as it was.
; Entry: any widths; reached by a jump (JMP or JML) where a task ends.
; Exit: never returns; at the scheduler
; with M and X 8-bit, Y = the slot, A = $0326, S = $0326, N and Z from
; $26 (clear); D, DB and the other flags unchanged.
%at($81BD20)
TaskExit:
    sep.b #$30
    ldy.w !task_current
    jsr.w TaskSlotFree
; Onto the scheduler's stack and into its loop (TaskWait and the task
; exec call end here too).
TaskToScheduler:
    lda.b #!task_scheduler_stack>>8
    xba
    lda.b #!task_scheduler_stack&255
    tcs
    jmp.w !task_scheduler_next

; @routine 81:BD31 TaskWait
; WHAT IT DOES: Makes the running task wait A frames (its next turn comes
; once the NMI has counted them down), letting the other tasks run.
; HOW: Pushes B, D, X, Y and P on the task's stack, puts the count and the
; waiting state in its slot and saves S there, then moves onto the
; scheduler's own stack and into its loop. The scheduler resumes the task
; by pulling them back and returning (RTL) to the caller.
; Entry: A low byte = frames to wait; X and Y at either width (pushed at
; that width); called by JSL. Exit: when the task resumes, every register
; as it was at entry (the scheduler pulls them), the frames later.
TaskWait:
    phb
    phd
    phx
    phy
    php
    sep.b #$30
    ldy.w !task_current
    sta.w !task_wait,y
    lda.b #!TASK_WAITING
    sta.w !task_state,y
    tsc
    rep.b #$20
    sta.w !task_saved_s,y
    sep.b #$20
    bra TaskToScheduler

; @routine 81:BD4D TaskSlotFree
; WHAT IT DOES: Frees a task slot: the scheduler passes over it until a
; task is created there again.
; HOW: Its state byte becomes 0.
; Entry: M 8-bit; Y = the slot (its byte offset). Exit: A low byte = 0;
; N clear, Z set; the rest unchanged.
TaskSlotFree:
    lda.b #!TASK_FREE
    sta.w !task_state,y
    rts
