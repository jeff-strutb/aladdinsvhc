; player_carry.asm: actors and the player beside them: carried along when
; riding (as the actor steps), and faced. Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/actor/player_carry.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CARRIES = $43                  ; nonzero: the player rides the actor this frame
!KEPT_X = $48                   ; the actor's x pixels when last kept (word)
!KEPT_Y = $4A                   ; its y pixels when last kept (word)
!v_143b = $143B                 ; moved with the player's x (word)
!v_143e = $143E                 ; moved with the player's y (word)
!TARGET_X = $1A                 ; where some actors move to: x pixels (word)
!TARGET_Y = $1E                 ; and y pixels (word)
!LIFT_STEP = $44                ; the lifting actor's step: 0 waiting, negative counting, positive lifting
!v_1427 = $1427                 ; nonzero: the player is kept on the lifting actor
!LIFT_RESUME = $D99B            ; the player's code resumed while it is lifted
; Not yet filed: the test of the player against the lifting actor.
!LIFT_TEST = $8288CF

; @routine 83:C1EC ActorStepCarry
; WHAT IT DOES: Moves the actor a step along its facing (vector set $0E)
; and carries the player riding it along.
; HOW: ActorMoveByVector with X = $0E; then ActorCarryPlayer.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB the vectors' bank;
; called by JSR. Exit: as ActorCarryPlayer.
%at($83C1EC)
ActorStepCarry:
    ldx.b #$0E
    jsl.l ActorMoveByVector
    jsr.w ActorCarryPlayer
    rts

; @routine 83:C1F6 ActorStepToTargetCarry
; WHAT IT DOES: Moves the actor a step toward its target ($1A, $1E):
; across and up or down by vector set 8 on each axis it has not reached;
; then carries the player riding it along.
; HOW: Its x pixels (16-bit) unequal to TARGET_X: ActorMoveByVectorX with
; X = 8; its y pixels unequal to TARGET_Y: ActorMoveByVectorY with X = 8;
; then ActorCarryPlayer.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB the vectors' bank;
; called by JSR. Exit: as ActorCarryPlayer.
%at($83C1F6)
ActorStepToTargetCarry:
    rep.b #$20
    lda.b !actor_x+1
    cmp.b !TARGET_X
    beq .lC208
    sep.b #$20
    ldx.b #$08
    jsl.l ActorMoveByVectorX
    rep.b #$20
.lC208:
    lda.b !actor_y+1
    cmp.b !TARGET_Y
    beq .lC216
    sep.b #$20
    ldx.b #$08
    jsl.l ActorMoveByVectorY
.lC216:
    sep.b #$20
    jsr.w ActorCarryPlayer
    rts

; @routine 83:FB57 ActorLiftPlayerStep
; WHAT IT DOES: A step of an actor that lifts the player (from 83:C245):
; at $44 = 0, when the test at 82:88CF finds the player, the player is
; placed on it and $44 counts on; negative $44 counts on; positive, the
; player is kept on it while $1427 is set, else $44 becomes $F0.
; HOW: $44 zero: the test at 82:88CF; C clear: the player's $03-$04 =
; $D99B, PlayerSpotAboveActor with X, $44 up. Negative: $44 up. Positive:
; $1427 set, PlayerToActorAbove with X; else $44 = $F0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit; as the call made.
%at($83FB57)
ActorLiftPlayerStep:
    lda.b !LIFT_STEP
    beq .lFB67
    bmi .lFB7C
    lda.w !v_1427
    bne .lFB7F
    lda.b #$F0
    sta.b !LIFT_STEP
.lFB66:
    rts
.lFB67:
    jsl.l !LIFT_TEST
    bcs .lFB66
    rep.b #$20
    lda.w #!LIFT_RESUME
    sta.w !player+!actor_03
    sep.b #$20
    txa
    jsl.l PlayerSpotAboveActor
.lFB7C:
    inc.b !LIFT_STEP
    rts
.lFB7F:
    txa
    jsl.l PlayerToActorAbove
    rts

; @routine 83:FBD6 ActorCarryPlayerLong
; WHAT IT DOES: ActorCarryPlayer for callers in other banks.
; HOW: JSR ActorCarryPlayer, RTL.
; Entry: as ActorCarryPlayer; called by JSL. Exit: as ActorCarryPlayer.
%at($83FBD6)
ActorCarryPlayerLong:
    jsr.w ActorCarryPlayer
    rtl

; @routine 83:FBDA ActorCarryPlayer
; WHAT IT DOES: Carries the player along with the actor it rides: moves
; the player (and $143B, $143E) by how far the actor moved since its x and
; y were kept.
; HOW: Only when CARRIES is set: its x pixels less KEPT_X added to the
; player's x pixels and to $143B; its y pixels less KEPT_Y added to the
; player's y and to $143E; 16-bit A.
; Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit;
; not carrying: A low = 0, Z set; else A = the new $143E.
%at($83FBDA)
ActorCarryPlayer:
    lda.b !CARRIES
    beq .lFC0C
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.b !KEPT_X
    pha
    clc
    adc.w !player+!actor_x+1
    sta.w !player+!actor_x+1
    pla
    clc
    adc.w !v_143b
    sta.w !v_143b
    sec
    lda.b !actor_y+1
    sbc.b !KEPT_Y
    pha
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    pla
    clc
    adc.w !v_143e
    sta.w !v_143e
    sep.b #$20
.lFC0C:
    rts

; @routine 83:FC96 ActorFacePlayer
; WHAT IT DOES: Turns the actor to face the player: facing 1 (left) when
; it is at or right of the player, 0 when left of it.
; HOW: Its x pixels compared with the player's (16-bit X); the carry
; rotated into 0 is the facing.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit: M
; 8-bit, X 8-bit; A low = the facing; X = its x pixels' low byte; C clear.
%at($83FC96)
ActorFacePlayer:
    rep.b #$10
    ldx.b !actor_x+1
    cpx.w !player+!actor_x+1
    lda.b #$00
    rol a
    sta.b !actor_facing
    sep.b #$10
    rts
