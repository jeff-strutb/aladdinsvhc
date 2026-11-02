; kind6b.asm: actor kind $6B, which the camera trigger task starts with
; $07 = 1: shown in its first pose; with $07 nonzero it waits for the
; task's cue $00BF and goes on (code not yet reached by any script).
; C: src/aladdin/actor/kind6b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND6B_16 = $16                ; word: $D663
!CUE_BF = $00BF                 ; the camera trigger task's third cue: cleared as it starts

; @routine 83:EC2C Kind6BWait
; @actor
; WHAT IT DOES: Actor kind $6B: shown facing right in animation 0. With
; $07 zero it stays so; else it waits for CUE_BF and goes on.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; actor_2c bits 1-3 = 1
; (and $F1, or 2); KIND6B_16 = $D663; facing and flip 0. $07 zero:
; AnimStart 0, then BRK for good. Else AnimStart 0, then each frame
; (BRK first) until CUE_BF is nonzero; then on at $83:EC64, which no
; script reaches yet: those bytes are left in the intake.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $6B in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83EC2C)
Kind6BWait:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_2c
    and.b #$F1
    ora.b #$02
    sta.b !actor_2c
    rep.b #$20
    lda.w #$D663
    sta.b !KIND6B_16
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_07
    bne .cued
    lda.b #$00
    jsl.l AnimStart
.stay:
    brk.b #$00
    bra .stay
.cued:
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    lda.w !CUE_BF
    beq .wait
