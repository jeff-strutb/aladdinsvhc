; kind70_wait.asm: what actors of kind $70 (their start at $83:F370) run
; each frame they wait: their animation, played on. The actor loop calls
; it through its table by kind ($83:8F4F) when bit 7 of the record's byte
; at $09 is set (docs/aladdin/ACTORS.md).
; C: src/aladdin/actor/kind70_wait.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 83:F36B ActorKind70Wait
; WHAT IT DOES: The routine actors of kind $70 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: their animation
; plays on by a frame of time (AnimStep).
; HOW: JSL AnimStep, then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind;
; called by JSR through the actor loop's table by kind. Exit: as the C states.
%at($83F36B)
ActorKind70Wait:
    jsl.l AnimStep
    rts
