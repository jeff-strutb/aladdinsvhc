; priority_clear.asm: an actor's sprite priority bits cleared, except in
; the level that keeps them.
; C: src/aladdin/actor/priority_clear.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!LEVEL_KEEPS_PRIORITY = $0E     ; the level whose actors keep their priority bits
!PRIORITY_BITS = $30            ; bits 4-5 of the actor's $09

; @routine 84:81B2 ActorPriorityClear
; WHAT IT DOES: Clears the actor's sprite priority bits, except in level
; $0E.
; HOW: Unless the level's low byte is $0E, bits 4-5 of $09 cleared (TRB
; $30).
; Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR. Exit: A low byte = $30 or the level; Z from the TRB or
; the compare.
%at($8481B2)
ActorPriorityClear:
    lda.w !level
    cmp.b #!LEVEL_KEEPS_PRIORITY
    beq .keep
    lda.b #!PRIORITY_BITS
    trb.b !actor_09
.keep:
    rts
