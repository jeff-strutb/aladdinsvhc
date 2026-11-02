; course_platform.asm: pieces of the platform of kind $47 (its start at
; $83:C96E, its wait routine at $83:CAD1), which flies a course the player
; rides: the course's waypoints as it is worked out, the stretches where
; it turns, the dip it makes under the player's weight and its phases,
; how far it moved in a frame, and the platforms started along the course
; and kept within its live stretch.
; C: src/aladdin/actor/course_platform.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

; The platform's own fields of its record.
!COURSE_STEPS = $22             ; steps left before the next waypoint is kept
!COURSE_END = $24               ; offset of the next waypoint in the course (word)
!COURSE_SPEED = $20             ; its speed along the course
!DIP_PHASE = $13                ; what its dip is doing: an entry of the table at $83:CC34
!LAST_X = $19                   ; x pixels at the frame's start (word)
!LAST_Y = $1D                   ; y pixels at the frame's start (word)
!MOVED_X = $45                  ; x pixels moved in the frame (word)
!MOVED_Y = $47                  ; y pixels moved in the frame (word)
!RIDDEN = $49                   ; nonzero: the player stands on it this frame
!DIP = $4A                      ; how far it has sunk under the player: 24-bit, fraction first
!DIP_SOUND = $17                ; the sound its landing plays
!BASE_Y = $4D                   ; its y without the dip: 24-bit, fraction first
!COURSE_AT = $37                ; the offset of its waypoint in the course; the course maker's: where the next platform starts (word)
!LAST_DIRECTION = $1C           ; the direction it last took ($FF: none)
!JUMPED = $16                   ; nonzero: it was moved along the course this frame

; The course's waypoints, 8 bytes each: x and y (24-bit, fraction first),
; the direction and the speed.
!COURSE = $7F5000
; In the bank DB names: the number of turn stretches, then 6 bytes each:
; the first x pixel and the x pixel past it (words).
!TURN_ZONE_COUNT = $D2F7
!TURN_ZONES = $D2F8
; In the bank DB names: the rates (24-bit) it sinks and rises at.
!DIP_RATES = $D2ED
; In the bank DB names: the dip phases' routines (words), by DIP_PHASE.
!DIP_PHASES = $CC34
; In the bank DB names: per direction, the 6-byte step of a course
; move (x, then y, 24-bit), 8 bytes apart; and the platform's resume
; address for the direction (words).
!COURSE_STEPS_BY_DIRECTION = $D304
!RESUME_BY_DIRECTION = $D312
; The waypoint the course is live around (the platforms are kept within
; 12 waypoints of it), and an x the platforms are moved forward below.
!COURSE_LIVE = $0400
!v_1a44 = $1A44
; Not yet filed: the check of the platform's rider.
!RIDER_CHECK = $83CCF1

; @routine 83:CA11 PlatformSpawnOnCourse
; WHAT IT DOES: Starts one more platform on the course: a child of kind
; $47 at the waypoint the course maker's offset ($37) names; the next is
; six waypoints on.
; HOW: ActorCreateChild with kind $47; the child's x and y (6 bytes) from
; the waypoint, its own $37 the waypoint's offset; the maker's offset
; moves on $30.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; D = the
; course maker; called by JSR, four times, once the course is worked out.
; Exit: M 8-bit, X 8-bit; Y = the child (low byte); A low = the new
; offset's low byte.
%at($83CA11)
PlatformSpawnOnCourse:
    lda.b #$47
    jsl.l ActorCreateChild
    rep.b #$20
    txy
    ldx.b !COURSE_AT
    clc
    lda.l !COURSE,x
    sta.w !actor_x,y
    lda.l !COURSE+2,x
    sta.w !actor_x+2,y
    lda.l !COURSE+4,x
    sta.w !actor_y+1,y
    txa
    sta.w !COURSE_AT,y
    clc
    adc.w #$0030
    sta.b !COURSE_AT
    sep.b #$30
    rts

; @routine 83:CA3F PlatformCourseRecord
; WHAT IT DOES: Keeps a waypoint of the platform's course every 38 steps
; while the course is worked out: its position, direction and speed.
; HOW: The step count goes down; at 0 it starts again at 38 and the 8
; bytes go to the course at the end offset, which moves on by 8.
; Entry: M 8-bit, X 8-bit; D = the platform's record. Exit: no waypoint:
; N and Z from the count; A, X, Y and the other flags unchanged. A
; waypoint: A low = the speed, A high = the new end offset's high byte; X
; = the waypoint's offset (its low byte); N and Z from the speed; V and C
; from the offset's addition. M and X 8-bit; Y unchanged.
%at($83CA3F)
PlatformCourseRecord:
    dec.b !COURSE_STEPS
    bne .done
    lda.b #$26
    sta.b !COURSE_STEPS
    rep.b #$30
    ldx.b !COURSE_END
    lda.b !actor_x
    sta.l !COURSE,x
    lda.b !actor_x+2
    sta.l !COURSE+2,x
    lda.b !actor_y+1
    sta.l !COURSE+4,x
    clc
    txa
    adc.w #$0008
    sta.b !COURSE_END
    sep.b #$20
    lda.b !actor_facing
    sta.l !COURSE+6,x
    lda.b !COURSE_SPEED
    sta.l !COURSE+7,x
    sep.b #$10
.done:
    rts

; @routine 83:CAC1 PlatformDipStart
; WHAT IT DOES: Starts the platform's dip as the player lands on it: sound
; A kept for it, dip phase 1, the dip back to 0.
; HOW: A to $17, the phase to 1, then on into PlatformDipClear.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; A = the
; sound command; called by JSR. Exit: as PlatformDipClear; A low = 1.
%at($83CAC1)
PlatformDipStart:
    sta.b !DIP_SOUND
    lda.b #$01
    sta.b !DIP_PHASE

; @routine 83:CAC7 PlatformDipClear
; WHAT IT DOES: Takes away the platform's dip: it is back at its height.
; HOW: The three bytes of the dip are zeroed.
; Entry: M 8-bit; D = the platform's record. Exit: registers and flags
; unchanged.
%at($83CAC7)
PlatformDipClear:
    stz.b !DIP
    stz.b !DIP+1
    stz.b !DIP+2
    rts

; @routine 83:CAD1 PlatformWait
; WHAT IT DOES: The course platform's wait routine, each frame: its dip,
; how far it moved, its rider, a direction flip when its $42 is set, the
; course followed, and its animation by direction.
; HOW: JUMPED cleared; PlatformDipUpdate; PlatformMoveMeasure; the rider
; check at 83:CCF1; $42 set: cleared, $40 bit 0 flipped.
; PlatformCourseFollow; the direction ($40) unlike LAST_DIRECTION: kept,
; AnimStart with it. AnimStep.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR from the actor loop (83:94D0) by kind $47. Exit: as AnimStep.
%at($83CAD1)
PlatformWait:
    stz.b !JUMPED
    jsr.w PlatformDipUpdate
    jsr.w PlatformMoveMeasure
    jsr.w !RIDER_CHECK
    lda.b !actor_42
    beq .lCAE8
    stz.b !actor_42
    lda.b !actor_40
    eor.b #$01
    sta.b !actor_40
.lCAE8:
    jsr.w PlatformCourseFollow
    lda.b !actor_40
    cmp.b !LAST_DIRECTION
    beq .lCAF7
    sta.b !LAST_DIRECTION
    jsl.l AnimStart
.lCAF7:
    jsl.l AnimStep
    rts

; @routine 83:CAFC PlatformCourseSlower
; WHAT IT DOES: A step of the course while it is worked out, slowing: the
; course speed down by 1 (not below $20), the platform moved a step, and
; whether it is in a turn stretch.
; HOW: The speed less 1, held at $20 at least; PlatformCourseStep;
; PlatformTurnZoneTest.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
%at($83CAFC)
PlatformCourseSlower:
    sec
    lda.b !COURSE_SPEED
    sbc.b #$01
    cmp.b #$20
    bcs .lCB07
    lda.b #$20
.lCB07:
    sta.b !COURSE_SPEED
    jsr.w PlatformCourseStep
    jsr.w PlatformTurnZoneTest
    rts

; @routine 83:CB10 PlatformCourseFaster
; WHAT IT DOES: A step of the course while it is worked out, speeding: the
; course speed up by 1 (not above $80), the platform moved a step, and
; whether it is in a turn stretch.
; HOW: The speed plus 1, held at $80 at most; PlatformCourseStep;
; PlatformTurnZoneTest.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR. Exit: as PlatformTurnZoneTest: C clear in a turn stretch.
%at($83CB10)
PlatformCourseFaster:
    clc
    lda.b !COURSE_SPEED
    adc.b #$01
    cmp.b #$80
    bcc .lCB1B
    lda.b #$80
.lCB1B:
    sta.b !COURSE_SPEED
    jsr.w PlatformCourseStep
    jsr.w PlatformTurnZoneTest
    rts

; @routine 83:CB24 PlatformCourseStep
; WHAT IT DOES: Moves the platform a step along its direction at its
; course speed, while the course is worked out; the step's y goes to its
; height without the dip.
; HOW: The height without the dip ($4D-$4F) put in its y; the direction's
; 6-byte step (8 bytes a direction at $D304) to $000C-$0012;
; ActorMoveScaled with the course speed; the new y kept back at $4D-$4F.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR (by the slowing and speeding steps at 83:CAFC and 83:CB10). Exit: M
; 8-bit, X 8-bit (as ActorMoveScaled); A low = the y's top byte.
%at($83CB24)
PlatformCourseStep:
    lda.b !BASE_Y
    sta.b !actor_y
    lda.b !BASE_Y+1
    sta.b !actor_y+1
    lda.b !BASE_Y+2
    sta.b !actor_y+2
    lda.b !actor_facing
    asl a
    asl a
    asl a
    tax
    lda.w !COURSE_STEPS_BY_DIRECTION,x
    sta.w !scratch
    lda.w !COURSE_STEPS_BY_DIRECTION+1,x
    sta.w !scratch+1
    lda.w !COURSE_STEPS_BY_DIRECTION+2,x
    sta.w !scratch+2
    lda.w !COURSE_STEPS_BY_DIRECTION+3,x
    sta.w !scratch+4
    lda.w !COURSE_STEPS_BY_DIRECTION+4,x
    sta.w !scratch+5
    lda.w !COURSE_STEPS_BY_DIRECTION+5,x
    sta.w !scratch+6
    lda.b !COURSE_SPEED
    jsl.l ActorMoveScaled
    lda.b !actor_y
    sta.b !BASE_Y
    lda.b !actor_y+1
    sta.b !BASE_Y+1
    lda.b !actor_y+2
    sta.b !BASE_Y+2
    rts

; @routine 83:CB6D PlatformTurnZoneTest
; WHAT IT DOES: Tells whether the platform is in a stretch of the course
; where it turns: C clear when its x pixel is in one.
; HOW: The stretches, in order of x, each compared unsigned: below a
; stretch's first pixel, or past the last stretch, is out (C set); below
; the pixel past it is in (C clear).
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
; bank. Exit: A = its x pixel (16 bits); X = 6 times the stretch reached;
; Y = the stretches left from it (the count less its number); N and Z
; from the last compare (from the count when the stretches ran out). In:
; C clear. Out: C set. M 8-bit, X 8-bit; V unchanged.
%at($83CB6D)
PlatformTurnZoneTest:
    rep.b #$20
    ldy.w !TURN_ZONE_COUNT
    ldx.b #$00
    lda.b !actor_x+1
.zone:
    cmp.w !TURN_ZONES,x
    bcc .out
    cmp.w !TURN_ZONES+2,x
    bcc .in
    inx
    inx
    inx
    inx
    inx
    inx
    dey
    bne .zone
.out:
    sep.b #$21
    rts
.in:
    sep.b #$20
    rts

; @routine 83:CB93 PlatformCourseFollow
; WHAT IT DOES: Follows the course: on to the next waypoint once the
; platform passes its x; and when the platform is out of the stretch the
; course keeps live, moves it 24 waypoints on or back to the course's
; other end, with that waypoint's direction and speed.
; HOW: Its x pixels against the next waypoint's: past it, its offset ($37)
; moves on 8. The stretch: 12 waypoints either side of the live one
; (COURSE_LIVE, $0400); outside it, $C0 added (x below $1A44) or taken
; away; a negative offset leaves it; else x, y and height without the dip
; from the waypoint, its speed, the direction (to $40 and its facing), its
; resume address from the direction's entry at $D312, $1C = $FF, $42 and
; the dip phase zeroed, PlatformDipClear, $16 counted up. Its early
; return, SEP #$30 and RTS, lies before its entry, at 83:CB90.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit, X 8-bit;
; A, X and Y as left by the last step taken.
%at($83CB90)
PlatformCourseFollowEnd:
    sep.b #$30
    rts
PlatformCourseFollow:
    rep.b #$30
    ldx.b !COURSE_AT
    lda.b !actor_x+1
    cmp.l !COURSE+9,x
    bcc PlatformCourseFollowEnd
    clc
    txa
    adc.w #$0008
    sta.b !COURSE_AT
    lda.w !COURSE_LIVE
    asl a
    asl a
    asl a
    tax
    lda.b !actor_x+1
    cmp.l !COURSE-96+1,x
    bcc .lCBBB
    cmp.l !COURSE+96+1,x
    bcc .lCC15
.lCBBB:
    rep.b #$30
    lda.w #$00C0
    ldy.b !actor_x+1
    cpy.w !v_1a44
    bcc .lCBCA
    lda.w #$FF40
.lCBCA:
    clc
    adc.b !COURSE_AT
    bmi .lCC15
    sta.b !COURSE_AT
    tax
    lda.l !COURSE,x
    sta.b !actor_x
    lda.l !COURSE+2,x
    sta.b !actor_x+2
    lda.l !COURSE+4,x
    sta.b !actor_y+1
    sta.b !BASE_Y+1
    sep.b #$20
    lda.b !actor_y
    sta.b !BASE_Y
    lda.l !COURSE+7,x
    sta.b !COURSE_SPEED
    lda.l !COURSE+6,x
    sep.b #$10
    sta.b !actor_40
    sta.b !actor_facing
    asl a
    tax
    lda.w !RESUME_BY_DIRECTION,x
    sta.b !actor_03
    lda.w !RESUME_BY_DIRECTION+1,x
    sta.b !actor_04
    lda.b #$FF
    sta.b !LAST_DIRECTION
    stz.b !actor_42
    stz.b !DIP_PHASE
    jsr.w PlatformDipClear
    inc.b !JUMPED
.lCC15:
    sep.b #$30
    rts

; @routine 83:CC18 PlatformDipIdle
; WHAT IT DOES: The platform's dip phase 0: nothing to do (no one has
; stepped on it).
; HOW: RTS.
; Entry: any; called by JSR through the dip phase's table. Exit:
; registers and flags unchanged.
%at($83CC18)
PlatformDipIdle:
    rts

; @routine 83:CC19 PlatformDipUpdate
; WHAT IT DOES: Runs the platform's dip for the frame: its phase's
; routine, then its y set to its height without the dip plus the dip.
; HOW: JSR through the dip phase table at $83:CC34 by the phase; then the
; 24-bit sum of BASE_Y and DIP, a byte at a time, to its y.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR from the platform's wait routine (83:CAD1). Exit: M 8-bit; A low =
; the y's top byte; X as the phase routine left it; flags from the last
; add.
%at($83CC19)
PlatformDipUpdate:
    lda.b !DIP_PHASE
    asl a
    tax
    jsr.w (!DIP_PHASES,x)
    clc
    lda.b !BASE_Y
    adc.b !DIP
    sta.b !actor_y
    lda.b !BASE_Y+1
    adc.b !DIP+1
    sta.b !actor_y+1
    lda.b !BASE_Y+2
    adc.b !DIP+2
    sta.b !actor_y+2
    rts

; @routine 83:CC42 PlatformDipLand
; WHAT IT DOES: Dip phase 1: the platform is landed on: starts a child
; (hit box $4B) that keeps the platform as its $37, plays the landing's
; sound when the platform can be hit, and moves on to the next phase.
; HOW: ActorSpawnFacingY with hit box $4B, facing 0 and the platform's
; $40; started: the platform's address (TDC) to the child's $37-$38; bit 6
; of $09: SoundQueueAdd with the sound kept at $17. The phase steps up.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR through the dip phase's table. Exit: M 8-bit; a child started: X
; 8-bit; none: X as ActorSpawnFacingY left it.
%at($83CC42)
PlatformDipLand:
    lda.b #$4B
    ldx.b #$00
    ldy.b !actor_40
    jsl.l ActorSpawnFacingY
    bmi .lCC62
    tdc
    sta.w !actor_next,x
    xba
    sta.w !actor_next+1,x
    sep.b #$10
    bit.b !actor_09
    bvc .lCC62
    lda.b !DIP_SOUND
    jsl.l SoundQueueAdd
.lCC62:
    inc.b !DIP_PHASE
    rts

; @routine 83:CC65 PlatformDipSink
; WHAT IT DOES: Dip phase 2: the platform sinks; at 11 pixels down, on to
; the next phase.
; HOW: PlatformDipStep with the sinking rate (X = 0); the dip's pixels
; (16-bit) 11 or more: the phase steps up (a 16-bit increment of $13).
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
; pixels; C from the comparison.
%at($83CC65)
PlatformDipSink:
    ldx.b #$00
    jsr.w PlatformDipStep
    rep.b #$20
    clc
    lda.b !DIP+1
    cmp.w #$000B
    bcc .lCC76
    inc.b !DIP_PHASE
.lCC76:
    sep.b #$20
    rts

; @routine 83:CC79 PlatformDipRise
; WHAT IT DOES: Dip phase 3: the platform rises; under 3 pixels down, on
; to the next phase (waiting for the player to step off).
; HOW: PlatformDipStep with the rising rate (X = 3); the dip's pixels
; (16-bit) below 3: the phase steps up (a 16-bit increment of $13).
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR through the dip phase's table. Exit: M 8-bit; X = 3; A = the dip's
; pixels; C from the comparison.
%at($83CC79)
PlatformDipRise:
    ldx.b #$03
    jsr.w PlatformDipStep
    rep.b #$20
    clc
    lda.b !DIP+1
    cmp.w #$0003
    bcs .lCC8A
    inc.b !DIP_PHASE
.lCC8A:
    sep.b #$20
    rts

; @routine 83:CC8D PlatformDipWaitOff
; WHAT IT DOES: The platform's dip phase 4: waits until the player is
; off it, then on to the next phase (a last sink and rise).
; HOW: The ridden flag tested; zero: the phase steps on by one.
; Entry: M 8-bit; D = the platform's record; called by JSR through the
; dip phase's table. Exit: A low = the ridden flag; N and Z from it, or
; (it was zero) from the new phase. M 8-bit; A high, X, Y and the other
; flags unchanged.
%at($83CC8D)
PlatformDipWaitOff:
    lda.b !RIDDEN
    bne .done
    inc.b !DIP_PHASE
.done:
    rts

; @routine 83:CC94 PlatformDipSinkAgain
; WHAT IT DOES: Dip phase 5: once the player is off, the platform sinks
; again; at 11 pixels down, on to the last phase.
; HOW: As PlatformDipSink: PlatformDipStep with X = 0; 11 or more: the
; phase steps up (a 16-bit increment of $13).
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR through the dip phase's table. Exit: M 8-bit; X = 0; A = the dip's
; pixels; C from the comparison.
%at($83CC94)
PlatformDipSinkAgain:
    ldx.b #$00
    jsr.w PlatformDipStep
    rep.b #$20
    clc
    lda.b !DIP+1
    cmp.w #$000B
    bcc .lCCA5
    inc.b !DIP_PHASE
.lCCA5:
    sep.b #$20
    rts

; @routine 83:CCA8 PlatformDipRiseEnd
; WHAT IT DOES: Dip phase 6: the platform rises back; once past its
; height, the dip is zeroed and the phase back to 0 (idle).
; HOW: PlatformDipStep with the rising rate (X = 3); the dip's top byte
; negative: its three bytes and the phase zeroed.
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB = $80; called by
; JSR through the dip phase's table. Exit: M 8-bit; X = 3; A low = the
; dip's top byte.
%at($83CCA8)
PlatformDipRiseEnd:
    ldx.b #$03
    jsr.w PlatformDipStep
    lda.b !DIP+2
    bpl .lCCB9
    stz.b !DIP
    stz.b !DIP+1
    stz.b !DIP+2
    stz.b !DIP_PHASE
.lCCB9:
    rts

; @routine 83:CCBA PlatformDipStep
; WHAT IT DOES: Moves the platform's dip by a frame's rate: sinking
; (X = 0) or rising (X = 3).
; HOW: A 24-bit addition, a byte at a time, of the rate X picks to the
; dip.
; Entry: M 8-bit; X = the rate's offset (0 or 3); D = the platform's
; record; DB the rates' bank. Exit: A low = the dip's top byte; N, V, Z
; and C from its addition. M 8-bit; A high, X and Y unchanged.
%at($83CCBA)
PlatformDipStep:
    clc
    lda.w !DIP_RATES,x
    adc.b !DIP
    sta.b !DIP
    lda.w !DIP_RATES+1,x
    adc.b !DIP+1
    sta.b !DIP+1
    lda.w !DIP_RATES+2,x
    adc.b !DIP+2
    sta.b !DIP+2
    rts

; @routine 83:CCD1 PlatformPositionKeep
; WHAT IT DOES: Keeps the platform's x and y pixels at the start of the
; frame, to measure later how far it moved.
; HOW: Two word copies with 16-bit A.
; Entry: M 8-bit; D = the platform's record. Exit: A = its y pixel (16
; bits); N and Z from it. M 8-bit; X, Y and the other flags unchanged.
PlatformPositionKeep:
    rep.b #$20
    lda.b !actor_x+1
    sta.b !LAST_X
    lda.b !actor_y+1
    sta.b !LAST_Y
    sep.b #$20
    rts

; @routine 83:CCDE PlatformMoveMeasure
; WHAT IT DOES: Measures how far the platform moved in the frame, in x
; and y pixels, for carrying its rider.
; HOW: Its pixels less those kept at the frame's start, with 16-bit A.
; Entry: M 8-bit; D = the platform's record. Exit: A = the y move (16
; bits); N, V, Z and C from its subtraction. M 8-bit; X and Y unchanged.
PlatformMoveMeasure:
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.b !LAST_X
    sta.b !MOVED_X
    sec
    lda.b !actor_y+1
    sbc.b !LAST_Y
    sta.b !MOVED_Y
    sep.b #$20
    rts
