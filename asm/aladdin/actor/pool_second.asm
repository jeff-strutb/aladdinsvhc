; pool_second.asm: the second pool of records ($09CB-$0B0B): all of it freed at once.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/pool_second.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!POOL_SECOND = $09CB            ; the second pool's first record; five of $50 bytes
!POOL_SECOND_END = $0B5B        ; past its last

; @routine 83:95C1 SecondPoolFreeAll
; WHAT IT DOES: Frees every record of the second pool that is in use (as a
; level ends).
; HOW: D steps through the five records from $09CB: one whose state is
; nonzero gets ActorFree and ActorTileSlotRelease. D and P kept.
; Entry: any widths; DB a bank whose $0000-$1FFF is work RAM; called by
; JSL (from 81:E7B5). Exit: D and P as at entry; A, X, Y as the last
; record left them.
%at($8395C1)
SecondPoolFreeAll:
    phd
    php
    rep.b #$20
    lda.w #!POOL_SECOND
    tcd
.l95C9:
    ldx.b !actor_state
    beq .l95D7
    sep.b #$20
    jsr.w ActorFree
    jsr.w ActorTileSlotRelease
    rep.b #$20
.l95D7:
    clc
    tdc
    adc.w #$0050
    tcd
    cmp.w #!POOL_SECOND_END
    bne .l95C9
    plp
    pld
    rtl
