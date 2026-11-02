; children_start.asm: an actor that starts two children at its spawn points, the free list put
; in order first. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/children_start.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CHILD_COUNT = $14              ; children it has started (83:D078)
!v_035a = $035A
!v_036c = $036C

; @routine 83:C33A ActorStartTwoChildren
; WHAT IT DOES: Starts the actor's two children (at spawn points $1C and
; $1E, their $07 5 and 4), the free list's top two records first put in
; address order (the lower on top).
; HOW: The top entry of the free list compared with the one under it
; (16-bit); not below, the two swapped. Then the child start at 83:C4EC
; twice, X = $1C, Y = 5 and X = $1E, Y = 4.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; through the actor's table at 83:C324. Exit: M 8-bit, X 8-bit; as the
; child start left it.
%at($83C33A)
ActorStartTwoChildren:
    rep.b #$20
    ldx.w !actor_free_top
    lda.w !actor_free_list,x
    cmp.w !actor_free_list-2,x
    bcc .lC352
    pha
    lda.w !actor_free_list-2,x
    sta.w !actor_free_list,x
    pla
    sta.w !actor_free_list-2,x
.lC352:
    sep.b #$20
    ldx.b #$1C
    ldy.b #$05
    jsr.w ChildStartListed
    ldx.b #$1E
    ldy.b #$04
    jsr.w ChildStartListed
    rts

; @routine 83:D078 ActorVariantChildStep
; WHAT IT DOES: For an actor with a variant in bits 2-4 of $07: makes it
; variant 8 plus that, and starts a child (kind $30 at spawn point $3E),
; counting $14 up for it; no variant: sets its bit ($41) in $035A and
; counts $036C up.
; HOW: Variant: $07 = bits 2-4 shifted down OR 8; $14 up; spawn point $3E;
; ActorCreateChild8 with kind $30; result zero: its bit set in $035A and
; $14 back down. None: its bit set in $035A, $036C up.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; (83:D071). Exit: M 8-bit; as ActorCreateChild8, or after the count.
%at($83D078)
ActorVariantChildStep:
    lda.b !actor_07
    and.b #$1C
    beq .lD09A
    lsr a
    lsr a
    ora.b #$08
    sta.b !actor_07
    inc.b !CHILD_COUNT
    lda.b #$3E
    sta.b !actor_spawn_at
    lda.b #$30
    jsl.l ActorCreateChild8
    bne .lD099
    lda.b !actor_41
    tsb.w !v_035a
    dec.b !CHILD_COUNT
.lD099:
    rts
.lD09A:
    lda.b !actor_41
    tsb.w !v_035a
    inc.w !v_036c
    rts
