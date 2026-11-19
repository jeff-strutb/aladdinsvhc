; kind5d.asm: actor kind $5D of level 8, a cloud the Genie's head
; (kind $5C) makes: it rises to its height (by Aladdin's lives and its
; number), leaving sparkles; when Aladdin lands on it, it tells its maker,
; sinks under him and rises again, and again when he leaves it. Once its
; maker is gone it starts its timed end. Its wait is Kind5DWait
; (kind_waits_rest.asm).
; C: src/aladdin/actor/kind5d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K5D_PARENT = $37               ; its maker's record (word)
!K5D_LANDED = $40               ; counted up when Aladdin first lands on it (its maker waits for it)
!K5D_COUNT = $41                ; Kind5DRise: frames to its next sparkle
!K5D_HEIGHT_AT = $42            ; Kind5DRise: its height's offset in K5D_HEIGHTS
!K5D_BACK = $43                 ; Kind5DRise: its caller's return address while it rises (3 bytes)
!K5D_47 = $47                   ; cleared as it rises; purpose not yet established
!K5D_ON = $48                   ; nonzero when Aladdin landed on it this frame
!K5D_WAS_ON = $49               ; K5D_ON as the frame before (Kind5DWait keeps it)
!K5D_GO = $4A                   ; nonzero once he has landed: Kind5DWait starts its end
!K5D_STATE = $13                ; Kind5DWait's step: 2 starts its end at once
!K5D_PARENT_GONE = $4E          ; its maker's: nonzero once it has gone
!K5D_HEIGHTS = $D4A8            ; bank $80: by path the offset of its heights, there by twice its number the y it rises to (words)
!K5D_BOX = $50                  ; PlayerLandOnActor's box
!K5D_LAND_SOUND = $29           ; the sound as he lands on it
!K5D_RISE_SOUND = $28           ; the sound as it starts to rise
!K5D_RISE_SPEED = $45           ; ActorSpeedYSet's entry for its rise
!K5D_SINK = $49                 ; ActorMotionPreset's preset as it sinks under him
!K5D_BACK_UP = $4A              ; and as it rises again
!K5D_SPARKLE = $5F              ; the kind of its sparkles

; @routine 83:E29B Kind5DStart
; @actor
; WHAT IT DOES: Actor kind $5D, a cloud: rises (Kind5DRise), then waits
; for Aladdin to land on it; then marks it for its maker, sinks under him
; and rises back, and waits for him to leave it; then sinks and rises
; once more, and waits for him again.
; HOW: Kind5DRise (JSL); $08 1, $09 $90; K5D_ON, K5D_WAS_ON, K5D_GO and
; K5D_STATE 0; AnimStart 0. Each frame (BRK first) Kind5DParentCheck and
; Kind5DLandTest until K5D_ON differs from K5D_WAS_ON. K5D_LANDED up,
; K5D_GO 1; ActorMotionPreset $49 and Kind5DSinkStep each frame (BRK
; first) while its vertical speed's top byte is not negative; preset $4A
; and Kind5DRiseStep each frame while it is. Then the same wait for a
; change, the same sink and rise, and the first wait again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $5D in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83E29B)
Kind5DStart:
    jsl.l Kind5DRise
    lda.b #$01
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    stz.b !K5D_ON
    stz.b !K5D_WAS_ON
    stz.b !K5D_GO
    stz.b !K5D_STATE
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    jsr.w Kind5DParentCheck
    jsr.w Kind5DLandTest
    lda.b !K5D_ON
    cmp.b !K5D_WAS_ON
    beq .wait
    inc.b !K5D_LANDED
    lda.b #$01
    sta.b !K5D_GO
    ldy.b #!K5D_SINK
    jsl.l ActorMotionPreset
.sink:
    brk.b #$00
    jsr.w Kind5DSinkStep
    bpl .sink
    ldy.b #!K5D_BACK_UP
    jsl.l ActorMotionPreset
.rise:
    brk.b #$00
    jsr.w Kind5DRiseStep
    bmi .rise
.waitOff:
    brk.b #$00
    jsr.w Kind5DParentCheck
    jsr.w Kind5DLandTest
    lda.b !K5D_ON
    cmp.b !K5D_WAS_ON
    beq .waitOff
    ldy.b #!K5D_SINK
    jsl.l ActorMotionPreset
.sinkAgain:
    brk.b #$00
    jsr.w Kind5DSinkStep
    bpl .sinkAgain
    ldy.b #!K5D_BACK_UP
    jsl.l ActorMotionPreset
.riseAgain:
    brk.b #$00
    jsr.w Kind5DRiseStep
    bmi .riseAgain
    jmp.w .wait

; @routine 83:E30E Kind5DIdle
; @actor
; WHAT IT DOES: Kind $5D in its timed end: does nothing each frame until
; its wait ends it.
; HOW: BRK, again.
; Entry: D = the actor's record; reached by the actor loop's RTI,
; Kind5DWait having made it its resume address. Exit: never returns.
%at($83E30E)
Kind5DIdle:
    brk.b #$00
    bra Kind5DIdle

; @routine 83:E312 Kind5DLandTest
; WHAT IT DOES: Tells whether Aladdin lands on the cloud this frame: then
; sound $29 and K5D_ON 1, C clear; else K5D_ON 0, C set.
; HOW: K5D_ON 0; PlayerLandOnActor with box $50; landed (C clear):
; SoundQueueAdd $29, K5D_ON up, C clear.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: C as above; M 8-bit, X 8-bit.
%at($83E312)
Kind5DLandTest:
    stz.b !K5D_ON
    ldy.b #!K5D_BOX
    jsl.l PlayerLandOnActor
    bcs .off
    lda.b #!K5D_LAND_SOUND
    jsl.l SoundQueueAdd
    inc.b !K5D_ON
    clc
.off:
    rts

; @routine 83:E326 Kind5DSinkStep
; WHAT IT DOES: A frame of the cloud sinking: Aladdin landed on it is
; carried with it.
; HOW: Kind5DLandTest, its C kept (PHP); ActorKeepSpot, ActorFlyAgainst;
; landed: PlayerMoveWithActor. A = its vertical speed's top byte.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: A = $1F; N and Z from it.
%at($83E326)
Kind5DSinkStep:
    jsr.w Kind5DLandTest
    php
    jsl.l ActorKeepSpot
    jsl.l ActorFlyAgainst
    plp
    bcs .off
    jsl.l PlayerMoveWithActor
.off:
    lda.b !actor_speed_y+2
    rts

; @routine 83:E33C Kind5DRiseStep
; WHAT IT DOES: A frame of the cloud rising back: Aladdin landed on it is
; carried with it.
; HOW: As Kind5DSinkStep, with ActorFly.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: A = $1F; N and Z from it.
%at($83E33C)
Kind5DRiseStep:
    jsr.w Kind5DLandTest
    php
    jsl.l ActorKeepSpot
    jsl.l ActorFly
    plp
    bcs .off
    jsl.l PlayerMoveWithActor
.off:
    lda.b !actor_speed_y+2
    rts
%ends_at($83E352)

; @routine 83:E3AD Kind5DParentCheck
; WHAT IT DOES: Once the cloud's maker has gone, starts the cloud's timed
; end (Kind5DWait's step 2), if it has not started.
; HOW: Kind5DParentGone nonzero and K5D_STATE below 2: K5D_STATE 2.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; M 8-bit, X 8-bit; A as the last load.
%at($83E3AD)
Kind5DParentCheck:
    jsr.w Kind5DParentGone
    beq .on
    lda.b !K5D_STATE
    cmp.b #$02
    bcs .on
    lda.b #$02
    sta.b !K5D_STATE
.on:
    rts

; @routine 83:E3BD Kind5DParentGoneLong
; WHAT IT DOES: Kind5DParentGone for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as Kind5DParentGone; called by JSL. Exit: as Kind5DParentGone.
%at($83E3BD)
Kind5DParentGoneLong:
    jsr.w Kind5DParentGone
    rtl

; @routine 83:E3C1 Kind5DParentGone
; WHAT IT DOES: Reads whether the actor's maker (K5D_PARENT) has gone: its
; K5D_PARENT_GONE.
; HOW: 16-bit X = K5D_PARENT; A low = its byte at $4E; X 8-bit again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: A low = the byte; N and Z
; from it; X = K5D_PARENT's low byte.
%at($83E3C1)
Kind5DParentGone:
    rep.b #$10
    ldx.b !K5D_PARENT
    lda.w !K5D_PARENT_GONE,x
    sep.b #$10
    rts

; @routine 83:E3CB Kind5DRise
; WHAT IT DOES: The cloud's rise: from where its maker made it, up to its
; height (by Aladdin's lives and its number), with sound $28 and a
; sparkle (kind $5F) every 4 frames; then back to its caller.
; HOW: Its caller's return address (3 bytes) pulled into K5D_BACK.
; SoundQueueAdd $28; K5D_HEIGHT_AT = K5D_HEIGHTS by Kind5CPathByLives
; plus twice its $07; $21, $07, K5D_LANDED, K5D_COUNT and K5D_47 0;
; ActorSpeedYSet $45. Each frame (BRK first) ActorMoveY until its y is
; K5D_HEIGHTS's word at K5D_HEIGHT_AT or more; K5D_COUNT down past 0:
; ActorCreateChild8 kind $5F, K5D_COUNT 3, $07 bit 0 flipped. Then
; K5D_BACK pushed back, RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSL. Exit: by RTL to its caller; M 8-bit, X 8-bit.
%at($83E3CB)
Kind5DRise:
    pla
    sta.b !K5D_BACK
    pla
    sta.b !K5D_BACK+1
    pla
    sta.b !K5D_BACK+2
    lda.b #!K5D_RISE_SOUND
    jsl.l SoundQueueAdd
    jsr.w Kind5CPathByLives
    tax
    lda.b !actor_07
    asl a
    clc
    adc.w !K5D_HEIGHTS,x
    sta.b !K5D_HEIGHT_AT
    stz.b !actor_spawn_at
    stz.b !actor_07
    stz.b !K5D_LANDED
    stz.b !K5D_COUNT
    stz.b !K5D_47
    ldy.b #!K5D_RISE_SPEED
    jsl.l ActorSpeedYSet
.frame:
    brk.b #$00
    jsl.l ActorMoveY
    rep.b #$20
    ldx.b !K5D_HEIGHT_AT
    lda.b !actor_y+1
    cmp.w !K5D_HEIGHTS,x
    sep.b #$20
    bcs .there
    dec.b !K5D_COUNT
    bpl .frame
    lda.b #!K5D_SPARKLE
    jsl.l ActorCreateChild8
    lda.b #$03
    sta.b !K5D_COUNT
    lda.b !actor_07
    eor.b #$01
    sta.b !actor_07
    bra .frame
.there:
    lda.b !K5D_BACK+2
    pha
    lda.b !K5D_BACK+1
    pha
    lda.b !K5D_BACK
    pha
    rtl
%ends_at($83E42A)
