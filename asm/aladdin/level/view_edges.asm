; view_edges.asm: two of the view-edge rules the routine at $81:E97D
; picks by $145B (the table at $81:E986): what happens to a record that
; leaves the view sideways.
; C: src/aladdin/level/view_edges.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!VIEW_EDGE_RULE = $145B         ; the rule the table at $81:E986 picks (0-4)
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!EDGE_NONE = $04                ; the rule that does nothing (ViewEdgeNone)

; @routine 81:EA11 ViewEdgeHurt
; WHAT IT DOES: View-edge rule 3: when the record is more than 48 pixels
; past either side of the view, Aladdin is hurt and the rule becomes 4
; (none).
; HOW: With the view's x less 48 not negative: its x less 48 at or past
; the record's, or its x plus 304 below the record's, sets $145B to 4 and
; calls PlayerEnter8CFrom1Long.
; Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
; RAM; called by JSR from the table. Exit: M 8-bit. Hurt: as
; PlayerEnter8CFrom1Long leaves it. Otherwise: A = the last difference (16
; bits); N, V, Z, C from it.
%at($81EA11)
ViewEdgeHurt:
    rep.b #$20
    lda.w !VIEW_X
    sec
    sbc.w #48
    bmi .out
    sec
    sbc.b !actor_x+1
    bcc .right
.hurt:
    sep.b #$20
    lda.b #!EDGE_NONE
    sta.w !VIEW_EDGE_RULE
    jsl.l PlayerEnter8CFrom1Long
    rts
.right:
    lda.w !VIEW_X
    clc
    adc.w #304
    sec
    sbc.b !actor_x+1
    bcc .hurt
.out:
    sep.b #$20
    rts

; @routine 81:EA3C ViewEdgeNone
; WHAT IT DOES: View-edge rule 4: nothing.
; HOW: RTS.
; Entry: called by JSR from the table. Exit: every register and flag
; unchanged.
ViewEdgeNone:
    rts
