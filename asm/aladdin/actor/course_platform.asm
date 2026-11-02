; course_platform.asm: pieces of the platform of kind $47 (its start at
; $83:C96E, its wait routine at $83:CAD1), which flies a course the player
; rides: the course's waypoints as it is worked out, the stretches where
; it turns, the dip it makes under the player's weight, and how far it
; moved in a frame.
; C: src/aladdin/actor/course_platform.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

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

; The course's waypoints, 8 bytes each: x and y (24-bit, fraction first),
; the direction and the speed.
!COURSE = $7F5000
; In the bank DB names: the number of turn stretches, then 6 bytes each:
; the first x pixel and the x pixel past it (words).
!TURN_ZONE_COUNT = $D2F7
!TURN_ZONES = $D2F8
; In the bank DB names: the rates (24-bit) it sinks and rises at.
!DIP_RATES = $D2ED

; @routine 83:CA3F PlatformCourseRecord
; WHAT IT DOES: Keeps a waypoint of the platform's course every 38 steps
; while the course is worked out: its position, direction and speed.
; HOW: The step count goes down; at 0 it starts again at 38 and the 8
; bytes go to the course at the end offset, which moves on by 8.
; Entry: M 8-bit, X 8-bit; D = the platform's record. Exit: as the C states.
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

; @routine 83:CAC7 PlatformDipClear
; WHAT IT DOES: Takes away the platform's dip: it is back at its height.
; HOW: The three bytes of the dip are zeroed.
; Entry: M 8-bit; D = the platform's record. Exit: as the C states.
%at($83CAC7)
PlatformDipClear:
    stz.b !DIP
    stz.b !DIP+1
    stz.b !DIP+2
    rts

; @routine 83:CB6D PlatformTurnZoneTest
; WHAT IT DOES: Tells whether the platform is in a stretch of the course
; where it turns: C clear when its x pixel is in one.
; HOW: The stretches, in order of x, each compared unsigned: below a
; stretch's first pixel, or past the last stretch, is out (C set); below
; the pixel past it is in (C clear).
; Entry: M 8-bit, X 8-bit; D = the platform's record; DB the table's
; bank. Exit: as the C states.
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

; @routine 83:CC18 PlatformDipIdle
; WHAT IT DOES: The platform's dip phase 0: nothing to do (no one has
; stepped on it).
; HOW: RTS.
; Entry: any; called by JSR through the dip phase's table. Exit: as the C states.
%at($83CC18)
PlatformDipIdle:
    rts

; @routine 83:CC8D PlatformDipWaitOff
; WHAT IT DOES: The platform's dip phase 4: waits until the player is
; off it, then on to the next phase (a last sink and rise).
; HOW: The ridden flag tested; zero: the phase steps on by one.
; Entry: M 8-bit; D = the platform's record; called by JSR through the
; dip phase's table. Exit: as the C states.
%at($83CC8D)
PlatformDipWaitOff:
    lda.b !RIDDEN
    bne .done
    inc.b !DIP_PHASE
.done:
    rts

; @routine 83:CCBA PlatformDipStep
; WHAT IT DOES: Moves the platform's dip by a frame's rate: sinking
; (X = 0) or rising (X = 3).
; HOW: A 24-bit addition, a byte at a time, of the rate X picks to the
; dip.
; Entry: M 8-bit; X = the rate's offset (0 or 3); D = the platform's
; record; DB the rates' bank. Exit: as the C states.
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
; Entry: M 8-bit; D = the platform's record. Exit: as the C states.
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
; Entry: M 8-bit; D = the platform's record. Exit: as the C states.
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
