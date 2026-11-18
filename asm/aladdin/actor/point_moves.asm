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
!SPAWN_LOOK = $4D               ; the look ActorSpawnLookWait spawns
!HOP_TABLE = $D9BF              ; 5 bytes an entry: $20, vertical speed (2), horizontal speed (2)
!PROBE_X = !scratch             ; PointInGroundLong's point (words)
!PROBE_Y = !scratch+2
!PROBE_FLAGS = !scratch+6       ; its result: bits 0-2 set in the ground

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

; @routine 83:FC0D ActorSpawnLookWait
; @actor
; WHAT IT DOES: Spawns an actor of the look in $4D next to the actor,
; waiting a frame and trying again while no record is free; then back to
; its caller.
; HOW: Its return address pulled into GOTO_BACK ($4E); ActorSpawnSameLook
; with A = $4D; N clear: on into ActorBackToCaller. N set (no record): BRK
; ($83:FC1F) and again ($83:FC14); no script runs that.
; Entry: M 8-bit; D = the actor's record; called by JSR ($83:ACAC).
; Exit: as ActorBackToCaller.
%at($83FC0D)
ActorSpawnLookWait:
    rep.b #$20
    pla
    sta.b !GOTO_BACK
    sep.b #$20
    lda.b !SPAWN_LOOK
    jsl.l ActorSpawnSameLook
    db $30,$03                  ; bmi to $83:FC1F (BRK, BRA $FC14), no record free
    jmp.w ActorBackToCaller

; @routine 83:FD2C ActorHopFromTable
; @actor
; WHAT IT DOES: A hop by entry X of the hop table at $83:D9BF: the actor
; moves and falls each frame until it is back on the ground; then back to
; its caller.
; HOW: Its return address pulled into GOTO_BACK ($4E); from the entry: $20,
; the vertical speed $1D-$1E ($1F $FF: up), the horizontal speed $19-$1A
; ($1B 0). Each frame: BRK; ActorMoveX, ActorFall, ActorOutOfWallProbe4;
; again while $1F is negative or ActorOutOfGround says Z. Then on into
; ActorBackToCaller.
; Entry: M 8-bit, X 8-bit; X = the entry's offset; D = the actor's record;
; DB = $80; called by JSR. Exit: as ActorBackToCaller.
%at($83FD2C)
ActorHopFromTable:
    rep.b #$20
    pla
    sta.b !GOTO_BACK
    sep.b #$20
    lda.w !HOP_TABLE,x
    sta.b !actor_accel_y
    lda.w !HOP_TABLE+1,x
    sta.b !actor_speed_y
    lda.w !HOP_TABLE+2,x
    sta.b !actor_speed_y+1
    lda.b #$FF
    sta.b !actor_speed_y+2
    lda.w !HOP_TABLE+3,x
    sta.b !actor_speed_x
    lda.w !HOP_TABLE+4,x
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.frame:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorFall
    jsl.l ActorOutOfWallProbe4
    lda.b !actor_speed_y+2
    bmi .frame
    jsl.l ActorOutOfGround
    beq .frame
    jmp.w ActorBackToCaller

; @routine 83:FD6D ActorProbeAheadInGround
; WHAT IT DOES: Tests two points for ground: X ahead of the actor (by its
; facing) and Y above it, then Y above its own x: C set when either is in
; the ground.
; HOW: 16-bit: X negated when $14 is set; scratch $0C = it + the actor's x,
; $0E = its y - Y; PointInGroundLong (P kept); $0012 AND 7 nonzero: C set
; ($83:FDAE, no script runs that). Then $0C = its x; again. Both clear: M
; 8-bit, C clear.
; Entry: M 8-bit, X 8-bit; X, Y the offsets; D = the actor's record;
; called by JSR ($83:ADF5). Exit: M 8-bit; C as above.
%at($83FD6D)
ActorProbeAheadInGround:
    rep.b #$20
    txa
    ldx.b !actor_facing
    beq .ahead
    eor.w #$FFFF
    inc a
.ahead:
    clc
    adc.b !actor_x+1
    sta.w !PROBE_X
    tya
    eor.w #$FFFF
    inc a
    clc
    adc.b !actor_y+1
    sta.w !PROBE_Y
    php
    jsl.l PointInGroundLong
    plp
    lda.w !PROBE_FLAGS
    bit.w #$0007
    db $D0,$17                  ; bne to $83:FDAE (SEP #$20, SEC, RTS): in the ground
    lda.b !actor_x+1
    sta.w !PROBE_X
    php
    jsl.l PointInGroundLong
    plp
    lda.w !PROBE_FLAGS
    bit.w #$0007
    db $D0,$04                  ; bne to $83:FDAE
    sep.b #$20
    clc
    rts
