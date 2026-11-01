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
!TASK_NEW = $0C

; The return address on the stack, as a long pointer on the direct page
; (D = S): above the pushed D in TaskCreate, right at S in TaskExec. The
; arguments follow its byte, from offset 1.
!create_args = 3
!exec_args = 1
; Where a new task's stack top holds its entry (less one) and bank.
!TASK_TOP_ENTRY = 1
!TASK_TOP_BANK = 3
!CREATE_ARG_BYTES = 5

; @routine 81:BCB8 TaskCreate
; WHAT IT DOES: Starts a new task in a slot: it runs from its entry at its
; first turn, with the parameter byte its creator gives it.
; HOW: Reads the five bytes after the JSL (slot, parameter, entry word,
; bank) through the return address on the stack: marks the slot new, sets
; its parameter, empties its stack and puts the entry (less one, for the
; scheduler's RTL) and its bank at the stack's top. Then steps the return
; address over the five bytes and returns after them.
; Entry: any widths; called by JSL with the five bytes after it; DB a bank
; whose $0000-$1FFF is work RAM. Exit: past the five bytes; M and X 8-bit;
; X = the low byte of the new task's stack top; Y = 5; A low byte = the
; return bank (its high byte from the stepped address); C, V from the
; bank's ADC; D as at entry, N and Z from it (PLD).
%at($81BCB8)
TaskCreate:
    phd
    sep.b #$30
    tsc
    tcd
    ldy.b #1
    lda.b [!create_args],y
    iny
    tax
    lda.b #!TASK_NEW
    sta.w !task_state,x
    lda.b [!create_args],y
    sta.w !task_param,x
    iny
    rep.b #$30
    lda.w !task_stack_top,x
    sta.w !task_saved_s,x
    tax
    lda.b [!create_args],y
    dec a
    sta.w !TASK_TOP_ENTRY,x
    iny
    iny
    lda.b [!create_args],y
    sta.w !TASK_TOP_BANK,x
    clc
    lda.b !create_args,s
    adc.w #!CREATE_ARG_BYTES
    sta.b !create_args,s
    sep.b #$30
    lda.b !create_args+2,s
    adc.b #0
    sta.b !create_args+2,s
    pld
    rtl

; @routine 81:BCF6 TaskExec
; WHAT IT DOES: Starts the running task again at another entry, its stack
; emptied, from its next turn.
; HOW: Marks the slot new and empties its stack, reads the three bytes
; after the JSL (entry word, bank) through the return address on the
; stack and puts the entry (less one) and its bank at the stack's top;
; then moves onto the scheduler's stack and into its loop.
; Entry: any widths; called by JSL with the three bytes after it; DB a
; bank whose $0000-$1FFF is work RAM. Exit: never returns; at the
; scheduler with M and X 8-bit, D = the S at entry, Y = 3, X = the low byte
; of the stack top, A = $0326, S = $0326, N and Z from $26 (clear); C, V,
; DB unchanged.
TaskExec:
    sep.b #$30
    tsc
    tcd
    ldy.b #1
    ldx.w !task_current
    lda.b #!TASK_NEW
    sta.w !task_state,x
    rep.b #$30
    lda.w !task_stack_top,x
    sta.w !task_saved_s,x
    tax
    lda.b [!exec_args],y
    dec a
    pha
    iny
    iny
    lda.b [!exec_args],y
    sta.w !TASK_TOP_BANK,x
    pla
    sta.w !TASK_TOP_ENTRY,x
    sep.b #$30
    bra TaskToScheduler


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
