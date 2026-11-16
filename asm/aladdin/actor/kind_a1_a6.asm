; kind_a1_a6.asm: actor kinds $A1 in state $4C (hit: it flies off) and $A6
; (it stays until far above or below the point actors are kept in range
; of). Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind_a1_a6.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!RANGE_Y = $1A46  ; the y of the point actors are kept in range of (word)

!KIND_A1_HITS = $41             ; counted up each time Aladdin comes down on it
!KIND_A1_SAFE = $1B2B           ; nonzero: it goes straight to state $8C, untested

; @routine 84:A9C7 KindA1Touch
; WHAT IT DOES: What actors of kind $A1 do each frame they wait: animate;
; once it can no longer be hit it ends; else Aladdin touching it is hurt,
; or coming down on it is stood on it, which counts a hit, plays sound $0F
; and sends it to state $8C; not stood on, a shot can down it. With
; KIND_A1_SAFE set it goes to state $8C at once.
; HOW: AnimStep; BIT actor_09: bit 6 clear, its JSL's and the loop's JSR
; return addresses dropped (five pulls) and on into ActorEndPlaced at the
; loop's level. KIND_A1_SAFE zero: ActorBouncesOrHurtsPlayerLong, A - 1
; nonzero: ActorDownedByShotLong and back; zero: KIND_A1_HITS up and
; SoundQueueAdd $0F. Then bit 7 of actor_09 cleared (ASL, LSR) and state
; $8C.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL from the kind's stub ($83:940D) through the
; actor loop's wait table at $83:904F. Exit: ended: never returns. Shot
; test: as ActorDownedByShotLong. Else M and X 8-bit; A low = $8C, N set,
; Z clear, C clear.
%at($84A9C7)
KindA1Touch:
    jsl.l AnimStep
    bit.b !actor_09
    bvs .live
    pla
    pla
    pla
    pla
    pla
    jml.l ActorEndPlaced
.live:
    lda.w !KIND_A1_SAFE
    bne .to8c
    jsl.l ActorBouncesOrHurtsPlayerLong
    dec a
    bne .shot
    inc.b !KIND_A1_HITS
    lda.b #$0F
    jsl.l SoundQueueAdd
    bra .to8c
.shot:
    jsl.l ActorDownedByShotLong
    rtl
.to8c:
    asl.b !actor_09
    lsr.b !actor_09
    lda.b #$8C
    sta.b !actor_state
    rtl

; @routine 84:A9FC KindA1Hit
; @actor
; WHAT IT DOES: Actor kind $A1 in state $4C (hit): it flies off (motion
; $1F), after 8 frames when its $41 is set, until it can no longer be hit,
; and ends.
; HOW: Animation 0, $08 bit 4 set; $41 nonzero: COP 8; ActorMotionPreset
; $1F; each frame ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $A1 in state $4C (through the stub at
; $83:9412). Exit: never returns; ends in ActorEndPlaced.
%at($84A9FC)
KindA1Hit:
    lda.b #$00
    jsl.l AnimStart
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    lda.b !actor_41
    beq .lAA10
    lda.b #$08
    cop.b #$00
.lAA10:
    ldy.b #$1F
    jsl.l ActorMotionPreset
.lAA16:
    brk.b #$00
    jsl.l ActorFly
    bit.b !actor_09
    bvs .lAA16
    jml.l ActorEndPlaced


; @routine 84:AC3F KindA6Stay
; @actor
; WHAT IT DOES: Actor kind $A6: shown (animation 0, flipped by its $07)
; where it was placed until it is 256 or more pixels above or below the
; point actors are kept in range of, then ends.
; HOW: $08 = 3, AnimStart 0, flip = $07; each frame (BRK first) RANGE_Y
; less its y plus $0100, unsigned below $0200: again; else ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $A6 in state $0C (through the stub at
; $83:943E). Exit: never returns; ends in ActorEndPlaced.
%at($84AC3F)
KindA6Stay:
    lda.b #$03
    sta.b !actor_08
    lda.b #$00
    jsl.l AnimStart
    lda.b !actor_07
    sta.b !actor_flip
    rep.b #$20
.lAC4F:
    brk.b #$00
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w #$0100
    cmp.w #$0200
    bcc .lAC4F
    sep.b #$20
    jml.l ActorEndPlaced
