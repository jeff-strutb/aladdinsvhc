; kind_a2.asm: actor kind $A2: a shot another actor sends out, which turns
; back to it and is taken in, or falls away once its sender is hit.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind_a2.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KINDA2_TIME = $40  ; frames left of its way out
!KINDA2_SENDER = $4C  ; word: the record of the actor that sent it
!SENDER_OUT = $42  ; in the sender's record: its shots out (word)
!SENDER_HIT = $47  ; in the sender's record: nonzero once it is hit
!KINDA2_HEADINGS = $D501  ; headings by facing (0 right, 3 left) plus $07, in the bank DB names

; @routine 84:8249 KindA2Sent
; @actor
; WHAT IT DOES: Actor kind $A2 started new: a shot sent out by another
; actor (its record at $4C) along one of its headings for $20 frames; then
; it turns back and flies on until it reaches its sender, which takes it
; in (one shot fewer out), or falls away if the sender has been hit
; meanwhile.
; HOW: $08 bit 1 and $09 bits $90 set, its look $0600, sound $2F,
; animation $1A, both speeds cleared; its heading (in $14) from
; KINDA2_HEADINGS at $07 plus 0 facing right or 3 left. Each frame
; ActorSpeedAddVector (X = $22), ActorMoveRight and ActorMoveY; after $20
; frames the heading's bit 3 flipped (back), and on the same each frame:
; the sender's $47 set: animation $1B, bit 7 of $09 cleared and into the
; fall of KindA2Hit; else within $30 below and 16 above, and $20 across,
; of the sender: its $42 down by one, then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $A2 in state $0C (through the stub at
; $83:9416). Exit: never returns; ends in ActorEndPlaced.
%at($848249)
KindA2Sent:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    lda.b #$2F
    jsl.l SoundQueueAdd
    lda.b #$1A
    jsl.l AnimStart
    jsl.l ActorSpeedXClear
    jsl.l ActorSpeedYClear
    lda.b !actor_facing
    beq .l8278
    lda.b #$03
.l8278:
    clc
    adc.b !actor_07
    tax
    lda.w !KINDA2_HEADINGS,x
    sta.b !actor_facing
    lda.b #$20
    sta.b !KINDA2_TIME
.l8285:
    brk.b #$00
    ldx.b #$22
    jsl.l ActorSpeedAddVector
    jsl.l ActorMoveRight
    jsl.l ActorMoveY
    dec.b !KINDA2_TIME
    bne .l8285
    lda.b !actor_facing
    eor.b #$08
    sta.b !actor_facing
.l829F:
    sep.b #$30
    brk.b #$00
    ldx.b #$22
    jsl.l ActorSpeedAddVector
    jsl.l ActorMoveRight
    jsl.l ActorMoveY
    rep.b #$10
    ldx.b !KINDA2_SENDER
    lda.w !SENDER_HIT,x
    bne .l82E7
    rep.b #$30
    clc
    lda.w !actor_y+1,x
    adc.w #$0010
    sec
    sbc.b !actor_y+1
    cmp.w #$0030
    bcs .l829F
    clc
    lda.w !actor_x+1,x
    adc.w #$0010
    sec
    sbc.b !actor_x+1
    cmp.w #$0020
    bcs .l829F
    lda.w !SENDER_OUT,x
    dec a
    sta.w !SENDER_OUT,x
    sep.b #$30
    jml.l ActorEndPlaced
.l82E7:
    sep.b #$30
    lda.b #$1B
    jsl.l AnimStart
    lda.b #$80
    trb.b !actor_09
    bra KindA2Hit_l830E


; @routine 84:82F5 KindA2Hit
; @actor
; WHAT IT DOES: Actor kind $A2 started in state $4C (hit): its sender has
; one shot fewer out, and it falls away until it can no longer be hit,
; then ends.
; HOW: Sound $12, animation $1B; the sender's ($4C) $42 down by one
; (8-bit); then ActorMotionPreset $6D and ActorPriorityClear; each frame
; AnimStep and ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $A2 in state $4C (through the stub at
; $83:941F). Exit: never returns; ends in ActorEndPlaced.
%at($8482F5)
KindA2Hit:
    lda.b #$12
    jsl.l SoundQueueAdd
    lda.b #$1B
    jsl.l AnimStart
    rep.b #$10
    ldx.b !KINDA2_SENDER
    lda.w !SENDER_OUT,x
    dec a
    sta.w !SENDER_OUT,x
    sep.b #$10
.l830E:
    ldy.b #$6D
    jsl.l ActorMotionPreset
    jsr.w ActorPriorityClear
.l8317:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    lda.b !actor_09
    bit.b #$40
    bne .l8317
    jml.l ActorEndPlaced
