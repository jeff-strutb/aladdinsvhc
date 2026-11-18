; kind6b.asm: actor kind $6B, which the camera trigger task starts with
; $07 = 1: shown in its first pose; with $07 nonzero it waits for the
; task's cue $00BF, then the second cue $00BE, and walks off to the right
; with a song, a pause and a second walk, and ends.
; C: src/aladdin/actor/kind6b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND6B_16 = $16                ; word: $D663
!CUE_BF = $00BF                 ; the camera trigger task's third cue: cleared as it starts
!CUE_BE = $00BE                 ; the scene's cue to walk off
!KIND6B_FRAMES = $40            ; frames left of a walk
!KIND6B_SONG = $31              ; the song it starts as it walks off

; @routine 83:EC2C Kind6BWait
; @actor
; WHAT IT DOES: Actor kind $6B: shown facing right in animation 0. With
; $07 zero it stays so; else it waits for CUE_BF, moves $20 pixels right
; in animation 1, waits for CUE_BE, then after $40 frames starts the song
; $31 and walks right ($70 frames, speed $3F), pauses ($30 frames) and
; walks on ($60 frames), and ends.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; actor_2c bits 1-3 = 1
; (and $F1, or 2); KIND6B_16 = $D663; facing and flip 0. $07 zero:
; AnimStart 0, then BRK for good. Else AnimStart 0, then each frame
; (BRK first) until CUE_BF is nonzero; its x pixels + $20; AnimStart 1;
; actor_08 bits 0-1 cleared; each frame (BRK first) until CUE_BE; COP $40;
; SongPlay $31; AnimStart 3; KIND6B_FRAMES $70, ActorSpeedXSet $3F; each
; frame (BRK first) ActorMoveX, ActorSnapToGround, KIND6B_FRAMES down to
; 0; AnimStart 4, COP $30, AnimStart 3; $60 frames as before; JML
; ActorEndPlaced.
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
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$0020
    sta.b !actor_x+1
    sep.b #$20
    lda.b #$01
    jsl.l AnimStart
    lda.b #$03
    trb.b !actor_08
.wait_off:
    brk.b #$00
    lda.w !CUE_BE
    beq .wait_off
    lda.b #$40
    cop.b #$00
    lda.b #!KIND6B_SONG
    jsl.l SongPlay
    lda.b #$03
    jsl.l AnimStart
    lda.b #$70
    sta.b !KIND6B_FRAMES
    ldy.b #$3F
    jsl.l ActorSpeedXSet
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !KIND6B_FRAMES
    bne .walk
    lda.b #$04
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    lda.b #$03
    jsl.l AnimStart
    lda.b #$60
    sta.b !KIND6B_FRAMES
.walk_on:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !KIND6B_FRAMES
    bne .walk_on
    jml.l ActorEndPlaced
