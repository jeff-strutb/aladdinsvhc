; kind49.asm: actor kind $49, which carries the player along as it moves.
; C: src/aladdin/actor/kind49.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 84:804D Kind49Carry
; @actor
; WHAT IT DOES: Actor kind $49: each frame it carries the player along by
; however far it moved (its own moves are its kind's wait work).
; HOW: actor_09 bits $90 set (bit 7: its kind's routine runs each frame
; it waits); then each frame a yield and ActorCarryPlayerLong.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $49 in state $0C (the table at $83:8CAB,
; through the stub at $83:923C). Exit: never returns.
%at($84804D)
Kind49Carry:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
.ride:
    brk.b #$00
    jsl.l ActorCarryPlayerLong
    bra .ride
