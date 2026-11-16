; kind51.asm: actor kind $51, a thing that rises and settles (Aladdin can
; land on it once it has): its variant with $07 bit 1 set, which plays
; once and is gone, and the jump its code ends by.
; C: src/aladdin/actor/kind51.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND51_FRAMES = $40            ; frames left of the present step
!KIND51_SOLID = $41             ; nonzero: Aladdin can land on it (its wait tests it)

; @routine 83:D279 Kind51Variant
; @actor
; WHAT IT DOES: Kind $51 with $07 bit 1 set: after a pause when $07 bit 2
; is set, plays its animation 5 for $58 frames, then is gone for good.
; HOW: KIND51_SOLID cleared; $07 bit 2: KIND51_FRAMES = $0A frames
; (BRK each). AnimStart 5; KIND51_FRAMES = $58 frames of BRK and
; AnimStep; then Kind51End.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
; jump from its start (83:D247). Exit: never returns: as Kind51End.
%at($83D279)
Kind51Variant:
    stz.b !KIND51_SOLID
    lda.b !actor_07
    and.b #$04
    beq .play
    lda.b #$0A
    sta.b !KIND51_FRAMES
.pause:
    brk.b #$00
    dec.b !KIND51_FRAMES
    bne .pause
.play:
    lda.b #$05
    jsl.l AnimStart
    lda.b #$58
    sta.b !KIND51_FRAMES
.step:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND51_FRAMES
    bne .step
    jmp.w Kind51End

; @routine 83:D32F Kind51End
; @actor
; WHAT IT DOES: Where kind $51's code ends: the actor gone for good.
; HOW: JMP to ActorEndPlaced.
; Entry: D = the actor's record; reached by a jump from Kind51Variant
; (83:D29F). Exit: never returns: as ActorEndPlaced.
%at($83D32F)
Kind51End:
    jmp.w ActorEndPlaced
