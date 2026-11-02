; kind79.asm: actor kind $79, which starts below the point actors are
; kept in range of and rises until it is taken up (its $42 set), then
; carries the player (code not yet reached by any script).
; C: src/aladdin/actor/kind79.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!RANGE_Y = $1A46                ; the y of the point actors are kept in range of (word)
!KIND79_FIRST_Y = $45           ; the first of a pair: its y as it was placed (word)
!KIND79_TAKEN = $42             ; nonzero: it is taken up, and goes on
!KIND79_43 = $43
!KIND79_RISE = $6F              ; its vertical speed's entry in the speed table

; @routine 83:F541 Kind79Rise
; @actor
; WHAT IT DOES: Actor kind $79: starts 32 pixels (an even $07: the first
; of a pair, which marks its $07 odd) or 112 pixels (an odd $07) below
; the point actors are kept in range of, and rises each frame until it
; is taken up.
; HOW: Even $07: $07 up, its y kept in KIND79_FIRST_Y, its y RANGE_Y plus
; $20, actor_09 = $90, actor_12 = 8. Odd: its y RANGE_Y plus $70. Then
; actor_08 = 2, animation 0, ActorSpeedYSet $6F, $41, KIND79_TAKEN,
; KIND79_43 and actor_49 cleared; each frame (BRK first)
; ActorStepYBack, until KIND79_TAKEN is set; then on at $83:F593, which
; no script reaches yet: those bytes are left in the intake.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $79 in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83F541)
Kind79Rise:
    lda.b !actor_07
    and.b #$01
    bne .second
    inc.b !actor_07
    rep.b #$20
    lda.b !actor_y+1
    sta.b !KIND79_FIRST_Y
    clc
    lda.w !RANGE_Y
    adc.w #$0020
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$90
    sta.b !actor_09
    lda.b #$08
    sta.b !actor_12
    bra .placed
.second:
    rep.b #$20
    clc
    lda.w !RANGE_Y
    adc.w #$0070
    sta.b !actor_y+1
    sep.b #$20
.placed:
    lda.b #$02
    sta.b !actor_08
    lda.b #$00
    jsl.l AnimStart
    ldy.b #!KIND79_RISE
    jsl.l ActorSpeedYSet
    stz.b !actor_41
    stz.b !KIND79_TAKEN
    stz.b !KIND79_43
    stz.b !actor_49
.rise:
    brk.b #$00
    jsl.l ActorStepYBack
    lda.b !KIND79_TAKEN
    beq .rise
