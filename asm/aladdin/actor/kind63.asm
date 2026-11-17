; kind63.asm: actor kind $63, which watches for Aladdin to come within
; reach below it and throws at him (two children of hit box $A2 each
; round), and its per-frame work while it waits.
; C: src/aladdin/actor/kind63.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND63_HIT = $43               ; nonzero while it shows being hit: no shot test then
!KIND63_GONE = $47              ; counted up each frame it is out of range
!KIND63_OUT = $42               ; its throws still flying (ActorSpawnA2 counts them)
!KIND63_OUT_HIT = $46           ; KIND63_OUT as a shot found it (ActorShotHitFace)
!KIND63_AIM = $44               ; how far below it he is: 2 near, 1 farther, 0 farthest
!KIND63_HEALTH = $11
!KIND63_16 = $16                ; word: $D4E5
!KIND63_28 = $28                ; $FF
!KIND63_29 = $29                ; word: $891D
!SIDE = $002C                   ; its x compared with his, rotated in: bit 0 set when he is left of it (scratch, data bank)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; the player's y, whole pixels (word)
!REACH_Y = $D4ED                ; by its $07 (times 2): added to its y before his is taken off (word)
!AIM_NEAR = $D4F7               ; by its $07 (times 2): below this, aim 2
!AIM_MID = $D4F8                ; then below this, aim 1; else 0

; @routine 84:81FC Kind63Work
; WHAT IT DOES: What actors of kind $63 do each frame they wait: unless
; already hit, a shot from the second pool turns it to face Aladdin and
; hits it; Aladdin is hurt by it or stomps it; it animates and shows its
; own picture; and out of range by box $28 it ends, its $47 counted up.
; HOW: KIND63_HIT zero: ActorShotHitFace. Then ActorStompOrHurtLong,
; AnimStep, ActorPictureSend, ActorFreeOutOfRangeLong with X = $28; C
; set (out of range): KIND63_GONE up by one.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
; stub ($83:924D) through the actor loop's wait table at $83:8F4F. Exit:
; M and X 8-bit; C as ActorFreeOutOfRangeLong left it; in range the
; registers it left, else N and Z from the count.
%at($8481FC)
Kind63Work:
    lda.b !KIND63_HIT
    bne .tested
    jsr.w ActorShotHitFace
.tested:
    jsl.l ActorStompOrHurtLong
    jsl.l AnimStep
    jsl.l ActorPictureSend
    ldx.b #$28
    jsl.l ActorFreeOutOfRangeLong
    bcc .done
    inc.b !KIND63_GONE
.done:
    rtl

; @routine 84:8080 Kind63Start
; @actor
; WHAT IT DOES: Actor kind $63's start: shown facing right with one hit
; of health and nothing thrown yet, then on to watch for Aladdin.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; KIND63_28 = $FF,
; KIND63_16 = $D4E5, KIND63_29 = $891D; KIND63_OUT, KIND63_HIT and
; KIND63_GONE 0; health 1; facing and flip 0; on into Kind63Watch.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $63 in state $0C (the table at $83:8CAB, through the
; stub at $83:9249). Exit: never returns.
%at($848080)
Kind63Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b !KIND63_28
    rep.b #$20
    lda.w #$D4E5
    sta.b !KIND63_16
    lda.w #$891D
    sta.b !KIND63_29
    sep.b #$20
    stz.b !KIND63_OUT
    stz.b !KIND63_HIT
    stz.b !KIND63_GONE
    lda.b #$01
    sta.b !KIND63_HEALTH
    stz.b !actor_facing
    stz.b !actor_flip

; @routine 84:80AA Kind63Watch
; @actor
; WHAT IT DOES: Kind $63 watches (animation 0) until Aladdin is near and
; within reach below it, then turns toward him, picks how far to throw,
; and throws once (animation 4, a child of hit box $A2).
; HOW: AnimStart 0, COP $10; each frame (BRK first) ActorNearPlayer with
; Y = $30 until C clear; then its x against his, rotated into SIDE; its
; y plus REACH_Y by its $07, less his: a borrow, watch on. KIND63_AIM = 2,
; 1 or 0 by that against AIM_NEAR and AIM_MID; facing and flip = bit 0
; of SIDE. COP 8, AnimStart 4, COP $10, ActorSpawnA2 with A = 1, COP 8;
; on into Kind63ThrowAgain.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind63Start, Kind63WaitThrows or Kind63ShotHit. Exit: never returns.
%at($8480AA)
Kind63Watch:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
.watch:
    brk.b #$00
    ldy.b #$30
    jsl.l ActorNearPlayer
    bcs .watch
    lda.b !actor_07
    asl a
    tax
    rep.b #$20
    lda.b !actor_x+1
    cmp.w !player_x
    rol a
    sta.w !SIDE
    clc
    lda.b !actor_y+1
    adc.w !REACH_Y,x
    sec
    sbc.w !player_y
    sep.b #$20
    bcc .watch
    ldy.b #$02
    cmp.w !AIM_NEAR,x
    bcc .aimed
    dey
    cmp.w !AIM_MID,x
    bcc .aimed
    dey
.aimed:
    sty.b !KIND63_AIM
    lda.w !SIDE
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$08
    cop.b #$00
    lda.b #$04
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    lda.b #$01
    jsr.w ActorSpawnA2
    lda.b #$08
    cop.b #$00

; @routine 84:810B Kind63ThrowAgain
; @actor
; WHAT IT DOES: Kind $63's second throw: a pause (animation 1), then
; animation 5 and a child of hit box $A2.
; HOW: AnimStart 1, COP $28, AnimStart 5, COP $10, ActorSpawnA2 with A =
; 0, COP 8; on into Kind63WaitThrows.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind63Watch or Kind63ShotHit. Exit: never returns.
%at($84810B)
Kind63ThrowAgain:
    lda.b #$01
    jsl.l AnimStart
    lda.b #$28
    cop.b #$00
    lda.b #$05
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    lda.b #$00
    jsr.w ActorSpawnA2
    lda.b #$08
    cop.b #$00

; @routine 84:8128 Kind63WaitThrows
; @actor
; WHAT IT DOES: Kind $63 waits for its throws to be gone (animation 2
; while two fly, 1 while one does), then watches again.
; HOW: AnimStart 2; each frame (BRK first) until KIND63_OUT is below 2;
; AnimStart 1; each frame until it is 0; then Kind63Watch.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind63ThrowAgain or Kind63ShotHit. Exit: never returns.
%at($848128)
Kind63WaitThrows:
    lda.b #$02
    jsl.l AnimStart
.two:
    brk.b #$00
    lda.b !KIND63_OUT
    cmp.b #$02
    bcs .two
    lda.b #$01
    jsl.l AnimStart
.one:
    brk.b #$00
    lda.b !KIND63_OUT
    bne .one
    jmp.w Kind63Watch

; @routine 84:81CD Kind63ShotHit
; @actor
; WHAT IT DOES: Kind $63 hit by a shot (state $CC): with nothing thrown
; when it was hit, it throws a pair (animation 6) and watches again; else
; it shows the hit (animation 3) and goes on waiting for its throws, or
; throws again when one is left.
; HOW: KIND63_OUT_HIT zero: AnimStart 6, ActorSpawnPairA3, COP $18,
; Kind63Watch. Else KIND63_HIT up, AnimStart 3, COP $18, KIND63_HIT 0, a
; yield; KIND63_OUT less 1 zero: Kind63ThrowAgain, else Kind63WaitThrows.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $63 in state $CC (the table at $83:90A1, through
; the stub at $83:9256). Exit: never returns.
%at($8481CD)
Kind63ShotHit:
    lda.b !KIND63_OUT_HIT
    bne .shown
    lda.b #$06
    jsl.l AnimStart
    jsr.w ActorSpawnPairA3
    lda.b #$18
    cop.b #$00
    jmp.w Kind63Watch
.shown:
    inc.b !KIND63_HIT
    lda.b #$03
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    stz.b !KIND63_HIT
    brk.b #$00
    lda.b !KIND63_OUT
    dec a
    bne .waiting
    jmp.w Kind63ThrowAgain
.waiting:
    jmp.w Kind63WaitThrows

; @routine 84:81BE Kind63Knocked
; @actor
; WHAT IT DOES: Actor kind $63 in its second state (hit): KIND63_GONE
; counted up, its sprite priority cleared, and it is knocked away with
; animation 8, sound $0F and motion preset 3.
; HOW: KIND63_GONE up; ActorPriorityClear; A = 8, X = $0F, Y = 3; on into
; ActorKnockedAway (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $63 with state bit 7 set (the table at $83:8DFD,
; through the stub at $83:9252). Exit: as ActorKnockedAway.
%at($8481BE)
Kind63Knocked:
    inc.b !KIND63_GONE
    jsr.w ActorPriorityClear
    lda.b #$08
    ldx.b #$0F
    ldy.b #$03
    jml.l ActorKnockedAway

