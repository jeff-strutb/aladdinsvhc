; kinda5.asm: actor kind $A5 (stage a5 s2), by its $07: 0, the bomb kind
; $98 throws after each hit, which falls to the ground, bursts and ends;
; 1 to 3, a column of them set off from one of three places across the
; top, a child a frame; $10 and up, one of the column's pieces, at a
; height by its low bits, which poses and ends. Its wait stub is at
; $83:9435's neighbour (kind_wait.asm).
; C: src/aladdin/actor/kinda5.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_PROBES = $16             ; its probe list's address (word)
!KA5_PROBES = $D984             ; (data bank $80) the bomb's probes
!KA5_PLACES_X = $D988           ; (data bank $80) the column's x by $07 1-3, less one (words)
!KA5_HEIGHTS = $D98E            ; (data bank $80) a piece's y pixels by its $07's low four bits
!KA5_PIECE = $10                ; $07 bit 4: a piece of the column
!KA5_KIND = $A5                 ; the column's children are of this kind
!KA5_LAST = $1C                 ; the last piece's $07 plus one
!KA5_FALL = $93                 ; the bomb's fall: ActorSpeedYSet's entry

; @routine 84:AB9C KindA5Start
; @actor
; WHAT IT DOES: Actor kind $A5, by its $07: 0, a bomb (thrown by kind $98)
; that falls until it is in the ground, then bursts 8 pixels lower and
; ends; 1 to 3, a column set off from x KA5_PLACES_X entry $07 - 1 at y 0,
; one child of this kind a frame ($07 0, then $10 to $1B), then it ends;
; $10 up, a piece of a column put at y pixels KA5_HEIGHTS entry by its
; low four bits, posing (animation $07 AND 3) for $12 frames, then it
; ends.
; HOW: $08 bit 1 and $09 bits $90 set (TSB). $07 0: ACTOR_PROBES =
; KA5_PROBES (16-bit); AnimStart 4; ActorSpeedYSet with Y = KA5_FALL;
; each frame (BRK first) ActorMoveY, ActorOutOfGround until nonzero; y
; pixels' low byte + 8; AnimStart 5; COP 8. $07 bit 4: X = $07 AND $0F,
; Y = that AND 3; y pixels' low byte KA5_HEIGHTS,X; AnimStart with A = Y;
; COP $12. Else (16-bit) x pixels = KA5_PLACES_X entry ($07 AND 3) - 1,
; y pixels 0; ACTOR_SPAWN_AT 0, $07 0, ActorCreateChild8 with A =
; KA5_KIND; $07 $10; each frame (BRK first): ACTOR_SPAWN_AT 0,
; ActorCreateChild8 KA5_KIND, $07 up, until $07 = KA5_LAST. All end at
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $A5 in state $0C (through the stub at
; $83:9435). Exit: never returns.
%at($84AB9C)
KindA5Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    beq .bomb
    and.b #!KA5_PIECE
    beq .column
    lda.b !actor_07
    and.b #$0F
    tax
    and.b #$03
    tay
    lda.w !KA5_HEIGHTS,x
    sta.b !actor_y+1
    tya
    jsl.l AnimStart
    lda.b #$12
    cop.b #$00
    jmp.w .end
.column:
    rep.b #$20
    lda.b !actor_07
    and.w #$0003
    dec
    asl
    tax
    lda.w !KA5_PLACES_X,x
    sta.b !actor_x+1
    stz.b !actor_y+1
    sep.b #$20
    stz.b !actor_spawn_at
    stz.b !actor_07
    lda.b #!KA5_KIND
    jsl.l ActorCreateChild8
    lda.b #!KA5_PIECE
    sta.b !actor_07
.child:
    brk.b #$00
    stz.b !actor_spawn_at
    lda.b #!KA5_KIND
    jsl.l ActorCreateChild8
    inc.b !actor_07
    lda.b !actor_07
    cmp.b #!KA5_LAST
    bne .child
    jmp.w .end
.bomb:
    rep.b #$20
    lda.w #!KA5_PROBES
    sta.b !ACTOR_PROBES
    sep.b #$20
    lda.b #$04
    jsl.l AnimStart
    ldy.b #!KA5_FALL
    jsl.l ActorSpeedYSet
.fall:
    brk.b #$00
    jsl.l ActorMoveY
    jsl.l ActorOutOfGround
    beq .fall
    lda.b !actor_y+1
    clc
    adc.b #$08
    sta.b !actor_y+1
    lda.b #$05
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
.end:
    jml.l ActorEndPlaced
