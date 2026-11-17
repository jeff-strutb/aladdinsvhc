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
!REACH_TABLE = $D9B0            ; 8 bytes an entry: x and y reach, rise speed, step, limit
!RISE_SPEED = $1D               ; the rise's speed, 16ths (word; $1F its third byte)
!RISE_STEP = $20                ; added to the speed each frame
!RISE_LIMIT = $45               ; the speed's top (word)

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
; @pushes_return
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

; @routine 83:FCA5 ActorRiseWhenInReach
; @actor
; WHAT IT DOES: When Aladdin is in reach by entry X of the reach table,
; the actor rises a frame at a time, faster each frame up to a limit,
; until it is out of the ground; then back to its caller with C clear.
; Out of reach: C set at once.
; HOW: PlayerInReach; C set: M 8-bit, RTS. Else its return address
; pulled into GOTO_BACK; RISE_LIMIT = the entry's +4 byte * 16, RISE_SPEED
; = its +2 byte * 16 (third byte 0), RISE_STEP = its +3 byte. Each frame:
; BRK; RISE_SPEED + RISE_STEP, at most RISE_LIMIT; ActorMoveY;
; ActorOutOfGround, again while Z. C clear, on into ActorBackToCaller.
; Entry: M 8-bit, X 8-bit; X = the reach entry's offset (8 bytes an
; entry); D = the actor's record; DB = $80; called by JSR ($83:AC54,
; $83:AED6). Exit: in reach: as ActorBackToCaller, C clear; else M 8-bit,
; C set, as PlayerInReach.
%at($83FCA5)
ActorRiseWhenInReach:
    jsr.w PlayerInReach
    bcs .out
    pla
    sta.b !GOTO_BACK
    lda.w !REACH_TABLE+4,x
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    sta.b !RISE_LIMIT
    lda.w !REACH_TABLE+2,x
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    sta.b !RISE_SPEED
    sep.b #$20
    stz.b !RISE_SPEED+2
    lda.w !REACH_TABLE+3,x
    sta.b !RISE_STEP
.rise:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !RISE_STEP
    and.w #$00FF
    adc.b !RISE_SPEED
    cmp.b !RISE_LIMIT
    bcc .below
    lda.b !RISE_LIMIT
.below:
    sta.b !RISE_SPEED
    sep.b #$20
    jsl.l ActorMoveY
    jsl.l ActorOutOfGround
    beq .rise
    clc
    jmp.w ActorBackToCaller
.out:
    sep.b #$20
    rts
