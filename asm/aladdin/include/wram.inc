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
