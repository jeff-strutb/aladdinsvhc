; kind52.asm: actor kind $52's start, the third level end's guard of act 1
; stage 0. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind52.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND52_SHOOTABLE = $48         ; nonzero: Kind52Wait tests shots on it
!KIND52_FRAMES = $40            ; frames left of what it is doing
!KIND52_REST = $41              ; frames left before it may strike again
!KIND52_SPARE = $42             ; counted down each frame; set only by the unreached block
!KIND52_LIFE = $44              ; variant: frames left of turning toward the player
!KIND52_TURN_IN = $43           ; variant: frames left to its next turn
!KIND52_DONE_TURNING = $46      ; variant: nonzero once its turning time is over
!PLAYER_X_PIXEL = !player+!actor_x+1

; @routine 83:D359 Kind52Start
; @actor
; WHAT IT DOES: Actor kind $52 started. $07 zero: it waits until the
; player comes near, faces him and rises (animation 4, then 1), and can
; be shot from then on; then, each frame, facing him: within $38 pixels
; across it strikes (animation 2 for $6C frames, sound $20) unless it is
; resting after a strike; further away it waits, alternating animation 0
; ($10 or $20 frames) and 1 ($20 or $2C frames), until he comes within
; box $24. $07 nonzero: it flies toward the player by direction, turning
; one step toward him every $14 frames for a random time, and hurts him
; when its zone meets the test box; it ends out of range.
; HOW: $48 = 0, bit 1 of $08 set. $07 zero: $09 |= $90, $21 = $07 = $4C,
; animation 5; each frame until ActorNearPlayer box $34; ActorSideOfPlayer
; into $14 and its opposite into $15; animation 4, $17 frames; $48 = 1, 9
; frames; animation 1, $18 frames. The loop: each frame ActorSideOfPlayer
; again, the distance |x - player's x| picks 0 (below $38), 1 (below $78)
; or 2, $41 and $42 counted down to 0, then JMP ($D3F7,X). Way 0: $41
; nonzero, back to the loop; else animation 2 for $6C frames with sound
; $20 when $42 frames are left, then animation 0 and $41 = RandomFromList $0A.
; Ways 1 and 2: animation 0 for $10 frames (RandomNext negative) or $20,
; then animation 1 for $20 (negative) or $2C, and again; back to the loop
; on any frame ActorNearPlayer box $24 says he is inside. The variant:
; animation 7, bits 0-1 of $08 cleared, $14 times 8 (a direction),
; $44 = RandomFromList $10, $43 = $14, $46 = 0; each frame: until $44
; runs out, every $14 frames $14 one step (of 16) toward
; ActorDirectionToPlayer unless already on it; ActorMoveByVector $12,
; AnimStep, ActorEndOutOfRange 0; ActorZoneHasTestBox $34 met:
; PlayerEnter8CFacing.
; The block at $83:D42E-D46B (a COP wait, ActorSpawnSameLook with its hit
; box and a count up at $7E:FF00 by hit box, setting $42) is reached from
; nowhere: the table at $D3F7 holds $D3FD, $D46C, $D46C, and no branch,
; jump or pointer names it. Its C stands where the block does, under a
; label nothing jumps to.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $0C. Exit: never returns; the variant
; ends in ActorEndOutOfRange.
%at($83D359)
Kind52Start:
    stz.b !KIND52_SHOOTABLE
    lda.b #$02
    tsb.b !actor_08
    lda.b !actor_07
    beq .waiting
    jmp.w .variant
.waiting:
    lda.b #$90
    tsb.b !actor_09
    lda.b #$4C
    sta.b !actor_spawn_at
    sta.b !actor_07
    lda.b #$05
    jsl.l AnimStart
.until_near:
    brk.b #$00
    ldy.b #$34
    jsl.l ActorNearPlayer
    bcs .until_near
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    lda.b #$04
    jsl.l AnimStart
    lda.b #$17
    sta.b !KIND52_FRAMES
.rising:
    brk.b #$00
    dec.b !KIND52_FRAMES
    bne .rising
    lda.b #$01
    sta.b !KIND52_SHOOTABLE
    lda.b #$09
    sta.b !KIND52_FRAMES
.risen:
    brk.b #$00
    dec.b !KIND52_FRAMES
    bne .risen
    lda.b #$01
    jsl.l AnimStart
    lda.b #$18
    sta.b !KIND52_FRAMES
.standing:
    brk.b #$00
    dec.b !KIND52_FRAMES
    bne .standing
.loop:
    brk.b #$00
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    rep.b #$20
    lda.b !actor_x+1
    sec
    sbc.w !PLAYER_X_PIXEL
    bpl .distance
    eor.w #$FFFF
    inc a
.distance:
    cmp.w #$0038
    bcs .not_near
    ldx.b #$00
    bra .picked
.not_near:
    cmp.w #$0078
    bcs .far
    ldx.b #$02
    bra .picked
.far:
    ldx.b #$04
.picked:
    sep.b #$20
    lda.b !KIND52_REST
    beq .rested
    dec.b !KIND52_REST
.rested:
    lda.b !KIND52_SPARE
    beq .spare_out
    dec.b !KIND52_SPARE
.spare_out:
    jmp.w (.ways,x)
.ways:
    dw .near, .away, .away
.near:
    lda.b !KIND52_REST
    bne .back
    lda.b #$02
    jsl.l AnimStart
    lda.b #$6C
    sta.b !KIND52_FRAMES
.striking:
    brk.b #$00
    lda.b !KIND52_FRAMES
    cmp.b #$42
    bne .quiet
    lda.b #$20
    jsl.l SoundQueueAdd
.quiet:
    dec.b !KIND52_FRAMES
    bne .striking
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$0A
    jsl.l RandomFromList
    sta.b !KIND52_REST
.back:
    jmp.w .loop
; Reached from nowhere (see the header).
.unreached:
    lda.b !KIND52_SPARE
    bne .unreached_back
    lda.b #$06
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    lda.b !actor_flip
    eor.b #$01
    sta.b !actor_flip
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .unreached_none
    ldx.b !actor_06
    lda.l !kind_count,x
    inc a
    sta.l !kind_count,x
.unreached_none:
    lda.b !actor_flip
    eor.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldx.b #$0E
    jsl.l RandomFromList
    sta.b !KIND52_SPARE
.unreached_back:
    jmp.w .loop
.away:
    ldx.b #$10
    jsl.l RandomNext
    bmi .idle_for
    ldx.b #$20
.idle_for:
    stx.b !KIND52_FRAMES
    lda.b #$00
    jsl.l AnimStart
.idling:
    brk.b #$00
    dec.b !KIND52_FRAMES
    beq .idled
    ldy.b #$24
    jsl.l ActorNearPlayer
    bcs .idling
    jmp.w .loop
.idled:
    ldx.b #$20
    jsl.l RandomNext
    bmi .pose_for
    ldx.b #$2C
.pose_for:
    stx.b !KIND52_FRAMES
    lda.b #$01
    jsl.l AnimStart
.posing:
    brk.b #$00
    dec.b !KIND52_FRAMES
    beq .away
    ldy.b #$24
    jsl.l ActorNearPlayer
    bcs .posing
    jmp.w .loop
.variant:
    lda.b #$07
    jsl.l AnimStart
    lda.b #$03
    trb.b !actor_08
    asl.b !actor_facing
    asl.b !actor_facing
    asl.b !actor_facing
    ldx.b #$10
    jsl.l RandomFromList
    sta.b !KIND52_LIFE
    lda.b #$14
    sta.b !KIND52_TURN_IN
    stz.b !KIND52_DONE_TURNING
.flying:
    brk.b #$00
    lda.b !KIND52_DONE_TURNING
    bne .move
    dec.b !KIND52_LIFE
    bne .turn_due
    inc.b !KIND52_DONE_TURNING
.turn_due:
    dec.b !KIND52_TURN_IN
    bne .move
    lda.b #$14
    sta.b !KIND52_TURN_IN
    jsl.l ActorDirectionToPlayer
    sec
    sbc.b !actor_facing
    and.b #$0F
    beq .move
    cmp.b #$08
    ldx.b !actor_facing
    inx
    bcc .turned
    dex
    dex
.turned:
    txa
    and.b #$0F
    sta.b !actor_facing
.move:
    ldx.b #$12
    jsl.l ActorMoveByVector
    jsl.l AnimStep
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    ldy.b #$34
    jsr.w ActorZoneHasTestBox
    bcs .flying
    jsr.w PlayerEnter8CFacing
    bra .flying
