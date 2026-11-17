; kind_a8.asm: actor kind $A8, a figure of the level-end screen that waits for its
; cue and, when its $07 is below the count at $0375, poses again after its
; own delay with a sound; then it keeps animating.
; C: src/aladdin/actor/kind_a8.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CUE_AE = $00AE                 ; its cue
!V_0375 = $0375                 ; a count: figures with $07 below it pose again
!KA8_DELAYS = $D9A6             ; (data bank) by $07: frames before it poses again

; @routine 83:FB1A KindA8Start
; @actor
; WHAT IT DOES: Actor kind $A8: shown in pose 1; once cue $AE is set,
; if its $07 is below V_0375 it waits its delay, takes pose 0 and plays
; sound $28; then it animates for good.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bit 4 set (TSB); AnimStart 1; BRK
; until CUE_AE. $07 below V_0375 ($83:FB36, which no script meets): COP
; KA8_DELAYS by $07, AnimStart 0, SoundQueueAdd $28. Then each frame
; (BRK) AnimStep.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds KA8_DELAYS (bank $83); reached by
; the actor loop's jump for kind $A8 in state $0C (the table at $83:8DAB,
; through the stub at $83:9446). Exit: never returns.
%at($83FB1A)
KindA8Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$10
    tsb.b !actor_09
    lda.b #$01
    jsl.l AnimStart
.until_cue:
    brk.b #$00
    lda.w !CUE_AE
    beq .until_cue
    ldx.b !actor_07
    cpx.w !V_0375
    bcs .animate
    lda.w !KA8_DELAYS,x
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    lda.b #$28
    jsl.l SoundQueueAdd
.animate:
    brk.b #$00
    jsl.l AnimStep
    bra .animate
