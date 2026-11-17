; point_moves.asm: actor helpers that move an actor to a point over
; frames, keeping their caller's return address while they wait (so the
; actor's stack is as the actor loop expects at each BRK), and kind $94's
; per-frame work.
; C: src/aladdin/actor/point_moves.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!GOTO_X = $48                   ; the point it goes to (words)
!GOTO_Y = $4A
!GOTO_SPEED = $4C               ; its direction set's byte offset (ActorMoveByVector's X)
!GOTO_BACK = $4E                ; the caller's return address, kept while it waits (word)
!AIM_X = !scratch+4             ; ActorAimAt's point (words)
!AIM_Y = !scratch+6
!NEAR_X = !scratch+8            ; ActorNearPoint's point (words)
!NEAR_Y = !scratch+10

; @routine 83:FB15 Kind94Wait
; WHAT IT DOES: What actors of kind $94 do each frame they wait: animate.
; HOW: AnimStep, RTL.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
; stub ($83:939A) through the actor loop's wait table ($83:904F). Exit:
; as AnimStep.
%at($83FB15)
Kind94Wait:
    jsl.l AnimStep
    rtl

; @routine 83:FB4F ActorBackToCaller
; WHAT IT DOES: The end of a helper that kept its caller's return
; address at GOTO_BACK while it waited: puts it back and returns there.
; HOW: GOTO_BACK (16-bit) pushed; M 8-bit; RTS.
; Entry: X 8-bit; D = the actor's record; reached by JMP ($83:FCEF).
; Exit: M 8-bit; A = GOTO_BACK; N and Z from it.
%at($83FB4F)
ActorBackToCaller:
    rep.b #$20
    lda.b !GOTO_BACK
    pha
    sep.b #$20
    rts

; @routine 83:FC5D ActorGoToPoint
; @actor
; WHAT IT DOES: Moves the actor toward the point (GOTO_X, GOTO_Y) a step
; a frame, facing the way it goes, until it is within box $20 of it; then
; back to its caller.
; HOW: Its return address pulled into GOTO_BACK. Each frame: the point to
; scratch $10/$12, ActorAimAt (16-bit), the direction to the facing,
; ActorMoveByVector with set GOTO_SPEED; BRK; the point to scratch
; $14/$16, ActorNearPoint with box $20; outside (C set), again. GOTO_BACK
; pushed back, M and X 8-bit, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; called by
; JSR (from $83:AE90 and $83:AECF). Exit: M and X 8-bit; A = GOTO_BACK; C
; clear; Y as ActorNearPoint leaves it.
%at($83FC5D)
ActorGoToPoint:
    rep.b #$20
    pla
    sta.b !GOTO_BACK
.step:
    rep.b #$10
    lda.b !GOTO_X
    sta.w !AIM_X
    lda.b !GOTO_Y
    sta.w !AIM_Y
    jsl.l ActorAimAt
    sta.b !actor_facing
    ldx.b !GOTO_SPEED
    jsl.l ActorMoveByVector
    brk.b #$00
    rep.b #$20
    lda.b !GOTO_X
    sta.w !NEAR_X
    lda.b !GOTO_Y
    sta.w !NEAR_Y
    ldy.b #$20
    jsl.l ActorNearPoint
    bcs .step
    lda.b !GOTO_BACK
    pha
    sep.b #$30
    rts
