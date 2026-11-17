; kind4b.asm: actor kind $4B, a figure that sticks to another record for
; 24 frames, its animation following that record's. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind4b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K4B_FRAMES = $40                ; frames left
!K4B_SHOWN = $41                 ; the $07 its animation was last set from
!K4B_ANIM_0 = $05                ; its animations start here, by $07
!K4B_FOLLOWED_40 = $0040         ; the followed record's byte its $07 follows

; @routine 83:CE2A Kind4BStart
; @actor
; WHAT IT DOES: Actor kind $4B, a figure that sticks to the record its
; ACTOR_NEXT names for 24 frames, then ends: each frame it takes that
; record's place, and its animation follows that record's $40 (animation
; 5 plus it).
; HOW: ACTOR_09 = $10; AnimStart with $07 + 5; K4B_SHOWN = $FF; the
; palette byte ($2C) = 6; K4B_FRAMES = $18. Each frame: with 16-bit
; registers its x and y pixels from the record at ACTOR_NEXT; that
; record's $40 into its $07 when it differs; $07 unlike K4B_SHOWN: kept
; there, AnimResume with $07 + 5; AnimStep; BRK; K4B_FRAMES down; at 0,
; ActorEndPlaced. No script changes the record's $40 meanwhile (the
; store to $07): written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $4B
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83CE2A)
Kind4BStart:
    lda.b #$10
    sta.b !actor_09
    clc
    lda.b !actor_07
    adc.b #!K4B_ANIM_0
    jsl.l AnimStart
    lda.b #$FF
    sta.b !K4B_SHOWN
    lda.b #$06
    sta.b !actor_look+1
    lda.b #$18
    sta.b !K4B_FRAMES
.frame:
    rep.b #$30
    ldx.b !actor_next
    lda.w !actor_x+1,x
    sta.b !actor_x+1
    lda.w !actor_y+1,x
    sta.b !actor_y+1
    sep.b #$20
    lda.w !K4B_FOLLOWED_40,x
    cmp.b !actor_07
    beq .same
    sta.b !actor_07
.same:
    sep.b #$10
    lda.b !actor_07
    cmp.b !K4B_SHOWN
    beq .shown
    sta.b !K4B_SHOWN
    clc
    adc.b #!K4B_ANIM_0
    jsl.l AnimResume
.shown:
    jsl.l AnimStep
    brk.b #$00
    dec.b !K4B_FRAMES
    bne .frame
    jml.l ActorEndPlaced
