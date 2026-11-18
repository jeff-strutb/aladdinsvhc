; guard_chase.asm: the bank $A5 guard's chase: keeping pace with the
; player, and jumping up or down to a ledge when he is well above or
; below.
; C: src/aladdin/actor/guard_chase.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

!PROBES = $002C                 ; probes left (word, data bank)
!JUMP_MOTIONS = $D054           ; the motion for a jump up, by the probes left (data bank)
!GUARD_CODE_SET = $146B         ; which code it was last sent to: 1 stop, 2 run, $40 give up
!GUARD_PACE_ONCE = $146E        ; nonzero: one close step keeps pace instead of slowing
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; the player's y, whole pixels (word)
!player_speed = !player+!actor_speed_x ; the player's horizontal speed (word)

; @routine A5:F284 ActorLedgeAbove
; WHAT IT DOES: Looks for the top of the ground at a point offset from the
; actor, going up a tile at a time: C clear when a ground tile with an
; empty tile above it is found within X probes.
; HOW: The point's y = the actor's + A, its x = the actor's + Y (- Y
; facing left). PointInGroundLong (a ground tile has a kind in the
; behaviour's low 3 bits): while empty, a tile up and a probe less; once
; in ground, a tile up at a time until a probe finds none (C clear). C set
; when the probes run out.
; Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
; offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X 8-bit;
; C as above; $000C, $000E the last point; $002C the probes left.
%at($A5F284)
ActorLedgeAbove:
    rep.b #$20
    clc
    adc.b !actor_y+1
    sta.w !scratch+2
    txa
    sta.w !PROBES
    tya
    ldy.b !actor_facing
    beq .lF299
    eor.w #$FFFF
    inc a
.lF299:
    clc
    adc.b !actor_x+1
    sta.w !scratch
.lF29F:
    jsl.l PointInGroundLong
    sep.b #$30
    lda.w !v_0012
    and.b #$07
    bne .lF2BF
    rep.b #$20
    dec.w !PROBES
    beq .lF2DF
    clc
    lda.w !scratch+2
    adc.w #$FFF0
    sta.w !scratch+2
    bra .lF29F
.lF2BF:
    rep.b #$20
    clc
    lda.w !scratch+2
    adc.w #$FFF0
    sta.w !scratch+2
    jsl.l PointInGroundLong
    sep.b #$30
    lda.w !v_0012
    and.b #$07
    beq .lF2E1
    dec.w !PROBES
    bne .lF2BF
    rep.b #$20
.lF2DF:
    sec
    rts
.lF2E1:
    rep.b #$20
    clc
    rts


; @routine A5:F19D GuardLedgeJump
; WHAT IT DOES: When the player is well above or below the guard, picks a
; ledge toward him and starts the guard's jump to it, leaving its caller;
; C set when it does not jump (or returns at once when he is near its
; height).
; HOW: The player's y + 16 less the guard's. Above by more than 128, or by
; 65-128 and a random pick (RandomFromList, list $4A) allows:
; ActorProbeUpTowardPlayer (offset -48, 5 probes, 24 across); found with
; more than 2 probes left or on a tile with behaviour bit 3: the guard's
; code word ($03) = $E6A0, the caller's return dropped, facing the player,
; ActorMotionPreset by the probes left (table $D054), on a bit-3 tile its
; speed + $80. Below with $44 set ($44 then cleared): by less than 64 only
; on a random pick (list $4C); then ActorLedgeAbove (96 below, 5 probes,
; 32 ahead); found: code word $E6A0, return dropped, ActorMotionPreset
; $83.
; Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR from
; GuardChase. Exit: jumping: to GuardChase's caller, M 8-bit; else M
; 16-bit and C set, or C as the compare left it.
%at($A5F19D)
GuardLedgeJump:
    clc
    lda.w !player_y
    adc.w #$0010
    sec
    sbc.b !actor_y+1
    bpl .lF1B5
    cmp.w #$FFC0
    bcs .lF1C4
    cmp.w #$FF80
    bcs .lF1C5
    bra .lF1CF
.lF1B5:
    ldx.b !actor_44
    beq .lF1C4
    ldx.b #$00
    stx.b !actor_44
    cmp.w #$0040
    bcc .lF21B
    bra .lF225
.lF1C4:
    rts
.lF1C5:
    ldx.b #$4A
    jsl.l RandomFromList
    rep.b #$20
    beq .lF219
.lF1CF:
    lda.w #$FFD0
    ldx.b #$05
    ldy.b #$18
    jsr.w ActorProbeUpTowardPlayer
    bcs .lF240
    ldx.b #$02
    cpx.w !PROBES
    bcc .lF1E7
    bit.w #$0008
    beq .lF240
.lF1E7:
    lda.w #$E6A0
    sta.b !actor_03
    pla
    lda.b !actor_x+1
    cmp.w !player_x
    sep.b #$20
    lda.b #$00
    rol a
    sta.b !actor_facing
    sta.b !actor_flip
    ldx.w !PROBES
    ldy.w !JUMP_MOTIONS,x
    jsl.l ActorMotionPreset
    lda.w !v_0012
    and.b #$08
    beq .lF218
    rep.b #$20
    clc
    lda.b !actor_speed_x
    adc.w #$0080
    sta.b !actor_speed_x
    sep.b #$20
.lF218:
    rts
.lF219:
    sec
    rts
.lF21B:
    ldx.b #$4C
    jsl.l RandomFromList
    rep.b #$20
    beq .lF240
.lF225:
    lda.w #$0060
    ldx.b #$05
    ldy.b #$20
    jsr.w ActorLedgeAbove
    bcs .lF240
    lda.w #$E6A0
    sta.b !actor_03
    pla
    sep.b #$20
    ldy.b #$83
    jsl.l ActorMotionPreset
    rts
.lF240:
    sec
    rts


; @routine A5:F0F4 GuardChase
; WHAT IT DOES: The guard's chase step: GuardLedgeJump, then its speed and
; facing set by how far the player is ahead of it.
; HOW: GuardLedgeJump; then the player's x less the guard's (dx). Within
; 48 either way ($146E cleared if set, then as within 96): a moving guard
; slows by $30 (to 0); a stopped one gets code word $E5B6 and $146B = 1
; (once) and nothing more. Within 96: above a player speed of $C0 it
; closes on his speed by 8 a frame (0 if that goes negative), else takes
; $C0. Within 368: it speeds up by 8, to at most $300. Farther, with $49
; clear: code word $E794, $146B = $40, and back. Each new speed: code word
; $E5E8 and $146B = 2 (once), the speed's high byte cleared, the facing
; (and flip) toward the player.
; Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR. Exit:
; as GuardLedgeJump when it jumps; else M 8-bit when the facing was set, M
; 16-bit otherwise.
%at($A5F0F4)
GuardChase:
    jsr.w GuardLedgeJump
    sec
    lda.w !player_x
    sbc.b !actor_x+1
    clc
    adc.w #$0030
    cmp.w #$0060
    bcc GuardChaseClose
    clc
    adc.w #$0030
    cmp.w #$00C0
    bcc GuardChaseSpeedUp_lF158
    clc
    adc.w #$0110
    cmp.w #$02E0
    bcc GuardChaseSpeedUp
    ldx.b !actor_49
    bne GuardChaseSpeedUp
    ldx.b #$40
    stx.w !GUARD_CODE_SET
    lda.w #$E794
    sta.b !actor_03
    rts

; @routine A5:F127 GuardChaseSpeedUp
; WHAT IT DOES: GuardChase's speed-up step, which GuardRunOff also calls:
; the guard speeds up by 8, to at most $300, then (as every step of the
; chase) runs on (code word $E5E8 once) facing the player.
; HOW: 16-bit: $0300 below ACTOR_SPEED_X: kept; else + 8. Each new speed
; ($A5:F134): stored; GUARD_CODE_SET 2 and code word $E5E8 unless already
; 2; the speed's high byte cleared; its x against the player's: the carry
; to the facing and flip (8-bit).
; Entry: M 16-bit, X 8-bit; D = the guard; DB a bank whose $0000-$1FFF is
; work RAM; reached by GuardChase's branch, and called by JSR from
; GuardRunOff. Exit: M 8-bit; A = the facing; X = 0.
%at($A5F127)
GuardChaseSpeedUp:
    lda.w #$0300
    cmp.b !actor_speed_x
    bcc .lF134
.lF12E:
    clc
    lda.w #$0008
    adc.b !actor_speed_x
.lF134:
    sta.b !actor_speed_x
    ldx.b #$02
    cpx.w !GUARD_CODE_SET
    beq .lF145
    stx.w !GUARD_CODE_SET
    lda.w #$E5E8
    sta.b !actor_03
.lF145:
    ldx.b #$00
    stx.b !actor_speed_x+2
    lda.b !actor_x+1
    cmp.w !player_x
    sep.b #$20
    lda.b #$00
    rol a
    sta.b !actor_facing
    sta.b !actor_flip
    rts
.lF158:
    lda.w #$00C0
    cmp.w !player_speed
    bcs .lF134
    lda.b !actor_speed_x
    cmp.w !player_speed
    bcc .lF12E
    sec
    sbc.w #$0008
    bpl .lF134
    lda.w #$0000
    bra .lF134

; @routine A5:F172 GuardChaseClose
; WHAT IT DOES: GuardChase's close step (the player within 48), which
; GuardHoldLeft and GuardHoldRight also call: once after a pace kept
; ($146E), it keeps his pace; a moving guard slows by $30 (to 0); a
; stopped one is sent to GuardStop (code word $E5B6, once).
; HOW: GUARD_PACE_ONCE set: cleared, on as the pace step
; (GuardChaseSpeedUp_lF158). ACTOR_SPEED_X nonzero: less $30, at least
; 0, on as a new speed (GuardChaseSpeedUp_lF134). Zero: GUARD_CODE_SET 1
; and code word $E5B6 unless already 1.
; Entry: M 16-bit, X 8-bit; D = the guard; DB a bank whose $0000-$1FFF is
; work RAM; reached by GuardChase's branch, and called by JSR from
; GuardHoldLeft and GuardHoldRight. Exit: M 16-bit.
%at($A5F172)
GuardChaseClose:
    ldx.w !GUARD_PACE_ONCE
    beq .lF17E
    ldx.b #$00
    stx.w !GUARD_PACE_ONCE
    bra GuardChaseSpeedUp_lF158
.lF17E:
    lda.b !actor_speed_x
    beq .lF18D
    sec
    sbc.w #$0030
    bpl GuardChaseSpeedUp_lF134
    lda.w #$0000
    bra GuardChaseSpeedUp_lF134
.lF18D:
    ldx.b #$01
    cpx.w !GUARD_CODE_SET
    beq .lF19C
    stx.w !GUARD_CODE_SET
    lda.w #$E5B6
    sta.b !actor_03
.lF19C:
    rts
