; wram.inc: work RAM, as the S-CPU code names it (the same names as the C's
; wram.h, in lower case).

includeonce
; A variable whose purpose is not yet established keeps its address as its
; name until it is.

; Sound commands to the SPC700 (audio/sound_command.asm).
!sound_seq = $0002              ; sequence number of the last command sent; the driver echoes it on APU port 2
!sound_hold = $0371             ; nonzero: no command is sent this frame
!sound_sending = $0372          ; nonzero while a command is being sent
!sound_queue_read = $03BB       ; queue index of the next command to send (0-31)
!sound_queue_write = $03BD      ; queue index where the next command is added (0-31)
!sound_queue = $03BF            ; 32-entry ring of command bytes

; Tasks (system/task.asm): 8 slots of $18 bytes from $006F, indexed by a
; slot's byte offset (0, $18, ... $A8).
!task_state = $006F             ; 0 free, 1 waiting, 4 ready, 8 running, $0C new
!task_wait = $0070              ; frames left to wait
!task_saved_s = $0071           ; S where it stopped (word)
!task_stack_top = $0073         ; top of its stack (word)
!task_param = $0075             ; the byte its creator gave it
!task_current = $0327           ; the running task's slot
!task_slot_count = $0328        ; how many slots the scheduler scans
!task_scheduler_stack = $0326   ; the scheduler's own stack

; Joypads (system/joypad.asm): pad 1's word, then pad 2's.
!joy_held = $0377               ; buttons held
!joy_pressed = $037B            ; buttons newly pressed this frame

; Shadows of PPU and CPU registers, written to them in the vertical blank.
!nmitimen_shadow = $03B7        ; NMITIMEN: interrupts and auto joypad read
!inidisp_shadow = $03B8         ; INIDISP: forced blank and brightness
