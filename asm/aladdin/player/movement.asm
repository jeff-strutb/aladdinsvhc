; movement.asm: how actors move from their speeds, and the player's run
; settings.
; C: src/aladdin/player/movement.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!FALL_SPEED_MAX = $0580         ; the fastest an actor falls: 5.5 pixels a frame

; @routine 81:CE70 PlayerFlipByPad
; WHAT IT DOES: Turns the player's sprite the way the joypad points, when
; it points one way.
; HOW: PadFacing; when it gives a facing (not negative), that goes to the
; record's ACTOR_FLIP.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80. Exit: as
; PadFacing.
%at($81CE70)
PlayerFlipByPad:
    jsr.w PadFacing
    bmi .done
    sta.b !actor_flip
.done:
    rts

; @routine 81:D164 PlayerGroundYSave
; WHAT IT DOES: Notes the height the player stands at, from which falls
; are measured.
; HOW: Its Y in pixels goes to PLAYER_GROUND_Y.
; Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF
; is work RAM. Exit: M 8-bit; A = the Y; N and Z from it; X, Y unchanged.
%at($81D164)
PlayerGroundYSave:
    rep.b #$20
    lda.b !actor_y+1
    sta.w !player_ground_y
    sep.b #$20
    rts

; @routine 81:D16E PlayerTopSpeedHold
; WHAT IT DOES: Makes the player's present speed the fastest it may run
; (as when it lands from a jump at that speed).
; HOW: The speed's low two bytes go to PLAYER_TOP_SPEED.
; Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF
; is work RAM. Exit: A low = the speed's second byte; N, Z from it; the
; rest unchanged.
PlayerTopSpeedHold:
    lda.b !actor_speed_x
    sta.w !player_top_speed
    lda.b !actor_speed_x+1
    sta.w !player_top_speed+1
    rts

; @routine 81:D179 PlayerTopSpeedCap
; WHAT IT DOES: Keeps the player's top running speed at 1.5 pixels a
; frame or under.
; HOW: A top speed above $0180 becomes $0180.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit;
; A = $0180; N, Z, C from comparing it with the top speed (before); X, Y
; unchanged.
PlayerTopSpeedCap:
    rep.b #$20
    lda.w #$0180
    cmp.w !player_top_speed
    bcc .done
    sta.w !player_top_speed
.done:
    sep.b #$20
    rts

; @routine 81:D189 PlayerRunRatesSet
; WHAT IT DOES: Sets how fast the player gains and loses speed running on
; the ground.
; HOW: Accelerating $20, slowing $FFF8 (-8), turning $FFC0 (-64), in
; fractions of a pixel a frame each frame.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit;
; A = $FFC0; N set, Z clear; the rest unchanged.
PlayerRunRatesSet:
    rep.b #$20
    lda.w #$0020
    sta.w !player_run_accel
    lda.w #$FFF8
    sta.w !player_run_decel
    lda.w #$FFC0
    sta.w !player_turn_decel
    sep.b #$20
    rts

; @routine 81:D41C ActorStepX
; WHAT IT DOES: Moves an actor across by its speed, the way it faces.
; HOW: Facing right ($14 = 0), its 24-bit X plus its speed; facing left,
; minus.
; Entry: M 8-bit; D = the actor's record. Exit: A low = X's top byte; N,
; V, Z, C from the last add or subtract; X, Y unchanged.
%at($81D41C)
ActorStepX:
    lda.b !actor_facing
    bne .left
    clc
    lda.b !actor_x
    adc.b !actor_speed_x
    sta.b !actor_x
    lda.b !actor_x+1
    adc.b !actor_speed_x+1
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.b !actor_speed_x+2
    sta.b !actor_x+2
    rts
.left:
    sec
    lda.b !actor_x
    sbc.b !actor_speed_x
    sta.b !actor_x
    lda.b !actor_x+1
    sbc.b !actor_speed_x+1
    sta.b !actor_x+1
    lda.b !actor_x+2
    sbc.b !actor_speed_x+2
    sta.b !actor_x+2
    rts

; @routine 81:D448 ActorFallCap
; WHAT IT DOES: Keeps an actor that is falling from falling faster than
; 5.5 pixels a frame.
; HOW: Rising (its vertical speed negative): nothing; else the speed
; above 5.5 is cut to 5.5.
; Entry: M 8-bit; D = the actor's record. Exit: M 8-bit; rising: A low =
; the speed's top byte, N set; else A = $0580, N, Z, C from comparing it
; with the speed (before the cut); X, Y unchanged.
ActorFallCap:
    lda.b !actor_speed_y+2
    bpl ActorGravity_cap
    rts

; @routine 81:D44D ActorGravity
; WHAT IT DOES: Gravity: an actor's vertical speed grows by its
; acceleration, falling no faster than 5.5 pixels a frame.
; HOW: The 24-bit speed plus the 8-bit acceleration; when the speed is
; then downward, it is cut to 5.5.
; Entry: M 8-bit; D = the actor's record. Exit: M 8-bit; rising: A low =
; the speed's top byte, N set, C from the add; else A = $0580, N, Z, C
; from comparing it with the speed (before the cut); X, Y unchanged.
ActorGravity:
    clc
    lda.b !actor_speed_y
    adc.b !actor_accel_y
    sta.b !actor_speed_y
    lda.b !actor_speed_y+1
    adc.b #0
    sta.b !actor_speed_y+1
    lda.b !actor_speed_y+2
    adc.b #0
    sta.b !actor_speed_y+2
    bmi .done
.cap:
    rep.b #$20
    lda.w #!FALL_SPEED_MAX
    cmp.b !actor_speed_y
    bcs .keep
    sta.b !actor_speed_y
.keep:
    sep.b #$20
.done:
    rts
