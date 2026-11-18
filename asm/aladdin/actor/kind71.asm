; kind71.asm: actor kind $71, by its $07: one that paces left and right,
; one that steps back and then moves right for good, or one that stays,
; showing animations 1 and 0 in turn.
; C: src/aladdin/actor/kind71.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K71_LEG = $42                  ; frames left of a pace leg
!K71_PACE = $08                 ; $07 bit: it paces
!K71_RUNS = $10                 ; $07 bit: it steps back and moves right

; @routine 83:F370 Kind71Start
; @actor
; WHAT IT DOES: Actor kind $71, by its $07: bit 3, it paces, $90 frames
; one way and $90 the other at speed $80, first to the left; else bit 4,
; it steps back $171 pixels and moves right at speed $0100 for good; else
; it stays, facing by bit 0, in animation 1 for $39 frames and 0 for $38,
; again and again.
; HOW: actor_08 bit 1 and actor_09 bits $90 set. Bit 3: facing and flip
; 1, speed $80, AnimStart 0; each leg K71_LEG $90 frames (BRK first) of
; ActorMoveX, then facing and flip flipped. Bit 4: facing and flip 0; x
; pixels less $0170 with the borrow (REP #$21 clears C); AnimStart 0;
; speed $0100; each frame (BRK first) ActorMoveX. Else: facing and flip =
; bit 0; AnimStart 1, COP $39, AnimStart 0, COP $38, again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $71 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83F370)
Kind71Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    bit.b #!K71_PACE
    bne .pace
    bit.b #!K71_RUNS
    beq .stand
    stz.b !actor_facing
    stz.b !actor_flip
    rep.b #$21
    lda.b !actor_x+1
    sbc.w #$0170
    sta.b !actor_x+1
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
    stz.b !actor_speed_x
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.run:
    brk.b #$00
    jsl.l ActorMoveX
    bra .run
.stand:
    lda.b !actor_07
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.show:
    lda.b #$01
    jsl.l AnimStart
    lda.b #$39
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    lda.b #$38
    cop.b #$00
    bra .show
.pace:
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$80
    sta.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$00
    jsl.l AnimStart
.leg:
    lda.b #$90
    sta.b !K71_LEG
.pace_frame:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !K71_LEG
    bne .pace_frame
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    bra .leg
