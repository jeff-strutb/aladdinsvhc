; in_view.asm: which actors are near enough the view to be hit, and the
; lists of them, by their low two bits of $08, that other code walks.
; C: src/aladdin/actor/in_view.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; The lists: four counts (words), four next-entry offsets (words), and
; the entries (record addresses) from in_view_lists.
!in_view_counts = $1592
!in_view_next = $159A
!in_view_lists = $1492
!view_x = $1A44                  ; the point the boxes are measured from
!view_y = $1A46
; Tables in bank $81: per $12, a box (x offset, width, y offset, height;
; words, 8 bytes apart); the lists' first offsets; which count each list
; steps.
!in_view_boxes = $BA02
!in_view_starts = $BA2A
!in_view_count_of = $BA32
!ACTORS_LAST = 35                ; the records walked, from actor_escort

; @routine 81:9198 ActorsInView
; WHAT IT DOES: Marks which actors can be hit this frame (bit 6 of $09):
; those whose box, picked by their $12, holds the view's point; and lists
; those with bit 3 of $08 set, by their $08's low two bits.
; HOW: The counts cleared and the lists' next offsets set from their
; starts; then for each of the 35 records from actor_escort that is in use:
; the view point less its x and y, plus the box's offsets, under the box's
; width and height sets the bit, else clears it; with the bit set and bit
; 3 of $08 set, the record's address goes on its list and the list's count
; steps up.
; Entry: DB = $81; called by JSL. Exit: P and D as at entry; A, X, Y as
; the walk left them.
%at($819198)
ActorsInView:
    php
    phd
    rep.b #$30
    stz.w !in_view_counts
    stz.w !in_view_counts+2
    stz.w !in_view_counts+4
    stz.w !in_view_counts+6
    lda.w !in_view_starts
    sta.w !in_view_next
    lda.w !in_view_starts+2
    sta.w !in_view_next+2
    lda.w !in_view_starts+4
    sta.w !in_view_next+4
    lda.w !in_view_starts+6
    sta.w !in_view_next+6
    ldy.w #!ACTORS_LAST
    lda.w #!actor_escort
    tcd
.l91C7:
    lda.b !actor_state
    and.w #$00FF
    beq .l9224
    lda.b !actor_12
    and.w #$00FF
    tax
    sec
    lda.w !view_x
    sbc.b !actor_x+1
    clc
    adc.w !in_view_boxes,x
    cmp.w !in_view_boxes+2,x
    bcs .l921D
    sec
    lda.w !view_y
    sbc.b !actor_y+1
    clc
    adc.w !in_view_boxes+4,x
    cmp.w !in_view_boxes+6,x
    bcs .l921D
    lda.b !actor_08
    ora.w #$4000
    sta.b !actor_08
    lda.b !actor_08
    bit.w #$0008
    beq .l9224
    phy
    and.w #$0003
    asl a
    tax
    ldy.w !in_view_next,x
    tdc
    sta.w !in_view_lists,y
    inc.w !in_view_next,x
    inc.w !in_view_next,x
    txy
    ldx.w !in_view_count_of,y
    inc.w !in_view_counts,x
    ply
    bra .l9224
.l921D:
    lda.b !actor_08
    and.w #$BFFF
    sta.b !actor_08
.l9224:
    clc
    tdc
    adc.w #$0050
    tcd
    dey
    bne .l91C7
    pld
    plp
    rtl
