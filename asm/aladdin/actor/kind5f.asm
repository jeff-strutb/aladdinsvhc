; kind5f.asm: actor kind $5F, a brief picture: shown by its $07's
; animation for 31 frames, then gone.
; C: src/aladdin/actor/kind5f.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K5F_LEFT = $40                 ; frames left of its showing

; @routine 83:E465 Kind5FStart
; @actor
; WHAT IT DOES: Actor kind $5F: shows animation $07 for 31 frames, then
; ends for good.
; HOW: $09 = $10; AnimStart with A = $07; K5F_LEFT $1F; each frame (BRK
; first) AnimStep and K5F_LEFT down; at 0 JMP ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $5F in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83E465)
Kind5FStart:
    lda.b #$10
    sta.b !actor_09
    lda.b !actor_07
    jsl.l AnimStart
    lda.b #$1F
    sta.b !K5F_LEFT
.frame:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K5F_LEFT
    bne .frame
    jmp.w ActorEndPlaced
%ends_at($83E480)
