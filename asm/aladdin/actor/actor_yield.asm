; actor_yield.asm: where an actor's BRK or COP leads (the vectors' code,
; system/interrupts.asm): the actor waits, its resume place kept in its
; record, and the actor loop goes on (docs/aladdin/ACTORS.md).
; C: src/aladdin/actor/actor_yield.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!AY_PLAYER_LOOP_ON = $08DA      ; 1 while the player's record is run
!AY_WAIT = $01                  ; frames left to wait (the record's $01)
!AY_RESUME_P = $02              ; where it resumes: P, then PC and bank

; @routine 80:88E5 ActorYieldEntry
; @interrupt
; WHAT IT DOES: An actor's BRK: it waits one frame (ActorWaitEntry with A
; = 1).
; HOW: 8-bit registers, A = 1, on into ActorWaitEntry.
; Entry: a BRK's frame on the stack (P, then the PC past the BRK's
; signature byte and its bank); D = the actor's record; reached by the
; BRK vector's JML. Exit: as ActorWaitEntry.
%at($8088E5)
ActorYieldEntry:
    sep.b #$30
    lda.b #$01

; @routine 80:88E9 ActorWaitEntry
; @interrupt
; WHAT IT DOES: An actor waits A frames: its record marked waiting with
; the count, the place it resumes at kept in it, and the actor loop goes
; on with the next record (or ends the player's turn).
; HOW: CLI; AY_WAIT = A; state 1 (waiting); the frame pulled (16-bit): P
; and PC low into AY_RESUME_P, PC high and bank into AY_RESUME_P + 2;
; 8-bit registers; AY_PLAYER_LOOP_ON zero: JML ActorLoopBack, else JML
; PlayerLoopBack.
; Entry: a COP's frame on the stack (or a BRK's, from ActorYieldEntry); D
; = the actor's record; M 8-bit with A low byte = the frames; reached by
; the COP vector's JML. Exit: never returns: in the actor loop with S
; above the frame, M and X 8-bit, A = AY_PLAYER_LOOP_ON.
%at($8088E9)
ActorWaitEntry:
    cli
    sta.b !AY_WAIT
    lda.b #$01
    sta.b !actor_state
    rep.b #$20
    pla
    sta.b !AY_RESUME_P
    pla
    sta.b !AY_RESUME_P+2
    sep.b #$30
    lda.w !AY_PLAYER_LOOP_ON
    bne .player
    jml.l ActorLoopBack
.player:
    jml.l PlayerLoopBack
