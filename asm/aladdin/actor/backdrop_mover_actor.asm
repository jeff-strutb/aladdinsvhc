; backdrop_mover_actor.asm: actor kind $7C, the backdrop mover (helpers in
; backdrop_mover.c), and kind $A3, a shot that steps out of its sender and
; flies off. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/backdrop_mover_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!MOVER_FRAMES = $40  ; word: frames left of its move ($0333)
!MOVER_HUM = $42  ; frames until its hum is queued again
!MOVER_43 = $43
!A3_ANIMS = $D507  ; by $07, its animation (in the bank DB names)
!A3_STEPS = $D509  ; by $07, how far across it steps out (words)

; @routine 84:9044 BackdropMover
; @actor
; WHAT IT DOES: Actor kind $7C: moves the backdrop layer against the play
; field one way for $0333 frames with a hum every $30, stops with a clunk,
; waits, and goes back the other way, for good.
; HOW: Position cleared, $09 = $80, $42 and $43 cleared. Each move: $40 =
; $0333 (word); each frame (BRK first) $42 down, past zero: MoverSound $3F
; and $42 = $30; BackdropOffsetStep until it counts $40 out (Z set). Then
; MoverSound $0D, COP $3F, the facing flipped.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $7C in state $0C (through the stub at
; $83:928E). Exit: never returns.
%at($849044)
BackdropMover:
    rep.b #$20
    stz.b !actor_x
    stz.b !actor_x+2
    stz.b !actor_y+1
    sep.b #$20
    lda.b #$80
    sta.b !actor_09
    stz.b !MOVER_HUM
    stz.b !MOVER_43
.l9056:
    lda.b #$33
    sta.b !actor_40
    lda.b #$03
    sta.b !actor_41
.l905E:
    brk.b #$00
    dec.b !MOVER_HUM
    bpl .l906D
    lda.b #$3F
    jsr.w MoverSound
    lda.b #$30
    sta.b !MOVER_HUM
.l906D:
    jsr.w BackdropOffsetStep
    bne .l905E
    lda.b #$0D
    jsr.w MoverSound
    lda.b #$3F
    cop.b #$00
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    bra .l9056


; @routine 84:8349 KindA3Shot
; @actor
; WHAT IT DOES: Actor kind $A3: a shot facing right; shown, it waits 8
; frames, steps out across by A3_STEPS, waits 8 more, then flies off
; (motion $6C) until it can no longer be hit, and ends.
; HOW: $08 bit 1, $09 bit 7 set, its look $0400, flip and facing 0 (the
; look's low byte), animation A3_ANIMS by $07; COP 8; x pixels plus
; A3_STEPS by $07; COP 8; ActorMotionPreset $6C; each frame ActorFly while
; bit 6 of $09 is set; then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $A3 in state $0C (through the stub at
; $83:9423). Exit: never returns; ends in ActorEndPlaced.
%at($848349)
KindA3Shot:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
    rep.b #$20
    lda.w #$0400
    sta.b !actor_look
    sep.b #$20
    stz.b !actor_flip
    ldx.b !actor_07
    sta.b !actor_facing
    lda.w !A3_ANIMS,x
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b !actor_07
    asl a
    tax
    rep.b #$20
    clc
    lda.w !A3_STEPS,x
    adc.b !actor_x+1
    sta.b !actor_x+1
    sep.b #$20
    lda.b #$08
    cop.b #$00
    ldy.b #$6C
    jsl.l ActorMotionPreset
.l8389:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_09
    bit.b #$40
    bne .l8389
    jml.l ActorEndPlaced
