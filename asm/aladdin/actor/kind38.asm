; kind38.asm: actor kind $38, which rolls along until Aladdin is close,
; then turns to him and flies up at him. Its wait is Kind37Wait
; (actor_waits_small.asm), shared with kind $37, as is its second state,
; Kind37Second (here).
; C: src/aladdin/actor/kind38.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND38_16 = $16                ; word: $D1D6
!KIND38_47 = $47                ; ActorYByFrame47's frame kept
!KIND38_4A = $4A
!KIND38_4B = $4B                ; 8
!KIND38_UP = $45                ; set once it flies (Kind37Wait then ends it as other actors)
!KIND38_HIT = $46               ; Kind37Wait's: 1 bounced, 2 downed by a shot
!KIND38_CHECKS = $49            ; near checks left before it may turn
!KIND38_SIDE = $48              ; the side Aladdin is on (ActorSideOfPlayer)
!KIND38_FRAMES = $40            ; frames left of a pause

; @routine 83:BE01 Kind38Start
; @actor
; WHAT IT DOES: Actor kind $38: it rolls along bobbing with its frames
; until Aladdin is close across (or it meets a wall), then, on a random
; pick by which side he is on (else it rolls on for 16 more checks; no
; script has that), it turns to him, pauses, shows itself (animation 1)
; for 32 frames and flies up at him for good.
; HOW: ACTOR_08 = 2, ACTOR_09 = $90, facing and flip 1, AnimStart 0,
; $16 = $D1D6; $47 cleared, $4B = 8, $4A, KIND38_UP and KIND38_HIT
; cleared, KIND38_CHECKS = 1. ActorSpeedXSet with Y = $18; each frame
; (BRK) ActorMoveX, ActorYByFrame47; ActorOutOfWallProbe4 C set (no
; script): KIND38_SIDE from ActorSideOfPlayer, on at the turn. Else
; ActorNearPlayerX with Y = 2, C clear and KIND38_CHECKS counted down to
; 0: KIND38_SIDE from ActorSideOfPlayer; RandomFromList with X = (side +
; 3) * 2: zero, KIND38_CHECKS = $10 and roll on. The turn: KIND38_FRAMES
; = $10, facing and flip = bit 0 of KIND38_SIDE, $10 frames; KIND38_UP
; up, AnimStart 1, $20 frames; ActorMotionPreset with Y = $30; each frame
; (BRK) ActorFlyAgainst.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $38 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83BE01)
Kind38Start:
    lda.b #$02
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    rep.b #$20
    lda.w #$D1D6
    sta.b !KIND38_16
    sep.b #$20
    stz.b !KIND38_47
    lda.b #$08
    sta.b !KIND38_4B
    stz.b !KIND38_4A
    stz.b !KIND38_UP
    stz.b !KIND38_HIT
    lda.b #$01
    sta.b !KIND38_CHECKS
    bra .lBE30
.lBE30:
    ldy.b #$18
    jsl.l ActorSpeedXSet
.lBE36:
    brk.b #$00
    jsl.l ActorMoveX
    jsr.w ActorYByFrame47
    jsl.l ActorOutOfWallProbe4
    bcs .wall
    ldy.b #$02
    jsl.l ActorNearPlayerX
    bcs .lBE36
    dec.b !KIND38_CHECKS
    bne .lBE36
    jsl.l ActorSideOfPlayer
    sta.b !KIND38_SIDE
    clc
    adc.b #$03
    asl a
    tax
    jsl.l RandomFromList
    bne .lBE6E
    lda.b #$10
    sta.b !KIND38_CHECKS
    bra .lBE36
.wall:
    jsl.l ActorSideOfPlayer
    sta.b !KIND38_SIDE
.lBE6E:
    lda.b #$10
    sta.b !KIND38_FRAMES
    lda.b !KIND38_SIDE
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.lBE7A:
    brk.b #$00
    dec.b !KIND38_FRAMES
    bne .lBE7A
    inc.b !KIND38_UP
    lda.b #$01
    jsl.l AnimStart
    lda.b #$20
    sta.b !KIND38_FRAMES
.lBE8C:
    brk.b #$00
    dec.b !KIND38_FRAMES
    bne .lBE8C
    ldy.b #$30
    jsl.l ActorMotionPreset
.lBE98:
    brk.b #$00
    jsl.l ActorFlyAgainst
    bra .lBE98

; @routine 83:BEA0 Kind38StepEndOutOfRange
; WHAT IT DOES: A frame's animation step, then the actor ended for good
; when it is out of range of the camera's point by box $10; else it
; returns. Nothing in the game calls it (no JSR, JSL, JMP, pointer or
; word in the ROM names it): unit_bank83_keep_paths runs it in range.
; HOW: Its $07's bit 0 tested, with a BNE to the next instruction (no
; effect but the flags); AnimStep; X = $10; on to ActorEndOutOfRange
; (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB the boxes' bank
; ($80), whose $0000-$1FFF is work RAM; called by JSR from the actor's
; code at its own level. Exit: as ActorEndOutOfRange.
%at($83BEA0)
Kind38StepEndOutOfRange:
    lda.b !actor_07
    and.b #$01
    bne .step
.step:
    jsl.l AnimStep
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:BEAF Kind38End
; @actor
; WHAT IT DOES: Ends the actor for good (ActorEndPlaced). No branch, jump,
; word or resume address in the ROM leads to it: cov_kind38_end resumes a
; live kind $38 here.
; HOW: JML to ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's RTI at the actor's resume address. Exit: never
; returns: as ActorEndPlaced.
Kind38End:
    jml.l ActorEndPlaced

; @routine 83:BD72 Kind37Second
; @actor
; WHAT IT DOES: The second state of kinds $37 and $38 (the table at
; $83:8DFD), once Kind37Wait has downed one: animation 3 and sound $F;
; bounced on (KIND38_HIT 1), it shows itself $0A frames, rises $10
; frames, pauses $10 frames and drops away; else (downed by a shot) it
; flies off; either way it is ended out of range (box 0).
; HOW: AnimStart 3, SoundQueueAdd $F. KIND38_HIT 1: $0A frames of
; AnimStep (BRK); ActorMotionPresetY $26, $10 frames of AnimStep and
; ActorFallAgainst; $10 frames of AnimStep; ActorSpeedYSet $27; then
; each frame AnimStep, ActorMoveY and ActorEndOutOfRange with X = 0.
; Else ActorMotionPreset $1F, $0A frames (COP); then each frame
; AnimStep, ActorFly and ActorEndOutOfRange with X = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kinds $37 and $38 in state $8C. Exit: never
; returns (ended out of range).
%at($83BD72)
Kind37Second:
    lda.b #$03
    jsl.l AnimStart
    lda.b #$0F
    jsl.l SoundQueueAdd
    lda.b !KIND38_HIT
    dec a
    bne .shot
    lda.b #$0A
    sta.b !KIND38_FRAMES
.show:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND38_FRAMES
    bne .show
    lda.b #$10
    sta.b !KIND38_FRAMES
    ldy.b #$26
    jsl.l ActorMotionPresetY
.rise:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFallAgainst
    dec.b !KIND38_FRAMES
    bne .rise
    lda.b #$10
    sta.b !KIND38_FRAMES
.pause:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND38_FRAMES
    bne .pause
    ldy.b #$27
    jsl.l ActorSpeedYSet
.drop:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorMoveY
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .drop
.shot:
    ldy.b #$1F
    jsl.l ActorMotionPreset
    lda.b #$0A
    cop.b #$00
.fly:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .fly
