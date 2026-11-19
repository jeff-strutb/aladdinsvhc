; kind66.asm: actor kind $66 (stage a2 s1 and others) by its $07: platforms
; that swing up and down, rise when signalled, or carry Aladdin up and
; down holding him, a gate that waits for its parent to end, or the
; holder of kind $6C. Its wait is Kind66WaitWork (kind_waits_84.asm),
; through the stub at $83:944E; its start stub is at $83:944A.
; C: src/aladdin/actor/kind66.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K66_COUNT = !actor_40          ; frames left of a stroke
!K66_PERIOD = !actor_41         ; a stroke's frames
!K66_TOP = !actor_42            ; its y pixels as it starts (word)
!K66_GO = !actor_44             ; nonzero: signalled to move (set by its parent's Kind66TellOthers)
!K66_SIGNAL = !actor_45         ; what Kind66TellOthers sets in its siblings' K66_GO
!K66_LOW = !actor_46            ; its lowest y pixels: K66_TOP less $38 (word)
!K66_STEP = !actor_48           ; Kind66Wobble's step: 0 up a pixel, 1 still, 2 down a pixel, then still
!K66_PARENT = !actor_49         ; the record of the actor that spawned it (word)
!K66_HELD = $4B                 ; nonzero: no touch or shot tests (Kind66WaitWork)
!K66_PAUSE = !actor_4c          ; frames the carry waits before it takes the player again
!V_141B = $141B
!V_1427 = $1427                 ; nonzero: no landing on actors
!NO_LANDING = $145A             ; nonzero: frames with no hold on a ledge; negative: none
!K66_ZONE = $78                 ; the carry's zone for the player (ActorZoneHasTestPointLong's Y)

; @routine 84:839E Kind66Start
; @actor
; WHAT IT DOES: Actor kind $66, by its $07: bit 4, kind $6C's holder
; (pose 3, its touches off, on as Kind6CStart); bit 3, a gate
; (Kind66Gate); bit 2, a lift that carries Aladdin down while he stands on
; it and back up; bit 1, a platform that rises out of reach while
; signalled and comes back; else a platform swinging up and down for good
; (bit 0: the shorter swing).
; HOW: $08 bits 0-1 and $09 bits $90 set (TSB); K66_HELD 0. Bit 4:
; AnimStart 3, K66_HELD up, JML Kind6CStart. Bit 3: on at Kind66Gate.
; K66_GO 0. Bit 2 (the lift): Kind66Spawn X = $0B at spawn place $5A; $09
; bit 7 off; AnimStart 1; Kind66MarkTop; K66_SIGNAL and K66_PAUSE 0. Each
; frame (BRK first) ActorEndOutOfRangeLong X = 0; K66_PAUSE: down, the
; next frame; V_141B set or NO_LANDING negative: the next frame;
; ActorZoneHasTestPointLong Y = K66_ZONE, C set: the next frame. Taken
; (no script runs it): the player's code word $D9C5 (PlayerSwitchHang),
; Kind66PlayerHeldAt, BRK, Kind66PlayerOnTop, COP 4; each frame (BRK
; first) while V_1427 is set: its y pixels 2 lower (16-bit), past
; K66_TOP + $10 sound $15, K66_SIGNAL up, Kind66TellOthers, BRK until
; V_1427 clears; else Kind66PlayerOnTop. Then K66_SIGNAL 0,
; Kind66TellOthers; each frame (BRK first) up a pixel until at K66_TOP;
; K66_PAUSE $10; again. Else AnimStart 0, K66_STEP 0. Bit 1 (the rise):
; Kind66Spawn X = $0A at $58; ActorSpeedYSet Y = $48; Kind66MarkTop; each
; frame (BRK) until K66_GO; then each frame (BRK first) while K66_GO and
; its y at or below K66_LOW: Kind66Wobble, ActorStepYBack; K66_GO 0: COP
; $38, then each frame (BRK first) Kind66Wobble, ActorStepY until at
; K66_TOP (K66_GO set again: COP $18, back up). Else (the swing, no script
; runs it): Kind66Spawn X = $0A at $56; Y $48, X $80 (bit 0 shifted out
; set: Y $4B, X $40) into K66_COUNT and K66_PERIOD; ActorSpeedYSet; then
; for good K66_COUNT frames (BRK first) Kind66Wobble and ActorStepYBack,
; COP $0C, K66_PERIOD frames of Kind66Wobble and ActorStepY, COP $0C.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop for kind $66 in state $0C (the stub at $83:944A). Exit:
; never returns.
%at($84839E)
Kind66Start:
    lda.b #$03
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    stz.b !K66_HELD
    lda.b !actor_07
    bit.b #$10
    beq .l83BA
    lda.b #$03
    jsl.l AnimStart
    inc.b !K66_HELD
    jml.l Kind6CStart
.l83BA:
    and.b #$08
    beq .l83C1
    jmp.w Kind66Gate
.l83C1:
    stz.b !K66_GO
    lda.b !actor_07
    and.b #$04
    beq .l83CC
    jmp.w .l847D
.l83CC:
    lda.b #$00
    jsl.l AnimStart
    stz.b !K66_STEP
    lda.b !actor_07
    and.b #$02
    bne .l8423
    lda.b #$56
    sta.b !actor_spawn_at
    ldx.b #$0A
    jsr.w Kind66Spawn
    ldy.b #$48
    ldx.b #$80
    lsr.b !actor_07
    bcc .l83EF
    ldy.b #$4B
    ldx.b #$40
.l83EF:
    stx.b !K66_COUNT
    stx.b !K66_PERIOD
    jsl.l ActorSpeedYSet
.l83F7:
    brk.b #$00
    jsr.w Kind66Wobble
    jsl.l ActorStepYBack
    dec.b !K66_COUNT
    bne .l83F7
    lda.b #$0C
    cop.b #$00
    lda.b !K66_PERIOD
    sta.b !K66_COUNT
.l840C:
    brk.b #$00
    jsr.w Kind66Wobble
    jsl.l ActorStepY
    dec.b !K66_COUNT
    bne .l840C
    lda.b #$0C
    cop.b #$00
    lda.b !K66_PERIOD
    sta.b !K66_COUNT
    bra .l83F7
.l8423:
    lda.b #$58
    sta.b !actor_spawn_at
    ldx.b #$0A
    jsr.w Kind66Spawn
    ldy.b #$48
    jsl.l ActorSpeedYSet
    jsr.w Kind66MarkTop
.l8435:
    brk.b #$00
    lda.b !K66_GO
    beq .l8435
.l843B:
    brk.b #$00
    lda.b !K66_GO
    beq .l845A
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K66_LOW
    sep.b #$20
    bcc .l8454
    jsr.w Kind66Wobble
    jsl.l ActorStepYBack
    bra .l843B
.l8454:
    brk.b #$00
    lda.b !K66_GO
    bne .l8454
.l845A:
    lda.b #$38
    cop.b #$00
.l845E:
    brk.b #$00
    lda.b !K66_GO
    bne .l8477
    jsr.w Kind66Wobble
    jsl.l ActorStepY
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K66_TOP
    sep.b #$20
    bcc .l845E
    bra .l8435
.l8477:
    lda.b #$18
    cop.b #$00
    bra .l843B
.l847D:
    lda.b #$5A
    sta.b !actor_spawn_at
    ldx.b #$0B
    jsr.w Kind66Spawn
    lda.b #$80
    trb.b !actor_09
    lda.b #$01
    jsl.l AnimStart
    jsr.w Kind66MarkTop
    stz.b !K66_SIGNAL
    stz.b !K66_PAUSE
.l8497:
    brk.b #$00
    ldx.b #$00
    jsl.l ActorEndOutOfRangeLong
    lda.b !K66_PAUSE
    beq .l84A7
    dec.b !K66_PAUSE
    bra .l8497
.l84A7:
    lda.w !V_141B
    bne .l8497
    lda.w !NO_LANDING
    bmi .l8497
    ldy.b #!K66_ZONE
    jsl.l ActorZoneHasTestPointLong
    bcs .l8497
    lda.b #$C5
    sta.w !player+!actor_03
    lda.b #$D9
    sta.w !player+!actor_04
    jsr.w Kind66PlayerHeldAt
    brk.b #$00
    jsr.w Kind66PlayerOnTop
    lda.b #$04
    cop.b #$00
.l84CF:
    brk.b #$00
    lda.w !V_1427
    beq .l84FE
    rep.b #$21
    inc.b !actor_y+1
    inc.b !actor_y+1
    lda.b !K66_TOP
    adc.w #$0010
    cmp.b !actor_y+1
    sep.b #$20
    php
    jsr.w Kind66PlayerOnTop
    plp
    bcs .l84CF
    lda.b #$15
    jsl.l SoundQueueAdd
    inc.b !K66_SIGNAL
    jsr.w Kind66TellOthers
.l84F7:
    brk.b #$00
    lda.w !V_1427
    bne .l84F7
.l84FE:
    stz.b !K66_SIGNAL
    jsr.w Kind66TellOthers
.l8503:
    brk.b #$00
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K66_TOP
    beq .l8513
    dec.b !actor_y+1
    sep.b #$20
    bra .l8503
.l8513:
    sep.b #$20
    lda.b #$10
    sta.b !K66_PAUSE
    jmp.w .l8497

; @routine 84:851C Kind66PlayerOnTop
; WHAT IT DOES: Puts Aladdin on the lift: his x its x, his y its y + $32.
; HOW: 16-bit (REP #$21, carry clear): PLAYER_X = its x pixels, PLAYER_Y =
; its y pixels + $32.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR from Kind66Start's lift. Exit: M 8-bit; A low
; byte = the y's low byte; C, N, Z from the addition.
%at($84851C)
Kind66PlayerOnTop:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    adc.w #$0032
    sta.w !player+!actor_y+1
    sep.b #$20
    rts

; @routine 84:852E Kind66PlayerHeldAt
; WHAT IT DOES: Keeps the place Aladdin hangs from the lift at: its x, its
; y + $32.
; HOW: 16-bit (REP #$21, carry clear): PLAYER_POS_SAVED = its x pixels, +2
; = its y pixels + $32.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR from Kind66Start's lift. Exit: as
; Kind66PlayerOnTop.
%at($84852E)
Kind66PlayerHeldAt:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player_pos_saved
    lda.b !actor_y+1
    adc.w #$0032
    sta.w !player_pos_saved+2
    sep.b #$20
    rts

; @routine 84:8540 Kind66TellOthers
; WHAT IT DOES: Tells every live actor of its kind its K66_SIGNAL (their
; K66_GO), so the rising platforms move with the lift.
; HOW: X 16-bit from $0B5B by $50 while below $141C (the pool's
; records): state nonzero and kind = its kind: their K66_GO = K66_SIGNAL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from Kind66Start's lift. Exit: M
; and X 8-bit; X = $1C (the end's low byte); C set.
%at($848540)
Kind66TellOthers:
    rep.b #$10
    ldx.w #$0B5B
.l8545:
    lda.w !actor_state,x
    beq .l8556
    lda.w !actor_06,x
    cmp.b !actor_06
    bne .l8556
    lda.b !K66_SIGNAL
    sta.w !K66_GO,x
.l8556:
    rep.b #$21
    txa
    adc.w #$0050
    tax
    cpx.w #$141C
    sep.b #$20
    bcc .l8545
    sep.b #$10
    rts

; @routine 84:8567 Kind66MarkTop
; WHAT IT DOES: Keeps its height as it starts (K66_TOP) and its lowest
; ($38 above, K66_LOW).
; HOW: 16-bit (REP #$21): K66_TOP = its y pixels; less $37 with the carry
; clear (so $38) to K66_LOW.
; Entry: M 8-bit; D = the actor's record; called by JSR from Kind66Start.
; Exit: M 8-bit; A = K66_LOW (16 bits); flags from the subtraction.
%at($848567)
Kind66MarkTop:
    rep.b #$21
    lda.b !actor_y+1
    sta.b !K66_TOP
    sbc.w #$0037
    sta.b !K66_LOW
    sep.b #$20
    rts

; @routine 84:8575 Kind66Wobble
; WHAT IT DOES: A platform's wobble as it moves: K66_STEP counts 1, 2, 3:
; at 1 nothing, at 2 its y a pixel up, from 3 on still for good ($FF);
; the count wrapping to 0 moves it a pixel down.
; HOW: X 16-bit = its y pixels; K66_STEP up: 0, X up; 1, nothing; 2, X
; down; else K66_STEP $FF; X stored as its y pixels where it moved. No
; script runs it.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
; Kind66Start's swing and rise. Exit: M and X 8-bit; A = K66_STEP less
; what was taken off it; flags from that.
%at($848575)
Kind66Wobble:
    rep.b #$10
    ldx.b !actor_y+1
    inc.b !K66_STEP
    lda.b !K66_STEP
    beq .l858B
    dec
    beq .l8591
    dec
    beq .l858E
    lda.b #$FF
    sta.b !K66_STEP
    bra .l8591
.l858B:
    inx
    bra .l858F
.l858E:
    dex
.l858F:
    stx.b !actor_y+1
.l8591:
    sep.b #$10
    rts

; @routine 84:8594 Kind66Gate
; @actor
; WHAT IT DOES: Kind $66 with $07 bit 3: a piece that stays (pose 2)
; until the actor that spawned it has ended, then ends.
; HOW: $08 = 2; $09 bit 7 off (TRB); AnimStart 2; each frame (BRK first)
; until the state of the record at K66_PARENT (X 16-bit) is 0; JML
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the jump from Kind66Start. Exit: never returns.
%at($848594)
Kind66Gate:
    lda.b #$02
    sta.b !actor_08
    lda.b #$80
    trb.b !actor_09
    lda.b #$02
    jsl.l AnimStart
.l85A2:
    brk.b #$00
    rep.b #$10
    ldx.b !K66_PARENT
    lda.w !actor_state,x
    sep.b #$10
    bne .l85A2
    jml.l ActorEndPlaced

; @routine 84:85B3 Kind66Spawn
; WHAT IT DOES: Spawns a child of its kind with $07 = X at its spawn
; place, with its look and itself as the child's parent, and counts the
; kind's live actors up.
; HOW: Its $07 pushed, $07 = X; ActorSpawn with A = its kind ($06); N set
; (none): skip. Else (16-bit) the child's look = its look, the child's
; K66_PARENT = D; 8-bit: KIND_COUNT,X (X = its kind) up. $07 pulled back.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = the child's $07;
; ACTOR_SPAWN_AT set; DB = $80; called by JSR from Kind66Start. Exit: M
; and X 8-bit when spawned (else as ActorSpawn left them); A = its $07.
%at($8485B3)
Kind66Spawn:
    lda.b !actor_07
    pha
    stx.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .l85D8
    rep.b #$20
    lda.b !actor_look
    sta.w !actor_look,x
    tdc
    sta.w !K66_PARENT,x
    sep.b #$30
    ldx.b !actor_06
    lda.l !kind_count,x
    inc
    sta.l !kind_count,x
.l85D8:
    pla
    sta.b !actor_07
    rts
