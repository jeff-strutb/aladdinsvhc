; chain_followers.asm: the codes the actors of a chain (kinds $73 and
; $75's lists) are started on by ActorListStart: each waits the frames its
; start gave it in $44 (the leader's step apart), then starts its byte
; stream (ActorStreamStart) or slides its $42 to a target, and yields for
; good (ActorYieldForever). Actor code (BRK yields): the loop's RTI
; enters each at the address ActorListStart put in its record.
; C: src/aladdin/actor/chain_followers.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!FOLLOW_WAIT = !actor_44        ; frames left before it goes on
!FOLLOW_INDEX = !actor_40       ; its place in the chain
!FOLLOW_AT = !actor_42          ; the value it slides (word)
!FOLLOW_ROW = $16               ; the slide's row: 0, 2, 4 or 6 (word)
!FOLLOW_DONE = !actor_18        ; counted up once the slide has arrived
!FOLLOW_SIDE = $000E            ; 0: the target is above FOLLOW_AT, 2: below
; Tables in the data bank ($80)
!FOLLOW_TARGETS = $D6D3         ; by row (words): the offset of the row's targets; then by place (words)
!FOLLOW_STEP_ROWS = $D6BA       ; by row * 2, less one: the row's steps (words): toward a target above, below
!FOLLOW_WAITS = $D6CB           ; by place: the frames ChainFollowLate waits

; @routine 84:89A0 ChainFollowStream0
; @actor
; WHAT IT DOES: A chain's actor that waits its turn, then starts byte
; stream 0 and yields for good.
; HOW: Each frame (BRK first) FOLLOW_WAIT down, until 0; ActorStreamStart
; with X = 0; on at ActorYieldForever.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where ActorListStart (84:8802) set its code. Exit: never returns.
%at($8489A0)
ChainFollowStream0:
    brk.b #$00
    dec.b !FOLLOW_WAIT
    bne ChainFollowStream0
    ldx.b #$00
    jsr.w ActorStreamStart
    bra ActorYieldForever

; @routine 84:89AD ChainFollowStream2
; @actor
; WHAT IT DOES: A chain's actor that waits its turn, then starts byte
; stream 2 and yields for good.
; HOW: Each frame (BRK first) FOLLOW_WAIT down, until 0; ActorStreamStart
; with X = 2; on at ActorYieldForever.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where ActorListStartAD set its code, and from ChainFollowStream14.
; Exit: never returns.
%at($8489AD)
ChainFollowStream2:
    brk.b #$00
    dec.b !FOLLOW_WAIT
    bne ChainFollowStream2
    ldx.b #$02
    jsr.w ActorStreamStart
    jmp.w ActorYieldForever

; @routine 84:89BB ChainFollowStream14
; @actor
; WHAT IT DOES: A chain's actor that waits its turn and then starts byte
; stream 2 as ChainFollowStream2, unless its wait was one frame: then
; byte stream $14.
; HOW: BRK; FOLLOW_WAIT down: not 0, on in ChainFollowStream2's wait.
; Else (its start gives $10 steps: no script, nor any start, reaches it)
; ActorStreamStart with X = $14; on at ActorYieldForever.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where its leader (84:8B18) set its code. Exit: never returns.
%at($8489BB)
ChainFollowStream14:
    brk.b #$00
    dec.b !FOLLOW_WAIT
    bne ChainFollowStream2
    ldx.b #$14
    jsr.w ActorStreamStart
    jmp.w ActorYieldForever

; @routine 84:89C9 ChainSlideRow6
; @actor
; WHAT IT DOES: A chain's actor that waits its turn, then slides its value
; to its place's target in row 6, marks it done and yields for good.
; HOW: A = 6, on in ChainSlideRow0's body.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where its leader (84:8C41) set its code. Exit: never returns.
%at($8489C9)
ChainSlideRow6:
    lda.b #$06
    bra ChainSlideRow0_row

; @routine 84:89CD ChainSlideRow4
; @actor
; WHAT IT DOES: As ChainSlideRow6, in row 4.
; HOW: A = 4, on in ChainSlideRow0's body.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where its leader (84:8B08) set its code. Exit: never returns.
%at($8489CD)
ChainSlideRow4:
    lda.b #$04
    bra ChainSlideRow0_row

; @routine 84:89D1 ChainSlideRow2
; @actor
; WHAT IT DOES: As ChainSlideRow6, in row 2.
; HOW: A = 2, on in ChainSlideRow0's body.
; Entry: D = the actor's record; DB = $80; reached by the actor loop's
; RTI where its leader (84:8942, 84:8C2C) set its code. Exit: never
; returns.
%at($8489D1)
ChainSlideRow2:
    lda.b #$02
    bra ChainSlideRow0_row

; @routine 84:89D5 ChainSlideRow0
; @actor
; WHAT IT DOES: A chain's actor that waits its turn, then each frame moves
; its value FOLLOW_AT toward its place's target in its row (all at once
; when nearer than the row's step, else by the step), and once there marks
; itself done and yields for good.
; HOW: A = 0 (the others' A): FOLLOW_ROW = A, its high byte 0. FOLLOW_WAIT
; nonzero: each frame (BRK first) down, until 0. Each frame (BRK first),
; 16-bit: X = FOLLOW_ROW, Y = it * 2; X = FOLLOW_TARGETS,X + FOLLOW_INDEX's
; low byte * 2; FOLLOW_SIDE 0; the target FOLLOW_TARGETS,X less FOLLOW_AT
; (pushed); negative: FOLLOW_SIDE 2 and the difference negated. Less than
; the row's step (FOLLOW_STEP_ROWS+1,Y): the difference (pulled); else the
; step at FOLLOW_STEP_ROWS + Y + FOLLOW_SIDE + 1 (the carry). Added to
; FOLLOW_AT; until it is the target. FOLLOW_DONE up; on at
; ActorYieldForever.
; Entry: D = the actor's record; DB = $80; M 8-bit, X 8-bit; reached by
; the actor loop's RTI where its leader (84:88DF) set its code. Exit:
; never returns.
%at($8489D5)
ChainSlideRow0:
    lda.b #$00
.row:
    sta.b !FOLLOW_ROW
    stz.b !FOLLOW_ROW+1
    lda.b !FOLLOW_WAIT
    beq .frame
.wait:
    brk.b #$00
    dec.b !FOLLOW_WAIT
    bne .wait
.frame:
    brk.b #$00
    rep.b #$30
    lda.b !FOLLOW_ROW
    tax
    asl
    tay
    lda.b !FOLLOW_INDEX
    and.w #$00FF
    asl
    clc
    adc.w !FOLLOW_TARGETS,x
    tax
    stz.w !FOLLOW_SIDE
    sec
    lda.w !FOLLOW_TARGETS,x
    sbc.b !FOLLOW_AT
    pha
    bpl .near_test
    inc.w !FOLLOW_SIDE
    inc.w !FOLLOW_SIDE
    eor.w #$FFFF
    inc
.near_test:
    cmp.w !FOLLOW_STEP_ROWS+1,y
    pla
    bcc .add
    tya
    adc.w !FOLLOW_SIDE
    tay
    lda.w !FOLLOW_STEP_ROWS,y
.add:
    clc
    adc.b !FOLLOW_AT
    sta.b !FOLLOW_AT
    cmp.w !FOLLOW_TARGETS,x
    sep.b #$30
    bne .frame
    inc.b !FOLLOW_DONE
    jmp.w ActorYieldForever

; @routine 84:8A2E ChainFollowLate
; @actor
; WHAT IT DOES: A chain's actor that waits the frames its place gives,
; then starts the byte stream its place picks and yields for good.
; HOW: FOLLOW_WAIT = FOLLOW_WAITS,X (X = FOLLOW_INDEX); each frame (BRK
; first) down, until 0; ActorStreamStart with X = (FOLLOW_INDEX + 2) * 2;
; on at ActorYieldForever.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's RTI where its leader (84:8909, 84:8C67) set its code.
; Exit: never returns.
%at($848A2E)
ChainFollowLate:
    ldx.b !FOLLOW_INDEX
    lda.w !FOLLOW_WAITS,x
    sta.b !FOLLOW_WAIT
.wait:
    brk.b #$00
    dec.b !FOLLOW_WAIT
    bne .wait
    clc
    lda.b !FOLLOW_INDEX
    adc.b #$02
    asl
    tax
    jsr.w ActorStreamStart
    jmp.w ActorYieldForever
