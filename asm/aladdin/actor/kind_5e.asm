; kind_5e.asm: actor kind $5E: a brief show of its animation, then its end.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind_5e.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND5E_LEFT = $40  ; frames left

; @routine 83:E42A Kind5EBrief
; @actor
; WHAT IT DOES: Actor kind $5E: shows animation $0C for $23 frames, then
; ends (its $07 bit 2 picks the same either way).
; HOW: Its look $0600; $08 bits 0-1 and $09 bits $90 set; animation $0C
; (X) and $40 = $23 (Y), the same whether bit 2 of $07 is set; AnimStart;
; $23 frames (BRK first, $40 down); then the jump at $83:E461 into
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $5E in state $0C. Exit: never returns;
; ends in ActorEndPlaced.
%at($83E42A)
Kind5EBrief:
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    lda.b #$03
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    ldx.b #$0C
    ldy.b #$23
    lda.b !actor_07
    and.b #$04
    beq .lE449
    ldx.b #$0C
    ldy.b #$23
.lE449:
    txa
    sty.b !KIND5E_LEFT
    jsl.l AnimStart
.lE450:
    brk.b #$00
    dec.b !KIND5E_LEFT
    bne .lE450
    bra Kind5EEnd

; Kind5EBrief's end, after the routine at $83:E458 (not yet filed): into
; ActorEndPlaced.
%at($83E461)
Kind5EEnd:
    jml.l ActorEndPlaced
