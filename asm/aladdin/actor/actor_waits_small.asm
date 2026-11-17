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
!KIND28_STOMPS = $45  ; kind $28: nonzero, it is stomped or hurts; zero, it bounces the player
!KIND28_RIGHT = $46   ; kind $28: counted up each frame it faces right
!V_032D_WS = $032D    ; counted each frame (its bit 0: odd or even)
!KIND37_FREE = $45    ; kinds $37/$38: nonzero, it ends as other actors do
!KIND37_HIT = $46     ; 1 bounced, 2 downed by a shot
!KIND37_MAKER = $4C   ; the record that made it (word)
!KIND37_PARTNER = $4E ; the record it is freed with (word)
!KIND37_MAKER_COUNT = $004B ; in its maker's record: its count of these
!ACTOR_27_WS = $27   ; the picture its animation shows, as AnimStep sets it
!KIND2A_BOB = $49     ; kind $2A: nonzero, it bobs with its animation
!KIND2A_FRAME = $4A   ; kind $2A: the frame it last bobbed on
!KIND2A_UP = $4B      ; kind $2A: nonzero after frame 6, frame 5 then moves it down
!KIND2A_RIGHT = $4C   ; kind $2A: counted when downed facing right
!KIND2A_STILL = $47   ; kind $2A: nonzero, no animation, touch or shot test
!KIND2A_HITS = $48    ; kind $2A: counted when downed by a shot
!KIND3B_REST_Y = $4A  ; kind $3B: the y pixels it rises back to (word)
!KIND3B_RISE = $40    ; kind $3B: how far below them it is (low byte)
!KIND3B_LIFT_STEP = $44 ; kind $3B: ActorLiftPlayerStep's step

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

; @routine 83:C569 Kind40Wait
; WHAT IT DOES: What an actor of kind $40 runs each frame it waits: its
; animation, then its end when it is out of range by box $18.
; HOW: AnimStep; X = $18, then into ActorEndOutOfRange (a copy of
; ActorWaitAnimRange18).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83C569)
Kind40Wait:
    jsl.l AnimStep
    ldx.b #$18
    jmp.w ActorEndOutOfRange

; @routine 83:C966 Kind62Wait
; WHAT IT DOES: What an actor of kind $62 runs each frame it waits: the
; test whether it stops Aladdin's shot, then its end when it is out of
; range by box 0.
; HOW: ActorStopsShot; X = 0, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83C966)
Kind62Wait:
    jsr.w ActorStopsShot
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 83:C5D5 Kind7BWait
; WHAT IT DOES: What an actor of kind $7B runs each frame it waits: its
; animation, then its end when it is out of range by box $60.
; HOW: AnimStep; X = $60, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83C5D5)
Kind7BWait:
    jsl.l AnimStep
    ldx.b #$60
    jmp.w ActorEndOutOfRange

; @routine 83:FA45 Kind8AWait
; WHAT IT DOES: What an actor of kind $8A runs each frame it waits: its
; animation, then its end when it is out of range by box 0.
; HOW: AnimStep; X = 0, then into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
; through the actor loop's wait table (83:94D0). Exit: as
; ActorEndOutOfRange: in range it returns; out of range the actor ends.
%at($83FA45)
Kind8AWait:
    jsl.l AnimStep
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 83:A1BD Kind28Wait
; WHAT IT DOES: Kind $28's wait: animates; downed by shots; counts the
; frames it faces right; stomped or hurting as touched, or with its $45
; clear it bounces the player and, when it does (no script has it), it
; is downed itself (state $8C, its per-frame routine off); ended when out
; of range (box 8).
; HOW: AnimStep; ActorDownedByShot; its facing not negative: $46 up. $45
; set: ActorStompOrHurt; else ActorBouncesPlayer, Z clear: bit 7 of $09
; cleared, state $8C. Then ActorEndOutOfRange with X = 8.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorEndOutOfRange.
%at($83A1BD)
Kind28Wait:
    jsl.l AnimStep
    jsr.w ActorDownedByShot
    lda.b !actor_facing
    bmi .stomp
    inc.b !KIND28_RIGHT
.stomp:
    lda.b !KIND28_STOMPS
    beq .bounce
    jsr.w ActorStompOrHurt
    bra .range
.bounce:
    jsr.w ActorBouncesPlayer
    beq .range
    lda.b #$80
    trb.b !actor_09
    lda.b #$8C
    sta.b !actor_state
.range:
    ldx.b #$08
    jmp.w ActorEndOutOfRange

; @routine 83:B11C Kind3CWait
; WHAT IT DOES: Kind $3C's wait: drifts across, turning round on odd
; frames of $032D clear (no script has it); while high on the screen
; (its y pixel low byte below $D4) it bounces the player, with sound $12,
; and then turns its rise into a fall (no script has that); ended once
; left behind.
; HOW: $032D bit 0 clear: its facing flipped. ActorMoveX. Its $0F below
; $D4: ActorBouncesPlayer, Z clear: SoundQueueAdd $12; its speed y high
; byte negative: KIND3C_LEG = $55 less KIND3C_LEG, ActorSpeedYTurn. Then JML
; ActorEndIfLeftBehind.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorEndIfLeftBehind.
%at($83B11C)
Kind3CWait:
    lda.w !V_032D_WS
    bit.b #$01
    bne .move
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
.move:
    jsl.l ActorMoveX
    lda.b !actor_y+1
    cmp.b #$D4
    bcs .tail
    jsr.w ActorBouncesPlayer
    beq .tail
    lda.b #$12
    jsl.l SoundQueueAdd
    lda.b !actor_speed_y+2
    bpl .tail
    sec
    lda.b #$55
    sbc.b !KIND3C_LEG
    sta.b !KIND3C_LEG
    jsr.w ActorSpeedYTurn
.tail:
    jml.l ActorEndIfLeftBehind

; @routine 83:BD22 Kind37Wait
; WHAT IT DOES: The wait of kinds $37 and $38: bounces or hurts the player
; (bounced: it is downed, state $8C, no script has it); downed by shots
; (KIND37_HIT = 2, no script has it); animates; kind $38, or kind $37 with
; KIND37_FREE set, ends out of range (box $10); kind $37 with KIND37_FREE
; clear, once out of range, frees its partner (KIND37_PARTNER) and counts
; its maker's $4B down, then ends (no script has that).
; HOW: The test of $07 bit 0 branches to the next instruction either way.
; ActorBouncesOrHurtsPlayer; A = 1: state bits $8C set, bit 7 of $09
; cleared, KIND37_HIT = 1. ActorDownedByShot, C clear: KIND37_HIT = 2.
; AnimStep. Kind not $38: ActorEndOutOfRange with X = 0. Else X = $10;
; KIND37_FREE set: ActorEndOutOfRange; clear: ActorInRange, in range
; back; out: ActorFreePlacedAtY with Y = KIND37_PARTNER, its maker's
; $4B ($4B,X with X = KIND37_MAKER) down by one; its return dropped and
; into ActorEndPlaced (by the code at $83:96C6).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind
; (also the start entry of kind $37 in the table at $83:8CAB). Exit: as
; ActorEndOutOfRange; in range: as ActorInRange; ended: no return.
%at($83BD22)
Kind37Wait:
    lda.b !actor_07
    and.b #$01
    bne .test
.test:
    jsr.w ActorBouncesOrHurtsPlayer
    dec a
    bne .shot
    lda.b #$8C
    tsb.b !actor_state
    lda.b #$80
    trb.b !actor_09
    lda.b #$01
    sta.b !KIND37_HIT
.shot:
    jsr.w ActorDownedByShot
    bcs .anim
    lda.b #$02
    sta.b !KIND37_HIT
.anim:
    jsl.l AnimStep
    lda.b !actor_06
    cmp.b #$38
    beq .kind38
    ldx.b #$00
    jmp.w ActorEndOutOfRange
.kind38:
    ldx.b #$10
    lda.b !KIND37_FREE
    beq .own
    jmp.w ActorEndOutOfRange
.own:
    jsr.w ActorInRange
    bcs .far
    rts
.far:
    rep.b #$10
    ldy.b !KIND37_PARTNER
    jsr.w ActorFreePlacedAtY
    ldx.b !KIND37_MAKER
    dec.w !KIND37_MAKER_COUNT,x
    sep.b #$10
    jmp.w ActorEndOutOfRange_end

; @routine 83:A413 Kind2AWait
; WHAT IT DOES: Kind $2A's wait: while KIND2A_BOB is set it bobs with its
; animation (3 down on frame 4, 2 up or down on frame 5, 3 up on frame
; 6); unless KIND2A_STILL is set it animates, is stomped or hurts, and
; when downed by a shot (no script has it) counts KIND2A_HITS, and
; KIND2A_RIGHT when it faces right; ended when out of range (box 8).
; HOW: KIND2A_BOB set: X 16-bit = its y pixels; its frame ($27) unlike
; KIND2A_FRAME: kept, then frame 4: y + 3, KIND2A_UP = 0; frame 5: y - 2
; with KIND2A_UP clear, else + 2; frame 6: y - 3, KIND2A_UP up; the y
; stored. KIND2A_STILL clear: AnimStep, ActorStompOrHurt,
; ActorDownedByShot; C clear: facing not negative, KIND2A_RIGHT up;
; KIND2A_HITS up. Then ActorEndOutOfRange with X = 8.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorEndOutOfRange.
%at($83A413)
Kind2AWait:
    lda.b !KIND2A_BOB
    beq .still
    rep.b #$10
    ldx.b !actor_y+1
    lda.b !ACTOR_27_WS
    cmp.b !KIND2A_FRAME
    beq .kept
    sta.b !KIND2A_FRAME
    sec
    sbc.b #$04
    bne .frame5
    inx
    inx
    inx
    stz.b !KIND2A_UP
    bra .store
.frame5:
    dec a
    bne .frame6
    lda.b !KIND2A_UP
    bne .down
    dex
    dex
    bra .store
.down:
    inx
    inx
    bra .store
.frame6:
    dec a
    bne .kept
    dex
    dex
    dex
    inc.b !KIND2A_UP
.store:
    stx.b !actor_y+1
.kept:
    sep.b #$10
.still:
    lda.b !KIND2A_STILL
    bne .range
    jsl.l AnimStep
    jsr.w ActorStompOrHurt
    jsr.w ActorDownedByShot
    bcs .range
    lda.b !actor_facing
    bmi .hit
    inc.b !KIND2A_RIGHT
.hit:
    inc.b !KIND2A_HITS
.range:
    ldx.b #$08
    jmp.w ActorEndOutOfRange

; @routine 83:C21C Kind3BWait
; WHAT IT DOES: Kind $3B's wait, by its $07: bit 4 set, it rises back to
; KIND3B_REST_Y, carrying the player while it lifts him; bit 3 set, it
; animates and, once out of range, sets its level flag; else it lifts
; the player when he lands on it, and variant 2 stays while another
; actor shares its tile slot; ended when out of range (box $18).
; HOW: Bit 4: its y less KIND3B_REST_Y (below it: put at it, 0; no script
; has that) in KIND3B_RISE; LIFT_STEP positive: below $10, speed entry
; $5D, ActorMoveY and PlayerToActorBelow with A = 3; else KIND3B_RISE
; nonzero: speed entry $5A, ActorMoveY. Then AnimStep and ActorFreeOut-
; OfRange with X = $18. Bit 3: AnimStep, ActorFreeOutOfRange with X =
; $18; freed: LevelFlagFromMask with X = $07 & 7. Else: $4C = $1E,
; ActorTestCarries with X = 2, ActorLiftPlayerStep; variant 2 with its
; tile slot's users not 1: back; else ActorFreeOutOfRange with X = $18.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorFreeOutOfRange, LevelFlagFromMask or ActorLiftPlayerStep.
%at($83C21C)
Kind3BWait:
    lda.b !actor_07
    bit.b #$10
    bne .rise
    bit.b #$08
    beq .lift
    jsl.l AnimStep
    ldx.b #$18
    jsr.w ActorFreeOutOfRange
    bcc .back
    lda.b !actor_07
    and.b #$07
    tax
    jsr.w LevelFlagFromMask
.back:
    rts
.lift:
    lda.b #$1E
    sta.b !actor_4c
    ldx.b #$02
    jsr.w ActorTestCarries
    ldx.b #$02
    jsr.w ActorLiftPlayerStep
    lda.b !actor_07
    cmp.b #$02
    bne .range
    ldy.b !actor_tile_slot
    lda.w !actor_tile_slot_users,y
    dec a
    beq .range
    rts
.range:
    ldx.b #$18
    jmp.w ActorFreeOutOfRange
.rise:
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.b !KIND3B_REST_Y
    bcs .above
    lda.b !KIND3B_REST_Y
    sta.b !actor_y+1
    lda.w #$0000
.above:
    sep.b #$20
    sta.b !KIND3B_RISE
    lda.b !KIND3B_LIFT_STEP
    beq .free
    bmi .free
    lda.b !KIND3B_RISE
    cmp.b #$10
    bcs .anim
    ldy.b #$5D
    jsl.l ActorSpeedYSet
    jsl.l ActorMoveY
    lda.b #$03
    jsl.l PlayerToActorBelow
    bra .anim
.free:
    lda.b !KIND3B_RISE
    beq .anim
    ldy.b #$5A
    jsl.l ActorSpeedYSet
    jsl.l ActorMoveY
.anim:
    jsl.l AnimStep
    ldx.b #$18
    jmp.w ActorFreeOutOfRange
