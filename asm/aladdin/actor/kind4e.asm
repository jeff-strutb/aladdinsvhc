; kind4e.asm: actor kind $4E of stage a5 s0, a thing Aladdin bounces off
; when he comes down on it: it then plays a squash and spring back. Its
; wait is Kind4EWait (kind_waits_rest.asm). Named and described by hand.
; C: src/aladdin/actor/kind4e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!K4E_STILL = $42                ; nonzero: Aladdin does not bounce on it (while it springs)
!K4E_POSE = $41                 ; the animation of the squash or spring step

; @routine 83:CFA9 Kind4EStart
; @actor
; WHAT IT DOES: Actor kind $4E started: it rests in animation 6; Aladdin
; may bounce on it (Kind4EWait).
; HOW: Bits 0-1 of $08 and 7 of $09 set; then the rest: K4E_STILL = 0,
; AnimStart 6, BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $0C. Exit: never returns.
%at($83CFA9)
Kind4EStart:
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
.rest:
    stz.b !K4E_STILL
    lda.b #$06
    jsl.l AnimStart
.resting:
    brk.b #$00
    bra .resting

; @routine 83:CFC1 Kind4EBounced
; @actor
; WHAT IT DOES: Kind $4E after Aladdin bounced on it (Kind4EWait resumes
; its code here): squashed, it springs back up, then rests again.
; HOW: AnimStart 7, COP $10; K4E_STILL up; animations 5 down to 0, COP 8
; after each; COP $30; animations 0 up to 5, COP 8 after each; then the
; rest of Kind4EStart.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; the actor
; loop resumes it here (the resume address Kind4EWait wrote). Exit:
; never returns.
%at($83CFC1)
Kind4EBounced:
    lda.b #$07
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    inc.b !K4E_STILL
    lda.b #$05
    sta.b !K4E_POSE
.squash:
    lda.b !K4E_POSE
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    dec.b !K4E_POSE
    bpl .squash
    lda.b #$30
    cop.b #$00
    lda.b #$00
.spring:
    sta.b !K4E_POSE
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b !K4E_POSE
    inc a
    cmp.b #$06
    bcc .spring
    bra Kind4EStart_rest
