; actor_state_8c.asm: state $8C (with bit 7 of the byte at $08 set and of
; the byte at $09 cleared) for the actor's own record, and
; ActorStateSet8C for callers in other banks.
; C: src/aladdin/actor/actor_state_8c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 83:8254 ActorStateSet8CSelf
; WHAT IT DOES: Puts the actor into state $8C, with bit 7 of its byte at
; $08 set and of its byte at $09 cleared: ActorStateSet8C for the record
; the direct page is.
; HOW: ASL then LSR clears bit 7 of $09; ORA sets bit 7 of $08; then the
; state byte.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($838254)
ActorStateSet8CSelf:
    asl.b !actor_09
    lsr.b !actor_09
    lda.b !actor_08
    ora.b #$80
    sta.b !actor_08
    lda.b #$8C
    sta.b !actor_state
    rts

; @routine 83:8263 ActorStateSet8CLong
; WHAT IT DOES: ActorStateSet8C for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorStateSet8C; called by JSL. Exit: as the C states.
ActorStateSet8CLong:
    jsr.w ActorStateSet8C
    rtl
