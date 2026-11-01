; air_steer.asm: the player's speed across while it is in the air, and a
; few small player settings.
; C: src/aladdin/player/air_steer.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Speed tables (in the data bank, $80), by direction and dash.
!air_accel_toward = $C781
!air_accel_against = $C787
!air_top_toward = $C78A
!air_least_against = $C796
!air_settle = $C7A2

; @routine 81:CF2E PlayerAirSteer
; WHAT IT DOES: Steers the player across while it is in the air: held
; toward its facing it speeds up, against it slows down, each to a limit;
; with no direction its speed settles to a set speed.
; HOW: The direction bits come from V_1434 (when V_1428 is set; 0 there
; reads as 2) or V_0012. Bits 0-2 at 0, 2 or 3: no direction; the speed
; is brought to the settle speed of the dash row (ActorAccelerateX by $28
; when below, ActorDecelerateX by 8 when above, not under $0180). Else
; bit 6 against the facing at $14 picks: the same way, ActorAccelerateX
; by the row's rate, the speed cut to the row's top; the other way,
; ActorDecelerateX by the row's rate, the speed raised to the row's
; least. The rows are by the direction's low two bits and PLAYER_DASH.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80. Exit: M and
; X 8-bit; A, X, Y and the flags as the path taken leaves them.
%at($81CF2E)
PlayerAirSteer:
    lda.w !v_1428
    beq .from_0012
    ldx.w !v_1434
    bne .dir
    ldx.b #$02
    bra .dir
.from_0012:
    ldx.w !v_0012
.dir:
    txa
    and.b #$07
    beq .settle
    cmp.b #$02
    beq .settle
    cmp.b #$03
    beq .settle
    txa
    rol a
    rol a
    and.b #$01
    eor.b !actor_facing
    beq .against
    txa
    and.b #$03
    asl a
    pha
    clc
    adc.w !player_dash
    tay
    lda.w !air_accel_toward,y
    sta.b !actor_accel_x
    jsl.l ActorAccelerateX
    pla
    asl a
    clc
    adc.w !player_dash
    clc
    adc.w !player_dash
    tax
    rep.b #$20
    lda.w !air_top_toward,x
    cmp.b !actor_speed_x
    bcs .top_kept
    sta.b !actor_speed_x
.top_kept:
    sep.b #$20
    rts
.against:
    txa
    and.b #$03
    pha
    tay
    lda.w !air_accel_against,y
    sta.b !actor_accel_x
    jsl.l ActorDecelerateX
    pla
    asl a
    asl a
    clc
    adc.w !player_dash
    clc
    adc.w !player_dash
    tax
    rep.b #$20
    lda.w !air_least_against,x
    cmp.b !actor_speed_x
    bcc .least_kept
    sta.b !actor_speed_x
.least_kept:
    sep.b #$20
    rts
.settle:
    rep.b #$10
    lda.b #$00
    xba
    lda.w !player_dash
    asl a
    tay
    ldx.w !air_settle,y
    cpx.b !actor_speed_x
    beq .done
    bcc .above
    cpx.b !actor_speed_x
    bcc .speed_up
    stx.b !actor_speed_x
.speed_up:
    lda.b #$28
    sta.b !actor_accel_x
    jsl.l ActorAccelerateX
    bra .done
.above:
    lda.b #$08
    sta.b !actor_accel_x
    jsl.l ActorDecelerateX
    ldx.w #$0180
    cpx.b !actor_speed_x
    bcc .done
    stx.b !actor_speed_x
.done:
    sep.b #$10
    rts

; @routine 81:D855 PlayerSpeedLeast
; WHAT IT DOES: Keeps the player moving across at three quarters of a
; pixel a frame or more, counting the frames it had to.
; HOW: A speed under $00C0 becomes $00C0 and PLAYER_SLOWED steps up.
; Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: M 8-bit; A = the speed (or $00C0); N, Z, C from the
; compare, or from the count's step; X, Y unchanged.
%at($81D855)
PlayerSpeedLeast:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    cmp.w #$00C0
    bcs .done
    inc.w !player_slowed
    lda.w #$00C0
    sta.b !actor_speed_x
.done:
    sep.b #$20
    rts

; @routine 81:DACE PlayerPosRestore
; WHAT IT DOES: Puts the player back at the position kept for it.
; HOW: Its X and Y pixels from PLAYER_POS_SAVED.
; Entry: M 8-bit; D = the player's record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: M 8-bit; A = the Y; N, Z from it; X, Y unchanged.
%at($81DACE)
PlayerPosRestore:
    rep.b #$20
    lda.w !player_pos_saved
    sta.b !actor_x+1
    lda.w !player_pos_saved+2
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine 81:DDB4 PlayerFlagsClear
; WHAT IT DOES: Clears four of the player's flags and counts (V_1A08,
; V_141C, V_142D, V_144D).
; HOW: Each byte stored 0.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit:
; everything unchanged.
%at($81DDB4)
PlayerFlagsClear:
    stz.w !v_1a08
    stz.w !v_141c
    stz.w !v_142d
    stz.w !v_144d
    rts
