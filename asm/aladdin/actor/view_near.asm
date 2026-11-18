; view_near.asm: whether an actor is near the middle of the view, across
; or up and down. No code refers to either test.
;
; The actor is the record the direct page is. Coordinates are the
; positions' whole pixels.
; C: src/aladdin/actor/view_near.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

!view_left = $1773              ; the view's left edge (pixels, word)
!view_top = $1777               ; the view's top edge (pixels, word)
!VIEW_HALF = $0080              ; from the edge to the view's middle
!view_span_x = $BC7B            ; in the data bank ($80): half-widths, a word each
!view_span_y = $BC85            ; and half-heights

; @routine 81:AD48 ActorNearViewX
; WHAT IT DOES: Says whether the actor is within a half-width of the
; view's middle across ($80 past its left edge). No code refers to it.
; HOW: Span Y's half-width kept in scratch with its double; x - the view's
; left edge - $80 + the half-width is below the width (unsigned) when
; within.
; Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
; and that holds the spans ($80); Y = the span's byte offset; called by
; JSL. Exit: C clear when within; A = the sum (16 bits); N, Z, C from its
; compare; scratch $0C-$0F = half-width, width; M 8-bit; X, Y, V
; unchanged.
%at($81AD48)
ActorNearViewX:
    rep.b #$20
    lda.w !view_span_x,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    sec
    lda.b !actor_x+1
    sbc.w !view_left
    sec
    sbc.w #!VIEW_HALF
    clc
    adc.w !scratch
    cmp.w !scratch+2
    sep.b #$20
    rtl

; @routine 81:AD84 ActorNearViewY
; WHAT IT DOES: Says whether the actor is within a half-height of the
; view's middle up and down ($80 below its top). No code refers to it.
; HOW: As ActorNearViewX with the half-height of span Y, the actor's y
; and the view's top.
; Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
; and that holds the spans ($80); Y = the span's byte offset; called by
; JSL. Exit: C clear when within; A = the sum (16 bits); N, Z, C from its
; compare; scratch $0C-$0F = half-height, height; M 8-bit; X, Y, V
; unchanged.
%at($81AD84)
ActorNearViewY:
    rep.b #$20
    lda.w !view_span_y,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    sec
    lda.b !actor_y+1
    sbc.w !view_top
    sec
    sbc.w #!VIEW_HALF
    clc
    adc.w !scratch
    cmp.w !scratch+2
    sep.b #$20
    rtl
