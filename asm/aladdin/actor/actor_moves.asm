; actor_moves.asm: small steps of an actor's motion: speeding up and
; moving, moving along a direction from the ROM's table, keeping its
; place; and a byte's sign spread to a word.
; C: src/aladdin/actor/actor_moves.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

; A direction's step: x then y, 24 bits each (offsets from !scratch).
!step_x = !scratch
!step_y = !scratch+4
; The ROM's direction steps in bank $82: each byte of x, then of y, in
; its own table of $409 entries.
!dir_x_lo = $82C000
!dir_x_mid = $82C409
!dir_x_hi = $82C812
!dir_y_lo = $82CC1B
!dir_y_mid = $82D024
!dir_y_hi = $82D42D
!dir_by_facing = $BBB9            ; bank $81: a direction per entry, the facing added
; The spot an actor keeps (its record, the direct page).
!actor_spot_x = $4C
!actor_spot_y = $4E

; @routine 81:A4AF ActorSpeedUpMove
; WHAT IT DOES: Speeds the actor up by its acceleration, then moves it.
; HOW: The 24-bit horizontal speed plus the acceleration (a fraction),
; then ActorMove.
; Entry: M 8-bit; D = the actor; called by JSL. Exit: as ActorMove.
%at($81A4AF)
ActorSpeedUpMove:
    clc
    lda.b !actor_speed_x
    adc.b !actor_accel_x
    sta.b !actor_speed_x
    lda.b !actor_speed_x+1
    adc.b #$00
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    adc.b #$00
    sta.b !actor_speed_x+2
    bra .move
; Code no script reaches: the speed taken down by the acceleration, and
; by $20.
.slow:
    sec
    lda.b !actor_speed_x
    sbc.b !actor_accel_x
    sta.b !actor_speed_x
    bra .borrow
    sec
    lda.b !actor_speed_x
    sbc.b !actor_accel_y
    sta.b !actor_speed_x
.borrow:
    lda.b !actor_speed_x+1
    sbc.b #$00
    sta.b !actor_speed_x+1
    lda.b !actor_speed_x+2
    sbc.b #$00
    sta.b !actor_speed_x+2
.move:
    jmp.w ActorMove


; @routine 81:A73D ActorMoveAlong
; WHAT IT DOES: Moves the actor along direction Y of the ROM's direction
; table ($1B6 on), at speed A.
; HOW: The direction's step, a byte at a time from the six tables in bank
; $82, to $0C-$12; then as ActorMoveScaled.
; Entry: M 8-bit, X 16-bit; A = the speed; Y = the direction; D = the
; actor; called by JSL. Exit: as ActorMoveScaled.
%at($81A73D)
ActorMoveAlong:
    pha
    rep.b #$20
    tya
    clc
    adc.w #$01B6
    tax
    bra ActorMoveScaled_along


; @routine 81:A748 ActorMoveScaled
; WHAT IT DOES: Moves the actor by the step at $0C-$12 multiplied by
; speed A: x by the step's x, y by its y.
; HOW: Each step's low and middle bytes times the speed (MultiplyBy, the
; products overlapped into 24 bits), then added to the actor's x and y.
; Entry: M 8-bit; A = the speed; $0C-$0E = the step's x; $10-$12 = its y;
; D = the actor; called by JSL. Exit: M 8-bit, X 8-bit; $0C-$12 the
; scaled step.
%at($81A748)
ActorMoveScaled:
    rep.b #$10
    bra .scaled
; An entry no script takes: direction $BBB9 + X + the facing.
    pha
    rep.b #$30
    lda.b !actor_facing
    clc
    and.w #$00FF
    adc.w !dir_by_facing,x
    tax
.along:
    sep.b #$20
    lda.l !dir_x_lo,x
    sta.w !step_x
    lda.l !dir_x_mid,x
    sta.w !step_x+1
    lda.l !dir_x_hi,x
    sta.w !step_x+2
    lda.l !dir_y_lo,x
    sta.w !step_y
    lda.l !dir_y_mid,x
    sta.w !step_y+1
    lda.l !dir_y_hi,x
    sta.w !step_y+2
    pla
.scaled:
    sta.w !WRMPYA
    lda.w !step_x
    jsr.w MultiplyBy
    lda.w !step_x+1
    sty.w !step_x
    jsr.w MultiplyBy
    tya
    clc
    adc.w !step_x+1
    sta.w !step_x+1
    lda.w !step_y
    jsr.w MultiplyBy
    lda.w !step_y+1
    sty.w !step_y
    jsr.w MultiplyBy
    tya
    clc
    adc.w !step_y+1
    sta.w !step_y+1
    clc
    lda.b !actor_x
    adc.w !step_x
    sta.b !actor_x
    lda.b !actor_x+1
    adc.w !step_x+1
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.w !step_x+2
    sta.b !actor_x+2
    clc
    lda.b !actor_y
    adc.w !step_y
    sta.b !actor_y
    lda.b !actor_y+1
    adc.w !step_y+1
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.w !step_y+2
    sta.b !actor_y+2
    sep.b #$30
    rtl


; @routine 81:9D56 ActorKeepSpot
; WHAT IT DOES: Keeps the actor's place: its x and y pixels into $4C and
; $4E of its record.
; HOW: Two 16-bit copies.
; Entry: D = the actor; called by JSL. Exit: M 8-bit; A = the y pixels.
%at($819D56)
ActorKeepSpot:
    rep.b #$20
    lda.b !actor_x+1
    sta.b !actor_spot_x
    lda.b !actor_y+1
    sta.b !actor_spot_y
    sep.b #$20
    rtl


; @routine 81:945F SignExtend
; WHAT IT DOES: Spreads the sign of A's low byte to its high byte.
; HOW: Bit 7 clear: A & $FF; set: A | $FF00.
; Entry: M 16-bit; called by JSR. Exit: A the word; N and Z from it.
%at($81945F)
SignExtend:
    pha
    and.w #$0080
    bne .l946A
    pla
    and.w #$00FF
    rts
.l946A:
    pla
    ora.w #$FF00
    rts
