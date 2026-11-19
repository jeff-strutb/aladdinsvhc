; kind9e.asm: actor kind $9E, the figures of the Genie's screen (level
; $2F) that take their animation from the screen's state.
; C: src/aladdin/actor/kind9e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!G_AC = $00AC                   ; the Genie's screen: a pick (negative: a pose, by G_AF when $FF; 1, 2-3, 4 up: animation 3, 4, 5)
!G_AD = $00AD                   ; nonzero: a pick's animation is 1 instead
!G_AF = $00AF                   ; with G_AC $FF: zero, animation 7; else 9
!G_B0 = $00B0                   ; the y (word) way 2 follows, less $10
!G_B5 = $00B5                   ; the screen's end: 1, animation 8; other nonzero, $0A (way 0); way 2's cue

; @routine 84:A820 Kind9EStart
; @actor
; WHAT IT DOES: Actor kind $9E of the Genie's screen (level $2F), by its
; $07. Way 0: animation 0, then each frame by the screen's state: G_B5
; nonzero ends the wait with animation 8 (1) or $0A; else a pick in G_AC
; (cleared once taken) starts animation 3, 4 or 5 (1 if G_AD is set), or,
; $FF, 7 or 9 by G_AF (other negative: 0). Way 2: ActorPaletteFromRom $52, $10
; down by one; once G_B5 is nonzero, animation $0B ($0C when negative);
; then each frame its y = G_B0 + $10 and its x $34.
; HOW: ACTOR_08 |= 3, ACTOR_09 |= $90; JMP ($A82D,X) by $07: $A831, $A88F.
; Way 0 as above with a BRK each frame it waits. Way 2: Y = $52,
; ActorPaletteFromRom, DEC $10; BRK until G_B5; X = $0B, INX when
; negative; AnimStart X; then each frame (BRK first) 16-bit (REP #$21):
; G_B0 + $10 to $0F, $0034 to $0C.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $9E in state $0C (the table at $83:8DAB,
; through the stub at $83:93F7). Exit: never returns.
%at($84A820)
Kind9EStart:
    lda.b #$03
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    ldx.b !actor_07
    jmp.w (.ways,x)
.ways:
    dw .watch, .follow
.watch:
    lda.b #$00
.anim:
    jsl.l AnimStart
.check:
    lda.w !G_B5
    bmi .end_other
    bne .end_one
.wait:
    brk.b #$00
    lda.w !G_AC
    beq .check
    bmi .pose
    cmp.b #$02
    bcs .two_up
    lda.b #$03
    bra .pick
.two_up:
    cmp.b #$04
    bcs .four_up
    lda.b #$04
    bra .pick
.four_up:
    lda.b #$05
    bra .pick
.end_one:
    dec a
    bne .end_other
    lda.b #$08
    jsl.l AnimStart
    bra .wait
.end_other:
    lda.b #$0A
    jsl.l AnimStart
    bra .wait
.pose:
    ldy.b #$00
    inc a
    bmi .posed
    ldy.b #$07
    lda.w !G_AF
    beq .posed
    ldy.b #$09
.posed:
    tya
    jsl.l AnimStart
    stz.w !G_AC
    bra .check
.pick:
    ldy.w !G_AD
    beq .anim
    lda.b #$01
    bra .anim
.follow:
    ldy.b #$52
    jsl.l ActorPaletteFromRom
    dec.b $10
.cued:
    brk.b #$00
    ldx.b #$0B
    lda.w !G_B5
    beq .cued
    bpl .start
    inx
.start:
    txa
    jsl.l AnimStart
.place:
    brk.b #$00
    rep.b #$21
    lda.w !G_B0
    adc.w #$0010
    sta.b !actor_y+1
    lda.w #$0034
    sta.b !actor_x+1
    sep.b #$20
    bra .place
