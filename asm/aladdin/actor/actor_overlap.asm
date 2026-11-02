; actor_overlap.asm: an actor's own tests of what touches it: the records
; of the second free list overlapping its hit box, and the test box's
; centre inside a zone placed beside it by its facing.
; C: src/aladdin/actor/actor_overlap.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!SECOND_POOL = $09CB            ; the five records of the second free list, $50 apart
!SECOND_POOL_LEFT = $1490       ; the records left to test, less one (word)
; In the bank DB names: actors' half-width and half-height (words) by
; actor_06 (HIT_BOXES); the second pool's by the low 4 bits of its kind
; (SECOND_POOL_BOXES); a zone's half-width and height (bytes) by Y, the
; zone's offset added to $64 (ZONES).
!HIT_BOXES_OVERLAP = $C29A
!SECOND_POOL_BOXES = $C580
!ZONES = $BDA6
!ZONE_FIRST = $64
!TEST_BOX_X_OVERLAP = $1484     ; the test box's centre
!TEST_BOX_Y_OVERLAP = $1486
!TEST_BOX_HALF_W = $1488        ; the test box's half-width and half-height
!TEST_BOX_HALF_H = $148A

; ActorTouchesSecondPool's "none": C set, M 8-bit. It sits before the
; routine's entry.
%at($83856D)
SecondPoolNone:
    sep.b #$21
    rts

; @routine 83:8570 ActorTouchesSecondPoolLong
; WHAT IT DOES: ActorTouchesSecondPool for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorTouchesSecondPool; called by JSL. Exit: as the C states.
ActorTouchesSecondPoolLong:
    jsr.w ActorTouchesSecondPool
    rtl

; @routine 83:8578 ActorTouchesSecondPool
; WHAT IT DOES: Tells whether one of the five records of the second free
; list, in use and able to be hit, overlaps the actor's hit box; the
; actor tests none when its own byte at $08 has bit 7 set or it cannot be
; hit.
; HOW: The records from $09CB, $50 apart, state nonzero and bytes $08-$09
; & $4080 = $4000; each record's box (by its kind's low 4 bits) and the
; actor's (by ACTOR_06) give the sums of half-widths and of half-heights
; (scratch words $000C-$0013, the height's less 4), and the difference of
; positions plus a sum must be below its double, unsigned, x then y.
; Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM,
; the boxes' bank. Exit: as the C states.
%at($838578)
ActorTouchesSecondPool:
    lda.b !actor_08
    bmi SecondPoolNone
    bit.b !actor_09
    bvc SecondPoolNone
    rep.b #$30
    ldx.w #$0004
    stx.w !SECOND_POOL_LEFT
    ldy.w #!SECOND_POOL
.record:
    lda.w !actor_state,y
    and.w #$00FF
    beq .next
    lda.w !actor_08,y
    and.w #$4080
    cmp.w #$4000
    bne .next
    lda.w !actor_06,y
    and.w #$000F
    asl a
    asl a
    tax
    phy
    lda.b !actor_06
    and.w #$00FF
    asl a
    asl a
    tay
    lda.w !SECOND_POOL_BOXES,x
    clc
    adc.w !HIT_BOXES_OVERLAP,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    clc
    lda.w !SECOND_POOL_BOXES+2,x
    adc.w !HIT_BOXES_OVERLAP+2,y
    pha
    sec
    sbc.w #$0004
    sta.w !scratch+4
    pla
    asl a
    sta.w !scratch+6
    ply
    sec
    lda.b !actor_x+1
    sbc.w !actor_x+1,y
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .next
    sec
    lda.b !actor_y+1
    sbc.w !actor_y+1,y
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    bcc .hit
.next:
    clc
    tya
    adc.w #$0050
    tay
    dec.w !SECOND_POOL_LEFT
    bpl .record
    sep.b #$21
    rts
.hit:
    sep.b #$20
    rts

; @routine 83:87E8 ActorZoneHasTestPointLong
; WHAT IT DOES: ActorZoneHasTestPoint for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorZoneHasTestPoint; called by JSL. Exit: as the C states.
%at($8387E8)
ActorZoneHasTestPointLong:
    jsr.w ActorZoneHasTestPoint
    rtl

; @routine 83:87EC ActorZoneHasTestPoint
; WHAT IT DOES: Tells whether the test box's centre is inside zone Y
; beside the actor: a box placed by the actor's position and facing.
; HOW: $64 added to Y; $83:879B puts the zone's point (top, centre) in
; $000C and $000E; the centre's x less the point's, plus the half-width,
; must be below the width, and its y less the point's below the height,
; unsigned. The half-width, width and height are kept at $0010, $0012
; and $0016.
; Entry: M 8-bit, X 8-bit; Y = the zone's offset; D = the actor; DB a
; bank whose $0000-$1FFF is work RAM, the zones' bank. Exit: as the C states.
ActorZoneHasTestPoint:
    tya
    clc
    adc.b #!ZONE_FIRST
    tay
    jsr.w ActorZonePlace
    lda.w !ZONES,y
    and.w #$00FF
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    lda.w !ZONES+1,y
    and.w #$00FF
    sta.w !scratch+10
    clc
    lda.w !TEST_BOX_X_OVERLAP
    adc.w !scratch+4
    sec
    sbc.w !scratch
    bcc .out
    cmp.w !scratch+6
    bcs .out
    lda.w !TEST_BOX_Y_OVERLAP
    sec
    sbc.w !scratch+2
    bcc .out
    cmp.w !scratch+10
    sep.b #$30
    rts
.out:
    sep.b #$31
    rts

; @routine 83:8743 ActorZoneHurtsPlayer
; WHAT IT DOES: When the test box overlaps zone Y beside the actor, the
; player is hurt (PlayerEnter8CFacing).
; HOW: ActorZoneHasTestBox; C clear: PlayerEnter8CFacing, then C cleared.
; Entry: as ActorZoneHasTestBox. Exit: no overlap: as ActorZoneHasTestBox
; (C set). Hurt: C clear; the rest as PlayerEnter8CFacing leaves it.
%at($838743)
ActorZoneHurtsPlayer:
    jsr.w ActorZoneHasTestBox
    bcs .out
    jsr.w PlayerEnter8CFacing
    clc
    rts
.out:
    rts

; @routine 83:8752 ActorZoneHasTestBox
; WHAT IT DOES: Tells whether the test box overlaps zone Y beside the
; actor.
; HOW: ActorZonePlace; the zone's half-width plus the test box's ($1488)
; in $0010, doubled in $0012; its half-height plus the test box's ($148A)
; in $0014, doubled in $0016; the point's x less the test box's centre,
; plus the half-width sum, must be below the doubled sum, unsigned, then
; the same for y.
; Entry: M 8-bit; Y = the zone's offset; D = the actor; DB a bank whose
; $0000-$1FFF is work RAM, the zones' bank; called by JSR. Exit: C clear
; when they overlap; A = the last sum (16 bits), N and Z from the compare
; that decided, V from its addition; M 8-bit; X, Y unchanged.
%at($838752)
ActorZoneHasTestBox:
    jsr.w ActorZonePlace
    lda.w !ZONES,y
    and.w #$00FF
    clc
    adc.w !TEST_BOX_HALF_W
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    lda.w !ZONES+1,y
    and.w #$00FF
    clc
    adc.w !TEST_BOX_HALF_H
    sta.w !scratch+8
    asl a
    sta.w !scratch+10
    sec
    lda.w !scratch
    sbc.w !TEST_BOX_X_OVERLAP
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    bcs .out
    sec
    lda.w !scratch+2
    sbc.w !TEST_BOX_Y_OVERLAP
    clc
    adc.w !scratch+8
    cmp.w !scratch+10
.out:
    sep.b #$20
    rts

; @routine 83:879B ActorZonePlace
; WHAT IT DOES: Places zone Y beside the actor: the zone's point, ahead of
; the actor by its facing, in $000C (x) and $000E (y).
; HOW: The zone's signed x offset (byte 2 of its record) is added to the
; actor's x pixels when its flip byte is zero and taken from them
; otherwise; its signed y offset (byte 3) is added to its y pixels.
; Entry: M 8-bit; Y = the zone's offset (the zones' table index); D = the
; actor; DB a bank whose $0000-$1FFF is work RAM, the zones' bank; called
; by JSR. Exit: M 16-bit; A = the point's y; N, V, Z, C from its addition;
; X, Y unchanged.
%at($83879B)
ActorZonePlace:
    lda.b !actor_flip
    bne .flipped
    rep.b #$20
    lda.w !ZONES+2,y
    and.w #$00FF
    bit.w #$0080
    beq .ahead
    ora.w #$FF00
.ahead:
    clc
    adc.b !actor_x+1
    sta.w !scratch
    bra .y
.flipped:
    rep.b #$20
    lda.w !ZONES+2,y
    and.w #$00FF
    bit.w #$0080
    beq .behind
    ora.w #$FF00
.behind:
    sta.w !scratch
    sec
    lda.b !actor_x+1
    sbc.w !scratch
    sta.w !scratch
.y:
    lda.w !ZONES+3,y
    and.w #$00FF
    bit.w #$0080
    beq .down
    ora.w #$FF00
.down:
    clc
    adc.b !actor_y+1
    sta.w !scratch+2
    rts
