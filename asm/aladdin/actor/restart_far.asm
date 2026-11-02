; restart_far.asm: an actor of bank $A5 sent back to its restart point
; ($4E) once the player is far to its left.
; C: src/aladdin/actor/restart_far.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!actor_restart = $4E            ; direct page (the actor): where its code starts again (word)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!FAR_LEFT = $00C0               ; how far left of the actor the player must be

; @routine A5:EF83 ActorRestartIfPlayerFarLeft
; WHAT IT DOES: When the player is more than 192 pixels left of the actor,
; returns to the actor's restart point ($4E) instead of to the caller.
; HOW: The player's x + $C0 (+ C at entry) against the actor's x: lower,
; the return address pulled and $4E - 1 pushed in its place, so the RTS
; goes to $4E.
; Entry: C added in; D = the actor; called by JSR. Exit: M 8-bit; C
; clear when restarting, A = $4E - 1; else C set, A = the sum.
%at($A5EF83)
ActorRestartIfPlayerFarLeft:
    rep.b #$20
    lda.w !player_x
    adc.w #!FAR_LEFT
    cmp.b !actor_x+1
    bcs .near
    pla
    lda.b !actor_restart
    dec a
    pha
.near:
    sep.b #$20
    rts
