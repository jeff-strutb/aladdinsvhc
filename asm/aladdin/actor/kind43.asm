; kind43.asm: actor kind $43's start.
; C: src/aladdin/actor/kind43.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 83:B393 Kind43Start
; @actor
; WHAT IT DOES: Actor kind $43: a platform the player can ride. Ways 0-2
; ($07): it carries him for $18 frames once he is on it, then with sound
; $1A shows its next animation, waits $15 frames, spawns 3 (way 0) or 2
; actors of its own kind as way 3, each of a look 2 past the one before,
; marks its placement gone and ends. Way 3: no carrying; it rises when
; Aladdin is in reach, then ends.
; HOW: $08 bit 1 and $09 bits $90 set; $14, $15, CARRIES ($43) 0; its
; look table ($16) $D2BA; JMP ($B3B5,X) by $07. Ways 0-2 ($83:B3BD): $42
; (the carry count, which Kind43Wait's carry test runs on) $18, its box
; ($4C) and animation from $D2BE and $D2C7 by $07; each frame (BRK) until
; CARRIES is set; then $42 frames counted down; SoundQueueAdd $1A;
; AnimStart its animation + 1; $40 = $15 frames, CARRIES 0; $40 and $21
; from $D2C1 by $07 * 2; $07 = 3, $4D = its kind ($06); $40 times
; ActorSpawnLookWait and $21 up by 2; its placement's flag ($7E:C600 by
; $34) $80; JML ActorEndPlaced. Way 3 ($83:B426): $42 0; AnimStart 6;
; each frame (BRK) X = 0 and ActorRiseWhenInReach until C is clear; JML
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's jump for kind $43 in state $0C. Exit: never returns.
%at($83B393)
Kind43Start:
    lda.b $08
    ora.b #$02
    sta.b $08
    lda.b $09
    ora.b #$90
    sta.b $09
    stz.b $14
    stz.b $15
    stz.b $43
    rep.b #$20
    lda.w #$D2BA
    sta.b $16
    sep.b #$20
    lda.b $07
    asl a
    tax
    jmp.w (.ways,x)
.ways:
    dw .carry, .carry, .carry, .rise
.carry:
    lda.b #$18
    sta.b $42
    ldx.b $07
    lda.w $D2BE,x
    sta.b $4C
    ldx.b $07
    lda.w $D2C7,x
    jsl.l AnimStart
.wait_on:
    brk.b #$00
    lda.b $43
    beq .wait_on
.ride:
    brk.b #$00
    dec.b $42
    bne .ride
    lda.b #$1A
    jsl.l SoundQueueAdd
    ldx.b $07
    lda.w $D2C7,x
    inc a
    jsl.l AnimStart
    lda.b #$15
    sta.b $40
    stz.b $43
.shake:
    brk.b #$00
    dec.b $40
    bne .shake
    lda.b $07
    asl a
    tax
    lda.w $D2C1,x
    sta.b $40
    lda.w $D2C2,x
    sta.b $21
    lda.b #$03
    sta.b $07
    lda.b $06
    sta.b $4D
.pieces:
    jsr.w ActorSpawnLookWait
    inc.b $21
    inc.b $21
    dec.b $40
    bne .pieces
    ldx.b $34
    lda.b #$80
    sta.l $7EC600,x
    jml.l ActorEndPlaced
.rise:
    stz.b $42
    lda.b #$06
    jsl.l AnimStart
.rise_frame:
    brk.b #$00
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    bcs .rise_frame
    jml.l ActorEndPlaced
