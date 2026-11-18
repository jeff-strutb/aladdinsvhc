; guard_moves.asm: the bank $A5 guard's (actor kind $08) moves: the pieces
; its code word sends it to (stop, run, jump, give up, walk), the run's
; throw of up to three kind $90 children, its climb of a wall and its
; landing, and the small helpers they use.
; C: src/aladdin/actor/guard_moves.c.held (held until the checker can judge an
; actor loop's RTI to a rewritten code word).

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_player.inc"

arch 65816

!G_POINT_X = !scratch           ; the probe point PointInGround tests: x (word)
!G_POINT_Y = !scratch+2         ; its y (word)
!G_TILE = !scratch+6            ; PointInGround's tile behaviour: bits 0-2 its kind of ground, bit 3 a slope's side, bit 5 an edge, bit 6 a ledge
!G_MOVING = $41                 ; counted up while a move is under way, cleared when it ends
!G_LANDING = $42                ; frames left to settle after landing
!G_IN_AIR = $44                 ; set for the frame it leaves the ground
!G_BACK_OFF = $46               ; nonzero: a wall it cannot climb turns it back
!G_SLOTS = $1478                ; the three kind $90 children's slots (by their $07)
!G_PRESETS_FROM = $7A           ; the motion preset for a throw seen by slot 0 (slots 1, 2: $7B, $7C)
!G_PRESET_NONE = $7D            ; the motion preset with no throw seen
!G_STEP_AHEAD_X = $CFF4         ; (data bank) by speed and facing: x of the point ahead (word)
!G_STEP_AHEAD_Y = $D010         ; (data bank) by speed and facing: y of the point ahead (word)
!G_HIDE_A = $141C               ; nonzero: the guard waits before it comes back
!G_HIDE_B = $1427
!G_FALL_MAX = $0580             ; the fastest it falls
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)

; @routine A5:E5B6 GuardStop
; @actor
; WHAT IT DOES: The guard stopped (its code word $E5B6): each frame on the
; ground, it shows the pose for facing the player, facing away, or
; standing at an edge.
; HOW: Each frame (BRK): ActorSnapToGround; G_TILE bit 5 set: animation
; $16; else (16-bit X) its x against the player's, the carry against
; its flip: equal, animation 0; else 6 (ActorAnimChange).
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
; the actor loop at its code word. Exit: never returns.
%at($A5E5B6)
GuardStop:
    brk.b #$00
    jsl.l ActorSnapToGround
    lda.w !G_TILE
    bit.b #$20
    bne .edge
    rep.b #$10
    ldx.b !actor_x+1
    cpx.w !player_x
    sep.b #$10
    lda.b #$00
    rol a
    eor.b !actor_flip
    bne .away
    lda.b #$00
    jsr.w ActorAnimChange
    bra GuardStop
.away:
    lda.b #$06
    jsr.w ActorAnimChange
    bra GuardStop
.edge:
    lda.b #$16
    jsr.w ActorAnimChange
    bra GuardStop

; @routine A5:E5E8 GuardRun
; @actor
; WHAT IT DOES: The guard running (its code word $E5E8): it runs along
; the ground, posed by the ground under it; at a wall it climbs; where
; the ground ahead rises it jumps; off an edge it drops, and at a ledge
; below it throws up to three kind $90 children to find its way, then
; leaps by what they report.
; HOW: ActorSpeedXFromA5 9, animation 1. Each frame (BRK): ActorMoveX;
; ActorOutOfWall8 C set: on at GuardClimb; ActorSnapToGround zero: off
; the edge; else ActorAnimByTile and GuardStepAhead. Off the edge:
; AnimStart $0A, G_IN_AIR up, BRK, G_IN_AIR 0, G_MOVING up;
; ActorLedgeAhead C clear: preset $80. Else BRK; the spawn place and
; G_SLOTS cleared; up to three ActorSpawn of kind $90 with $07 0, 1, 2,
; each slot counted up. Each frame (BRK) until a slot has a high nibble
; (or all are 0: preset G_PRESET_NONE): low nibbles cleared, preset
; G_PRESETS_FROM + the first slot set. ActorMotionPreset, and on in the
; air (GuardJump after its start).
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
; the actor loop at its code word. Exit: never returns.
%at($A5E5E8)
GuardRun:
    ldy.b #$09
    jsr.w ActorSpeedXFromA5
    lda.b #$01
    jsr.w ActorAnimChange
.step:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w ActorOutOfWall8
    bcs .wall
    jsl.l ActorSnapToGround
    beq .off
    jsr.w ActorAnimByTile
    jsr.w GuardStepAhead
    bra .step
.wall:
    jmp.w GuardClimb
.off:
    lda.b #$0A
    jsl.l AnimStart
    inc.b !G_IN_AIR
    brk.b #$00

; @routine A5:E618 GuardRunDown
; @actor
; WHAT IT DOES: The guard off an edge (GuardRun's drop, also GuardFlip's):
; with no ledge below, it drops with preset $80; at a ledge it throws up to
; three kind $90 children to find its way and leaps by what they report.
; HOW: G_IN_AIR 0, G_MOVING up; ActorLedgeAhead C clear: Y = $80, on at
; the preset. Else BRK; the spawn place and G_SLOTS cleared; up to three
; ActorSpawn of kind $90 with $07 0, 1, 2, each slot counted up. Each
; frame (BRK) until a slot has a high nibble (or all are 0: Y =
; G_PRESET_NONE): low nibbles cleared, Y = G_PRESETS_FROM + the first slot
; set. ActorMotionPreset, and on in the air (GuardJump after its start).
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached from
; GuardRun's fall off an edge and by GuardFlip's jump. Exit: never
; returns.
%at($A5E618)
GuardRunDown:
    stz.b !G_IN_AIR
    inc.b !G_MOVING
    jsr.w ActorLedgeAhead
    bcs .throw
    ldy.b #$80
    jmp.w .preset
.throw:
    brk.b #$00
    stz.b !actor_spawn_at
    stz.w !G_SLOTS
    stz.w !G_SLOTS+1
    stz.w !G_SLOTS+2
    lda.b #$90
    jsl.l ActorSpawn
    bmi .thrown
    lda.b #$00
    sta.w !actor_07,x
    inc.w !G_SLOTS
    sep.b #$10
    lda.b #$90
    jsl.l ActorSpawn
    bmi .thrown
    lda.b #$01
    sta.w !actor_07,x
    inc.w !G_SLOTS+1
    sep.b #$10
    lda.b #$90
    jsl.l ActorSpawn
    bmi .thrown
    lda.b #$02
    sta.w !actor_07,x
    inc.w !G_SLOTS+2
    sep.b #$10
.thrown:
    brk.b #$00
    lda.w !G_SLOTS
    ora.w !G_SLOTS+1
    ora.w !G_SLOTS+2
    beq .none
    and.b #$F0
    beq .thrown
    lda.b #$0F
    trb.w !G_SLOTS
    trb.w !G_SLOTS+1
    trb.w !G_SLOTS+2
    ldy.b #!G_PRESETS_FROM
    lda.w !G_SLOTS
    bne .preset
    iny
    lda.w !G_SLOTS+1
    bne .preset
    iny
    lda.w !G_SLOTS+2
    bne .preset
.none:
    ldy.b #!G_PRESET_NONE
.preset:
    jsl.l ActorMotionPreset
    bra GuardJump_air

; @routine A5:E6A0 GuardJump
; @actor
; WHAT IT DOES: The guard in the air (its code word $E6A0): it flies with
; gravity, posed rising or falling; landing on a slope it turns down it
; and lands; at a wall it climbs it (or turns back, or lands on a ledge
; it can stand on); on the ground it settles for up to 10 frames, until
; the player is within 48, and stops.
; HOW: G_MOVING up; gravity $38. Each frame (BRK): ActorAnimRiseFall,
; ActorFly. Falling, ActorSettleOnFloor nonzero and G_TILE bit 3: facing
; by its bit 7 (flip), G_MOVING 0, on at GuardLand. ActorOutOfWall8 C
; set: G_TILE bit 3 set: as a slope's side below; bit 6 clear: G_BACK_OFF
; set: GuardFlip, else GuardClimb. Else GroundBehindBit3 nonzero: G_TILE
; bit 7 against its flip: equal, GuardLand. Rising, or ActorOutOfGround
; zero: again. Landed: G_LANDING $0A, animation $0A, ActorSpeedXClear;
; each frame ActorSnapToGround, BRK, until the player is within 48 or
; G_LANDING runs out; G_MOVING 0, on at GuardStop.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
; the actor loop at its code word; also reached by jumps from the
; guard's other moves. Exit: never returns.
%at($A5E6A0)
GuardJump:
    inc.b !G_MOVING
    lda.b #$38
    sta.b !actor_accel_y
.air:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bpl .wall_test
    jsl.l ActorSettleOnFloor
    beq .wall_test
    lda.w !G_TILE
    bit.b #$08
    beq .wall_test
    rol a
    lda.b #$00
    rol a
    sta.b !actor_flip
    sta.b !actor_flip
    stz.b !G_MOVING
    jmp.w GuardLand
.wall_test:
    jsr.w ActorOutOfWall8
    bcc .behind
    lda.w !G_TILE
    bit.b #$08
    bne .slope
    bit.b #$40
    bne .behind
    lda.b !G_BACK_OFF
    bne .back
    stz.b !G_MOVING
    jmp.w GuardClimb
.back:
    jmp.w GuardFlip
.slope:
    lda.w !G_TILE
    rol a
    lda.b #$00
    rol a
    eor.b !actor_flip
    bne .fall
    stz.b !G_MOVING
    jmp.w GuardLand
.behind:
    jsl.l GroundBehindBit3
    bne .slope
.fall:
    lda.b !actor_speed_y+2
    bmi .again
    jsl.l ActorOutOfGround
    bne .landed
.again:
    jmp.w .air
.landed:
    lda.b #$0A
    sta.b !G_LANDING
    lda.b #$0A
    jsr.w ActorAnimChange
    jsl.l ActorSpeedXClear
.settle:
    jsl.l ActorSnapToGround
    brk.b #$00
    rep.b #$20
    sec
    lda.w !player_x
    sbc.b !actor_x+1
    clc
    adc.w #$0030
    cmp.w #$0060
    sep.b #$20
    bcs .done
    dec.b !G_LANDING
    bne .settle
.done:
    stz.b !G_MOVING
    jmp.w GuardStop

; @routine A5:E73B GuardClimb
; @actor
; WHAT IT DOES: The guard's climb of a wall: up it while the wall goes
; on; at its top, over it (GuardJump); where the point above is solid,
; it turns back (GuardFlip).
; HOW: G_MOVING up; animation $0B; ActorSpeedXFromA5 0, ActorSpeedYFromA5
; 0; each frame (BRK) ActorMoveY, ActorMoveX, ActorOutOfWallProbe4: C
; clear, over ($A5:E785: G_MOVING 0, ActorSpeedXFromA5 3,
; ActorSpeedYFromA5 3, on at GuardJump); else the point 24 above its x
; (G_POINT_X, G_POINT_Y), PointInGroundLong C set: on at GuardFlip.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
; jumps from GuardRun and GuardJump. Exit: never returns.
%at($A5E73B)
GuardClimb:
    inc.b !G_MOVING
    lda.b #$0B
    jsr.w ActorAnimChange
    ldy.b #$00
    jsr.w ActorSpeedXFromA5
    ldy.b #$00
    jsr.w ActorSpeedYFromA5
.up:
    brk.b #$00
    jsl.l ActorMoveY
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    bcc .over
    rep.b #$20
    lda.b !actor_x+1
    sta.w !G_POINT_X
    clc
    lda.b !actor_y+1
    adc.w #$FFE8
    sta.w !G_POINT_Y
    jsl.l PointInGroundLong
    bcs GuardFlip
    bra .up

; @routine A5:E774 GuardFlip
; @actor
; WHAT IT DOES: The guard turned back: facing and flip swapped, animation
; 8, and down off the edge (GuardRunDown).
; HOW: ACTOR_FACING EOR 1 to the facing and flip; AnimStart 8; on at
; GuardRunDown.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
; jumps from GuardJump and GuardClimb. Exit: never returns.
%at($A5E774)
GuardFlip:
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$08
    jsl.l AnimStart
    jmp.w GuardRunDown

GuardClimb_over:
    stz.b !G_MOVING
    ldy.b #$03
    jsr.w ActorSpeedXFromA5
    ldy.b #$03
    jsr.w ActorSpeedYFromA5
    jmp.w GuardJump

; @routine A5:E794 GuardGiveUp
; @actor
; WHAT IT DOES: The guard giving up the chase (its code word $E794): it
; drops out of sight and can't be hit; once nothing holds it back it is
; put where the player is, and once it can be hit again it comes back
; into view facing him and leaps.
; HOW: ACTOR_09 bit 7 and ACTOR_08 bit 3 cleared; BRK while G_HIDE_A or
; G_HIDE_B; its x and y = the player's; BRK while ACTOR_09 bit 6 is set;
; ACTOR_08 bit 3 and ACTOR_09 bit 7 set; facing and flip toward the
; player; G_MOVING 0; ActorSpeedXFromA5 3, ActorSpeedYFromA5 3; on at
; GuardJump.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
; the actor loop at its code word. Exit: never returns.
%at($A5E794)
GuardGiveUp:
    lda.b #$80
    trb.b !actor_09
    lda.b #$08
    trb.b !actor_08
.hold:
    lda.w !G_HIDE_A
    ora.w !G_HIDE_B
    beq .there
    brk.b #$00
    bra .hold
.there:
    lda.w !player+!actor_x+1
    sta.b !actor_x+1
    lda.w !player+!actor_x+2
    sta.b !actor_x+2
    lda.w !player+!actor_y+1
    sta.b !actor_y+1
    lda.w !player+!actor_y+2
    sta.b !actor_y+2
.hidden:
    brk.b #$00
    lda.b !actor_09
    bit.b #$40
    bne .hidden
    lda.b #$08
    tsb.b !actor_08
    lda.b #$80
    tsb.b !actor_09
    rep.b #$20
    lda.b !actor_x+1
    cmp.w !player_x
    sep.b #$20
    lda.b #$00
    rol a
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !G_MOVING
    ldy.b #$03
    jsr.w ActorSpeedXFromA5
    ldy.b #$03
    jsr.w ActorSpeedYFromA5
    jmp.w GuardJump

; @routine A5:E7EB GuardLand
; @actor
; WHAT IT DOES: The guard landed on a slope or ledge: set at the tile's
; centre, pose 7 for 32 frames, its speed cleared and a small hop
; (ActorSpeedYFromA5 6), then stopped.
; HOW: G_MOVING up; GuardSnapToTile; ActorAnimChange 7; COP $20;
; ActorSpeedXClear; ActorSpeedYFromA5 6; G_MOVING 0; on at GuardStop.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; reached by
; jumps from GuardJump. Exit: never returns.
%at($A5E7EB)
GuardLand:
    inc.b !G_MOVING
    jsr.w GuardSnapToTile
    lda.b #$07
    jsr.w ActorAnimChange
    lda.b #$20
    cop.b #$00
    jsl.l ActorSpeedXClear
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
    stz.b !G_MOVING
    jmp.w GuardStop

; @routine A5:E807 GuardWalk
; @actor
; WHAT IT DOES: The guard once the player has no health left (its code
; word $E807): it walks in place for good.
; HOW: G_MOVING up; animation 4; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; resumed by
; the actor loop at its code word. Exit: never returns.
%at($A5E807)
GuardWalk:
    inc.b !G_MOVING
    lda.b #$04
    jsr.w ActorAnimChange
.idle:
    brk.b #$00
    bra .idle
assert pc() <= $A5E812

; @routine A5:EAB3 ActorFlyFallCapped
; WHAT IT DOES: ActorFly, with the fall held to G_FALL_MAX at most.
; HOW: ActorFly; falling (the speed's top byte not negative): 16-bit, the
; speed's low word G_FALL_MAX at most.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; M 8-bit.
%at($A5EAB3)
ActorFlyFallCapped:
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .out
    rep.b #$20
    lda.w #!G_FALL_MAX
    cmp.b !actor_speed_y
    bcs .slow
    sta.b !actor_speed_y
.slow:
    sep.b #$20
.out:
    rts

; @routine A5:EAC9 GuardSnapToTile
; WHAT IT DOES: Sets the guard at the centre of the tile the last probe
; found: its x at the tile's middle, its y 8 above the tile's top.
; HOW: 16-bit: x = (G_POINT_X AND $FFF0) + 8; y = (G_POINT_Y AND $FFF0)
; - 8.
; Entry: M 8-bit; D = the guard's record; called by JSR after a probe
; (PointInGround). Exit: M 8-bit.
%at($A5EAC9)
GuardSnapToTile:
    rep.b #$20
    lda.w !G_POINT_X
    and.w #$FFF0
    clc
    adc.w #$0008
    sta.b !actor_x+1
    lda.w !G_POINT_Y
    and.w #$FFF0
    sec
    sbc.w #$0008
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine A5:EAE6 GuardStepAhead
; WHAT IT DOES: The running guard's look ahead: where the ground rises in
; front of it, it jumps (it leaves its caller and goes to GuardJump).
; HOW: X = ((speed's middle byte * 2) EOR facing) * 2; the point = its x
; + G_STEP_AHEAD_X, its y + G_STEP_AHEAD_Y (16-bit) by X;
; PointInGroundLong; G_TILE's kind 0: RTS. Else its JSR's return pulled,
; ActorSpeedYFromA5 9, on at GuardJump.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB = $A5; called by JSR
; from GuardRun. Exit: M 8-bit (no ground ahead); else never returns.
%at($A5EAE6)
GuardStepAhead:
    lda.b !actor_speed_x+1
    asl a
    eor.b !actor_facing
    asl a
    tax
    rep.b #$20
    clc
    lda.w !G_STEP_AHEAD_X,x
    adc.b !actor_x+1
    sta.w !G_POINT_X
    clc
    lda.w !G_STEP_AHEAD_Y,x
    adc.b !actor_y+1
    sta.w !G_POINT_Y
    jsl.l PointInGroundLong
    lda.w !G_TILE
    and.b #$07
    bne .rise
    rts
.rise:
    pla
    pla
    ldy.b #$09
    jsr.w ActorSpeedYFromA5
    jmp.w GuardJump
assert pc() <= $A5EB17
