; kind21.asm: actor kind $21's start: by its $07, one of two figures that
; play out a little scene, or a third part no script runs.
; C: src/aladdin/actor/kind21.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!V_146B = $146B                 ; part 0 clears it as it starts and counts it up halfway
!KIND21_28 = $28                ; $FF
!KIND21_16 = $16                ; word: $D06A
!KIND21_29 = $29                ; word: $8909
!FRAMES = $40                   ; frames left of a step
!KIND21_PART2 = $F3D1           ; part 2's code (no script runs it)

; @routine A5:F309 Kind21Start
; @actor
; WHAT IT DOES: Actor kind $21: by its $07, part 0 waits 64 frames, walks
; left for 128, poses (3) for 128, counts V_146B up, turns right (pose
; $1E) for 16 frames, then walks right faster (2) for 128 and stays; part 1
; watches Aladdin's x: pose $0F while its bit 7 is clear, pose 4 while it
; is set, checking every 8 frames.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; KIND21_28 = $FF, its
; sprite attributes $0220, KIND21_16 = $D06A, KIND21_29 = $8909; then by
; $07 through .parts. Part 0: V_146B 0, AnimStart 1, facing and flip 1,
; speed bytes 0, 1, 0 ($19-$1B), frame counts in FRAMES with BRK each frame and
; ActorMoveX while walking; the second walk with $1A = 3. Part 1: facing
; and flip 0; AnimStart $0F, COP 8 until bit 7 of the player's x's low
; byte is set; AnimStart 4, COP 8 until it is clear; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $21
; in state $0C (the table at $83:8CAB, through the stub at $83:9226).
; Exit: never returns.
%at($A5F309)
Kind21Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b !KIND21_28
    rep.b #$20
    lda.w #$0220
    sta.b !actor_look
    lda.w #$D06A
    sta.b !KIND21_16
    lda.w #$8909
    sta.b !KIND21_29
    sep.b #$20
    lda.b !actor_07
    asl a
    tax
    jmp.w (.parts,x)
.parts:
    dw .part0, .part1, !KIND21_PART2
.part0:
    stz.w !V_146B
    lda.b #$01
    jsl.l AnimStart
    lda.b #$40
    sta.b !FRAMES
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !actor_speed_x
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.wait:
    brk.b #$00
    dec.b !FRAMES
    bne .wait
    lda.b #$80
    sta.b !FRAMES
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !FRAMES
    bne .walk
    lda.b #$03
    jsl.l AnimStart
    lda.b #$80
    sta.b !FRAMES
.pose:
    brk.b #$00
    dec.b !FRAMES
    bne .pose
    inc.w !V_146B
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$1E
    jsl.l AnimStart
    lda.b #$10
    sta.b !FRAMES
.turn:
    brk.b #$00
    dec.b !FRAMES
    bne .turn
    lda.b #$02
    jsl.l AnimStart
    lda.b #$03
    sta.b !actor_speed_x+1
    lda.b #$80
    sta.b !FRAMES
.walk2:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !FRAMES
    bne .walk2
.stay:
    brk.b #$00
    bra .stay
.part1:
    stz.b !actor_facing
    stz.b !actor_flip
.away:
    lda.b #$0F
    jsl.l AnimStart
.away_wait:
    lda.w !player_x
    bmi .near
    lda.b #$08
    cop.b #$00
    bra .away_wait
.near:
    lda.b #$04
    jsl.l AnimStart
.near_wait:
    lda.w !player_x
    bpl .away
    lda.b #$08
    cop.b #$00
    bra .near_wait
