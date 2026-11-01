; motion.asm: an actor's movement: its speeds and gravity, and its position
; stepped by them.
;
; Every routine here works on the actor whose record the direct page is
; (actor.inc). Positions and speeds are 24-bit; the horizontal speed is a
; size along the actor's facing, the vertical one is signed (down is
; positive). Each 24-bit step is done a byte at a time with the carry, so
; A and the flags end as the last byte's ADC or SBC leaves them.
; C: src/aladdin/actor/motion.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; The motion presets: per preset (Y), the horizontal speed's three bytes
; and the vertical speed's three bytes and gravity, each in a table of its
; own.
!motion_preset_speed_x_hi = $81BF3F
!motion_preset_speed_x_mid = $81BFCC
!motion_preset_speed_x_frac = $81C059
!motion_preset_speed_y_hi = $81C0E6
!motion_preset_speed_y_mid = $81C173
!motion_preset_speed_y_frac = $81C200
!motion_preset_gravity = $81C28D
; Horizontal and vertical speeds, three bytes per entry (Y the entry's byte
; offset).
!speed_x_table = $81BAAB
!speed_y_table = $81BB20

; @routine 81:A3CE ActorMotionReset
; WHAT IT DOES: Stops the actor: no speed either way, no horizontal
; acceleration, and gravity A.
; HOW: Clears the horizontal speed and acceleration and the vertical speed
; with word stores, then stores A as the gravity.
; Entry: M 8-bit; D = the actor; A low byte = the gravity.
; Exit: A, X, Y and the flags unchanged; M 8-bit.
%at($81A3CE)
ActorMotionReset:
    rep.b #$20
    stz.b !actor_speed_x
    stz.b !actor_speed_x+2
    stz.b !actor_speed_y
    sep.b #$20
    stz.b !actor_speed_y+2
    sta.b !actor_accel_y
    rtl

; @routine 81:A3DD ActorMotionPreset
; WHAT IT DOES: Gives the actor the speeds and gravity of motion preset Y:
; a jump or a throw's arc.
; HOW: Copies the horizontal speed's three bytes from the preset tables,
; fraction first, then goes on into ActorMotionPresetY.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the preset.
; Exit: as ActorMotionPresetY.
ActorMotionPreset:
    lda.w !motion_preset_speed_x_frac,y
    sta.b !actor_speed_x
    lda.w !motion_preset_speed_x_mid,y
    sta.b !actor_speed_x+1
    lda.w !motion_preset_speed_x_hi,y
    sta.b !actor_speed_x+2
; @routine 81:A3EC ActorMotionPresetY
; WHAT IT DOES: Gives the actor the vertical speed and gravity of motion
; preset Y.
; HOW: Copies the vertical speed's three bytes and the gravity from the
; preset tables, fraction first.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the preset.
; Exit: A low byte = the gravity; N, Z from it; X, Y, C, V unchanged.
ActorMotionPresetY:
    lda.w !motion_preset_speed_y_frac,y
    sta.b !actor_speed_y
    lda.w !motion_preset_speed_y_mid,y
    sta.b !actor_speed_y+1
    lda.w !motion_preset_speed_y_hi,y
    sta.b !actor_speed_y+2
    lda.w !motion_preset_gravity,y
    sta.b !actor_accel_y
    rtl

; @routine 81:A413 ActorSpeedXSet
; WHAT IT DOES: Sets the actor's horizontal speed from the speed table.
; HOW: Copies the three bytes of the entry at byte offset Y.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the entry's offset.
; Exit: A low byte = the speed's high byte; N, Z from it; X, Y, C, V
; unchanged.
%at($81A413)
ActorSpeedXSet:
    lda.w !speed_x_table,y
    sta.b !actor_speed_x
    lda.w !speed_x_table+1,y
    sta.b !actor_speed_x+1
    lda.w !speed_x_table+2,y
    sta.b !actor_speed_x+2
    rtl

; @routine 81:A423 ActorSpeedYSetGravity
; WHAT IT DOES: Sets the actor's gravity to A and its vertical speed from
; the speed table.
; HOW: Stores A as the gravity, then goes on into ActorSpeedYSet.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; A low byte = the
; gravity; Y = the entry's offset. Exit: as ActorSpeedYSet.
ActorSpeedYSetGravity:
    sta.b !actor_accel_y
; @routine 81:A425 ActorSpeedYSet
; WHAT IT DOES: Sets the actor's vertical speed from the speed table.
; HOW: Copies the three bytes of the entry at byte offset Y.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the entry's offset.
; Exit: A low byte = the speed's high byte; N, Z from it; X, Y, C, V
; unchanged.
ActorSpeedYSet:
    lda.w !speed_y_table,y
    sta.b !actor_speed_y
    lda.w !speed_y_table+1,y
    sta.b !actor_speed_y+1
    lda.w !speed_y_table+2,y
    sta.b !actor_speed_y+2
    rtl

; @routine 81:A435 ActorStopX
; WHAT IT DOES: Stops the actor's horizontal movement and its horizontal
; acceleration.
; HOW: Clears the acceleration, then goes on into ActorSpeedXClear.
; Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
ActorStopX:
    stz.b !actor_accel_x
; @routine 81:A437 ActorSpeedXClear
; WHAT IT DOES: Stops the actor's horizontal movement.
; HOW: Clears the three bytes of its horizontal speed.
; Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
ActorSpeedXClear:
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    rtl

; @routine 81:A43E ActorStopY
; WHAT IT DOES: Stops the actor's vertical movement and takes its gravity
; away.
; HOW: Clears the gravity, then goes on into ActorSpeedYClear.
; Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
ActorStopY:
    stz.b !actor_accel_y
; @routine 81:A440 ActorSpeedYClear
; WHAT IT DOES: Stops the actor's vertical movement.
; HOW: Clears the three bytes of its vertical speed.
; Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
ActorSpeedYClear:
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    rtl

; @routine 81:A4E3 ActorStepY
; WHAT IT DOES: Moves the actor down by its vertical speed (up when the
; speed is negative). The same steps as ActorMoveY, a routine of its own.
; HOW: Adds the 24-bit vertical speed to the 24-bit y position.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
; byte; N, V, Z, C from its ADC; X, Y unchanged.
%at($81A4E3)
ActorStepY:
    clc
    lda.b !actor_y
    adc.b !actor_speed_y
    sta.b !actor_y
    lda.b !actor_y+1
    adc.b !actor_speed_y+1
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.b !actor_speed_y+2
    sta.b !actor_y+2
    rtl

; @routine 81:A4F7 ActorStepYBack
; WHAT IT DOES: Moves the actor against its vertical speed (up when the
; speed is positive).
; HOW: Takes the 24-bit vertical speed from the 24-bit y position.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
; byte; N, V, Z, C from its SBC; X, Y unchanged.
ActorStepYBack:
    sec
    lda.b !actor_y
    sbc.b !actor_speed_y
    sta.b !actor_y
    lda.b !actor_y+1
    sbc.b !actor_speed_y+1
    sta.b !actor_y+1
    lda.b !actor_y+2
    sbc.b !actor_speed_y+2
    sta.b !actor_y+2
    rtl

; @routine 81:A50B ActorFall
; WHAT IT DOES: One frame of a fall with no horizontal movement: gravity
; pulls the vertical speed down, and the actor moves by it.
; HOW: Adds the gravity to the 24-bit vertical speed, then ActorMoveY.
; Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
ActorFall:
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
    bra ActorMoveY

; @routine 81:A535 ActorFlyAgainst
; WHAT IT DOES: One frame of flight with the vertical pull reversed: the
; acceleration is taken from the vertical speed, and the actor moves by
; its speeds.
; HOW: Takes the gravity from the 24-bit vertical speed, then ActorMove.
; Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
%at($81A535)
ActorFlyAgainst:
    sec
    lda.b !actor_speed_y
    sbc.b !actor_accel_y
    sta.b !actor_speed_y
    lda.b !actor_speed_y+1
    sbc.b #0
    sta.b !actor_speed_y+1
    lda.b !actor_speed_y+2
    sbc.b #0
    sta.b !actor_speed_y+2
    bra ActorMove

; @routine 81:A54A ActorFly
; WHAT IT DOES: One frame of flight: gravity pulls the vertical speed
; down, and the actor moves by its speeds.
; HOW: Adds the gravity to the 24-bit vertical speed, then ActorMove.
; Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
ActorFly:
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
; @routine 81:A55D ActorMove
; WHAT IT DOES: Moves the actor by its speeds: horizontally the way it
; faces, and vertically.
; HOW: ActorMoveXNear, then ActorMoveY.
; Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
ActorMove:
    jsr.w ActorMoveXNear
; @routine 81:A560 ActorMoveY
; WHAT IT DOES: Moves the actor by its vertical speed.
; HOW: Adds the 24-bit vertical speed to the 24-bit y position.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
; byte; N, V, Z, C from its ADC; X, Y unchanged.
ActorMoveY:
    clc
    lda.b !actor_y
    adc.b !actor_speed_y
    sta.b !actor_y
    lda.b !actor_y+1
    adc.b !actor_speed_y+1
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.b !actor_speed_y+2
    sta.b !actor_y+2
    rtl

; @routine 81:A5A7 ActorMoveX
; WHAT IT DOES: Moves the actor by its horizontal speed the way it faces.
; HOW: ActorMoveXNear, for callers in other banks.
; Entry, Exit: as ActorMoveXNear; called by JSL.
%at($81A5A7)
ActorMoveX:
    jsr.w ActorMoveXNear
    rtl

; @routine 81:A5AB ActorMoveRight
; WHAT IT DOES: Moves the actor right by its horizontal speed, whichever
; way it faces.
; HOW: ActorMoveRightNear, for callers in other banks.
; Entry, Exit: as ActorMoveRightNear; called by JSL.
ActorMoveRight:
    jsr.w ActorMoveRightNear
    rtl

; @routine 81:A5AF ActorMoveXNear
; WHAT IT DOES: Moves the actor by its horizontal speed the way it faces.
; HOW: Facing right, adds the 24-bit horizontal speed to the 24-bit x
; position (ActorMoveRightNear); facing left, takes it away.
; Entry: M 8-bit; D = the actor; called by JSR. Exit: A low byte = the
; position's high byte; N, V, Z, C from its ADC or SBC; X, Y unchanged.
ActorMoveXNear:
    lda.b !actor_facing
    bne ActorMoveXNear_left
; @routine 81:A5B3 ActorMoveRightNear
; WHAT IT DOES: Moves the actor right by its horizontal speed.
; HOW: Adds the 24-bit horizontal speed to the 24-bit x position.
; Entry: M 8-bit; D = the actor; called by JSR. Exit: A low byte = the
; position's high byte; N, V, Z, C from its ADC; X, Y unchanged.
ActorMoveRightNear:
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
; ActorMoveXNear facing left: x position minus the horizontal speed.
ActorMoveXNear_left:
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

; @routine 81:A5FF ActorAccelerateX
; WHAT IT DOES: Speeds the actor up horizontally by its acceleration.
; HOW: Adds the acceleration to the 24-bit horizontal speed.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the speed's high byte;
; N, V, Z, C from its ADC; X, Y unchanged.
%at($81A5FF)
ActorAccelerateX:
    clc
    lda.b !actor_speed_x
    adc.b !actor_accel_x
    sta.b !actor_speed_x
    lda.b !actor_speed_x+1
    adc.b #0
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    adc.b #0
    sta.b !actor_speed_x+2
    rtl

; @routine 81:A613 ActorAccelerateY
; WHAT IT DOES: Gravity pulls the actor's vertical speed down.
; HOW: Adds the gravity to the 24-bit vertical speed.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the speed's high byte;
; N, V, Z, C from its ADC; X, Y unchanged.
ActorAccelerateY:
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
    rtl

; @routine 81:A63B ActorDecelerateX
; WHAT IT DOES: Slows the actor down horizontally by its acceleration,
; stopping it rather than turning it round.
; HOW: Takes the acceleration from the 24-bit horizontal speed; a negative
; result becomes 0.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the high byte the SBC
; made; N, V, Z, C from it; X, Y unchanged.
%at($81A63B)
ActorDecelerateX:
    sec
    lda.b !actor_speed_x
    sbc.b !actor_accel_x
    sta.b !actor_speed_x
    lda.b !actor_speed_x+1
    sbc.b #0
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    sbc.b #0
    sta.b !actor_speed_x+2
    bpl .done
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.done:
    rtl

; @routine 81:A66B ActorDecelerateY
; WHAT IT DOES: Takes the actor's gravity from its vertical speed,
; stopping at 0: a rise that slows to a halt.
; HOW: Takes the gravity from the 24-bit vertical speed; a negative result
; becomes 0.
; Entry: M 8-bit; D = the actor. Exit: A low byte = the high byte the SBC
; made; N, V, Z, C from it; X, Y unchanged.
%at($81A66B)
ActorDecelerateY:
    sec
    lda.b !actor_speed_y
    sbc.b !actor_accel_y
    sta.b !actor_speed_y
    lda.b !actor_speed_y+1
    sbc.b #0
    sta.b !actor_speed_y+1
    lda.b !actor_speed_y+2
    sbc.b #0
    sta.b !actor_speed_y+2
    bpl .done
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
.done:
    rtl
