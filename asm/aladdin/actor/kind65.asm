; kind65.asm: actor kind $65, the continues counter shown in play: its
; picture is the count of continues left, changed as the count changes;
; with none left as it starts, it ends. Kinds $61, $64 and $6C go on here in their
; second state.
; C: src/aladdin/actor/kind65.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_state.inc"

arch 65816

!K65_SHOWN = $40                ; the count it shows
!K65_FIRST_PICTURE = $07        ; the animation for 0; the count is added

; @routine 83:E5BB Kind65Start
; @actor
; WHAT IT DOES: Actor kind $65 (and the second state of kinds $61, $64
; and $6C): shows the continues left as its animation (7 plus the count),
; changing it when the count changes; with none left as it starts, it
; ends for good.
; HOW: $08 bit 1, $09 bits 7 and 4 set; its look $0600; facing and flip
; 0. Continues 0: JMP ActorEndPlaced. Else K65_SHOWN the count, AnimStart
; with it plus 7; each frame (BRK first) until the count differs from
; K65_SHOWN, then the count again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $65 in state $0C (the table at $83:8CAB),
; and for kinds $61, $64, $6C in a state with bit 7 set ($83:8DFD).
; Exit: never returns.
%at($83E5BB)
Kind65Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.w !continues
    bne .show
    jmp.w ActorEndPlaced
.show:
    sta.b !K65_SHOWN
    clc
    adc.b #!K65_FIRST_PICTURE
    jsl.l AnimStart
.frame:
    brk.b #$00
    lda.w !continues
    cmp.b !K65_SHOWN
    beq .frame
    bra .show
%ends_at($83E5EC)
