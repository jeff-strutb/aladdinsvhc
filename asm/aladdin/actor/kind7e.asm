; kind7e.asm: actor kind $7E, the platforms Aladdin bounces on (stage a4
; s1): by its $07 one rises and falls in turn, drifts across and back
; aslant, or is the hidden centre of two or three that circle it. Its wait
; (the bounce) is Kind7EWaitWork in kind_waits_84.asm.
; C: src/aladdin/actor/kind7e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K7E_DOWN = $40                 ; 0: it moves against its vertical speed (ActorStepYBack), else with it
!K7E_LEFT = $41                 ; frames left of its glide; for a centre, children left to make, less one
!K7E_BOUNCE = $42               ; frames left of a bounce (the player held on it; Kind7EWaitWork)
!K7E_CENTRE_X = $43             ; a circler: the x it circles (pixels, word)
!K7E_CENTRE_Y = $45             ; and the y (pixels, word)
!K7E_STAYS = $47                ; nonzero: never freed out of range (a circler)
!K7E_RADIUS = $48               ; a circler's distance from its centre (ActorMoveAlong's speed); a centre's for its children
!K7E_ANGLE = $49                ; a circler: its direction from the centre, 0-$1FF (word)
!K7E_CENTRE = $4B               ; a circler: its centre's record (word)
!K7E_TURN = $4D                 ; a circler: 0 its angle counts up, 1 down
!K7E_GLIDE = $4E                ; frames it glides at full speed
!K7E_NEXT = $4F                 ; a centre: the entry of K7E_ANGLES its next child starts at
!K7E_ANGLES = $D828             ; (data bank $80) the children's first angles (words): 0, $AA, $154, then $40, $140
!K7E_GRAVITY = $06              ; its vertical acceleration (ActorMotionReset)
!K7E_TOP_SPEED = $C0            ; the vertical speed's fraction at which it stops speeding up
!K7E_TURN_WAIT = $20            ; frames it waits each time it turns
!K7E_RANGE_BOX = $00            ; a centre: its box for ActorEndOutOfRangeLong

; @routine 83:92A8 Kind7EEnter
; @actor
; WHAT IT DOES: Where actors of kind $7E go on from the actor loop in
; their start state (the table at $83:8CAB): Kind7EStart.
; HOW: JML to Kind7EStart ($84:91A5).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind7EStart.
%at($8392A8)
Kind7EEnter:
    jml.l Kind7EStart
%ends_at($8392AC)

; @routine 84:91A5 Kind7EStart
; @actor
; WHAT IT DOES: Actor kind $7E, a platform Aladdin bounces on, by its $07:
; 0 rises, glides, slows to a stop, waits 32 frames and comes back down,
; and so on for good; 5 and 6 do the same aslant, rightward or leftward
; first; 1 to 4 make two or three children of kind $7E ($07 7 or 8) set
; round it, and stay unseen where they were placed until out of range;
; 7 and 8 (the children) circle their centre's place, one way or the
; other, until the centre's record is free.
; HOW: K7E_STAYS 0; JMP through the table below by $07 times 2.
; 0: Kind7EShow, ActorMotionReset with gravity 6, K7E_DOWN and
; K7E_BOUNCE 0, K7E_GLIDE $36. 5 and 6: facing 0 or 1, Kind7EShow,
; K7E_DOWN and K7E_BOUNCE 0, K7E_GLIDE $44, ActorMotionReset 6,
; acceleration across 3. Then for good: each frame (BRK first)
; ActorAccelerateX and ActorAccelerateY, Kind7EStep, until the vertical
; speed's fraction reaches $C0; K7E_GLIDE frames (BRK first) of
; Kind7EStep; each frame (BRK first) ActorDecelerateX and
; ActorDecelerateY, Kind7EStep, until the fraction is 0; K7E_DOWN and the
; facing flipped; COP for 32 frames.
; 1 and 3: $07 = 7; 2 and 4: $07 = 8. 1 and 2: K7E_RADIUS $B2, three
; children from entry 2 down; 3 and 4: K7E_RADIUS $74, two from entry 4
; down (Kind7ERing). Then each frame (BRK first) ActorEndOutOfRangeLong
; by box 0.
; 7 and 8: K7E_TURN 0 or 1; Kind7EShow, K7E_BOUNCE 0, K7E_STAYS up. Each
; frame (BRK first), 16-bit X: K7E_CENTRE's state 0: X 8-bit,
; ActorEndPlaced. Else its place at K7E_CENTRE_X/Y (fractions 0), then
; ActorMoveAlong direction K7E_ANGLE at K7E_RADIUS; the angle one up (or
; down by K7E_TURN), AND $1FF.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $7E in state $0C (the table at $83:8CAB,
; through the stub at $83:92A8). Exit: never returns.
%at($8491A5)
Kind7EStart:
    stz.b !K7E_STAYS
    lda.b !actor_07
    asl a
    tax
    jmp.w (.ways,x)
.ways:
    dw .rise
    dw .ring3_7
    dw .ring3_8
    dw .ring2_7
    dw .ring2_8
    dw .aslant_right
    dw .aslant_left
    dw .circle_up
    dw .circle_down
.rise:
    jsr.w Kind7EShow
    lda.b #!K7E_GRAVITY
    jsl.l ActorMotionReset
    stz.b !K7E_DOWN
    stz.b !K7E_BOUNCE
    lda.b #$36
    sta.b !K7E_GLIDE
.speed_up:
    brk.b #$00
    jsl.l ActorAccelerateX
    jsl.l ActorAccelerateY
    lda.b !actor_speed_y
    cmp.b #!K7E_TOP_SPEED
    bcs .glide
    jsr.w Kind7EStep
    bra .speed_up
.glide:
    lda.b !K7E_GLIDE
    sta.b !K7E_LEFT
.gliding:
    brk.b #$00
    jsr.w Kind7EStep
    dec.b !K7E_LEFT
    bne .gliding
.slow_down:
    brk.b #$00
    jsl.l ActorDecelerateX
    jsl.l ActorDecelerateY
    lda.b !actor_speed_y
    beq .turn
    jsr.w Kind7EStep
    bra .slow_down
.turn:
    lda.b !K7E_DOWN
    eor.b #$01
    sta.b !K7E_DOWN
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    lda.b #!K7E_TURN_WAIT
    cop.b #$00
    bra .speed_up
.ring3_7:
    lda.b #$07
    sta.b !actor_07
    bra .ring3
.ring3_8:
    lda.b #$08
    sta.b !actor_07
.ring3:
    lda.b #$B2
    sta.b !K7E_RADIUS
    lda.b #$02
    sta.b !K7E_LEFT
    lda.b #$02
    sta.b !K7E_NEXT
.ring:
    jsr.w Kind7ERing
.centre:
    brk.b #$00
    ldx.b #!K7E_RANGE_BOX
    jsl.l ActorEndOutOfRangeLong
    bra .centre
.ring2_7:
    lda.b #$07
    sta.b !actor_07
    bra .ring2
.ring2_8:
    lda.b #$08
    sta.b !actor_07
.ring2:
    lda.b #$74
    sta.b !K7E_RADIUS
    lda.b #$01
    sta.b !K7E_LEFT
    lda.b #$04
    sta.b !K7E_NEXT
    bra .ring
.aslant_right:
    stz.b !actor_facing
.aslant:
    jsr.w Kind7EShow
    stz.b !K7E_DOWN
    stz.b !K7E_BOUNCE
    lda.b #$44
    sta.b !K7E_GLIDE
    lda.b #!K7E_GRAVITY
    jsl.l ActorMotionReset
    lda.b #$03
    sta.b !actor_accel_x
    jmp.w .speed_up
.aslant_left:
    lda.b #$01
    sta.b !actor_facing
    bra .aslant
.circle_up:
    stz.b !K7E_TURN
    bra .circle
.circle_down:
    lda.b #$01
    sta.b !K7E_TURN
.circle:
    jsr.w Kind7EShow
    stz.b !K7E_BOUNCE
    inc.b !K7E_STAYS
.circling:
    brk.b #$00
    rep.b #$10
    ldx.b !K7E_CENTRE
    lda.w !actor_state,x
    beq .centre_gone
    ldx.b !K7E_CENTRE_X
    stx.b !actor_x+1
    stz.b !actor_x
    ldx.b !K7E_CENTRE_Y
    stx.b !actor_y+1
    stz.b !actor_y
    ldy.b !K7E_ANGLE
    lda.b !K7E_RADIUS
    jsl.l ActorMoveAlong
    lda.b !K7E_TURN
    lsr a
    rep.b #$20
    lda.b !K7E_ANGLE
    inc a
    bcc .angle
    dec a
    dec a
.angle:
    and.w #$01FF
    sta.b !K7E_ANGLE
    sep.b #$20
    bra .circling
.centre_gone:
    sep.b #$10
    jml.l ActorEndPlaced

; @routine 84:92BC Kind7EStep
; WHAT IT DOES: Kind $7E's step: across by its speed the way it faces, and
; up or down by its vertical speed as K7E_DOWN says.
; HOW: ActorMoveX; K7E_DOWN zero: ActorStepYBack, else ActorStepY.
; Entry: M 8-bit; D = the actor's record; called by JSR from
; Kind7EStart. Exit: as ActorStepYBack or ActorStepY leave them.
Kind7EStep:
    jsl.l ActorMoveX
    lda.b !K7E_DOWN
    bne .down
    jsl.l ActorStepYBack
    rts
.down:
    jsl.l ActorStepY
    rts

; @routine 84:92CE Kind7EShow
; WHAT IT DOES: Shows a kind $7E with animation 0, its wait (the bounce)
; to run each frame it waits.
; HOW: $08 bit 1 and $09 bits $90 set (TSB); AnimStart 0.
; Entry: M 8-bit; D = the actor's record; called by JSR from
; Kind7EStart. Exit: as AnimStart leaves them.
Kind7EShow:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    rts

; @routine 84:92DD Kind7ERing
; WHAT IT DOES: Makes a kind $7E centre's children, K7E_LEFT + 1 of them,
; each to circle the centre's place at K7E_RADIUS from its own first angle.
; HOW: Each time: ActorSpawn with A = its kind; one made (N clear, X =
; the child): its look ($2B, $2C) copied; 16-bit, K7E_CENTRE = this
; record, K7E_ANGLE = K7E_ANGLES entry K7E_NEXT, K7E_CENTRE_X/Y = this
; one's place; 8-bit, K7E_RADIUS copied. Then (made or not) K7E_NEXT
; and K7E_LEFT down, again while K7E_LEFT is not negative.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR from Kind7EStart. Exit: M and X 8-bit; K7E_LEFT = $FF.
Kind7ERing:
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .next
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_look+1
    sta.w !actor_look+1,x
    rep.b #$20
    tdc
    sta.w !K7E_CENTRE,x
    lda.b !K7E_NEXT
    and.w #$00FF
    asl a
    tay
    lda.w !K7E_ANGLES,y
    sta.w !K7E_ANGLE,x
    lda.b !actor_x+1
    sta.w !K7E_CENTRE_X,x
    lda.b !actor_y+1
    sta.w !K7E_CENTRE_Y,x
    sep.b #$20
    lda.b !K7E_RADIUS
    sta.w !K7E_RADIUS,x
.next:
    sep.b #$30
    dec.b !K7E_NEXT
    dec.b !K7E_LEFT
    bpl Kind7ERing
    rts
%ends_at($84931C)
