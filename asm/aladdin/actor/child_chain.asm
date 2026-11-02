; child_chain.asm: an actor's chain of children: each new child linked to
; the one before, placed on its parent, numbered; and the nine-piece
; chain built at (712, 192), its head and tail kept. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/actor/child_chain.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!CHAIN_LAST = $40  ; word: the record of the last child created; in a child, its number
!CHAIN_COUNT = $42  ; children created so far
!CHAIN_BEFORE = $4E  ; in a child: the record of the child after it

; The nine-piece chain: its head's and tail's records (words; the head's
; is the actor some bank-$84 code works on).
!CHAIN_HEAD = $1B14
!CHAIN_TAIL = $1B16
!KIND_73 = $73  ; the head
!KIND_75 = $75  ; the three pieces after it
!KIND_74 = $74  ; the five pieces after those

; @routine 84:94EE ChildChainAdd
; WHAT IT DOES: Creates a child of kind A and adds it to the actor's
; chain: the new child's $37 is the last one's record, the last one's $4E
; the new one's; the child is put at the actor's position and numbered
; (its $40 = the count so far); it becomes the last.
; HOW: ActorCreateChild (X = the new record; not tested); $40 nonzero: new
; $37 = $40, $40's $4E = the new record (16-bit). The new x and y pixels =
; the actor's; its $40 = $42; $42 up; $40 = the new record.
; Entry: M 8-bit, X 8-bit; A = the kind; D = the parent's record; DB =
; $84; called by JSR (84:94AE and the eight calls after it). Exit: M
; 8-bit, X 8-bit; X = the new record's low byte.
%at($8494EE)
ChildChainAdd:
    jsl.l ActorCreateChild
    ldy.b !CHAIN_LAST
    beq .l9502
    rep.b #$20
    tya
    sta.w !actor_next,x
    txa
    sta.w !CHAIN_BEFORE,y
    sep.b #$20
.l9502:
    rep.b #$20
    lda.b !actor_x+1
    sta.w !actor_x+1,x
    lda.b !actor_y+1
    sta.w !actor_y+1,x
    sep.b #$20
    lda.b !CHAIN_COUNT
    sta.w !CHAIN_LAST,x
    inc.b !CHAIN_COUNT
    stx.b !CHAIN_LAST
    sep.b #$10
    rts

; @routine 84:948E ChainBuildNine
; WHAT IT DOES: Builds the actor's chain of nine children at x 712, y
; 192: a head of kind $73, three pieces of kind $75, five of kind $74;
; keeps the head's record in CHAIN_HEAD and the tail's in CHAIN_TAIL.
; HOW: The free list sorted (ActorFreeListSort) for as many passes as
; half its top offset; the actor's x and y pixels set; $21, CHAIN_LAST
; and CHAIN_COUNT cleared; ChildChainAdd nine times, CHAIN_LAST copied
; to CHAIN_HEAD after the first and to CHAIN_TAIL after the last (two
; 8-bit copies each).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; called by
; JSR (84:93E3). Exit: as ChildChainAdd; A low byte = the tail's high
; byte, N and Z from it.
%at($84948E)
ChainBuildNine:
    lda.w !actor_free_top
    lsr a
    jsl.l ActorFreeListSort
    rep.b #$20
    lda.w #$02C8
    sta.b !actor_x+1
    lda.w #$00C0
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_spawn_at
    stz.b !CHAIN_LAST
    stz.b !CHAIN_LAST+1
    stz.b !CHAIN_COUNT
    lda.b #!KIND_73
    jsr.w ChildChainAdd
    lda.b !CHAIN_LAST
    sta.w !CHAIN_HEAD
    lda.b !CHAIN_LAST+1
    sta.w !CHAIN_HEAD+1
    lda.b #!KIND_75
    jsr.w ChildChainAdd
    lda.b #!KIND_75
    jsr.w ChildChainAdd
    lda.b #!KIND_75
    jsr.w ChildChainAdd
    lda.b #!KIND_74
    jsr.w ChildChainAdd
    lda.b #!KIND_74
    jsr.w ChildChainAdd
    lda.b #!KIND_74
    jsr.w ChildChainAdd
    lda.b #!KIND_74
    jsr.w ChildChainAdd
    lda.b #!KIND_74
    jsr.w ChildChainAdd
    lda.b !CHAIN_LAST
    sta.w !CHAIN_TAIL
    lda.b !CHAIN_LAST+1
    sta.w !CHAIN_TAIL+1
    rts
