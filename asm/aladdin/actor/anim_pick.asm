; anim_pick.asm: choosing which animation an actor starts.
; C: src/aladdin/actor/anim_pick.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; In the bank DB names: an animation by the actor's index at $40.
!ANIM_BY_40 = $D714

; @routine 84:8A48 ActorAnimBy40
; WHAT IT DOES: Starts the animation the actor's index at $40 picks.
; HOW: The index selects a byte of the table at $D714; AnimStart.
; Entry: M 8-bit, X 8-bit; D = the actor; DB the table's bank. Exit: as the C states.
%at($848A48)
ActorAnimBy40:
    ldx.b !actor_40
    lda.w !ANIM_BY_40,x
    jsl.l AnimStart
    rts

; @routine A5:EA15 ActorAnimChange
; WHAT IT DOES: Starts animation A unless it is the one the actor already
; plays.
; HOW: A doubled is compared with the actor's animation offset; AnimStart
; when they differ.
; Entry: M 8-bit, X 8-bit; A = the animation; D = the actor. Exit: as the C states.
%at($A5EA15)
ActorAnimChange:
    asl a
    cmp.b !ACTOR_ANIM
    beq .same
    lsr a
    jsl.l AnimStart
.same:
    rts
