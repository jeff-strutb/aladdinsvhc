; hit_box_long.asm: ActorHitsTestBoxIfPlayerFree for callers in other
; banks.
; C: src/aladdin/actor/hit_box_long.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 83:82F4 ActorHitsTestBoxIfPlayerFreeLong
; WHAT IT DOES: ActorHitsTestBoxIfPlayerFree for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorHitsTestBox; called by JSL. Exit: as the C states.
%at($8382F4)
ActorHitsTestBoxIfPlayerFreeLong:
    jsr.w ActorHitsTestBoxIfPlayerFree
    rtl
