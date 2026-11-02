; sound_queue_f6.asm: sound command $F6 queued with the parameter the
; caller gives (the entry inside SoundQueueF6With1, past its LDA #1, whose
; bytes asm/aladdin/audio/sound_command.asm files).
; C: src/aladdin/audio/sound_queue_f6.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 81:F9D4 SoundQueueF6
; WHAT IT DOES: Queues sound command $F6 with parameter A, keeping $F6 as
; the last special command.
; HOW: The parameter waits in A's high byte while $F6 is queued, then
; SoundQueueAdd for it (not queued when it is 0).
; Entry: M 8-bit, X 8-bit; A = the parameter; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: as the C states.
%at($81F9D4)
SoundQueueF6:
    ; the code is SoundQueueF6With1's from its XBA on
