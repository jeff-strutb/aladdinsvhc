; kind90.asm: actor kind $90, a thing that flies a set path from a slot of
; its $07 in steps, each step's flight made at once, and reports in its
; slot how it ended: $80 when it came down onto ground, $40 when it hit a
; wall whose probe result has bit 6 clear, 0 when its steps ran out.
; C: src/aladdin/actor/kind90.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/actor.inc"

arch 65816

!K90_FLIGHT = $40               ; flights left in this step
!K90_STEPS = $41                ; steps left
!K90_16 = $16                   ; word: $CF8F
!K90_SLOTS = $1478              ; by $07: its slot (low four bits nonzero: go on); how it ended
!K90_STEP_COUNTS = $D878        ; (data bank) by $07: K90_STEPS
!K90_PRESETS = $D875            ; by $07: its motion preset
!K90_FLIGHTS = $D87B            ; by $07: flights a step

; @routine A5:F4D2 Kind90Start
; @actor
; WHAT IT DOES: Actor kind $90: flies by the preset its $07 names, in
; steps of several flights each made within one frame, while its slot in
; K90_SLOTS stays live; it ends, reporting in the slot $80 when it is in
; the ground, $40 when it meets a wall whose probe has bit 6 clear
; ($A5:F50D, which no script meets), 0 when its steps run out.
; HOW: ACTOR_09 bit 7 set; K90_16 $CF8F; K90_STEPS from K90_STEP_COUNTS,
; ActorMotionPreset with K90_PRESETS, by $07. Each step: K90_FLIGHT from
; K90_FLIGHTS; the slot's low nibble 0: ActorEndPlaced at once; that many
; ActorFly; ActorOutOfWallProbe4 C set and V_0012 bit 6 clear: slot $40;
; ActorOutOfGround nonzero: slot $80; else BRK and the next step, after
; the last slot 0. Each report: the slot written, ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the actor loop's jump for kind $90 in state $0C (the table at $83:8DAB,
; through the stub at $83:9362). Exit: never returns.
%at($A5F4D2)
Kind90Start:
    lda.b #$80
    tsb.b !actor_09
    rep.b #$20
    lda.w #$CF8F
    sta.b !K90_16
    sep.b #$20
    ldx.b !actor_07
    lda.w !K90_STEP_COUNTS,x
    sta.b !K90_STEPS
    ldy.w !K90_PRESETS,x
    jsl.l ActorMotionPreset
.step:
    ldx.b !actor_07
    lda.w !K90_FLIGHTS,x
    sta.b !K90_FLIGHT
    lda.w !K90_SLOTS,x
    and.b #$0F
    bne .fly
    jml.l ActorEndPlaced
.fly:
    jsl.l ActorFly
    dec.b !K90_FLIGHT
    bne .fly
    jsl.l ActorOutOfWallProbe4
    bcc .ground
    lda.w !v_0012
    bit.b #$40
    beq .wall
.ground:
    jsl.l ActorOutOfGround
    bne .landed
    brk.b #$00
    dec.b !K90_STEPS
    bne .step
    lda.b #$00
.report:
    ldx.b !actor_07
    sta.w !K90_SLOTS,x
    jml.l ActorEndPlaced
.wall:
    lda.b #$40
    bra .report
.landed:
    lda.b #$80
    bra .report
