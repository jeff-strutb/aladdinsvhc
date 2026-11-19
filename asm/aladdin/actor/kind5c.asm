; kind5c.asm: actor kind $5C of level 8, the Genie's head that leads
; Aladdin across: once he is right of x $08A0 it flies in to x $0900 and
; then along a path (one by his lives) of points, bobbing about its line;
; at each point it makes the next of its children (clouds, kinds $5D, $8B,
; $8C) and, after the first, waits until he has landed on the last one;
; at the path's end it flies off and ends. Out of view, it ends; with
; Aladdin out of health it only bobs. Its wait is Kind5CWait and its bob
; ActorBobAboutLine (kind_waits_rest.asm).
; C: src/aladdin/actor/kind5c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_state.inc"

arch 65816

!K5C_PATH = $16                 ; its path: points of 4 bytes (x word, the child's kind, 0), $FFFF ends it (word)
!K5C_FIRST_DONE = $18           ; nonzero once its first child is made (it waits for the next ones)
!K5C_CHILD = $37                ; its last child's record (word)
!K5C_LINE = $40                 ; the y (word) it bobs about
!K5C_TARGET = $42               ; the point it flies to: x pixels (word)
!K5C_TOP_SPEED = $44            ; its speed's limit by distance (word)
!K5C_SPEED = $46                ; its speed across (word: fraction, pixels)
!K5C_STEP = $48                 ; its speed's change a frame by distance
!K5C_HEADING = $49              ; the way it moves: 0 right, 1 left
!K5C_AT = $4A                   ; its point's offset in the path
!K5C_GONE = $4E                 ; counted up as it ends
!K5C_LIVES = $0364              ; Aladdin's lives left
!K5C_BOB = $13                  ; ActorBobAboutLine's state
!K5C_CHILD_LANDED = $40         ; a child's: nonzero once Aladdin has landed on it
!K5C_START_X = $08A0            ; Aladdin's x it waits for, and its own x as it comes in
!K5C_IN_X = $0900               ; the x it flies in to
!K5C_LOOK = $890B               ; its K5C_29 (word)
!K5C_29 = $29                   ; set to K5C_LOOK at its start; purpose not yet established (word)
!K5C_PATHS = $D428              ; bank $80: its path by Kind5CPathByLives (words)
!K5C_SPEEDS = $D408             ; bank $80: by distance / 32, its speed limit (word) and step
!K5C_IN_PRESET = $45            ; ActorMotionPreset's preset for its flight in
!K5C_OUT_PRESET = $46           ; and for its flight off
!K5C_TURN_STEP = $08            ; its speed's change a frame while it turns about
!K5C_NEAR = $0080               ; distances from here on take the farthest speed
!K5C_FAR = $0070                ; the distance they are taken as
!K5C_TAKEN = $000C              ; Kind5CMove: what it takes from the speed

; @routine 83:E000 Kind5CStart
; @actor
; WHAT IT DOES: Actor kind $5C: waits until Aladdin is right of x $08A0,
; then comes in at that x above the view and flies in to x $0900; there it
; bobs about its line and follows its path (by his lives): to each point,
; then (from the second point on, once he has landed on its last child)
; its poses, the point's child made (its $07 the point's number), the next
; point; at the path's end it waits 176 frames, flies off until out of
; view, and ends.
; HOW: 16-bit: each frame (BRK first) until $08A0 is below his x; x
; pixels $08A0, $29 $890B, y pixels $FFFF; 8-bit: $12 $18, $08 2,
; K5C_SPEED, K5C_STEP, K5C_HEADING, K5C_GONE and K5C_FIRST_DONE 0;
; K5C_PATH from K5C_PATHS by Kind5CPathByLives; K5C_AT 0; $21 $50, $09
; $90, redraw; ActorMotionPreset $45, AnimStart 0; each frame (BRK first)
; ActorFlyAgainst until its x is $0900 or more. AnimStart 1; flip 1;
; K5C_LINE its y; K5C_BOB up; COP $3F. Each point: K5C_TARGET from it;
; each frame (BRK first) Kind5CFace, Kind5CSpeedPick, Kind5CMove until its
; x reaches it. First point: K5C_FIRST_DONE up; else each frame (BRK
; first) until its child's K5C_CHILD_LANDED is set. AnimStart 2, COP $0F,
; AnimStart 3; each frame (BRK first) until ActorCreateChild with the
; point's kind makes one ($07 = K5C_AT / 4): K5C_CHILD it (16-bit X
; stored), the child's $37-$38 ours, its K5C_CHILD_LANDED 0; COP $0F; the
; next point, until its x word is $FFFF. Then COP $B0, K5C_BOB 0,
; AnimStart 0, ActorMotionPreset $46; each frame (BRK first)
; ActorFlyAgainst while $09 bit 6 is set; Kind5CUnhittable, COP $FF,
; K5C_GONE up, JMP ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $5C in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83E000)
Kind5CStart:
    rep.b #$20
.wait:
    brk.b #$00
    lda.w #!K5C_START_X
    cmp.w !player+!actor_x+1
    bcs .wait
    sta.b !actor_x+1
    lda.w #!K5C_LOOK
    sta.b !K5C_29
    lda.w #$FFFF
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$18
    sta.b !actor_12
    lda.b #$02
    sta.b !actor_08
    stz.b !K5C_SPEED
    stz.b !K5C_SPEED+1
    stz.b !K5C_STEP
    stz.b !K5C_HEADING
    stz.b !K5C_GONE
    stz.b !K5C_FIRST_DONE
    jsr.w Kind5CPathByLives
    asl a
    tax
    lda.w !K5C_PATHS,x
    sta.b !K5C_PATH
    lda.w !K5C_PATHS+1,x
    sta.b !K5C_PATH+1
    stz.b !K5C_AT
    lda.b #$50
    sta.b !actor_spawn_at
    lda.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    ldy.b #!K5C_IN_PRESET
    jsl.l ActorMotionPreset
    lda.b #$00
    jsl.l AnimStart
.in:
    brk.b #$00
    jsl.l ActorFlyAgainst
    rep.b #$20
    lda.b !actor_x+1
    cmp.w #!K5C_IN_X
    sep.b #$20
    bcc .in
    lda.b #$01
    jsl.l AnimStart
    lda.b #$01
    sta.b !actor_flip
    lda.b !actor_y+1
    sta.b !K5C_LINE
    lda.b !actor_y+2
    sta.b !K5C_LINE+1
    inc.b !K5C_BOB
    lda.b #$3F
    cop.b #$00
.point:
    ldy.b !K5C_AT
    lda.b (!K5C_PATH),y
    sta.b !K5C_TARGET
    iny
    lda.b (!K5C_PATH),y
    sta.b !K5C_TARGET+1
.fly:
    brk.b #$00
    jsr.w Kind5CFace
    jsr.w Kind5CSpeedPick
    jsr.w Kind5CMove
    ldy.b !K5C_AT
    rep.b #$20
    lda.b !actor_x+1
    cmp.b (!K5C_PATH),y
    sep.b #$20
    bcs .there
    sep.b #$20
    bra .fly
.there:
    lda.b !K5C_FIRST_DONE
    bne .landed
    inc.b !K5C_FIRST_DONE
    bra .make
.landed:
    brk.b #$00
    rep.b #$10
    ldx.b !K5C_CHILD
    lda.w !K5C_CHILD_LANDED,x
    sep.b #$10
    beq .landed
.make:
    lda.b #$02
    jsl.l AnimStart
    lda.b #$0F
    cop.b #$00
    lda.b #$03
    jsl.l AnimStart
.child:
    brk.b #$00
    lda.b !K5C_AT
    pha
    lsr a
    lsr a
    sta.b !actor_07
    ply
    iny
    iny
    lda.b (!K5C_PATH),y
    jsl.l ActorCreateChild
    bcs .child
    stx.b !K5C_CHILD
    tdc
    sta.w !K5C_CHILD,x
    xba
    sta.w !K5C_CHILD+1,x
    stz.w !K5C_CHILD_LANDED,x
    sep.b #$10
    lda.b #$0F
    cop.b #$00
    ldy.b !K5C_AT
    iny
    iny
    iny
    iny
    sty.b !K5C_AT
    lda.b (!K5C_PATH),y
    cmp.b #$FF
    bne .point
    lda.b #$B0
    cop.b #$00
    stz.b !K5C_BOB
    lda.b #$00
    jsl.l AnimStart
    ldy.b #!K5C_OUT_PRESET
    jsl.l ActorMotionPreset
.off:
    brk.b #$00
    jsl.l ActorFlyAgainst
    bit.b !actor_09
    bvs .off
    jsr.w Kind5CUnhittable
    lda.b #$FF
    cop.b #$00
    inc.b !K5C_GONE
    jmp.w ActorEndPlaced

; @routine 83:E128 Kind5CGone
; @actor
; WHAT IT DOES: Kind $5C out of the view: it can no longer be hit, waits
; 31 frames and ends for good.
; HOW: Kind5CUnhittable; K5C_GONE up; COP $1F; JMP ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's RTI, Kind5CWait having made it its resume address.
; Exit: never returns.
%at($83E128)
Kind5CGone:
    jsr.w Kind5CUnhittable
    inc.b !K5C_GONE
    lda.b #$1F
    cop.b #$00
    jmp.w ActorEndPlaced

; @routine 83:E134 Kind5CUnhittable
; WHAT IT DOES: Makes the actor one that cannot be hit and is not tested:
; $09 bit 7 and $08 bit 3 cleared.
; HOW: ASL then LSR of $09; $08 AND $F7.
; Entry: M 8-bit; D = the actor's record; called by JSR. Exit: A = the
; new $08; N and Z from it; C = bit 7 of $09 before.
%at($83E134)
Kind5CUnhittable:
    asl.b !actor_09
    lsr.b !actor_09
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    rts

; @routine 83:E13F Kind5CSpeedPick
; WHAT IT DOES: Picks kind $5C's speed limit and step by its distance to
; its point: the nearer, the slower.
; HOW: 16-bit: |its x less K5C_TARGET|, $80 or more taken as $70; / 4
; AND $1C picks the entry of K5C_SPEEDS: K5C_TOP_SPEED its word, K5C_STEP
; its next byte.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit; X = the entry's offset; Y = the step.
%at($83E13F)
Kind5CSpeedPick:
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.b !K5C_TARGET
    bpl .ahead
    eor.w #$FFFF
    inc a
.ahead:
    cmp.w #!K5C_NEAR
    bcc .near
    lda.w #!K5C_FAR
.near:
    lsr a
    lsr a
    and.w #$001C
    tax
    lda.w !K5C_SPEEDS,x
    sta.b !K5C_TOP_SPEED
    ldy.w !K5C_SPEEDS+2,x
    sty.b !K5C_STEP
    sep.b #$20
    rts

; @routine 83:E167 Kind5CMove
; WHAT IT DOES: Moves kind $5C across by its speed: heading the way it
; faces, its speed grows by its step up to its limit (or drops to it);
; facing the other way, its speed drops by 8 a frame until it turns
; about.
; HOW: 16-bit X. Facing ($14) as K5C_HEADING: limit below the speed:
; the speed = the limit; else the speed + the step (16-bit by bytes), at
; most the limit. Else (turning, $000C = 8) or above the limit ($000C =
; the step): the speed less $000C; below 0: speed 0, K5C_HEADING the
; facing. Then its 24-bit x plus the speed (heading right) or less it.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; M 8-bit, X 8-bit; A = its x's top byte; C from its addition.
%at($83E167)
Kind5CMove:
    rep.b #$10
    lda.b !actor_facing
    cmp.b !K5C_HEADING
    bne .turn
    ldx.b !K5C_TOP_SPEED
    cpx.b !K5C_SPEED
    bcc .slower
    clc
    lda.b !K5C_SPEED
    adc.b !K5C_STEP
    sta.b !K5C_SPEED
    lda.b !K5C_SPEED+1
    adc.b #$00
    sta.b !K5C_SPEED+1
    cpx.b !K5C_SPEED
    bcs .move
    stx.b !K5C_SPEED
    bra .move
.turn:
    lda.b #!K5C_TURN_STEP
    bra .less
.slower:
    lda.b !K5C_STEP
.less:
    sta.w !K5C_TAKEN
    sec
    lda.b !K5C_SPEED
    sbc.w !K5C_TAKEN
    sta.b !K5C_SPEED
    lda.b !K5C_SPEED+1
    sbc.b #$00
    sta.b !K5C_SPEED+1
    lda.b #$00
    sbc.b #$00
    bpl .move
    stz.b !K5C_SPEED
    stz.b !K5C_SPEED+1
    lda.b !actor_facing
    sta.b !K5C_HEADING
.move:
    sep.b #$10
    lda.b !K5C_HEADING
    bne .left
    clc
    lda.b !actor_x
    adc.b !K5C_SPEED
    sta.b !actor_x
    lda.b !actor_x+1
    adc.b !K5C_SPEED+1
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.b #$00
    sta.b !actor_x+2
    bra .done
.left:
    sec
    lda.b !actor_x
    sbc.b !K5C_SPEED
    sta.b !actor_x
    lda.b !actor_x+1
    sbc.b !K5C_SPEED+1
    sta.b !actor_x+1
    lda.b !actor_x+2
    sbc.b #$00
    sta.b !actor_x+2
.done:
    rts

; @routine 83:E1DE Kind5CFace
; WHAT IT DOES: Faces kind $5C toward its point: $14 0 when the point is
; at or right of its x, 1 when left.
; HOW: 16-bit: X 0; K5C_TARGET below its x pixels: X 1; STX $14.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; M 8-bit; X = the facing; C from the compare.
%at($83E1DE)
Kind5CFace:
    rep.b #$20
    ldx.b #$00
    lda.b !K5C_TARGET
    cmp.b !actor_x+1
    bcs .right
    inx
.right:
    stx.b !actor_facing
    sep.b #$20
    rts
%ends_at($83E1EE)

; @routine 83:E22A Kind5CPathByLives
; WHAT IT DOES: Kind $5C's path (and its children's) by Aladdin's lives:
; 0 with one, 1 with two or three, 2 with four or more.
; HOW: A 0; X = lives less one: zero, done; A up; below 3, done; A up.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A = the path;
; X = lives less one; N, Z and C from the last DEX, INC or CPX.
%at($83E22A)
Kind5CPathByLives:
    lda.b #$00
    ldx.w !K5C_LIVES
    dex
    beq .done
    inc a
    cpx.b #$03
    bcc .done
    inc a
.done:
    rts
%ends_at($83E239)

; @routine 83:E282 Kind5CDone
; @actor
; WHAT IT DOES: Kind $5C with Aladdin out of health: its pose 4, and only
; its bob from then on, animated and drawn.
; HOW: AnimStart 4, ActorPictureSend; each frame (BRK first)
; ActorBobAboutLine, AnimStep, ActorPictureSend.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's RTI, Kind5CWait having made it its resume address.
; Exit: never returns.
%at($83E282)
Kind5CDone:
    lda.b #$04
    jsl.l AnimStart
    jsl.l ActorPictureSend
.frame:
    brk.b #$00
    jsr.w ActorBobAboutLine
    jsl.l AnimStep
    jsl.l ActorPictureSend
    bra .frame
%ends_at($83E29B)
