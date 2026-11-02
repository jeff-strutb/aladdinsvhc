; run_speed.asm: the player's running speed from the joypad: up toward the top speed,
; slowed with no direction, turned against. Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/player/run_speed.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; @routine 81:D1A0 PlayerRunSpeed
; WHAT IT DOES: Steps the player's running speed by the joypad: held
; toward its facing, up by the run rate to the top speed; no direction,
; down by the slowing rate to 0 (unless held up); held the other way, down
; by the turning rate, and past 0 it turns round.
; HOW: PadFacing: the same as the facing, PLAYER_RUN_ACCEL added and
; capped at PLAYER_TOP_SPEED; none, unless PLAYER_SLOWED, PLAYER_RUN_DECEL
; added to a nonzero speed, zeroed when it goes negative; the other way,
; PLAYER_TURN_DECEL added; negative, the speed negated and the facing
; flipped.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code. Exit: M 8-bit; A as the last step left it.
%at($81D1A0)
PlayerRunSpeed:
    jsr.w PadFacing
    bmi .lD1BD
    cmp.b !actor_facing
    bne .lD1D6
    rep.b #$20
    clc
    lda.b !actor_speed_x
    adc.w !player_run_accel
    cmp.w !player_top_speed
    bcc .lD1B9
    lda.w !player_top_speed
.lD1B9:
    sta.b !actor_speed_x
    bra .lD1F2
.lD1BD:
    lda.w !player_slowed
    bne .lD1F4
    rep.b #$20
    lda.b !actor_speed_x
    beq .lD1F2
    lda.w !player_run_decel
    clc
    adc.b !actor_speed_x
    sta.b !actor_speed_x
    bpl .lD1F2
    stz.b !actor_speed_x
    bra .lD1F2
.lD1D6:
    rep.b #$20
    lda.w !player_turn_decel
    clc
    adc.b !actor_speed_x
    sta.b !actor_speed_x
    bpl .lD1F2
    eor.w #$FFFF
    inc a
    sta.b !actor_speed_x
    sep.b #$20
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    bra .lD1F4
.lD1F2:
    sep.b #$20
.lD1F4:
    rts
