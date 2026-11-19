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
!CHILD_A = $40                  ; ActorEndFarWithChildren: its first child's record (word)
!CHILD_B = $42                  ; its second child's record (word)
!CHILD_COUNT = $44              ; how many children it has (1 or 2)
; In the bank DB names, 8 bytes an entry by X: added to the x and y
; differences, then the spans they must be below (words).
!RANGE_BOXES = $CE73
!X_RANGE_BOXES = $CEDB             ; the across-only boxes: offset, span (words), by X
!Y_RANGE_BOXES = $CEEB             ; the up-and-down boxes likewise

; @routine 83:95FA ActorEndOutOfView
; WHAT IT DOES: Ends the actor for good when it is out of view (its $09
; bit 6 clear, as ActorsInView leaves it); else returns. Nothing in the
; game calls it (no JSR, JSL, JMP, pointer or word in the ROM names it):
; unit_bank83_out_of_view runs it in view.
; HOW: BIT $09; V set: RTS. Else two bytes pulled (its JSR's return
; address) and on to ActorEndPlaced.
; @unreachable 83:95FE-9602 The end: it pulls a caller's return that no
; caller pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM
; names 83:95FA, and a unit call at it would pull the stub's own return
; and go into the actor loop with no actor running (master, 2026-11-19).
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR from the actor's code at its own level. Exit:
; in view: N, V and Z from the BIT; else never returns.
%at($8395FA)
ActorEndOutOfView:
    bit.b !actor_09
    bvs .keep
    pla
    pla
    jmp.w ActorEndPlaced
.keep:
    rts

; @routine 83:9604 ActorEndOutOfViewLong
; WHAT IT DOES: ActorEndOutOfView for callers in other banks. Nothing in
; the game calls it (no JSL, JMP, pointer or word names it):
; unit_bank83_out_of_view runs it in view.
; HOW: BIT $09; V set: RTL. Else three bytes pulled (its JSL's return
; address) and on to ActorEndPlaced.
; @unreachable 83:9608-960D The end: it pulls a caller's return that no
; caller pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM
; names 83:9604, and a unit call at it would pull the stub's own return
; and go into the actor loop with no actor running (master, 2026-11-19).
; Entry: as ActorEndOutOfView; called by JSL. Exit: as ActorEndOutOfView.
ActorEndOutOfViewLong:
    bit.b !actor_09
    bvs .keep
    pla
    pla
    pla
    jmp.w ActorEndPlaced
.keep:
    rtl

; @routine 83:960F ActorEndOutOfViewDrop4
; WHAT IT DOES: ActorEndOutOfView one call further down: out of view it
; drops its own return and its caller's. Nothing in the game calls it (no
; JSR, JMP, pointer or word names it): unit_bank83_out_of_view runs it in
; view.
; HOW: BIT $09; V set: to ActorEndOutOfView's RTS (83:9603). Else four
; bytes pulled (two JSRs' return addresses) and on to ActorEndPlaced.
; @unreachable 83:9613-9619 The end: it pulls a caller's return that no
; caller pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM
; names 83:960F, and a unit call at it would pull the stub's own return
; and go into the actor loop with no actor running (master, 2026-11-19).
; Entry: as ActorEndOutOfView; called by JSR from a routine the actor's
; code called by JSR. Exit: as ActorEndOutOfView.
ActorEndOutOfViewDrop4:
    bit.b !actor_09
    bvs ActorEndOutOfView_keep
    pla
    pla
    pla
    pla
    jmp.w ActorEndPlaced

; @routine 83:961A ActorEndOutOfViewLongDrop8
; WHAT IT DOES: ActorEndOutOfViewLong further down: out of view it drops
; eight bytes of returns. Nothing in the game calls it (no JSL, JMP,
; pointer or word names it): unit_bank83_out_of_view runs it in view.
; HOW: BIT $09; V set: to ActorEndOutOfViewLong's RTL (83:960E). Else
; eight bytes pulled and on to ActorEndPlaced.
; @unreachable 83:961E-9628 The end: it pulls a caller's return that no
; caller pushes: no JSR, JSL, JMP, branch, pointer or word in the ROM
; names 83:961A, and a unit call at it would pull the stub's own return
; and go into the actor loop with no actor running (master, 2026-11-19).
; Entry: as ActorEndOutOfView; called by JSL, eight bytes of returns
; above the actor's own level. Exit: as ActorEndOutOfView.
ActorEndOutOfViewLongDrop8:
    bit.b !actor_09
    bvs ActorEndOutOfViewLong_keep
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    jmp.w ActorEndPlaced

; The way out ActorInRange shares, after the routine before: 8-bit
; registers.
WidthsReturn8:
    sep.b #$30
    rts

; @routine 83:962C ActorInRange
; WHAT IT DOES: Tells whether the actor is out of range of the camera's
; point: C set when it is, on either axis, by the box X picks.
; HOW: For x then y: the point less the actor's pixels, plus the box's
; offset, compared unsigned with its span. In range: out through the
; SEP #$30; RTS at $83:9629, after the routine before.
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

; @routine 83:964D ActorInRangeLong
; WHAT IT DOES: ActorInRange for callers in other banks, with M and X
; 8-bit after it. Nothing in the game calls it (no JSL, JMP, pointer or
; word names it): unit_bank83_ranges runs it.
; HOW: JSR ActorInRange; SEP #$30; RTL.
; Entry: as ActorInRange; called by JSL. Exit: C set when out of range;
; M and X 8-bit.
%at($83964D)
ActorInRangeLong:
    jsr.w ActorInRange
    sep.b #$30
    rtl

; @routine 83:9653 ActorInRange8
; WHAT IT DOES: ActorInRange, with M and X 8-bit after it either way.
; No caller in the ROM (2026-11-18): no JSR, JMP, JSL or JML to it, no word
; $9653 (a table entry) or $9652 (a pushed RTS address) in bank $83, and
; none of the 233 scripts' traces runs it; unit_actor_in_range8 calls it.
; HOW: JSR ActorInRange; SEP #$30; RTS.
; Entry: as ActorInRange; called by JSR. Exit: C set when out of range;
; M and X 8-bit; A, N, V and Z as ActorInRange leaves them.
%at($839653)
ActorInRange8:
    jsr.w ActorInRange
    sep.b #$30
    rts

; @routine 83:9659 ActorEndOutOfRangeLong
; WHAT IT DOES: Ends the actor for good when it is out of range of the
; camera's point by box X; otherwise returns to it. For callers in other
; banks.
; HOW: ActorInRange (JSR); in range: SEP #$30, RTL. Out of range: SEP
; #$30, the JSL's three return bytes pulled, on into ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSL from the
; actor's code at its own level (84:9235, 84:849B). Exit: in range: as
; ActorInRange, M and X 8-bit. Out of range: never returns; as
; ActorEndPlaced.
%at($839659)
ActorEndOutOfRangeLong:
    jsr.w ActorInRange
    bcs .out
    sep.b #$30
    rtl
.out:
    sep.b #$30
    pla
    pla
    pla
    bra ActorEndPlaced

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

; @routine 83:9677 ActorFreeOutOfRangeXLong
; WHAT IT DOES: ActorFreeOutOfRangeX for callers in other banks. Nothing
; in the game calls it (no JSL, JMP, pointer or word names it; the word
; at 83:9EEB is an operand byte): unit_bank83_ranges runs it.
; HOW: JSR ActorFreeOutOfRangeX, then RTL.
; Entry: as ActorFreeOutOfRangeX; called by JSL. Exit: as
; ActorFreeOutOfRangeX.
%at($839677)
ActorFreeOutOfRangeXLong:
    jsr.w ActorFreeOutOfRangeX
    rtl

; @routine 83:967B ActorFreeOutOfRangeX
; WHAT IT DOES: Frees the actor (ActorFreePlaced) when it is out of range
; of the camera's point across, by box X; else nothing. Nothing in the
; game calls it but its long entry, which nothing calls.
; HOW: ActorInRangeX; C set: ActorFreePlaced. SEP #$20.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSR. Exit: M
; 8-bit; C set when out of range.
ActorFreeOutOfRangeX:
    jsr.w ActorInRangeX
    bcc .done
    jsr.w ActorFreePlaced
.done:
    sep.b #$20
    rts

; @routine 83:9686 ActorEndOutOfRangeX
; WHAT IT DOES: Ends the actor for good when it is out of range of the
; camera's point across, by box X (ActorEndDropReturn); else returns.
; Nothing in the game calls it but its long entry, which nothing calls:
; unit_bank83_ranges runs it in range.
; HOW: ActorInRangeX; C set: BCS to ActorEndDropReturn; SEP #$20, RTS.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSR from the
; actor's code at its own level. Exit: in range: M 8-bit, C clear; else
; never returns.
ActorEndOutOfRangeX:
    jsr.w ActorInRangeX
    bcs ActorEndDropReturn
    sep.b #$20
    rts

; @routine 83:968E ActorInRangeXLong
; WHAT IT DOES: ActorInRangeX for callers in other banks. Nothing in the
; game calls it (no JSL, JMP, pointer or word names it):
; unit_bank83_ranges runs it.
; HOW: JSR ActorInRangeX, then RTL.
; Entry: as ActorInRangeX; called by JSL. Exit: as ActorInRangeX.
ActorInRangeXLong:
    jsr.w ActorInRangeX
    rtl

; @routine 83:9692 ActorInRangeX
; WHAT IT DOES: Tells whether the actor is out of range of the camera's
; point across, by box X of the across-only boxes: C set when it is.
; HOW: 16-bit: the point's x less the actor's x pixels, plus the box's
; offset (X_RANGE_BOXES), compared unsigned with its span; SEP #$20.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSR. Exit: M
; 8-bit; C set when out of range; A = the sum (16 bits).
ActorInRangeX:
    rep.b #$20
    sec
    lda.w !RANGE_X
    sbc.b !actor_x+1
    clc
    adc.w !X_RANGE_BOXES,x
    cmp.w !X_RANGE_BOXES+2,x
    sep.b #$20
    rts

; @routine 83:96A4 ActorInRangeYLong
; WHAT IT DOES: ActorInRangeY for callers in other banks. Nothing in the
; game calls it (no JSL, JMP, pointer or word names it):
; unit_bank83_ranges runs it.
; HOW: JSR ActorInRangeY, then RTL.
; Entry: as ActorInRangeY; called by JSL. Exit: as ActorInRangeY.
ActorInRangeYLong:
    jsr.w ActorInRangeY
    rtl

; @routine 83:96A8 ActorInRangeY
; WHAT IT DOES: Tells whether the actor is out of range of the camera's
; point up or down, by box X of the up-and-down boxes: C set when it is.
; Nothing in the game calls it but its long entry, which nothing calls
; (the word at 83:897A is an operand byte).
; HOW: 16-bit: the point's y less the actor's y pixels, plus the box's
; offset (Y_RANGE_BOXES), compared unsigned with its span; SEP #$20.
; Entry: M 8-bit, X 8-bit; X = the box's offset; D = the actor; DB the
; boxes' bank, whose $0000-$1FFF is work RAM; called by JSR. Exit: M
; 8-bit; C set when out of range; A = the sum (16 bits).
ActorInRangeY:
    rep.b #$20
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w !Y_RANGE_BOXES,x
    cmp.w !Y_RANGE_BOXES+2,x
    sep.b #$20
    rts

; @routine 83:96BA ActorEndOutOfRangeXLong
; WHAT IT DOES: ActorEndOutOfRangeX for callers in other banks. Nothing in
; the game calls it (no JSL, JMP, pointer or word names it):
; unit_bank83_ranges runs it in range.
; HOW: JSR ActorEndOutOfRangeX, then RTL.
; Entry: as ActorEndOutOfRangeX; called by JSL. Exit: in range as
; ActorEndOutOfRangeX; else never returns.
ActorEndOutOfRangeXLong:
    jsr.w ActorEndOutOfRangeX
    rtl
%ends_at($8396BE)

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

; @routine 83:96C6 ActorEndDropReturn
; WHAT IT DOES: Ends the actor for good from a routine its code called:
; the call's return address dropped, then as ActorEndPlaced.
; HOW: 8-bit registers; two bytes pulled (the JSR's return address); on
; into ActorEndPlaced. ActorEndOutOfRange's own way out.
; Entry: D = the actor's record; DB a bank whose $0000-$1FFF is work RAM;
; reached by a jump from a routine the actor's code called by JSR, one
; return address above the actor loop's stack level. Exit: never returns;
; as ActorEndPlaced.
ActorEndDropReturn:
    sep.b #$30
    pla
    pla

; @routine 83:96CA ActorEndPlaced
; @actor
; WHAT IT DOES: Ends an actor of the first pool for good: its placement
; may place it again, and the actor loop goes on to the next record.
; HOW: ActorFreePlaced, then into the actor loop where it takes the next
; record; the actor's code is left there.
; Entry: D = the actor's record; DB a bank whose $0000-$1FFF is work RAM;
; reached by a jump from the actor's code, at the actor loop's stack
; level. Exit: never returns; in the actor loop with the registers
; ActorTileSlotRelease leaves.
ActorEndPlaced:
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

; @routine 83:C4AB ActorEndFarWithChildren
; WHAT IT DOES: Ends an actor and its one or two children for good once it
; is 256 pixels or more from the range point, across or up and down,
; unless it can be hit (bit 6 of $09).
; HOW: BIT $09, V set: back. Else RANGE_X less its x plus $100 and
; RANGE_Y less its y plus $100, each below $200: back (M 8-bit). Far:
; ActorFreePlacedAtY for the child at CHILD_A, and for CHILD_B when
; CHILD_COUNT is not 1 (no script runs this); its return dropped and on
; into ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the actor's code at its own
; level. Exit: near: M 8-bit, N, Z and C from the y test (V set: N, V, Z
; from the BIT). Far: never returns; as ActorEndPlaced.
%at($83C4AB)
ActorEndFarWithChildren:
    bit.b !actor_09
    bvs .back
    rep.b #$20
    sec
    lda.w !RANGE_X
    sbc.b !actor_x+1
    clc
    adc.w #$0100
    cmp.w #$0200
    bcs .far
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w #$0100
    cmp.w #$0200
    bcc .near
.far:
    sep.b #$20
    rep.b #$10
    ldy.b !CHILD_A
    jsr.w ActorFreePlacedAtY
    lda.b !CHILD_COUNT
    dec a
    beq .one
    ldy.b !CHILD_B
    jsr.w ActorFreePlacedAtY
.one:
    sep.b #$10
    pla
    pla
    jmp.w ActorEndPlaced
.near:
    sep.b #$20
.back:
    rts
