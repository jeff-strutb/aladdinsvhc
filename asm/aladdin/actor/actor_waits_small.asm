; actor_waits_small.asm: small actor routines: two routines actors run each
; frame they wait (hit tests, then their end out of range), kind $82's
; start (shown, and stays), a turn of the vertical speed, and a place on
; the escort actor. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/actor_waits_small.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!WAIT_HITS = $42  ; counted up each frame a shot does not down it

; @routine 83:AA47 ActorWaitHitsRange8
; WHAT IT DOES: What an actor (entry $2D of the actor loop's wait table at
; $83:8F4F) runs each frame it waits: its animation and picture, the
; stomp-or-hurt and shot tests, then its end when it is out of range by
; box 8.
; HOW: AnimStep, ActorPictureSend, ActorStompOrHurt, ActorHitByShotCC; X =
; 8, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83AA47)
ActorWaitHitsRange8:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    jsr.w ActorStompOrHurt
    jsr.w ActorHitByShotCC
    ldx.b #$08
    jmp.w ActorEndOutOfRange


; @routine 83:B630 ActorWaitHitsRange0
; WHAT IT DOES: What an actor (entry $2F of the actor loop's wait table at
; $83:8F4F) runs each frame it waits: the stomp-or-hurt and shot tests
; ($42 counted up when no shot downs it), its animation, then its end when
; it is out of range by box 0.
; HOW: ActorStompOrHurt; ActorDownedByShot, C clear: $42 up; AnimStep; X =
; 0, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0), or from 83:1FE0. Exit: as
; ActorEndOutOfRange.
%at($83B630)
ActorWaitHitsRange0:
    jsr.w ActorStompOrHurt
    jsr.w ActorDownedByShot
    bcs .lB63A
    inc.b !WAIT_HITS
.lB63A:
    jsl.l AnimStep
    ldx.b #$00
    jmp.w ActorEndOutOfRange


; @routine 84:951D Kind82Show
; @actor
; WHAT IT DOES: Actor kind $82: shown with animation 0 (bits $B0 of $09,
; bit 1 of $08 set) and stays.
; HOW: $08 bit 1 set, $09 bits $B0 set, AnimStart 0, BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $82 in state $0C (through the stub at
; $83:92CF). Exit: never returns.
%at($84951D)
Kind82Show:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$B0
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.l952F:
    brk.b #$00
    bra .l952F


; @routine 83:B109 ActorSpeedYTurn
; WHAT IT DOES: Turns the actor's vertical speed round: the 24-bit speed
; negated (its high byte inverted, the low word negated).
; HOW: The speed's top byte EOR $FF; the low word EOR $FFFF plus 1
; (16-bit); M 8-bit.
; Entry: M 8-bit; D = the actor's record; called by JSR (83:B0FA). Exit: M
; 8-bit; A = the new low word; N and Z from it.
%at($83B109)
ActorSpeedYTurn:
    lda.b !actor_speed_y+2
    eor.b #$FF
    sta.b !actor_speed_y+2
    rep.b #$20
    lda.b !actor_speed_y
    eor.w #$FFFF
    inc a
    sta.b !actor_speed_y
    sep.b #$20
    rts


; @routine 83:EB4B ActorOnEscort
; WHAT IT DOES: Puts the actor on the escort actor: its x the escort's,
; its y 4 above the escort's.
; HOW: 16-bit copies of the escort's x and of its y less 4; M 8-bit.
; Entry: M 8-bit; D = the actor's record; called by JSR (83:EB00, 83:EB17,
; 83:EB2C). Exit: M 8-bit; A = the new y pixels.
%at($83EB4B)
ActorOnEscort:
    rep.b #$20
    lda.w !actor_escort+!actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !actor_escort+!actor_y+1
    adc.w #$FFFC
    sta.b !actor_y+1
    sep.b #$20
    rts
