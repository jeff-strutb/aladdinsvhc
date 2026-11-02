; kind70_start.asm: the start of actor kind $70 (and of kind $6E in state
; $4C): a scripted actor that either rises, flies off and carries the
; escort actor once it meets it, or waits to be hit and then cues the
; scene on. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind70_start.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND70_CARRYING = $40  ; frames of the first flight; then nonzero while it carries the escort
!SCENE_CUE = $146C  ; counted up by scripted actors as the scene goes on
!SCENE_CUE_C2 = $C2  ; counted up when the hit one has waited; another actor waits for it

; @routine 83:F296 Kind70Start
; @actor
; WHAT IT DOES: A scripted actor's start. $07 zero: it rises (animations
; 0, 1, 2 with waits), flies off for $28 frames, then flies on under
; gravity until out of range; when the escort actor, not rising, comes
; within 8 pixels across and 16 down of it, it counts SCENE_CUE up and
; carries the escort 30 pixels to its left from then on. $07 nonzero: it
; faces left and waits until it can be hit, counts SCENE_CUE up, waits $80
; frames, counts SCENE_CUE_C2 up and plays animations 4 and 5, then stays.
; HOW: Bits $90 set in $09. First way: animation 0, COP $20; animation 1,
; COP $17; position +16 x, -13 y; animation 2, ActorMotionPreset $4F; $28
; frames of ActorFly; gravity $18; then each frame ActorFlyAgainst and
; ActorEndOutOfRange (box 0); carrying: the escort's x = its x - 30, y =
; its y; not: the escort's speed y not negative, (x - 24) - its x below 8
; and (y + 16) - its y below 16, then SCENE_CUE and $40 up. Second way:
; facing and flip 1, animation 3; BRK until bit 6 of $09; SCENE_CUE up,
; COP $80; SCENE_CUE_C2 up, animation 4, COP $20, animation 5, then BRK
; for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $70 in state $0C and kind $6E in state
; $4C. Exit: never returns: its first way ends in ActorEndOutOfRange.
%at($83F296)
Kind70Start:
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    beq .lF2A1
    jmp.w .lF339
.lF2A1:
    stz.b !KIND70_CARRYING
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$17
    cop.b #$00
    rep.b #$21
    lda.b !actor_x+1
    adc.w #$0010
    sta.b !actor_x+1
    lda.b !actor_y+1
    sec
    sbc.w #$000D
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$4F
    jsl.l ActorMotionPreset
    lda.b #$28
    sta.b !KIND70_CARRYING
.lF2DA:
    brk.b #$00
    jsl.l ActorFly
    dec.b !KIND70_CARRYING
    bne .lF2DA
    lda.b #$18
    sta.b !actor_accel_y
.lF2E8:
    brk.b #$00
    jsl.l ActorFlyAgainst
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    lda.b !KIND70_CARRYING
    beq .lF30B
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$FFE2
    sta.w !actor_escort+!actor_x+1
    lda.b !actor_y+1
    sta.w !actor_escort+!actor_y+1
.lF307:
    sep.b #$20
    bra .lF2E8
.lF30B:
    lda.w !actor_escort+!actor_speed_y+2
    bmi .lF2E8
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$FFE8
    sec
    sbc.w !actor_escort+!actor_x+1
    cmp.w #$0008
    bcs .lF307
    clc
    lda.b !actor_y+1
    adc.w #$0010
    sec
    sbc.w !actor_escort+!actor_y+1
    cmp.w #$0010
    bcs .lF307
    sep.b #$20
    inc.w !SCENE_CUE
    inc.b !KIND70_CARRYING
    bra .lF2E8
.lF339:
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$03
    jsl.l AnimStart
.lF345:
    brk.b #$00
    lda.b !actor_09
    bit.b #$40
    beq .lF345
    inc.w !SCENE_CUE
    lda.b #$80
    cop.b #$00
    inc.w !SCENE_CUE_C2
    lda.b #$04
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$05
    jsl.l AnimStart
.lF367:
    brk.b #$00
    bra .lF367
