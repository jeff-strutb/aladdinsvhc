; child_chain.asm: an actor's chain of children: each new child linked to
; the one before, placed on its parent, numbered. Drafted by cdraft.py;
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
