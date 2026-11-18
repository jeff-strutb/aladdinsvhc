; kind44.asm: actor kind $44's start.
; C: src/aladdin/actor/kind44.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 83:B45B Kind44Start
; @actor
; WHAT IT DOES: Actor kind $44: shows animation 4 for $20 frames, then
; ends.
; HOW: $08 bit 1 and $09 bits $90 set; its look ($2B) $0600; $14, $15 0;
; $40 = $20; AnimStart 4; $20 frames (BRK first); JML ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's jump for kind $44 in state $0C. Exit: never returns.
%at($83B45B)
Kind44Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$20
    sta.b !actor_40
    lda.b #$04
    jsl.l AnimStart
.frame:
    brk.b #$00
    dec.b !actor_40
    bne .frame
    jml.l ActorEndPlaced
