; kind_a0.asm: actor kind $A0, a ledge that gives way: once Aladdin stands
; on it or hangs from it, it shakes for 32 frames, then falls, carrying him
; down while he stands on it.
; C: src/aladdin/actor/kind_a0.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!KA0_ON = $40                   ; the fall: nonzero while Aladdin stands on it
!KA0_FRAMES = $41               ; the shake's frames left
!KA0_13 = $13                   ; 7
!PLAYER_380 = $000380           ; bit 2 set: Aladdin cannot hang (no hold test)
!KEPT_Y = !scratch              ; its y (pixels, two bytes) before the frame's fall
!SOLID_BOX = $30                ; ActorSolidForPlayer's box
!CANNOT_HANG = $04

; @routine 84:A8E0 KindA0Start
; @actor
; WHAT IT DOES: Actor kind $A0, a ledge that gives way: solid for
; Aladdin (and to hang from) in pose 0; once he stands on it or hangs
; from it, 32 frames more as it is, then pose 1 and it falls (gravity
; $0A), moving him down with it while he stands on it; it stays solid
; and holdable as it falls.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; AnimStart 0; KA0_13 7.
; Each frame (BRK): ActorSolidForPlayerLong with Y = $30, C clear: on;
; PLAYER_380 bit 2 clear: ActorHoldPlayerLong, C clear: on. Then
; KA0_FRAMES $20 frames of BRK, the same tests (whatever they say).
; AnimStart 1; ActorSpeedYSetGravity with A = $0A, Y = 0. Each frame
; (BRK): KA0_ON 0; ActorSolidForPlayerLong, C clear: KA0_ON up and its
; y kept at KEPT_Y; ActorFall; KA0_ON set: Aladdin's y + (its y less
; KEPT_Y); PLAYER_380 bit 2 clear: ActorHoldPlayerLong. Only the first
; wait runs in the scripts; the rest is written from its bytes
; ($84:A90C-A973).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $A0
; in state $0C (the table at $83:8DAB, through the stub at $83:9400).
; Exit: never returns.
%at($84A8E0)
KindA0Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    lda.b #$07
    sta.b !KA0_13
.until_on:
    brk.b #$00
    ldy.b #!SOLID_BOX
    jsl.l ActorSolidForPlayerLong
    bcc .shake
    lda.l !PLAYER_380
    bit.b #!CANNOT_HANG
    bne .until_on
    jsl.l ActorHoldPlayerLong
    bcc .shake
    bra .until_on
.shake:
    lda.b #$20
    sta.b !KA0_FRAMES
.shaking:
    brk.b #$00
    ldy.b #!SOLID_BOX
    jsl.l ActorSolidForPlayerLong
    lda.l !PLAYER_380
    bit.b #!CANNOT_HANG
    bne .shake_count
    jsl.l ActorHoldPlayerLong
.shake_count:
    dec.b !KA0_FRAMES
    bne .shaking
    lda.b #$01
    jsl.l AnimStart
    lda.b #$0A
    ldy.b #$00
    jsl.l ActorSpeedYSetGravity
.falling:
    brk.b #$00
    stz.b !KA0_ON
    ldy.b #!SOLID_BOX
    jsl.l ActorSolidForPlayerLong
    bcs .fall
    inc.b !KA0_ON
    lda.b !actor_y+1
    sta.w !KEPT_Y
    lda.b !actor_y+2
    sta.w !KEPT_Y+1
.fall:
    jsl.l ActorFall
    lda.b !KA0_ON
    beq .hold
    rep.b #$20
    lda.b !actor_y+1
    sec
    sbc.w !KEPT_Y
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
.hold:
    lda.l !PLAYER_380
    bit.b #!CANNOT_HANG
    bne .falling
    jsl.l ActorHoldPlayerLong
    bra .falling
