; children_start.asm: an actor that starts two children at its spawn points, the free list put
; in order first. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/children_start.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Not yet filed: starts a child (hit box $3D) at spawn point X with $07 =
; Y, kept in the actor's list of children ($40 + 2 * $44).
!CHILD_START = $83C4EC

; @routine 83:C33A ActorStartTwoChildren
; WHAT IT DOES: Starts the actor's two children (at spawn points $1C and
; $1E, their $07 5 and 4), the free list's top two records first put in
; address order (the lower on top).
; HOW: The top entry of the free list compared with the one under it
; (16-bit); not below, the two swapped. Then the child start at 83:C4EC
; twice, X = $1C, Y = 5 and X = $1E, Y = 4.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
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
    jsr.w !CHILD_START
    ldx.b #$1E
    ldy.b #$04
    jsr.w !CHILD_START
    rts
