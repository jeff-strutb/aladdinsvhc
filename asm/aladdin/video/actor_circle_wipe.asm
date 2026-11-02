; actor_circle_wipe.asm: a circle wipe that grows round an actor: one step a
; call, the window table copied where the HDMA reads it. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/video/actor_circle_wipe.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!WIPE_RADIUS = $45  ; the circle's radius, one up each step (word)
!LAYER0_X = $1773  ; layer 0's x pixels (word)
!LAYER0_Y = $1777  ; layer 0's y pixels (word)

; @routine 83:EAAF ActorCircleWipeStep
; WHAT IT DOES: One step of a circle wipe round the actor: the radius
; grows by one and the circle is drawn round its place on screen (8 pixels
; above its y), then the $200-byte window table is copied from $7E:5000 to
; $7F:F500.
; HOW: $45 up (8-bit); X = its x less LAYER0_X, Y = its y less LAYER0_Y
; less 8, A = $45 (16-bit); CircleWipeStep; MVN $7E:5000 to $7F:F500, $200
; bytes, DB kept; M and X 8-bit.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; (83:EA38, 83:EA56). Exit: M and X 8-bit; A = $FFFF, X = $5200, Y = $F700
; (low bytes).
%at($83EAAF)
ActorCircleWipeStep:
    inc.b !WIPE_RADIUS
    rep.b #$30
    sec
    lda.b !actor_x+1
    sbc.w !LAYER0_X
    tax
    sec
    lda.b !actor_y+1
    sbc.w !LAYER0_Y
    sec
    sbc.w #$0008
    tay
    lda.b !WIPE_RADIUS
    jsl.l CircleWipeStep
    phb
    ldx.w #$5000
    ldy.w #$F500
    lda.w #$01FF
    mvn $7E,$7F
    plb
    sep.b #$30
    rts
