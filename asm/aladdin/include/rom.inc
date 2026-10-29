; rom.inc: places in ROM the filed code names before their own area is
; filed (tables it reads, routines it calls).

includeonce

; The task scheduler's loop, where a task call hands over (system/task.asm).
!task_scheduler_next = $81BC7F
; The per-frame update of the play screen (system/frame.asm).
!frame_update = $81830D
