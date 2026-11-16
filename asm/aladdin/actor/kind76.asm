; kind76.asm: actor kind $76's start: a figure that poses, waits, then
; fades its palette away under colour math and cues the scene on.
; C: src/aladdin/actor/kind76.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!K76_28 = $28                   ; $FF
!K76_29 = $29                   ; word: $891B
!K76_FADE = $40                 ; the fade's step: 0 down to -$1F
!SUB_LAYERS = $039A             ; layers kept for the subscreen while the fade runs
!CUE_1B1F = $1B1F               ; the scene goes on once this is nonzero (kind81_scene.asm)

; @routine 84:8FBA Kind76Start
; @actor
; WHAT IT DOES: Actor kind $76: poses (2) for $BD frames, then (1) for
; $16, queues sound $46, and fades its own sprite palette down to black
; over 31 frames with colour math on, then cues the scene on (CUE_1B1F)
; and ends.
; HOW: ACTOR_08 = 3, ACTOR_09 = $80, K76_29 = $891B, flip 1, K76_28 =
; $FF; AnimStart 2, COP $BD, AnimStart 1, COP $16; SoundQueueAdd $46;
; ACTOR_09 = $20; PaletteSave of one palette, 8 + its palette bits
; (ACTOR_LOOK's high byte AND $0E, halved); K76_FADE 0, PaletteFadeStep;
; CGWSEL 2, CGADSUB $12, SUB_LAYERS $10; each frame (BRK) K76_FADE down
; by one and PaletteFadeStep of one palette by it, until it is $E1; then
; CGWSEL $30, CGADSUB and SUB_LAYERS 0, CUE_1B1F counted up,
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $76
; in state $0C (the table at $83:8CAB, through the stub at $83:9281).
; Exit: never returns; ends in ActorEndPlaced.
%at($848FBA)
Kind76Start:
    lda.b #$03
    sta.b !actor_08
    lda.b #$80
    sta.b !actor_09
    rep.b #$20
    lda.w #$891B
    sta.b !K76_29
    sep.b #$20
    lda.b #$01
    sta.b !actor_flip
    lda.b #$FF
    sta.b !K76_28
    lda.b #$02
    jsl.l AnimStart
    lda.b #$BD
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$16
    cop.b #$00
    lda.b #$46
    jsl.l SoundQueueAdd
    lda.b #$20
    sta.b !actor_09
    lda.b !actor_look+1
    and.b #$0E
    lsr a
    ora.b #$08
    ldy.b #$01
    jsl.l PaletteSave
    lda.b #$00
    sta.b !K76_FADE
    ldy.b #$01
    jsl.l PaletteFadeStep
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$12
    sta.w !cgadsub_shadow
    lda.b #$10
    sta.w !SUB_LAYERS
.fade:
    brk.b #$00
    dec.b !K76_FADE
    lda.b !K76_FADE
    ldy.b #$01
    jsl.l PaletteFadeStep
    lda.b !K76_FADE
    cmp.b #$E1
    bne .fade
    lda.b #$30
    sta.w !cgwsel_shadow
    stz.w !cgadsub_shadow
    stz.w !SUB_LAYERS
    inc.w !CUE_1B1F
    jml.l ActorEndPlaced
