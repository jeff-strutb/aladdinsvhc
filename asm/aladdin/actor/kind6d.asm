; kind6d.asm: actor kind $6D (the pots and the like that the wait routine
; Kind6DWait drives): what it does once downed (its state with bit 7 set):
; knocked away, a sound, and off until it is out of range.
; C: src/aladdin/actor/kind6d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K6D_FRAMES = $40               ; frames left before it flies off
!K6D_DOWNED = $48               ; counted up by Kind6DWait when a shot downs it
!K6D_KNOCKED = $1F              ; the motion preset it is knocked away with
!K6D_POSE = $07                 ; its animation once downed
!K6D_SOUND = $0F                ; the sound of its fall

; @routine 83:EFF0 Kind6DSecond
; @actor
; WHAT IT DOES: Actor kind $6D downed (its state with bit 7 set): knocked
; away in pose 7, with sound $0F unless a shot downed it while it faced
; right; it turns to face right, holds 10 frames, then flies each frame
; until it is out of range.
; HOW: ActorMotionPreset $1F, AnimStart 7, K6D_FRAMES $0A. K6D_DOWNED zero
; or its facing with bit 7 set: SoundQueueAdd $0F. Facing bit 7 cleared.
; Each frame (BRK) AnimStep, K6D_FRAMES times; then each frame (BRK)
; AnimStep, ActorFly and ActorEndOutOfRange with X = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $6D in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns; ends when out of range.
%at($83EFF0)
Kind6DSecond:
    ldy.b #!K6D_KNOCKED
    jsl.l ActorMotionPreset
    lda.b #!K6D_POSE
    jsl.l AnimStart
    lda.b #$0A
    sta.b !K6D_FRAMES
    lda.b !K6D_DOWNED
    beq .sound
    lda.b !actor_facing
    bpl .face
.sound:
    lda.b #!K6D_SOUND
    jsl.l SoundQueueAdd
.face:
    lda.b !actor_facing
    bpl .hold
    and.b #$7F
    sta.b !actor_facing
.hold:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K6D_FRAMES
    bne .hold
.fly:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .fly
