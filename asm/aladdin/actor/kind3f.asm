; kind3f.asm: actor kind $3F, a platform that crumbles a while after
; Aladdin first rides it. Its wait is Kind3FWait ($83:B1E8), which tests
; the ride while K3F_CARRYING is nonzero.
; C: src/aladdin/actor/kind3f.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K3F_CARRYING = $42             ; nonzero: its wait tests the ride; frames left once ridden
!K3F_CARRIES = $43              ; nonzero: Aladdin rides it this frame (its wait's ActorTestCarries)
!K3F_FRAMES = $40               ; frames left of its crumbling
!K3F_TEST_BOX = $4C             ; the ride test's box
!K3F_TEST_BOXES = $D299         ; K3F_TEST_BOX by $07 (bytes, bank DB)

; @routine 83:B19A Kind3FStart
; @actor
; WHAT IT DOES: Actor kind $3F, a platform: it waits until Aladdin rides
; it, then $30 frames on it crumbles (its animation $07 * 2 + 1, sound
; $2C) and ends $38 frames later.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; facing and flip 0;
; K3F_CARRIES 0; K3F_CARRYING = $30; K3F_TEST_BOX from K3F_TEST_BOXES by
; $07; AnimStart $07 * 2. Each frame (BRK) until K3F_CARRIES is set;
; then each frame K3F_CARRYING down to 0 (the wait stops testing);
; AnimStart $07 * 2 + 1; SoundQueueAdd $2C; K3F_FRAMES = $38 frames;
; K3F_CARRIES 0; then JML ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $3F in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83B19A)
Kind3FStart:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !K3F_CARRIES
    lda.b #$30
    sta.b !K3F_CARRYING
    ldx.b !actor_07
    lda.w !K3F_TEST_BOXES,x
    sta.b !K3F_TEST_BOX
    lda.b !actor_07
    asl a
    jsl.l AnimStart
.unridden:
    brk.b #$00
    lda.b !K3F_CARRIES
    beq .unridden
.ridden:
    brk.b #$00
    dec.b !K3F_CARRYING
    bne .ridden
    lda.b !actor_07
    asl a
    inc a
    jsl.l AnimStart
    lda.b #$2C
    jsl.l SoundQueueAdd
    lda.b #$38
    sta.b !K3F_FRAMES
    stz.b !K3F_CARRIES
.crumbling:
    brk.b #$00
    dec.b !K3F_FRAMES
    bne .crumbling
    jml.l ActorEndPlaced
