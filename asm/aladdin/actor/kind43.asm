; kind43.asm: actor kind $43's start.
; C: src/aladdin/actor/kind43.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Its record's fields (actor.inc has the shared ones).
!K43_LOOK_TABLE = $16           ; its look table (word)
!K43_COUNT = $40                ; frames left of its shake, then pieces left to spawn
!K43_CARRY_COUNT = $42          ; the carry count, which Kind43Wait's carry test runs on
!K43_CARRIES = $43              ; nonzero: Aladdin rides it (its wait's carry test sets it)
!K43_TEST_BOX = $4C             ; the carry test's box

; Tables (the same names as the C's).
!K43_BOXES = $D2BE              ; the carry test's box by $07 (bytes, DB)
!K43_PIECES = $D2C1             ; by $07: pieces, then the first look (byte pairs, DB)
!K43_ANIMS = $D2C7              ; its animation by $07 (bytes, DB)
!K43_PLACEMENT_FLAGS = $7EC600  ; by placement: $80 once it is gone


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
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !K43_CARRIES
    rep.b #$20
    lda.w #$D2BA
    sta.b !K43_LOOK_TABLE
    sep.b #$20
    lda.b !actor_07
    asl a
    tax
    jmp.w (.ways,x)
.ways:
    dw .carry, .carry, .carry, .rise
.carry:
    lda.b #$18
    sta.b !K43_CARRY_COUNT
    ldx.b !actor_07
    lda.w !K43_BOXES,x
    sta.b !K43_TEST_BOX
    ldx.b !actor_07
    lda.w !K43_ANIMS,x
    jsl.l AnimStart
.wait_on:
    brk.b #$00
    lda.b !K43_CARRIES
    beq .wait_on
.ride:
    brk.b #$00
    dec.b !K43_CARRY_COUNT
    bne .ride
    lda.b #$1A
    jsl.l SoundQueueAdd
    ldx.b !actor_07
    lda.w !K43_ANIMS,x
    inc a
    jsl.l AnimStart
    lda.b #$15
    sta.b !K43_COUNT
    stz.b !K43_CARRIES
.shake:
    brk.b #$00
    dec.b !K43_COUNT
    bne .shake
    lda.b !actor_07
    asl a
    tax
    lda.w !K43_PIECES,x
    sta.b !K43_COUNT
    lda.w !K43_PIECES+1,x
    sta.b !actor_spawn_at
    lda.b #$03
    sta.b !actor_07
    lda.b !actor_06
    sta.b !actor_4d
.pieces:
    jsr.w ActorSpawnLookWait
    inc.b !actor_spawn_at
    inc.b !actor_spawn_at
    dec.b !K43_COUNT
    bne .pieces
    ldx.b !actor_34
    lda.b #$80
    sta.l !K43_PLACEMENT_FLAGS,x
    jml.l ActorEndPlaced
.rise:
    stz.b !K43_CARRY_COUNT
    lda.b #$06
    jsl.l AnimStart
.rise_frame:
    brk.b #$00
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    bcs .rise_frame
    jml.l ActorEndPlaced
