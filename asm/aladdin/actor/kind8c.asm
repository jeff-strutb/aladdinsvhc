; kind8c.asm: actor kind $8C, a cloud that rises as kind $5D's does and that Aladdin bounces off, for as
; long as its maker lasts and a little after. Its wait is Kind8CWaitWork (kind_waits_84.asm).
; C: src/aladdin/actor/kind8c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K8C_BOUNCED = $40              ; set once Aladdin has bounced off it
!K8C_TIMER = $41                ; frames left of its part

; @routine 84:9979 Kind8CStart
; @actor
; WHAT IT DOES: Actor kind $8C (placed once in levels $08 and $18, at x
; 16383: made by other code): a cloud that rises as kind $5D's does
; (Kind5DRise), and that Aladdin bounces off (sound $3B, animation $12)
; for as long as its maker lasts and for $57 frames after, then it ends.
; HOW: Kind5DRise; $08 = 2, $09 = $80 (its kind's wait runs), the
; palette byte ($2C) = 6; AnimStart $11. Each frame (BRK)
; Kind5DParentGoneLong: gone, to the last part; else
; ActorBouncesPlayerLong, bounced: K8C_BOUNCED ($40) = 1, Kind8CBounced,
; K8C_TIMER ($41) = $B0, and each frame (BRK) the same tests
; (Kind8CBounced on a bounce) and K8C_TIMER down to 0. Last part: $08
; bit 4 set; K8C_TIMER = $57; each frame (BRK) ActorBouncesPlayerLong
; (Kind8CBounced on a bounce) and K8C_TIMER down to 0; JML
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML of
; kind $8C's stub (Kind8CEnter, $83:9337) from the actor loop in the
; start state.
; Exit: never returns: ends in ActorEndPlaced.
%at($849979)
Kind8CStart:
    jsl.l Kind5DRise
    lda.b #$02
    sta.b !actor_08
    lda.b #$80
    sta.b !actor_09
    lda.b #$06
    sta.b !actor_look+1
    lda.b #$11
    jsl.l AnimStart
.until_bounced:
    brk.b #$00
    jsl.l Kind5DParentGoneLong
    bne .the_end
    jsl.l ActorBouncesPlayerLong
    beq .until_bounced
    lda.b #$01
    sta.b !K8C_BOUNCED
    jsr.w Kind8CBounced
    lda.b #$B0
    sta.b !K8C_TIMER
.until_gone:
    brk.b #$00
    jsl.l Kind5DParentGoneLong
    bne .the_end
    jsl.l ActorBouncesPlayerLong
    beq .not_again
    jsr.w Kind8CBounced
.not_again:
    dec.b !K8C_TIMER
    bne .until_gone
.the_end:
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    lda.b #$57
    sta.b !K8C_TIMER
.last:
    brk.b #$00
    jsl.l ActorBouncesPlayerLong
    beq .last_none
    jsr.w Kind8CBounced
.last_none:
    dec.b !K8C_TIMER
    bne .last
    jml.l ActorEndPlaced

; @routine 84:99DA Kind8CBounced
; WHAT IT DOES: What kind $8C does as Aladdin bounces off it: its
; animation $12 and sound $3B.
; HOW: AnimStart $12; SoundQueueAdd $3B; RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
; Kind8CStart. Exit: M and X 8-bit.
%at($8499DA)
Kind8CBounced:
    lda.b #$12
    jsl.l AnimStart
    lda.b #$3B
    jsl.l SoundQueueAdd
    rts
