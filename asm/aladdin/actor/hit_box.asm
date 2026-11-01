; hit_box.asm: whether an actor's box overlaps the box actors are tested
; against.
; C: src/aladdin/actor/hit_box.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

; The box tested against: centre x and y, half-width and half-height
; (words). Actors' half-sizes: two words an entry by !actor_06, in the
; bank DB names.
!TEST_BOX_X = $1484
!TEST_BOX_Y = $1486
!TEST_BOX_HALF_W = $1488
!TEST_BOX_HALF_H = $148A
!HIT_BOXES = $C29A

; @routine 83:8312 ActorHitsTestBox
; WHAT IT DOES: Tells whether the actor, when it can be hit (bit 6 of its
; byte at $09), overlaps the test box.
; HOW: The two half-widths and the two half-heights are added (in scratch
; words, with their doubles); the boxes overlap on an axis when the
; difference of centres plus the sum is below the double, unsigned. X is
; tested first.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank. Exit: as the C states.
%at($838312)
ActorHitsTestBox:
    bit.b !actor_09
    bvc .not_hit
    lda.b !actor_06
    rep.b #$30
    and.w #$00FF
    asl a
    asl a
    tay
    clc
    lda.w !HIT_BOXES,y
    adc.w !TEST_BOX_HALF_W
    sta.w !scratch
    asl a
    sta.w !scratch+2
    clc
    lda.w !HIT_BOXES+2,y
    adc.w !TEST_BOX_HALF_H
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    sec
    lda.b !actor_x+1
    sbc.w !TEST_BOX_X
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .not_hit
    sec
    lda.b !actor_y+1
    sbc.w !TEST_BOX_Y
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    sep.b #$30
    rts
.not_hit:
    sep.b #$31
    rts

; @routine 83:8300 ActorHitsTestBoxLong
; WHAT IT DOES: ActorHitsTestBox for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorHitsTestBox; called by JSL. Exit: as the C states.
%at($838300)
ActorHitsTestBoxLong:
    jsr.w ActorHitsTestBox
    rtl
