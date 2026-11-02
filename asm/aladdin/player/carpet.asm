; carpet.asm: the magic carpet ride's steering: the joypad's eight
; directions push the speeds, the carpet tilts when it climbs or dives,
; and with no direction held it drifts back to its cruising speed and
; levels off. Two rides use it: the one whose record has $06 = 1 and
; another (the "Alt" routines), each with its own rates and limits.
; Drafted by cdraft.py; named and described by hand.
;
; A steering direction ($14) is 0 right, then clockwise to 7 (2 down, 4
; left, 6 up); 8 slows down across, 9 speeds up, 10 neither; negative
; none. The rate tables (in bank $80) give per direction an across rate
; and an up-down rate, signed by bit 7 (set: take away).
; C: src/aladdin/player/carpet.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; The ride's own fields of the rider's record.
!CARPET_DIRECTION = $14         ; the steering direction (see above)
!CARPET_DIVING = $41            ; nonzero: tilted to dive (move 3)
!CARPET_CLIMBING = $42          ; nonzero: tilted to climb (move 1)
!CARPET_SPED_UP = $44           ; nonzero: it has sped up to cruise (direction 9)
!CARPET_SLOWED = $45            ; nonzero: it has slowed to cruise (direction 8)

; In bank $80: per direction, the across and the up-down rate (bytes).
!CARPET_RATES = $C7B5           ; the ride with $06 = 1
!CARPET_RATES_ALT = $C7CB       ; the other ride
; In bank $80: per direction, 0 level, positive dive, negative climb.
!CARPET_TILT = $C7E1

; @routine 81:E555 CarpetLevelOut
; WHAT IT DOES: Levels the carpet off when it is tilted: out of a dive
; (move 4) or out of a climb (move 2).
; HOW: The dive flag set: cleared, PlayerMoveStart with move 4. Else the
; climb flag set: cleared, PlayerMoveStart with move 2.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Also reached by
; JMP from CarpetTilt. Exit: not tilted: A low = 0, Z set; levelled: as
; PlayerMoveStart. M 8-bit, X 8-bit.
%at($81E555)
CarpetLevelOut:
    lda.b !CARPET_DIVING
    beq .lE561
    stz.b !CARPET_DIVING
    lda.b #$04
    jsr.w PlayerMoveStart
    rts
.lE561:
    lda.b !CARPET_CLIMBING
    beq .lE56C
    stz.b !CARPET_CLIMBING
    lda.b #$02
    jsr.w PlayerMoveStart
.lE56C:
    rts

; @routine 81:E56D CarpetTilt
; WHAT IT DOES: Tilts the carpet the way it is steered: diving (move 3)
; when steered down, climbing (move 1) when steered up; level steering
; levels it off.
; HOW: The tilt table by the direction: 0, on into CarpetLevelOut;
; positive, unless already diving, the dive flag set, the climb flag
; cleared and PlayerMoveStart with move 3; negative, the same for a climb
; with move 1.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). The direction
; 0-7. Exit: steered up or down: A low = 1, except a new dive: as
; PlayerMoveStart; level: as CarpetLevelOut. M 8-bit, X 8-bit.
%at($81E56D)
CarpetTilt:
    ldx.b !CARPET_DIRECTION
    lda.w !CARPET_TILT,x
    beq .lE594
    bmi .lE584
    lda.b !CARPET_DIVING
    bne .lE591
    inc.b !CARPET_DIVING
    stz.b !CARPET_CLIMBING
    lda.b #$03
    jsr.w PlayerMoveStart
    rts
.lE584:
    lda.b !CARPET_CLIMBING
    bne .lE591
    inc.b !CARPET_CLIMBING
    stz.b !CARPET_DIVING
    lda.b #$01
    jsr.w PlayerMoveStart
.lE591:
    lda.b #$01
    rts
.lE594:
    jmp.w CarpetLevelOut

; @routine 81:E597 CarpetSteerXAlt
; WHAT IT DOES: Pushes the other ride's speed across by its rate for the
; steering direction.
; HOW: No direction (negative): nothing. Else the across rate from the
; other ride's table, then on in CarpetSteerX: added to the 24-bit speed,
; or taken away when bit 7 is set.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
; CarpetSteerX.
%at($81E597)
CarpetSteerXAlt:
    lda.b !CARPET_DIRECTION
    bmi CarpetSteerX_done
    asl a
    tax
    lda.w !CARPET_RATES_ALT,x
    bmi CarpetSteerX_take
    bra CarpetSteerX_add

; @routine 81:E5A4 CarpetSteerX
; WHAT IT DOES: Pushes the ride's speed across by its rate for the
; steering direction.
; HOW: No direction (negative): nothing. Else the across rate from the
; table: bit 7 clear, added to the 24-bit speed; set, its low 7 bits (kept
; at $002C) taken away. A byte at a time.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; X = the direction times 2; A low = the speed's top byte; flags
; from the last add or subtract. No direction: A low = the direction, N
; set.
%at($81E5A4)
CarpetSteerX:
    lda.b !CARPET_DIRECTION
    bmi .done
    asl a
    tax
    lda.w !CARPET_RATES,x
    bmi .take
.add:
    clc
    adc.b !actor_speed_x
    sta.b !actor_speed_x
    lda.b !actor_speed_x+1
    adc.b #$00
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    adc.b #$00
    sta.b !actor_speed_x+2
    rts
.take:
    and.b #$7F
    sta.w !scratch+32
    lda.b !actor_speed_x
    sec
    sbc.w !scratch+32
    sta.b !actor_speed_x
    lda.b !actor_speed_x+1
    sbc.b #$00
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    sbc.b #$00
    sta.b !actor_speed_x+2
.done:
    rts

; @routine 81:E5DB CarpetSteerYAlt
; WHAT IT DOES: Pushes the other ride's vertical speed by its rate for the
; steering direction.
; HOW: No direction (negative): nothing. Else the up-down rate from the
; other ride's table; a rate of 0: CarpetSteadyY, then the same rate read
; again; then on in CarpetSteerY: added to the 24-bit speed, or taken away
; when bit 7 is set.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: as
; CarpetSteerY.
%at($81E5DB)
CarpetSteerYAlt:
    lda.b !CARPET_DIRECTION
    bmi CarpetSteerY_done
    asl a
    tax
    lda.w !CARPET_RATES_ALT+1,x
    bne .lE5EC
    jsr.w CarpetSteadyY
    lda.w !CARPET_RATES_ALT+1,x
.lE5EC:
    bmi CarpetSteerY_take
    bra CarpetSteerY_add

; @routine 81:E5F0 CarpetSteerY
; WHAT IT DOES: Pushes the ride's vertical speed by its rate for the
; steering direction.
; HOW: No direction (negative): nothing. Else the up-down rate from the
; table; a rate of 0: CarpetSteadyY, then the same rate read again. Bit 7
; clear, added to the 24-bit speed; set, its low 7 bits (kept at $002C)
; taken away. A byte at a time.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; X = the direction times 2; A low = the speed's top byte; flags
; from the last add or subtract. No direction: A low = the direction, N
; set.
%at($81E5F0)
CarpetSteerY:
    lda.b !CARPET_DIRECTION
    bmi .done
    asl a
    tax
    lda.w !CARPET_RATES+1,x
    bne .lE601
    jsr.w CarpetSteadyY
    lda.w !CARPET_RATES+1,x
.lE601:
    bmi .take
.add:
    clc
    adc.b !actor_speed_y
    sta.b !actor_speed_y
    lda.b !actor_speed_y+1
    adc.b #$00
    sta.b !actor_speed_y+1
    lda.b !actor_speed_y+2
    adc.b #$00
    sta.b !actor_speed_y+2
    rts
.take:
    and.b #$7F
    sta.w !scratch+32
    lda.b !actor_speed_y
    sec
    sbc.w !scratch+32
    sta.b !actor_speed_y
    lda.b !actor_speed_y+1
    sbc.b #$00
    sta.b !actor_speed_y+1
    lda.b !actor_speed_y+2
    sbc.b #$00
    sta.b !actor_speed_y+2
.done:
    rts

; @routine 81:E62F CarpetCruiseXAlt
; WHAT IT DOES: With no direction held, steers the other ride back toward
; its cruising speed across, once each way.
; HOW: Once it has both sped up and slowed down to cruise: direction 10
; (neither). Else the speed (16-bit, from its fraction byte) below $0181:
; direction 9 and the sped-up flag; at or above: direction 8 and the
; slowed flag (the two ends of CarpetCruiseX).
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; A low = the direction set.
%at($81E62F)
CarpetCruiseXAlt:
    lda.b !CARPET_SPED_UP
    beq .lE637
    lda.b !CARPET_SLOWED
    bne CarpetCruiseX_neither
.lE637:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    cmp.w #$0181
    bcs CarpetCruiseX_slow
    bra CarpetCruiseX_faster

; @routine 81:E643 CarpetCruiseX
; WHAT IT DOES: With no direction held, steers the ride toward its
; cruising speed across.
; HOW: The speed (16-bit, from its fraction byte) below $0300: direction 9
; (speed up) and the sped-up flag set to 9; else direction 8 (slow down)
; and the slowed flag set to 8. After its end, direction 10 for
; CarpetCruiseXAlt.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; A low = the direction set; C from the comparison.
%at($81E643)
CarpetCruiseX:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    cmp.w #$0300
    bcs .slow
.faster:
    sep.b #$20
    lda.b #$09
    sta.b !CARPET_DIRECTION
    sta.b !CARPET_SPED_UP
    rts
.slow:
    sep.b #$20
    lda.b #$08
    sta.b !CARPET_DIRECTION
    sta.b !CARPET_SLOWED
    rts
.neither:
    lda.b #$0A
    sta.b !CARPET_DIRECTION
    rts

; @routine 81:E664 CarpetSteadyY
; WHAT IT DOES: Steadies the carpet up and down: steers against its
; vertical speed until it is small, then stops it.
; HOW: The vertical speed (16-bit, from its fraction byte): 7 or more
; down, direction 6 (up); below -8, direction 2 (down); between, the speed
; zeroed (all three bytes) and no direction ($FF).
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; A low = the direction set.
%at($81E664)
CarpetSteadyY:
    rep.b #$20
    lda.b !actor_speed_y
    beq .lE684
    bmi .lE678
    cmp.w #$0007
    bcc .lE684
    sep.b #$20
    lda.b #$06
    sta.b !CARPET_DIRECTION
    rts
.lE678:
    cmp.w #$FFF8
    bcs .lE684
    sep.b #$20
    lda.b #$02
    sta.b !CARPET_DIRECTION
    rts
.lE684:
    stz.b !actor_speed_y
    sep.b #$20
    stz.b !actor_speed_y+2
    lda.b #$FF
    sta.b !CARPET_DIRECTION
    rts

; @routine 81:E68F CarpetSpeedXLimitAlt
; WHAT IT DOES: Keeps the other ride's speed across between its least and
; most: $0020 and $0380 (an eighth of a pixel and three and a half pixels
; a frame).
; HOW: The speed's fraction byte and pixels as a 16-bit word, compared
; unsigned and set to the limit it passes.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; A = the speed word as limited, or as it was; C from the last
; comparison.
%at($81E68F)
CarpetSpeedXLimitAlt:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    cmp.w #$0380
    bcc .lE6A0
    lda.w #$0380
    sta.b !actor_speed_x
    bra .lE6AB
.lE6A0:
    sec
    cmp.w #$0020
    bcs .lE6AB
    lda.w #$0020
    sta.b !actor_speed_x
.lE6AB:
    sep.b #$20
    rts

; @routine 81:E6AE CarpetSpeedXLimit
; WHAT IT DOES: Keeps the ride's speed across between its least and most:
; $0020 and $0480 (an eighth of a pixel and four and a half pixels a
; frame).
; HOW: The speed's fraction byte and pixels as a 16-bit word, compared
; unsigned and set to the limit it passes.
; Entry: M 8-bit, X 8-bit; D = the carpet rider's record (the player's);
; DB = $80; called by JSR from the ride's loop (81:E49C). Exit: M 8-bit, X
; 8-bit; A = the speed word as limited, or as it was; C from the last
; comparison.
%at($81E6AE)
CarpetSpeedXLimit:
    rep.b #$20
    lda.b !actor_speed_x
    sec
    cmp.w #$0480
    bcc .lE6BF
    lda.w #$0480
    sta.b !actor_speed_x
    bra .lE6CA
.lE6BF:
    sec
    cmp.w #$0020
    bcs .lE6CA
    lda.w #$0020
    sta.b !actor_speed_x
.lE6CA:
    sep.b #$20
    rts
