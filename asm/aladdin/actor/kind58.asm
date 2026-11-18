; kind58.asm: actor kind $58, the platforms of stage a3 s0 that move to
; and fro along a direction, which Aladdin rides or hangs from. Its wait
; is Kind58Wait (kind_waits_rest.asm). Named and described by hand.
; C: src/aladdin/actor/kind58.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_state.inc"

arch 65816

!K58_28 = $28                   ; set to $FF at its start
!K58_16 = $16                   ; word: $D3C0
!K58_FRAMES = $40               ; frames left of this leg or rest
!K58_LEG = $41                  ; frames of its first leg (a half leg); the others are twice
!K58_CARRIES = $43              ; nonzero: Aladdin rides it this frame (ActorTestCarries)
!K58_LIFT = $44                 ; ActorLiftPlayerStep's step: 0 none, positive hung from, negative let go
!K58_POSE = $45                 ; its animation: 0, 1 ridden, 3 the variant, 4/5 hung from, $FF Aladdin dead
!K58_COPIER = $42               ; frames to Kind58Wait's copier test
; In bank $80, by $07: the first leg's frames, and its direction.
!K58_LEGS = $D3C4
!K58_DIRS = $D3CA
!PLAYER_FLIP = !player+!actor_flip

; @routine 83:DBBB Kind58Start
; @actor
; WHAT IT DOES: Actor kind $58 started: a platform that moves along its
; direction and back, resting $40 frames at each end, showing by its
; animation whether Aladdin rides it or hangs from it.
; HOW: Bits 1 of $08 and $90 of $09 set, $28 = $FF, $16 = $D3C0, $15, $43
; and $44 = 0. $07 bit 4 clear: pose 0 (AnimStart 0); set: $07 &= $0F,
; pose 3 (AnimStart 3). K58_LEG = K58_FRAMES = K58_LEGS[$07], $14 =
; K58_DIRS[$07]. Then for good: each frame (BRK first) Kind58Pose,
; ActorMoveByVector $14 and ActorCarryPlayer for K58_FRAMES frames; $40
; frames of Kind58Pose; 2 * K58_LEG frames of Kind58Pose,
; 81:A897 (ActorMoveByVector backwards) $14 and ActorCarryPlayer; $40 frames of
; Kind58Pose; the next legs forward are 2 * K58_LEG frames too.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $0C. Exit: never returns.
%at($83DBBB)
Kind58Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b !K58_28
    rep.b #$20
    lda.w #$D3C0
    sta.b !K58_16
    sep.b #$20
    stz.b !actor_flip
    stz.b !K58_CARRIES
    stz.b !K58_LIFT
    lda.b !actor_07
    bit.b #$10
    bne .variant
    lda.b #$00
    sta.b !K58_POSE
    jsl.l AnimStart
    bra .legs
.variant:
    and.b #$0F
    sta.b !actor_07
    lda.b #$03
    sta.b !K58_POSE
    jsl.l AnimStart
.legs:
    ldx.b !actor_07
    lda.w !K58_LEGS,x
    sta.b !K58_LEG
    sta.b !K58_FRAMES
    lda.w !K58_DIRS,x
    sta.b !actor_facing
.forth:
    brk.b #$00
    jsr.w Kind58Pose
    ldx.b #$14
    jsl.l ActorMoveByVector
    jsr.w ActorCarryPlayer
    dec.b !K58_FRAMES
    bne .forth
    lda.b #$40
    sta.b !K58_FRAMES
.rest_far:
    brk.b #$00
    jsr.w Kind58Pose
    dec.b !K58_FRAMES
    bne .rest_far
    lda.b !K58_LEG
    asl a
    sta.b !K58_FRAMES
.back:
    brk.b #$00
    jsr.w Kind58Pose
    ldx.b #$14
    jsl.l $81A897               ; the reverse of ActorMoveByVector, not yet filed
    jsr.w ActorCarryPlayer
    dec.b !K58_FRAMES
    bne .back
    lda.b #$40
    sta.b !K58_FRAMES
.rest_near:
    brk.b #$00
    jsr.w Kind58Pose
    dec.b !K58_FRAMES
    bne .rest_near
    lda.b !K58_LEG
    asl a
    sta.b !K58_FRAMES
    bra .forth

; @routine 83:DC4E Kind58Pose
; WHAT IT DOES: Kind $58's pose for the frame: ridden, hung from, or
; neither, and the dead pose once Aladdin has no health; starts the
; animation when the pose changes.
; HOW: By K58_POSE: negative, RTS. 0 or 3: health 0, pose $FF
; (AnimStart 2); else pose 1 when K58_CARRIES; else, K58_LIFT positive,
; pose 4 + the player's flip; else RTS. 1: pose 0 unless K58_CARRIES; when
; it rides, K58_COPIER = $20 and RTS. 2: pose $FF. 4 and up: pose 0 once
; K58_LIFT is 0. A new pose: K58_POSE = it, AnimStart with it.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit.
%at($83DC4E)
Kind58Pose:
    lda.b !K58_POSE
    bmi .done
    beq .free
    dec a
    beq .ridden
    dec a
    beq .dead
    dec a
    beq .free
    lda.b !K58_LIFT
    bne .done
    ldx.b #$00
    bra .pose
.ridden:
    ldx.b #$00
    lda.b !K58_CARRIES
    beq .pose
    lda.b #$20
    sta.b !K58_COPIER
    rts
.free:
    lda.w !health
    beq .dead
    ldx.b #$01
    lda.b !K58_CARRIES
    bne .pose
    lda.b !K58_LIFT
    beq .done
    bmi .done
    clc
    lda.w !PLAYER_FLIP
    adc.b #$04
    tax
.pose:
    stx.b !K58_POSE
    txa
    jsl.l AnimStart
.done:
    rts
.dead:
    lda.b #$FF
    sta.b !K58_POSE
    lda.b #$02
    jsl.l AnimStart
    rts
