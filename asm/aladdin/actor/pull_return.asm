; pull_return.asm: two helpers of the actors that ride Aladdin along, which keep their caller's return address (PLA, in GOTO_BACK) while
; they wait and go back to it by ActorBackToCaller; nothing in the ROM calls either. cov_pull_return calls each from a program of three
; instructions poked into work RAM that an actor record's resume address starts.
; C: src/aladdin/actor/pull_return.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!GOTO_BACK = $4E                ; the caller's return address, kept while it waits (word)
!CARRY_FRAMES = !actor_40       ; frames left of a stretch
!CARRY_REST = !actor_41         ; frames of the rest after the moving stretch; the bob's half length
!BOB_COUNT = !actor_47          ; frames of the bob left
!BOB_STEP = !actor_45           ; its y step a frame (word)

; @routine 83:FBB5 ActorCarryMoveWait
; @actor
; WHAT IT DOES: A helper that nothing calls: for $40 frames the actor
; moves by its vertical speed carrying Aladdin if it carries him, then
; waits $41 frames, then goes back to its caller.
; HOW: The return address pulled into GOTO_BACK (16-bit); each frame (BRK
; first) ActorMoveY and ActorCarryPlayer, CARRY_FRAMES down to 0; then
; CARRY_FRAMES = CARRY_REST and a BRK a frame until it is 0; on at
; ActorBackToCaller.
; Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit:
; as ActorBackToCaller.
%at($83FBB5)
ActorCarryMoveWait:
    rep.b #$20
    pla
    sta.b !GOTO_BACK
    sep.b #$20
.move:
    brk.b #$00
    jsl.l ActorMoveY
    jsr.w ActorCarryPlayer
    dec.b !CARRY_FRAMES
    bne .move
    lda.b !CARRY_REST
    sta.b !CARRY_FRAMES
.rest:
    brk.b #$00
    dec.b !CARRY_FRAMES
    bne .rest
    jmp.w ActorBackToCaller

; @routine 83:FC23 ActorCarryBob
; @actor
; WHAT IT DOES: A helper that nothing calls: the actor bobs in y by X
; pixels a frame, A frames in all, turning the step round every A / 2
; frames, carrying Aladdin if it carries him, then goes back to its
; caller.
; HOW: BOB_COUNT = A, CARRY_REST = A / 2; the return address pulled into
; GOTO_BACK; BOB_STEP = X (16-bit). Each stretch CARRY_FRAMES = CARRY_REST;
; each frame ActorCarryPlayer, BRK, BOB_COUNT down (at 0: on at
; ActorBackToCaller), y pixels (16-bit) plus BOB_STEP, CARRY_FRAMES down;
; at the end of a stretch BOB_STEP negated.
; Entry: M 8-bit; A = the frames; X 8-bit = the step; D = the actor's
; record; DB = $80; called by JSR. Exit: as ActorBackToCaller.
%at($83FC23)
ActorCarryBob:
    sta.b !BOB_COUNT
    lsr a
    sta.b !CARRY_REST
    rep.b #$20
    pla
    sta.b !GOTO_BACK
    txa
    sta.b !BOB_STEP
.stretch:
    sep.b #$20
    lda.b !CARRY_REST
    sta.b !CARRY_FRAMES
.frame:
    jsr.w ActorCarryPlayer
    brk.b #$00
    dec.b !BOB_COUNT
    beq .back
    rep.b #$20
    clc
    lda.b !actor_y+1
    adc.b !BOB_STEP
    sta.b !actor_y+1
    sep.b #$20
    dec.b !CARRY_FRAMES
    bne .frame
    rep.b #$20
    lda.b !BOB_STEP
    eor.w #$FFFF
    inc a
    sta.b !BOB_STEP
    bra .stretch
.back:
    jmp.w ActorBackToCaller
