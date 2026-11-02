; actor_waits_small.asm: small actor routines: two routines actors run each
; frame they wait (hit tests, then their end out of range), kind $82's
; start (shown, and stays), a turn of the vertical speed, a place on
; the escort actor, kind $69's wait and its end riding the escort, kind $3C's
; start (it drifts up and down), two more wait routines, and a flight
; frame that stops rising. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/actor_waits_small.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!WAIT_HITS = $42  ; counted up each frame a shot does not down it

; Kind $69's end: what it sets in the escort's record, and its count.
!ESCORT_CODE = !actor_escort+!actor_03   ; where the escort's code goes on (word)
!ESCORT_ON_CODE = $E554                  ; the escort's code once kind $69 is done
!KIND69_FRAMES = $40                     ; frames left of each of its two poses
!KIND3C_16 = $16                         ; word: $D282
!KIND3C_LEG = $40                        ; frames left before its vertical speed turns
!KIND69_RIDING = $42                     ; nonzero: kind $69 rides on the escort
!V_00BD = $00BD                          ; counted up as each such end finishes; a camera trigger task clears it

; @routine 83:AA47 ActorWaitHitsRange8
; WHAT IT DOES: What an actor (entry $2D of the actor loop's wait table at
; $83:8F4F) runs each frame it waits: its animation and picture, the
; stomp-or-hurt and shot tests, then its end when it is out of range by
; box 8.
; HOW: AnimStep, ActorPictureSend, ActorStompOrHurt, ActorHitByShotCC; X =
; 8, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83AA47)
ActorWaitHitsRange8:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    jsr.w ActorStompOrHurt
    jsr.w ActorHitByShotCC
    ldx.b #$08
    jmp.w ActorEndOutOfRange


; @routine 83:B630 ActorWaitHitsRange0
; WHAT IT DOES: What an actor (entry $2F of the actor loop's wait table at
; $83:8F4F) runs each frame it waits: the stomp-or-hurt and shot tests
; ($42 counted up when no shot downs it), its animation, then its end when
; it is out of range by box 0.
; HOW: ActorStompOrHurt; ActorDownedByShot, C clear: $42 up; AnimStep; X =
; 0, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0), or from 83:1FE0. Exit: as
; ActorEndOutOfRange.
%at($83B630)
ActorWaitHitsRange0:
    jsr.w ActorStompOrHurt
    jsr.w ActorDownedByShot
    bcs .lB63A
    inc.b !WAIT_HITS
.lB63A:
    jsl.l AnimStep
    ldx.b #$00
    jmp.w ActorEndOutOfRange


; @routine 84:951D Kind82Show
; @actor
; WHAT IT DOES: Actor kind $82: shown with animation 0 (bits $B0 of $09,
; bit 1 of $08 set) and stays.
; HOW: $08 bit 1 set, $09 bits $B0 set, AnimStart 0, BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $82 in state $0C (through the stub at
; $83:92CF). Exit: never returns.
%at($84951D)
Kind82Show:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$B0
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.l952F:
    brk.b #$00
    bra .l952F


; @routine 83:B109 ActorSpeedYTurn
; WHAT IT DOES: Turns the actor's vertical speed round: the 24-bit speed
; negated (its high byte inverted, the low word negated).
; HOW: The speed's top byte EOR $FF; the low word EOR $FFFF plus 1
; (16-bit); M 8-bit.
; Entry: M 8-bit; D = the actor's record; called by JSR (83:B0FA). Exit: M
; 8-bit; A = the new low word; N and Z from it.
%at($83B109)
ActorSpeedYTurn:
    lda.b !actor_speed_y+2
    eor.b #$FF
    sta.b !actor_speed_y+2
    rep.b #$20
    lda.b !actor_speed_y
    eor.w #$FFFF
    inc a
    sta.b !actor_speed_y
    sep.b #$20
    rts


; @routine 83:EB4B ActorOnEscort
; WHAT IT DOES: Puts the actor on the escort actor: its x the escort's,
; its y 4 above the escort's.
; HOW: 16-bit copies of the escort's x and of its y less 4; M 8-bit.
; Entry: M 8-bit; D = the actor's record; called by JSR (83:EB00, 83:EB17,
; 83:EB2C). Exit: M 8-bit; A = the new y pixels.
%at($83EB4B)
ActorOnEscort:
    rep.b #$20
    lda.w !actor_escort+!actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !actor_escort+!actor_y+1
    adc.w #$FFFC
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine 83:EADC Kind69EscortEnd
; @actor
; WHAT IT DOES: Actor kind $69's code in state $8C: it sets the escort
; going on at $E554 of its bank, rides on the escort through two
; animations of 32 frames each, counts itself in $00BD and ends.
; HOW: The escort's $08 bit 3 set, its $07 = 6, its code word $E554;
; animation 2, then each frame (BRK first) AnimStep and ActorOnEscort,
; 32 times; animation 3, the same 32 times; V_00BD up; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $69 in state $8C (the table at
; $83:8DFD). Exit: never returns.
%at($83EADC)
Kind69EscortEnd:
    lda.b #$08
    tsb.w !actor_escort+!actor_08
    lda.b #$06
    sta.w !actor_escort+!actor_07
    rep.b #$20
    lda.w #!ESCORT_ON_CODE
    sta.w !ESCORT_CODE
    sep.b #$20
    lda.b #$02
    jsl.l AnimStart
    lda.b #$20
    sta.b !KIND69_FRAMES
.first:
    brk.b #$00
    jsl.l AnimStep
    jsr.w ActorOnEscort
    dec.b !KIND69_FRAMES
    bne .first
    lda.b #$03
    jsl.l AnimStart
    lda.b #$20
    sta.b !KIND69_FRAMES
.second:
    brk.b #$00
    jsl.l AnimStep
    jsr.w ActorOnEscort
    dec.b !KIND69_FRAMES
    bne .second
    inc.w !V_00BD
    jmp.w ActorEndPlaced

; @routine 83:B0CB Kind3CDrift
; @actor
; WHAT IT DOES: Actor kind $3C: shown facing right, given the speeds of
; entries $15 (horizontal) and $42 (vertical), it drifts up and down: its
; vertical speed turns every $55 frames.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; KIND3C_16 = $D282;
; facing and flip 0; animation 0; ActorSpeedXSet $15, ActorSpeedYSet $42;
; KIND3C_LEG $55 and ActorSpeedYTurn, then a yield. What it does after
; that (each frame ActorMoveY, the count down, at 0 again from $55 and a
; turn) is at $83:B0FF, which no script reaches: those bytes are left in
; the intake.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $3C in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83B0CB)
Kind3CDrift:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D282
    sta.b !KIND3C_16
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
    ldy.b #$15
    jsl.l ActorSpeedXSet
    ldy.b #$42
    jsl.l ActorSpeedYSet
    lda.b #$55
    sta.b !KIND3C_LEG
    jsr.w ActorSpeedYTurn
    brk.b #$00

; @routine 83:EB24 Kind69Wait
; WHAT IT DOES: What actor kind $69 runs each frame it waits: its
; animation; while it rides the escort it stays on it, can stop the
; player's shots, and when Aladdin lands on it, it goes into state $8C
; (its end, Kind69EscortEnd) with sound $12.
; HOW: AnimStep; KIND69_RIDING zero: nothing more. Else ActorOnEscort,
; the escort's $08 bit 3 cleared, ActorStopsShot, PlayerLandsOnActor;
; landed (Z clear): bit 7 of its $09 cleared, its state $8C,
; SoundQueueAdd $12.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: M 8-bit; not
; riding: as AnimStep; else as PlayerLandsOnActor or SoundQueueAdd.
%at($83EB24)
Kind69Wait:
    jsl.l AnimStep
    lda.b !KIND69_RIDING
    beq .done
    jsr.w ActorOnEscort
    lda.b #$08
    trb.w !actor_escort+!actor_08
    jsr.w ActorStopsShot
    jsr.w PlayerLandsOnActor
    beq .done
    lda.b #$80
    trb.b !actor_09
    lda.b #$8C
    sta.b !actor_state
    lda.b #$12
    jsl.l SoundQueueAdd
.done:
    rts

; @routine 83:ABD8 ActorWaitEndLowKinds
; WHAT IT DOES: What an actor runs each frame it waits: its animation; an
; actor whose $07 has a low nibble below 2 ends when it is out of range by
; box 0.
; HOW: AnimStep; ACTOR_07 AND $0F against 2: 2 or more, RTS; below, X = 0
; and on into ActorEndOutOfRange (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as AnimStep with A
; low = the nibble and C set; or as ActorEndOutOfRange.
%at($83ABD8)
ActorWaitEndLowKinds:
    jsl.l AnimStep
    lda.b !actor_07
    and.b #$0F
    cmp.b #$02
    bcc .ending
    rts
.ending:
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 84:95D4 ActorWaitGrabsPlayer
; WHAT IT DOES: The wait work the actor loop's stub at $83:92DC calls each
; frame: the actor's animation, and when the player, free to be caught,
; meets its box, he goes into state $8C facing it.
; HOW: AnimStep; ActorHitsTestBoxIfPlayerFreeLong: met (C clear),
; PlayerEnter8CFacingLong.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL. Exit: as
; the test (C set) or as PlayerEnter8CFacingLong.
%at($8495D4)
ActorWaitGrabsPlayer:
    jsl.l AnimStep
    jsl.l ActorHitsTestBoxIfPlayerFreeLong
    bcs .done
    jsl.l PlayerEnter8CFacingLong
.done:
    rtl

; @routine 84:A7D4 ActorFlyAgainstNoRise
; WHAT IT DOES: A frame of flight with the vertical pull reversed, which
; stops the actor's vertical motion once its vertical speed has gone
; negative (upward).
; HOW: ActorFlyAgainst; with the speed's top byte negative, the
; acceleration ($20) and the 24-bit vertical speed cleared.
; Entry: M 8-bit; D = the actor; called by JSR (84:A630, A6B0, A6D9,
; A74E). Exit: M 8-bit; as ActorFlyAgainst, or A low = the speed's top
; byte, N set.
%at($84A7D4)
ActorFlyAgainstNoRise:
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .done
    stz.b !actor_accel_y
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
.done:
    rts
