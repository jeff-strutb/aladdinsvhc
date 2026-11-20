; rom.inc: places in ROM the filed code names before their own area is
; filed (tables it reads, routines it calls).

includeonce

; The task scheduler's loop, where a task call hands over (system/task.asm).
!task_scheduler_next = $81BC7F

; The PAL console's screen (84:AFCC), which LevelEndTask and ChainPieceEnd go
; to when $005D is set; not filed (no script reaches it on this NTSC cart).
!pal_screen = $84AFCC
