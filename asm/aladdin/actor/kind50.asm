; kind50.asm: actor kind $50 of level 0, which rises out of the ground in
; steps, waits facing Aladdin, sinks back and waits again, for good. Its
; wait is Kind50Wait (kind_waits_rest.asm). Named and described by hand.
; C: src/aladdin/actor/kind50.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!K50_FRAMES = $40               ; frames left of its wait below or on top
!K50_TOP = $41                  ; frames it waits each time ($20 or $40)
!K50_STEP = $42                 ; frames left of a step
!K50_POSE = $43                 ; its animation: 6 down, 0 up
!K50_DOWNED = $45               ; counted up by Kind50Wait when a shot downs it

; @routine 83:D109 Kind50Start
; @actor
; WHAT IT DOES: Actor kind $50 started: hidden in the ground, it waits a
; random time, rises in 6 steps, waits on top, sinks in 6 steps, waits,
; and again, always facing Aladdin.
; HOW: K50_DOWNED = 0, $08 = 3, $09 |= $90, Kind50Face; pose 6
; (AnimStart); K50_FRAMES = RandomFromList 4; K50_TOP = $40 when
; RandomNext is negative, else $20. Below: each frame (BRK first)
; Kind50Face for K50_FRAMES + 1 frames; K50_FRAMES = K50_TOP. Up: every 7
; frames (BRK) pose one less (AnimStart) and Kind50Up8, down to pose 0.
; On top: each frame (BRK) Kind50Face for K50_FRAMES frames; Kind50Still.
; Down: every 7 frames pose one more (AnimStart) and Kind50Down8, up to
; pose 6; K50_FRAMES = K50_TOP, Kind50Still, and below again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $0C. Exit: never returns.
%at($83D109)
Kind50Start:
    stz.b !K50_DOWNED
    lda.b #$03
    sta.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    jsr.w Kind50Face
    lda.b #$06
    sta.b !K50_POSE
    jsl.l AnimStart
    ldx.b #$04
    jsl.l RandomFromList
    sta.b !K50_FRAMES
    jsl.l RandomNext
    bmi .long_top
    lda.b #$20
    bra .top_set
.long_top:
    lda.b #$40
.top_set:
    sta.b !K50_TOP
.below:
    brk.b #$00
    jsr.w Kind50Face
    dec.b !K50_FRAMES
    bpl .below
    lda.b !K50_TOP
    sta.b !K50_FRAMES
.rise:
    lda.b #$07
    sta.b !K50_STEP
.rise_step:
    brk.b #$00
    dec.b !K50_STEP
    bne .rise_step
    dec.b !K50_POSE
    bmi .up
    lda.b !K50_POSE
    jsl.l AnimStart
    jsr.w Kind50Up8
    bra .rise
.up:
    inc.b !K50_POSE
.on_top:
    brk.b #$00
    jsr.w Kind50Face
    dec.b !K50_FRAMES
    bne .on_top
    jsr.w Kind50Still
.sink:
    lda.b #$07
    sta.b !K50_STEP
.sink_step:
    brk.b #$00
    dec.b !K50_STEP
    bne .sink_step
    inc.b !K50_POSE
    lda.b !K50_POSE
    cmp.b #$07
    beq .down
    jsl.l AnimStart
    jsr.w Kind50Down8
    bra .sink
.down:
    dec.b !K50_POSE
    lda.b !K50_TOP
    sta.b !K50_FRAMES
    jsr.w Kind50Still
    bra .below

; @routine 83:D18E Kind50Still
; WHAT IT DOES: Nothing: kind $50 calls it where it turns between rising
; and sinking.
; HOW: RTS.
; Entry: called by JSR. Exit: all unchanged.
%at($83D18E)
Kind50Still:
    rts

; @routine 83:D18F Kind50Down8
; WHAT IT DOES: Moves the actor 8 pixels down.
; HOW: 16-bit A, carry cleared (REP #$21): y pixels + 8.
; Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit; C
; from the add.
%at($83D18F)
Kind50Down8:
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$0008
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine 83:D19B Kind50Up8
; WHAT IT DOES: Moves the actor 8 pixels up.
; HOW: 16-bit A, carry cleared (REP #$21): y pixels - 7 - 1 (SBC with the
; carry clear).
; Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit; C
; from the subtract.
%at($83D19B)
Kind50Up8:
    rep.b #$21
    lda.b !actor_y+1
    sbc.w #$0007
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine 83:D1A7 Kind50Face
; WHAT IT DOES: Turns the actor to face Aladdin.
; HOW: ActorSideOfPlayer into its facing and its flip (the same value).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: as ActorSideOfPlayer.
%at($83D1A7)
Kind50Face:
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    sta.b !actor_flip
    rts
