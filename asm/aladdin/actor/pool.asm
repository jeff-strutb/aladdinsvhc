; pool.asm: actor records taken from the free list.
;
; The records an actor can be created in are kept on a stack of their
; addresses (actor_free_list, actor_free_top the offset of its top word).
; C: src/aladdin/actor/pool.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

incsrc "../include/wram.inc"
arch 65816

; @routine 81:AB3F ActorAlloc
; WHAT IT DOES: Takes a free actor record, if there is one.
; HOW: The free list's top offset, negative when none is free; else the
; offset steps down a word and the address at the old top is the record.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: none free: Y = the offset; N set; X 8-bit and unchanged.
; Taken: X 16-bit; X = the record's address; Y = the old top offset; N, Z
; from X (N clear). A, C, V unchanged.
%at($81AB3F)
ActorAlloc:
    ldy.w !actor_free_top
    bmi .none
    rep.b #$10
    dec.w !actor_free_top
    dec.w !actor_free_top
    ldx.w !actor_free_list,y
.none:
    rtl


; @routine 81:AADD ActorRecordsInit
; WHAT IT DOES: Empties every actor record and fills the free lists for a
; new level.
; HOW: Clears all the records; the second free list holds the 5 records
; from $09CB and the first the 28 from $0B5B, the highest address at the
; bottom; each of the 28 gets its number (28 down to 1) at $33; the four
; animated-tile slots get no holders.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: M and X 8-bit; X = $FF; A = the pool walk's last index less $50
; (16 bits; $FFB0); N set, Z clear (DEX); C clear (the last SBC); Y, V
; unchanged; scratch $0C = 0.
%at($81AADD)
ActorRecordsInit:
    rep.b #$10
    ldx.w #$0AF0
.clear:
    stz.w !actor_records,x
    dex
    bpl .clear
    rep.b #$20
    sep.b #$10
    ldx.b #8
    stx.w !actor_free_top_b
    lda.w #$09CB
.list_b:
    sta.w !actor_free_list_b,x
    clc
    adc.w #80
    dex
    dex
    bpl .list_b
    ldx.b #54
    stx.w !actor_free_top
    lda.w #!actor_pool
.list:
    sta.w !actor_free_list,x
    clc
    adc.w #80
    dex
    dex
    bpl .list
    sep.b #$20
    rep.b #$10
    lda.b #28
    sta.w !scratch
    ldx.w #80*27
.number:
    lda.w !scratch
    sta.w !actor_pool+!actor_33,x
    dec.w !scratch
    rep.b #$20
    txa
    sec
    sbc.w #80
    tax
    sep.b #$20
    bpl .number
    ldx.w #3
.slot:
    stz.w !actor_tile_slot_users+1,x
    dex
    bpl .slot
    sep.b #$30
    rtl

; @routine 81:ADC0 ActorFreeListSort
; WHAT IT DOES: Orders the top of the free list so the lowest record
; addresses are taken first, when at least A records are free.
; HOW: With fewer than A free, nothing (a path the scripts never take,
; the code after this routine). Else A passes of a bubble sort from the
; top down: each pass compares A pairs of neighbouring entries, and swaps
; a pair whose lower entry is the smaller; the passes are counted in
; scratch $0C.
; Entry: M 8-bit, X 8-bit; A low byte = the passes; DB a bank whose
; $0000-$1FFF is work RAM. Exit: none free: Y = the top offset; N set.
; Sorted: M 8-bit; scratch $0C = 0; Y = the top offset; X = 0; A = the
; last entry compared or swapped (16 bits); Z set; C from the last
; compare; V unchanged.
%at($81ADC0)
ActorFreeListSort:
    ldy.w !actor_free_top
    bmi .done
    sta.w !scratch
    stz.w !scratch+1
    tya
    lsr a
    cmp.w !scratch
    bcc .done+1             ; past the RTL: too few free (code no script runs)
    rep.b #$20
.pass:
    phy
    ldx.w !scratch
.pair:
    lda.w !actor_free_list-2,y
    cmp.w !actor_free_list,y
    bcs .next
    pha
    lda.w !actor_free_list,y
    sta.w !actor_free_list-2,y
    pla
    sta.w !actor_free_list,y
.next:
    dey
    dey
    dex
    bne .pair
    ply
    dec.w !scratch
    bne .pass
    sep.b #$20
.done:
    rtl

; @routine 81:ADFC ActorFreeListSortHigh
; WHAT IT DOES: Orders the top of the free list so the highest record
; addresses are taken first, when at least A records are free; with
; fewer, says so.
; HOW: With fewer than A free (the top offset / 2 below A), A = $FF. Else
; A passes of a bubble sort from the top down: each pass compares A pairs
; of neighbouring entries, and swaps a pair whose lower entry is the
; larger or the same; the passes are counted in scratch $0C.
; Entry: M 8-bit, X 8-bit; A low byte = the passes; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: none free: Y = the top
; offset; N set. Too few: A low byte = $FF; N set. Sorted: M 8-bit; scratch $0C = 0; Y = the top
; offset; X = 0; A = the last entry compared or swapped (16 bits); Z set;
; C from the last compare; V unchanged.
%at($81ADFC)
ActorFreeListSortHigh:
    ldy.w !actor_free_top
    bmi .done
    sta.w !scratch
    stz.w !scratch+1
    tya
    lsr a
    cmp.w !scratch
    bcc .few
    rep.b #$20
.pass:
    phy
    ldx.w !scratch
.pair:
    lda.w !actor_free_list-2,y
    cmp.w !actor_free_list,y
    bcc .next
    pha
    lda.w !actor_free_list,y
    sta.w !actor_free_list-2,y
    pla
    sta.w !actor_free_list,y
.next:
    dey
    dey
    dex
    bne .pair
    ply
    dec.w !scratch
    bne .pass
    sep.b #$20
.done:
    rtl
.few:
    lda.b #$FF
    rtl

; @routine 81:AE38 ActorFreeListTake
; WHAT IT DOES: Takes A / 2 + 1 records off the top of the free list at
; once, ordered so the lowest address comes first; with too few free, says
; so.
; HOW: With the top offset below A (or none free), C clear. Else the top
; steps down A + 2 bytes, and A / 2 passes of a bubble sort (counted in
; scratch $0E) order the entries taken: each pass compares neighbouring
; pairs from the old top down and swaps a pair whose upper entry is the
; smaller.
; Entry: M 8-bit, X 8-bit; A low byte = the bytes (twice the records less
; one); DB a bank whose $0000-$1FFF is work RAM; called by JSL. Exit: too
; few: C clear; Y = the top offset. Taken: C set; Y = the offset of the
; first record taken (the new top + 2); M 8-bit; scratch $0C = A + 2, $0E
; = 0; X = 0.
ActorFreeListTake:
    ldy.w !actor_free_top
    bmi .few
    sta.w !scratch
    cpy.w !scratch
    bcc .few
    pha
    lsr a
    sta.w !scratch+2
    stz.w !scratch+3
    pla
    inc a
    inc a
    sta.w !scratch
    sec
    tya
    sbc.w !scratch
    sta.w !actor_free_top
    rep.b #$20
.pass:
    phy
    ldx.w !scratch+2
.pair:
    lda.w !actor_free_list,y
    cmp.w !actor_free_list-2,y
    bcs .next
    pha
    lda.w !actor_free_list-2,y
    sta.w !actor_free_list,y
    pla
    sta.w !actor_free_list-2,y
.next:
    dey
    dey
    dex
    bne .pair
    ply
    dec.w !scratch+2
    bne .pass
    sep.b #$20
    ldy.w !actor_free_top
    iny
    iny
    sec
    rtl
.few:
    clc
    rtl

; @routine 81:EA3D ActorAllocBLong
; WHAT IT DOES: ActorAllocB for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorAllocB; called by JSL. Exit: as ActorAllocB.
%at($81EA3D)
ActorAllocBLong:
    jsr.w ActorAllocB
    rtl

; @routine 81:EA41 ActorAllocB
; WHAT IT DOES: Takes a free record from the second pool (the five
; records from $09CB), if there is one.
; HOW: As ActorAlloc, on the second free list: its top offset, negative
; when none is free; else the offset steps down a word and the address at
; the old top is the record.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR. Exit: none free: Y = the offset; N set; X 8-bit and
; unchanged. Taken: X 16-bit; X = the record's address; Y = the old top
; offset; N, Z from X (N clear). A, C, V unchanged.
%at($81EA41)
ActorAllocB:
    ldy.w !actor_free_top_b
    bmi .none
    rep.b #$10
    dec.w !actor_free_top_b
    dec.w !actor_free_top_b
    ldx.w !actor_free_list_b,y
.none:
    rts
