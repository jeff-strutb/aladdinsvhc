; restart_far.asm: the escort waiting at a spot until Aladdin comes near,
; then following him; sent back to its restart point ($4E) once he is far
; to its left.
; C: src/aladdin/actor/restart_far.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!actor_restart = $4E            ; direct page (the actor): where its code starts again (word)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!FAR_LEFT = $00C0               ; how far left of the actor the player must be
!ESCORT_RESTART_A = $EF2B       ; EscortWaitSpot0's restart point: EscortWaitSpot4
!ESCORT_RESTART_C = $EF0C       ; EscortWaitSpot0C's: EscortStaySpot18
!ESCORT_RESTART_10 = $EFC5      ; EscortWaitSpot10's: its wait at spot $14

; @routine A5:EF0C EscortStaySpot18
; @actor
; WHAT IT DOES: The escort stands at spot $18 for good.
; HOW: ActorPlaceAtSpot with X = $18, then BRK for good.
; Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
; through the escort's table at $A5:E56A, or as EscortWaitSpot0C's
; restart point. Exit: never returns.
%at($A5EF0C)
EscortStaySpot18:
    ldx.b #$18
    jsr.w ActorPlaceAtSpot
.stay:
    brk.b #$00
    bra .stay

; @routine A5:EF15 EscortWaitSpot0
; @actor
; WHAT IT DOES: The escort, flipped, in its first pose at spot 0, then
; following Aladdin, restarting at EscortWaitSpot4.
; HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = 0; A = the restart
; point (XBA'd in); on into EscortFollow.
; Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
; through the escort's table at $A5:E56A. Exit: never returns.
%at($A5EF15)
EscortWaitSpot0:
    lda.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$00
    jsr.w ActorPlaceAtSpot
    lda.b #!ESCORT_RESTART_A>>8
    xba
    lda.b #!ESCORT_RESTART_A&255
    bra EscortFollow

; @routine A5:EF2B EscortWaitSpot4
; @actor
; WHAT IT DOES: The escort, flipped, in its first pose at spot 4, then
; following Aladdin, restarting at EscortStaySpot18: EscortWaitSpot0's
; restart point, where it goes once he is far to its left.
; HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = 4; A = the restart
; point, EscortStaySpot18 (XBA'd in); on into EscortFollow.
; Entry: M 8-bit, X 8-bit; D = the escort's record; reached by
; ActorRestartIfPlayerFarLeft's RTS to the restart point EscortWaitSpot0
; set. Exit: never returns.
EscortWaitSpot4:
    lda.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$04
    jsr.w ActorPlaceAtSpot
    lda.b #!ESCORT_RESTART_C>>8
    xba
    lda.b #!ESCORT_RESTART_C&255

; @routine A5:EF3F EscortFollow
; @actor
; WHAT IT DOES: The escort waits until Aladdin is near, then follows him
; on foot (speed $12, animation 2), falling off ledges and landing again;
; whenever he is more than 192 pixels to its left it goes back to its
; restart point (A, kept at $4E).
; HOW: $4E word = A; each frame (BRK first) until ActorNearPlayerX with
; Y = 8 clears C; flip 0. Walk: ActorSpeedXFromA5 Y = $12, ActorAnimChange
; 2; each frame ActorRestartIfPlayerFarLeft, ActorMoveX, until
; ActorSnapToGround finds no ground (zero); then ActorMotionPreset $7E and
; each frame ActorRestartIfPlayerFarLeft, ActorAnimRiseFall, ActorFly,
; until ActorOutOfGround is nonzero: walk again.
; Entry: M 8-bit, X 8-bit; A = the restart point; D = the escort's
; record; reached from EscortWaitSpot0, 0C and 10. Exit: never returns.
%at($A5EF3F)
EscortFollow:
    sta.b !actor_restart
    xba
    sta.b !actor_restart+1
.near:
    brk.b #$00
    ldy.b #$08
    jsl.l ActorNearPlayerX
    bcs .near
    stz.b !actor_flip
.walk:
    ldy.b #$12
    jsr.w ActorSpeedXFromA5
    lda.b #$02
    jsr.w ActorAnimChange
.step:
    brk.b #$00
    jsr.w ActorRestartIfPlayerFarLeft
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    bne .step
    ldy.b #$7E
    jsl.l ActorMotionPreset
.fall:
    brk.b #$00
    jsr.w ActorRestartIfPlayerFarLeft
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    bne .walk
    bra .fall

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

; @routine A5:EF97 EscortWaitSpot0C
; @actor
; WHAT IT DOES: The escort, flipped, in its first pose at spot $0C, then
; following Aladdin, restarting at EscortStaySpot18.
; HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = $0C; A = the
; restart point; on into EscortFollow.
; Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
; through the escort's table at $A5:E56A. Exit: never returns.
%at($A5EF97)
EscortWaitSpot0C:
    lda.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$0C
    jsr.w ActorPlaceAtSpot
    lda.b #!ESCORT_RESTART_C>>8
    xba
    lda.b #!ESCORT_RESTART_C&255
    jmp.w EscortFollow

; @routine A5:EFAE EscortWaitSpot10
; @actor
; WHAT IT DOES: The escort, flipped, in its first pose at spot $10, then
; following Aladdin; sent back, it waits at spot $14 until he is near
; (and goes on in code no script runs).
; HOW: Flip 1, AnimStart 0, ActorPlaceAtSpot with X = $10; A = the
; restart point, $EFC5; on into EscortFollow. At the restart point: flip
; 1, AnimStart 0, ActorPlaceAtSpot with X = $14, each frame (BRK first)
; until ActorNearPlayerX with Y = 8 clears C; what follows ($A5:EFDE) no
; script reaches: those bytes are left in the intake.
; Entry: M 8-bit, X 8-bit; D = the escort's record; reached by the jump
; through the escort's table at $A5:E56A, and at $EFC5 by
; ActorRestartIfPlayerFarLeft's return. Exit: never returns.
%at($A5EFAE)
EscortWaitSpot10:
    lda.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$10
    jsr.w ActorPlaceAtSpot
    lda.b #!ESCORT_RESTART_10>>8
    xba
    lda.b #!ESCORT_RESTART_10&255
    jmp.w EscortFollow
.restart:
    lda.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$14
    jsr.w ActorPlaceAtSpot
.near:
    brk.b #$00
    ldy.b #$08
    jsl.l ActorNearPlayerX
    bcs .near
