; sounds, takes a motion preset and flies off the way he faces until it
; is out of range, where it ends. Kinds reach it from their second state
; (kind $20's at 83:9C19). Drafted by cdraft.py; named, completed and
; described by hand.
; C: src/aladdin/actor/knocked_away.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

!KA_STEPS = $40                ; frames left of its first poses
!KA_ANIM_AT = $2F              ; the frame AnimResume takes up the animation at
!KA_LEVEL_A = $0D              ; on these levels a falling actor drops behind the layers
!KA_LEVEL_B = $02

; @routine 83:9532 ActorKnockedAway
; WHAT IT DOES: Knocks the actor away: animation A, sound X, motion preset
; Y; it faces away from Aladdin, poses 16 frames, then flies off until it
; is out of range and ends.
; HOW: X and Y kept on the stack; AnimStart A, ActorPictureSend;
; ActorMotionPreset Y; SoundQueueAdd X; its facing and flip Aladdin's
; flip with bit 0 turned; STEPS 16 frames (BRK) of AnimStep and
; ActorPictureSend; then fly_off.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; A = the
; animation, X = the sound, Y = the preset; reached by a kind's jump.
; Exit: never returns.
%at($839532)
ActorKnockedAway:
    phx
    phy
    jsl.l AnimStart
    jsl.l ActorPictureSend
    ply
    jsl.l ActorMotionPreset
    pla
    jsl.l SoundQueueAdd
    lda.w !player+!actor_flip       ; away from Aladdin
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #16
    sta.b !KA_STEPS
.pose:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorPictureSend
    dec.b !KA_STEPS
    bne .pose
.fly:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorPictureSend
    jsl.l ActorFly
    lda.w !level
    cmp.b #!KA_LEVEL_A
    beq .behind
    cmp.b #!KA_LEVEL_B
    bne .range
.behind:
    lda.b !actor_speed_y+2          ; (no script runs this)
    bmi .range
    lda.b #$30
    trb.b !actor_09
.range:
    ldx.b #0
    jsr.w ActorEndOutOfRange
    bra .fly

; @routine 83:9589 ActorKnockedAwayResumed
; WHAT IT DOES: Knocks the actor away with its animation taken up where it
; is: ANIM_AT = X, motion preset Y, five frames' wait, then it flies off
; until it is out of range and ends.
; HOW: Y kept on the stack; ANIM_AT = X; AnimResume, ActorPictureSend;
; ActorMotionPreset Y; COP 5; then fly_off (into ActorKnockedAway's
; flight).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; A = the
; animation, X its frame, Y the preset; reached by a kind's jump.
; Exit: never returns.
ActorKnockedAwayResumed:
    phy
    stx.b !KA_ANIM_AT
    jsl.l AnimResume
    jsl.l ActorPictureSend
    ply
    jsl.l ActorMotionPreset
    lda.b #$05
    cop.b #$00
    bra ActorKnockedAway_fly

; @routine 83:9C19 Kind20Hit
; WHAT IT DOES: Actor kind $20's guard struck: from in front (its facing
; not negative) knocked away (animation 4, sound $0F, preset 3); from
; behind turned round, sound 7, and knocked away from where its
; animation is (animation 4 at frame 3, preset $4C). With PLACE bit 4
; (no script) its palette from ROM entry 5 first.
; HOW: PLACE bit 4: ActorPaletteFromRom Y = 5. ACTOR_FACING not
; negative: A 4, X $0F, Y 3, on to ActorKnockedAway. Else facing = its
; bit 0, flip the other; SoundQueueAdd 7; A 4, X 3, Y $4C,
; ActorKnockedAwayResumed.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; the actor loop's jump for kind $20 in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($839C19)
Kind20Hit:
    lda.b !actor_07
    and.b #$10
    beq .face
    ldy.b #$05                     ; (no script runs this)
    jsl.l ActorPaletteFromRom
.face:
    lda.b !actor_facing
    bmi .behind
    lda.b #$04
    ldx.b #$0F
    ldy.b #$03
    jmp.w ActorKnockedAway
.behind:
    and.b #$01
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    lda.b #$07
    jsl.l SoundQueueAdd
    lda.b #$04
    ldx.b #$03
    ldy.b #$4C
    jmp.w ActorKnockedAwayResumed
