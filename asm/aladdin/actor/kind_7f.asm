; kind_7f.asm: actor kind $7F: a backdrop figure that drifts across for
; good, slower the higher its $07. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/kind_7f.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND7F_16 = $16  ; word: $D832
!KIND7F_SLOWER = $40  ; $07 times 16

; @routine 84:934B Kind7FDrift
; @actor
; WHAT IT DOES: Actor kind $7F: shown with the animation $07 picks (bits
; from $07 also in $08), facing right, it drifts across at $70 less 16
; times $07 (a fraction of a pixel a frame) for good.
; HOW: $08 |= 2 | $07; $09 bits $90 set; $16 = $D832; AnimStart $07;
; facing and flip 0; $40 = $07 * 16; its speed's fraction = $70 - $40;
; each frame (BRK first) ActorMoveX.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $7F in state $0C (through the stub at
; $83:92B5). Exit: never returns.
%at($84934B)
Kind7FDrift:
    lda.b !actor_08
    ora.b #$02
    ora.b !actor_07
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D832
    sta.b !KIND7F_16
    sep.b #$20
    lda.b !actor_07
    jsl.l AnimStart
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_07
    asl a
    asl a
    asl a
    asl a
    sta.b !KIND7F_SLOWER
    lda.b #$70
    sec
    sbc.b !KIND7F_SLOWER
    sta.b !actor_speed_x
.l937B:
    brk.b #$00
    jsl.l ActorMoveX
    bra .l937B
