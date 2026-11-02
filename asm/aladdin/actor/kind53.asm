; kind53.asm: actor kind $53, a thing another actor holds, then lets drop:
; it falls, bounces along the ground and breaks. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/actor/kind53.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND53_16 = $16                ; word: $D38C
!KIND53_LET_GO = $45            ; nonzero: its holder has let it go (copied from the holder's)
!KIND53_BOUNCES = $40           ; bounces left; then frames left breaking
!KIND53_COUNTER = $4C           ; word: the record counting its kind's live ones at $4B
!KIND53_HOLDER = $4E            ; word: the record of the actor holding it
!COUNTER_LIVE = $4B             ; in the counting record: how many are live
; By its holder's animation frame, 4 bytes each: where it is held, x and
; y pixels from the holder's (words).
!KIND53_HELD_AT = $D394

; @routine 83:D566 Kind53Held
; @actor
; WHAT IT DOES: Actor kind $53 started new: held by another actor, at the
; place its holder's animation frame gives, until the holder lets it go;
; then it falls to the ground, bounces 8 times along it (turned round at
; walls) and breaks.
; HOW: $08 = 3, $09 = $90, $16 = $D38C, its look $0600, facing left,
; animation 7. Each frame while $45 is 0: the holder's ($4E) facing and
; $45 copied, its position the holder's plus the offset in $83:D394 by
; the holder's frame. Let go: ActorMotionReset ($10), facing the flip's
; bit 0; ActorFall each frame to the ground; sound $30, ActorMotionPreset
; $42 and a flight; then 8 times: sound $30 when it can be hit,
; ActorSpeedYSet $4E and a flight. Bit 7 of $09 cleared, then the break
; Kind53Thrown ends in.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $0C. Exit: never returns; ends in
; ActorEndPlaced.
%at($83D566)
Kind53Held:
    lda.b #$03
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D38C
    sta.b !KIND53_16
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    stz.b !KIND53_LET_GO
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$07
    jsl.l AnimStart
.held:
    brk.b #$00
    lda.b !KIND53_LET_GO
    bne .let_go
    rep.b #$10
    ldx.b !KIND53_HOLDER
    lda.w !actor_facing,x
    sta.b !actor_facing
    lda.w !actor_flip,x
    sta.b !actor_flip
    lda.w !ACTOR_FRAME,x
    sep.b #$10
    asl a
    asl a
    tax
    rep.b #$20
    lda.w !KIND53_HELD_AT,x
    sta.b !actor_x+1
    lda.w !KIND53_HELD_AT+2,x
    sta.b !actor_y+1
    rep.b #$11
    ldx.b !KIND53_HOLDER
    lda.w !actor_x+1,x
    adc.b !actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !actor_y+1,x
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    lda.w !KIND53_LET_GO,x
    sta.b !KIND53_LET_GO
    sep.b #$10
    beq .held
.let_go:
    lda.b #$10
    jsl.l ActorMotionReset
    lda.b !actor_flip
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.falling:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .falling
    lda.b #$30
    jsl.l SoundQueueAdd
    ldy.b #$42
    jsl.l ActorMotionPreset
.first_flight:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    bcc .first_air
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.first_air:
    jsl.l ActorOutOfGround
    beq .first_flight
    lda.b #$08
    sta.b !KIND53_BOUNCES
.bounce:
    lda.b !actor_09
    and.b #$40
    beq .silent
    lda.b #$30
    jsl.l SoundQueueAdd
.silent:
    ldy.b #$4E
    jsl.l ActorSpeedYSet
.flight:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    bcc .air
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.air:
    jsl.l ActorOutOfGround
    beq .flight
    dec.b !KIND53_BOUNCES
    bne .bounce
    lda.b #$80
    trb.b !actor_09
    bra Kind53Thrown_break

; @routine 83:D694 Kind53Thrown
; @actor
; WHAT IT DOES: Actor kind $53 started in state $4C: thrown; it flies
; until it reaches the ground, then breaks.
; HOW: ActorMotionPreset $4B; each frame AnimStep and ActorMove until
; ActorOutOfGround. The break: animation 8, sound $24 when it can be hit,
; $28 frames of animation, the count at $4B of the record at $4C down by
; one, then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for state $4C. Exit: never returns; ends in
; ActorEndPlaced.
%at($83D694)
Kind53Thrown:
    ldy.b #$4B
    jsl.l ActorMotionPreset
.flying:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorMove
    jsl.l ActorOutOfGround
    beq .flying
.break:
    lda.b #$08
    jsl.l AnimStart
    lda.b !actor_09
    and.b #$40
    beq .quiet
    lda.b #$24
    jsl.l SoundQueueAdd
.quiet:
    lda.b #$28
    sta.b !KIND53_BOUNCES
.breaking:
    brk.b #$00
    jsl.l AnimStep
    dec.b !KIND53_BOUNCES
    bne .breaking
    rep.b #$10
    ldx.b !KIND53_COUNTER
    dec.w !COUNTER_LIVE,x
    sep.b #$10
    jml.l ActorEndPlaced
