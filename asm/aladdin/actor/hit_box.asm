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
!PLAYER_STATE_8C_COUNT = $141B   ; counted up on entering state $8C

; @routine 83:8304 ActorHitsTestBoxIfPlayerFree
; WHAT IT DOES: ActorHitsTestBoxIfActive, while Aladdin can be hit: bit 7
; of his byte at $08 clear and no state $8C counted; otherwise "no".
; HOW: The two tests, then on into ActorHitsTestBoxIfActive.
; Entry: as ActorHitsTestBox. Exit: as the C states.
%at($838304)
ActorHitsTestBoxIfPlayerFree:
    lda.w !player+!actor_08
    bmi NoHitReturn
    lda.w !PLAYER_STATE_8C_COUNT
    bne NoHitReturn

; @routine 83:830E ActorHitsTestBoxIfActive
; WHAT IT DOES: ActorHitsTestBox, for an actor whose byte at $08 has bit
; 7 clear; otherwise "no".
; HOW: The byte's sign, then on into ActorHitsTestBox.
; Entry: as ActorHitsTestBox. Exit: as the C states.
ActorHitsTestBoxIfActive:
    lda.b !actor_08
    bmi NoHitReturn

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
    bvc NoHitReturn
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
    bcs NoHitReturn
    sec
    lda.b !actor_y+1
    sbc.w !TEST_BOX_Y
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
    sep.b #$30
    rts

; The hit tests' shared "no": C set, 8-bit registers.
NoHitReturn:
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

; @routine 83:82F8 ActorHitsTestBoxIfActiveLong
; WHAT IT DOES: ActorHitsTestBoxIfActive for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorHitsTestBox; called by JSL. Exit: as the C states.
%at($8382F8)
ActorHitsTestBoxIfActiveLong:
    jsr.w ActorHitsTestBoxIfActive
    rtl

; @routine 83:80A5 ActorHurtsPlayerOnTouch
; WHAT IT DOES: When the actor touches Aladdin while he can be hit, hurts
; him (state $8C), turned away from the actor.
; HOW: ActorHitsTestBoxIfPlayerFree; on a hit, on into
; PlayerEnter8CFacing.
; Entry: as ActorHitsTestBox. Exit: as the C states.
%at($8380A5)
ActorHurtsPlayerOnTouch:
    jsr.w ActorHitsTestBoxIfPlayerFree
    bcs .no_hit
    jmp.w PlayerEnter8CFacing
.no_hit:
    rts
