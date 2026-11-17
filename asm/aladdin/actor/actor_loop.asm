; player's record and then every other record ($50 bytes apart from
; $092B) is looked at: a free one is skipped; a waiting one counts down
; and runs its kind's wait routine (bit 7 of $09); one whose wait ran out
; goes back into its actor by RTI at the resume address in its $02-$05;
; one to start goes to its kind's code by JMP through the kind tables.
; The actor comes back by BRK or COP (the handler at $80:88E5 JMLs to
; PlayerLoopBack or ActorLoopBack) or by ending (a jump to ActorLoopNext's
; re-entry ActorLoopBack). The C ends where the loop hands control to an
; actor (actor_hand_off), as the original's record does.
; C: src/aladdin/actor/actor_loop.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!AL_PLAYER_LOOP_ON = $08DA       ; 1 while the player's record is run
!AL_RECORDS_LEFT = $032F         ; records still to look at this frame
!AL_RECORD_NUMBER = $0330        ; the record being looked at, from 0
!AL_RECORDS_END = $0B5B          ; past the last record
!AL_PAUSE_BLOCK = $0404          ; nonzero: only the first records run
!AL_RESUME_P = $02               ; where it resumes: P, then PC and bank
!AL_WAITS = $838F4F              ; wait routines by kind, kinds below $80
!AL_WAITS_80 = $83904F           ; and from $80
!AL_STARTS = $838CAB             ; start entries by kind: state $0C-$7F
!AL_STARTS_80 = $838DAB
!AL_SECONDS = $838DFD            ; state bit 7 ($80-$BF)
!AL_SECONDS_80 = $838EFD
!AL_THIRDS = $8390A1             ; state bits 7 and 6 ($C0 up)
!AL_THIRDS_80 = $8391A1

; @routine 83:9475 PlayerLoop
; @actorloop
; WHAT IT DOES: The player's turn of the actor loop: its record run (to
; its actor's code) when its wait runs out or it starts, else its wait
; routine.
; HOW: D kept; PLAYER_LOOP_ON counted up; D = the player's record. State
; 0: done. State 1: frames left ($01) counted down; at 0, run_record;
; else on at PlayerLoopBack. Any other state: run_record. Done:
; PLAYER_LOOP_ON 0, D back, RTL.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSL. Exit: by RTL, or none: handed to the player's code.
%at($839475)
PlayerLoop:
    phd
    inc.w !AL_PLAYER_LOOP_ON
    lda.b #!player>>8
    xba
    lda.b #!player
    tcd
    lda.b !actor_state
    beq PlayerLoopBack_done
    cmp.b #$01
    bne ActorLoopNext_run
    dec.b !actor_01
    beq ActorLoopNext_run

; @routine 83:948B PlayerLoopBack
; @actorloop
; @pushed 2
; WHAT IT DOES: The player's turn of the actor loop after its actor came
; back (the BRK handler's JML) or while it waits: its wait routine, then
; the turn ends.
; HOW: ACTOR_09 bit 7: its kind's wait routine (JSR through WAITS or
; WAITS_80, the latter run by no script); PLAYER_LOOP_ON 0; D back; RTL.
; Entry: M 8-bit, X 8-bit; D = the player's record; the loop's D pushed
; above PlayerLoop's return. Exit: by RTL to PlayerLoop's caller.
PlayerLoopBack:
    lda.b !actor_09
    bpl .done
    lda.b !actor_06
    bpl .low
    asl a                           ; (no script runs this)
    tax
    jsr.w (!AL_WAITS_80,x)
    bra .done
.low:
    asl a
    tax
    jsr.w (!AL_WAITS,x)
.done:
    stz.w !AL_PLAYER_LOOP_ON
    pld
    rtl

; @routine 83:94A4 ActorLoop
; @actorloop
; WHAT IT DOES: The other actors' turns: each of the $23 records from
; $092B looked at in turn and run (handed to its actor's code) when due.
; HOW: D kept; RECORDS_LEFT $23, RECORD_NUMBER 0; D = $092B; then as
; ActorLoopNext from its record test: state 0 skipped; state 1 counted
; down, at 0 run_record, else the wait routine; other states run_record.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSL. Exit: by RTL, or none: handed to an actor's code.
ActorLoop:
    phd
    lda.b #$23
    sta.w !AL_RECORDS_LEFT
    stz.w !AL_RECORD_NUMBER
    lda.b #$09
    xba
    lda.b #$2B
    tcd
.record:
    lda.b !actor_state
    beq ActorLoopNext
    cmp.b #$01
    bne ActorLoopNext_run
    dec.b !actor_01
    beq ActorLoopNext_run

; @routine 83:94BF ActorLoopBack
; @actorloop
; @pushed 2
; WHAT IT DOES: The actor loop after an actor came back (the BRK handler's
; JML, or the actor ending) or while it waits: the record's wait routine,
; then on to the next record.
; HOW: ACTOR_09 bit 7: its kind's wait routine; then as ActorLoopNext.
; Entry: M 8-bit, X 8-bit; D = the record; the loop's D pushed above
; ActorLoop's return. Exit: by RTL to ActorLoop's caller, or none: handed
; to an actor's code.
ActorLoopBack:
    lda.b !actor_09
    bpl ActorLoopNext
    lda.b !actor_06
    bpl .low
    asl a
    tax
    jsr.w (!AL_WAITS_80,x)
    bra ActorLoopNext
.low:
    asl a
    tax
    jsr.w (!AL_WAITS,x)

; @routine 83:94D3 ActorLoopNext
; @actorloop
; @pushed 2
; WHAT IT DOES: The actor loop's step to the next record, and the record
; run (run_record, from 83:94F1) its tests share.
; HOW: D + $50; at RECORDS_END with PAUSE_BLOCK set, done; RECORD_NUMBER
; up, RECORDS_LEFT down, at 0 done; else the next record's test. Done: D
; back, RTL. Run: state 8; below $0C, $04-$05 and $02-$03 pushed and RTI
; into the actor; else JMP through STARTS, SECONDS or THIRDS (or their
; _80 halves) by the state's bits 7-6 and the kind (THIRDS_80 run by no
; script).
; Entry: M 8-bit, X 8-bit; D = the record; the loop's D pushed above
; ActorLoop's return. Exit: as ActorLoopBack.
ActorLoopNext:
    rep.b #$20
    clc
    tdc
    adc.w #$0050
    tcd
    cmp.w #!AL_RECORDS_END
    sep.b #$20
    bcc .count
    lda.w !AL_PAUSE_BLOCK
    bne .done
.count:
    inc.w !AL_RECORD_NUMBER
    dec.w !AL_RECORDS_LEFT
    bne ActorLoop_record
.done:
    pld
    rtl
.run:
    ldx.b #$08
    stx.b !actor_state
    cmp.b #$0C
    bcs .start
    rep.b #$20                      ; back into its actor
    lda.b !AL_RESUME_P+2
    pha
    lda.b !AL_RESUME_P
    pha
    rti
.start:
    asl a
    bcs .second
    lda.b !actor_06
    bmi .start_80
    asl a
    tax
    jmp.w (!AL_STARTS,x)
.start_80:
    asl a
    tax
    jmp.w (!AL_STARTS_80,x)
.second:
    asl a
    bcs .third
    lda.b !actor_06
    bmi .second_80
    asl a
    tax
    jmp.w (!AL_SECONDS,x)
.second_80:
    asl a
    tax
    jmp.w (!AL_SECONDS_80,x)
.third:
    lda.b !actor_06
    bmi .third_80
    asl a
    tax
    jmp.w (!AL_THIRDS,x)
.third_80:
    asl a                           ; (no script runs this)
    tax
    jmp.w (!AL_THIRDS_80,x)
