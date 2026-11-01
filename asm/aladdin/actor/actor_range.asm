; actor_range.asm: actors of the first pool that move out of range of the
; camera are ended, and their placement may place them again.
; C: src/aladdin/actor/actor_range.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Where the actor loop takes the next record (docs/aladdin/ACTORS.md).
!actor_loop_next = $8394BF
!PLACED_FLAGS = $7EC600         ; a byte per placement: bit 7 set, never placed again
!RANGE_X = $1A44                ; the point actors are kept in range of (words)
!RANGE_Y = $1A46
; In the bank DB names, 8 bytes an entry by X: added to the x and y
; differences, then the spans they must be below (words).
!RANGE_BOXES = $CE73

; The end of the routine before ($83:95FA up, not yet filed), the way out
; ActorInRange shares: 8-bit registers.
%at($839629)
WidthsReturn8:
    sep.b #$30
    rts

; @routine 83:962C ActorInRange
; WHAT IT DOES: Tells whether the actor is out of range of the camera's
; point: C set when it is, on either axis, by the box X picks.
; HOW: For x then y: the point less the actor's pixels, plus the box's
; offset, compared unsigned with its span. In range: out through the
; SEP #$30; RTS at $83:9629, the end of the routine before.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM. Exit: as the C states.
ActorInRange:
    rep.b #$20
    sec
    lda.w !RANGE_X
    sbc.b !actor_x+1
    clc
    adc.w !RANGE_BOXES,x
    cmp.w !RANGE_BOXES+2,x
    bcs .out
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w !RANGE_BOXES+4,x
    cmp.w !RANGE_BOXES+6,x
    bcc WidthsReturn8
.out:
    rts

; @routine 83:9668 ActorFreeOutOfRangeLong
; WHAT IT DOES: ActorFreeOutOfRange for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorFreeOutOfRange; called by JSL. Exit: as the C states.
%at($839668)
ActorFreeOutOfRangeLong:
    jsr.w ActorFreeOutOfRange
    rtl

; @routine 83:966C ActorFreeOutOfRange
; WHAT IT DOES: Ends the actor (ActorFreePlaced) when it is out of range
; of the camera's point by box X.
; HOW: ActorInRange, then ActorFreePlaced when C is set; 8-bit registers
; either way.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM. Exit: as the C states.
ActorFreeOutOfRange:
    jsr.w ActorInRange
    bcc .done
    jsr.w ActorFreePlaced
.done:
    sep.b #$30
    rts

; @routine 83:96BE ActorEndOutOfRange
; WHAT IT DOES: Ends the actor for good when it is out of range of the
; camera's point by box X; otherwise returns to it.
; HOW: ActorInRange; out of range, the call's return address is dropped,
; ActorFreePlaced, and into the actor loop for the next record.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSR from the
; actor's code at its own level. Exit: as the C states.
%at($8396BE)
ActorEndOutOfRange:
    jsr.w ActorInRange
    bcs .end
    sep.b #$30
    rts
.end:
    sep.b #$30
    pla
    pla
    jsr.w ActorFreePlaced
    jmp.w !actor_loop_next

; @routine 83:96E2 ActorFreePlaced
; WHAT IT DOES: Ends an actor of the first pool: its placement may place
; it again (unless marked never), its record is cleared and goes back on
; the free stack, and its animated-tile slot is let go.
; HOW: The placed flag (by the actor's byte at $34) is cleared unless bit
; 7 is set; ActorClear; the record's address pushed (2 bytes) on the free
; stack; then on into ActorTileSlotRelease.
; Entry: any widths; D = the actor's record; DB a bank whose $0000-$1FFF
; is work RAM. Exit: as the C states.
%at($8396E2)
ActorFreePlaced:
    sep.b #$30
    ldx.b !actor_34
    lda.l !PLACED_FLAGS,x
    bmi .clear
    lda.b #$00
    sta.l !PLACED_FLAGS,x
.clear:
    jsr.w ActorClear
    inc.w !actor_free_top
    inc.w !actor_free_top
    ldy.w !actor_free_top
    tdc
    sta.w !actor_free_list,y
    xba
    sta.w !actor_free_list+1,y

; @routine 83:96D4 ActorFreePlacedAtY
; WHAT IT DOES: ActorFreePlaced for the actor whose record Y points at,
; from another actor's code.
; HOW: P and D are kept on the stack; D = Y for the call.
; Entry: X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($8396D4)
ActorFreePlacedAtY:
    php
    phd
    phy
    pld
    jsr.w ActorFreePlaced
    pld
    plp
    rts
