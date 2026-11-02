; kind_animate.asm: per-frame routines of actor kinds that animate and send
; their picture each frame they wait. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/kind_animate.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 83:FAAE KindAnimateSendFAAE
; WHAT IT DOES: One frame of an actor kind (the one the kind table's stub
; at 83:9355 starts at 83:FA4F): animates and sends its picture.
; HOW: AnimStep, then ActorPictureSend.
; Entry: D = the actor; called by JSL from the kind's stub (83:9359) while
; the actor waits. Exit: as ActorPictureSend.
%at($83FAAE)
KindAnimateSendFAAE:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    rtl
