; view_edges.asm: the view-edge rules: what happens to the player when he
; reaches a side of the view. ViewEdgeRuleRun runs the rule $145B picks
; from the table at $81:E986: 0 PlayerKeepOnScreenX, 1 PlayerKeepOnScreen
; (player/player_bounds.asm), 2 ViewEdgePush, 3 ViewEdgeHurt, 4
; ViewEdgeNone.
; C: src/aladdin/level/view_edges.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!VIEW_EDGE_RULE = $145B         ; the rule the table at $81:E986 picks (0-4)
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!EDGE_NONE = $04                ; the rule that does nothing (ViewEdgeNone)
!VIEW_EDGE_RULES = $E986        ; in bank $81: the rules' routines (words), by rule
; In bank $80: x pixels from the view's left edge (words): 16, the least
; the player is kept at; 240, the most; and at +8, 0 (the edge itself).
!VIEW_X_LIMITS = $C9B9

; @routine 81:E97D ViewEdgeRuleRun
; WHAT IT DOES: Runs the view-edge rule in force: what happens to the
; player as he reaches a side of the view.
; HOW: JSR through VIEW_EDGE_RULES by VIEW_EDGE_RULE times 2.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's frame work. Exit: as the rule leaves them; X
; entering the rule = the rule times 2.
%at($81E97D)
ViewEdgeRuleRun:
    lda.w !VIEW_EDGE_RULE
    asl a
    tax
    jsr.w (!VIEW_EDGE_RULES,x)
    rts

; @routine 81:E9CD ViewEdgePush
; WHAT IT DOES: View-edge rule 2: at either side of the view (16 pixels
; in), the player is pushed back onto the screen while he is off firm
; ground; on firm ground he is not, and once at or left of the view's
; left edge he is hurt and the rule becomes 3 (ViewEdgeHurt).
; HOW: VIEW_X plus 16, and plus 240, compared with the record's x pixels.
; Past neither: nothing. Past one: PlayerOnFirmGroundProbe8, then
; PlayerOnFirmGroundProbe4; neither firm: PlayerKeepOnScreenX. Firm:
; VIEW_X (plus 0) not below his x: PlayerEnter8CFrom1Long with 8-bit
; registers, then VIEW_EDGE_RULE up by one. No script runs this rule:
; the whole routine is written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from ViewEdgeRuleRun. Exit: M 8-bit, X 8-bit. Pushed: as
; PlayerKeepOnScreenX leaves A and the flags. Hurt: N and Z from the new
; rule. Otherwise: A = the last sum (16 bits); N, Z, C from its
; comparison, V from its addition.
%at($81E9CD)
ViewEdgePush:
    rep.b #$20
    clc
    lda.w !VIEW_X
    adc.w !VIEW_X_LIMITS
    cmp.b !actor_x+1
    bcs .edge
    clc
    lda.w !VIEW_X
    adc.w !VIEW_X_LIMITS+2
    cmp.b !actor_x+1
    bcs .out
.edge:
    sep.b #$20
    jsl.l PlayerOnFirmGroundProbe8
    bcs .firm
    jsl.l PlayerOnFirmGroundProbe4
    bcs .firm
    jsr.w PlayerKeepOnScreenX
    bra .out
.firm:
    rep.b #$20
    clc
    lda.w !VIEW_X
    adc.w !VIEW_X_LIMITS+8
    cmp.b !actor_x+1
    bcc .out
    sep.b #$30
    jsl.l PlayerEnter8CFrom1Long
    inc.w !VIEW_EDGE_RULE
.out:
    sep.b #$30
    rts

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
