; kind97.asm: actor kind $97, a dropper kept in place: by its $07, it
; waits for the point actors are kept in range of to come into a window,
; then drops a child (kind $37, from above that point) and waits for it to
; end, or takes a free record and sends a carrier pair, and waits for that.
; No script meets any of it: written from its bytes.
; C: src/aladdin/actor/kind97.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K97_TIMER = $40                ; frames left to wait (the first wait $1E, else 1)
!K97_BUSY = $41                 ; nonzero while its child is out
!K97_TAKEN = $4A                ; the free record ActorFreeListTake gave (Y)
!K97_COUNT = $4B                ; at 4 or more it waits (counted by its pairs)
!K97_CHILD = $4E                ; its child's record (word)
!K97_CREATED = $0020            ; the record ActorCreate made (word)
!K97_RANGE_X = $1A44            ; the x of the point actors are kept in range of (word)
!K97_RANGE_Y = $1A46            ; its y (word)
!K97_DROP_LO = $D8FE            ; (data bank) the window of K97_RANGE_X it drops in: low, high (words)
!K97_DROP_HI = $D900
!K97_PAIR_A_LO = $D902          ; (data bank) the window it sends a pair in, $07 AND $11 nonzero: low, high
!K97_PAIR_A_HI = $D904
!K97_PAIR_B_LO = $D906          ; (data bank) the window for $07 AND $11 zero: low, high
!K97_PAIR_B_HI = $D908
!K97_CHILD_KIND = $37           ; the kind of the child it drops
!K97_KIND = $97                 ; its own kind, back in ACTOR_06 after the drop

; @routine 83:93B5 Kind97Enter
; @actor
; WHAT IT DOES: Where actors of kind $97 go on from the actor loop in
; its start state (the table at $83:8DAB): Kind97Start. No script meets
; it: written from its bytes.
; HOW: JML to Kind97Start ($84:9F4F).
; Entry: D = the actor's record; reached by the actor loop's jump (JMP
; (table,X)) by kind. Exit: never returns: as Kind97Start.
%at($8393B5)
Kind97Enter:
    jml.l Kind97Start

; @routine 84:9F4F Kind97Start
; @actor
; WHAT IT DOES: Actor kind $97: with $07 bit 0 clear, a dropper: after 30
; frames, each time it is idle and the point actors are kept in range of
; lies within its window, it drops a child and waits until the child's
; record is free. With bit 0 set, a sender: each frame its count is below
; 4 and it is idle, once its timer runs out and the point lies within the
; window its $07 picks, it takes a free record, sends a carrier pair and
; waits until its child's record is free. No script meets it: written from
; its bytes.
; HOW: $07 bit 0 clear: K97_TIMER $1E, K97_BUSY 0; BRK until K97_TIMER
; counts out; then each frame (BRK) while K97_BUSY or K97_RANGE_X outside
; K97_DROP_LO..K97_DROP_HI, again; Kind97DropChild (JSR); 16-bit X, BRK
; until K97_CHILD's state is 0; K97_BUSY 0, X 8-bit, to the timer wait.
; Bit 0 set: K97_TIMER 1, K97_BUSY and K97_COUNT 0; each frame (BRK):
; K97_COUNT 4 or more, K97_BUSY set, or K97_TIMER down to nonzero: again;
; K97_TIMER 1; K97_RANGE_X within K97_PAIR_A ($07 AND $11 nonzero) or
; K97_PAIR_B: ActorFreeListTake with A = 2, C set: K97_TAKEN = Y,
; CarrierPairSend (JSR); BRK, 16-bit X, until K97_CHILD's state is 0;
; K97_BUSY 0, X 8-bit; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; Kind97Enter's JML. Exit: never returns.
%at($849F4F)
Kind97Start:
    lda.b !actor_07
    and.b #$01
    bne .sender
    lda.b #$1E
    sta.b !K97_TIMER
    stz.b !K97_BUSY
    bra .timer
.sender:
    lda.b #$01
    sta.b !K97_TIMER
    stz.b !K97_BUSY
    stz.b !K97_COUNT
    bra .send
.timer:
    brk.b #$00
    dec.b !K97_TIMER
    bne .timer
.idle:
    brk.b #$00
    lda.b !K97_BUSY
    bne .idle
    rep.b #$20
    lda.w !K97_RANGE_X
    cmp.w !K97_DROP_LO
    bcc .test
    lda.w !K97_DROP_HI
    cmp.w !K97_RANGE_X
.test:
    sep.b #$20
    bcc .idle
    jsr.w Kind97DropChild
.child:
    rep.b #$10
    brk.b #$00
    ldx.b !K97_CHILD
    lda.w !actor_state,x
    bne .child
    stz.b !K97_BUSY
    sep.b #$10
    bra .timer
.send:
    sep.b #$20
    brk.b #$00
    lda.b !K97_COUNT
    cmp.b #$04
    bcs .send
    lda.b !K97_BUSY
    bne .send
    dec.b !K97_TIMER
    bne .send
    inc.b !K97_TIMER
    lda.b !actor_07
    and.b #$11
    beq .window_b
    rep.b #$20
    lda.w !K97_RANGE_X
    cmp.w !K97_PAIR_A_LO
    bcc .send
    lda.w !K97_PAIR_A_HI
    cmp.w !K97_RANGE_X
    bcc .send
    bra .take
.window_b:
    rep.b #$20
    lda.w !K97_RANGE_X
    cmp.w !K97_PAIR_B_LO
    bcc .send
    lda.w !K97_PAIR_B_HI
    cmp.w !K97_RANGE_X
    bcc .send
.take:
    sep.b #$20
    lda.b #$02
    jsl.l ActorFreeListTake
    bcc .send
    sty.b !K97_TAKEN
    jsr.w CarrierPairSend
.pair:
    brk.b #$00
    rep.b #$10
    ldx.b !K97_CHILD
    lda.w !actor_state,x
    bne .pair
    stz.b !K97_BUSY
    sep.b #$10
    bra .send

; @routine 84:9FFB Kind97WaitWork
; WHAT IT DOES: What actors of kind $97 run each frame they wait, by JSL
; from the kind's stub ($83:93B9): an RTS on either path ($07 bit 0 clear
; or set). An RTS pulls only two of the JSL's three return bytes, so it
; goes on at the word pulled + 1 in this bank ($84:93BD for the stub's
; JSL) with the JSL's bank byte left on the stack. No script meets it:
; written from its bytes.
; HOW: LDA $07, AND #1, BNE to the second RTS; RTS.
; Entry: M 8-bit; D = the actor's record; called by JSL. Exit: at the
; word pulled + 1 in bank $84; A = $07 AND 1, Z from it.
%at($849FFB)
Kind97WaitWork:
    lda.b !actor_07
    and.b #$01
    bne .odd
    rts
.odd:
    rts

; @routine 84:A003 Kind97DropChild
; WHAT IT DOES: Kind $97's drop: a child of kind $37 made as if from 80
; above the point actors are kept in range of, with $07 one more than its
; own, placed and started; while it is out, K97_BUSY counts and
; K97_TIMER is $78. No script meets it: written from its bytes.
; HOW: 16-bit: its x and y pushed; x = K97_RANGE_X, y = K97_RANGE_Y - $50;
; ACTOR_06 = K97_CHILD_KIND; ActorCreate; C clear (made): 16-bit X =
; K97_CREATED, kept in K97_CHILD; the child's $07 = its $07 + 1; spawn
; place $44; ActorSpawnPlace; the child's state $0C; K97_BUSY up,
; K97_TIMER $78. X 8-bit; its x and y pulled back; ACTOR_06 = K97_KIND.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from Kind97Start. Exit: M and X
; 8-bit; A = K97_KIND.
%at($84A003)
Kind97DropChild:
    rep.b #$20
    lda.b !actor_x+1
    pha
    lda.b !actor_y+1
    pha
    lda.w !K97_RANGE_X
    sta.b !actor_x+1
    sec
    lda.w !K97_RANGE_Y
    sbc.w #$0050
    sta.b !actor_y+1
    sep.b #$20
    lda.b #!K97_CHILD_KIND
    sta.b !actor_06
    jsl.l ActorCreate
    bcs .none
    rep.b #$10
    ldx.w !K97_CREATED
    stx.b !K97_CHILD
    lda.b !actor_07
    inc a
    sta.w !actor_07,x
    lda.b #$44
    sta.b !actor_spawn_at
    jsl.l ActorSpawnPlace
    lda.b #$0C
    sta.w !actor_state,x
    inc.b !K97_BUSY
    lda.b #$78
    sta.b !K97_TIMER
.none:
    sep.b #$10
    rep.b #$20
    pla
    sta.b !actor_y+1
    pla
    sta.b !actor_x+1
    sep.b #$20
    lda.b #!K97_KIND
    sta.b !actor_06
    rts
assert pc() == $84A056
