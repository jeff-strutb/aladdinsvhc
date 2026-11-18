; kind86.asm: actor kind $86 (stage a5 s0), a pair that drops and rises
; again: the placed one ($07 0) and its child ($07 4) kept $2B above it.
; Here its wait (the hold, the hurting zone, the child kept above, the end
; out of view) and its second state's end; its start, the cycle (hold,
; drop, bounce, rise) and the carrying of the player standing on it.
; C: src/aladdin/actor/kind86.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K86_HOLD_AT = $13              ; ActorHoldPlayer's offset for the player
!K86_LEFT = $4A                 ; the drop's frames left (its start's count)
!K86_PAIR = $4B                 ; the placed one: its child's record (word)
!K86_ABOVE = $002B              ; how far above it the child is kept (pixels)
!K86_ZONE = $5C                 ; its zone that hurts the player (ActorZoneHurtsPlayer's Y)
!K86_REACH_X = $00E0            ; the box round the range point it stays in: offset, span across
!K86_SPAN_X = $01C0
!K86_REACH_Y = $0100            ; and up and down
!K86_SPAN_Y = $0200
!RANGE_X = $1A44                ; the point actors are kept in range of (words)
!RANGE_Y = $1A46
!K86_COUNT = $40                ; frames left of each step of its cycle
!K86_HOME_Y = $43               ; its y pixels as it started: the rise ends above it (word)
!K86_LAST_Y = $47               ; its y pixels as this frame began (word)
!K86_STOOD = $49                ; nonzero: the player stands on it this frame
!K86_STAND_BOX = $24            ; its box for ActorSolidForPlayer
!K86_4F = $4F                   ; cleared as it starts; purpose not yet established

; @routine 83:9303 ActorKind86Wait
; WHAT IT DOES: The routine actors of kind $86 run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind86WaitWork.
; HOW: JSL Kind86WaitWork ($84:9766), then RTS; its C makes the JSL's
; return frame (jsl_frame).
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind86WaitWork leaves them.
%at($839303)
ActorKind86Wait:
    jsl.l Kind86WaitWork
    rts
%ends_at($839308)

; @routine 84:962D Kind86Start
; @actor
; WHAT IT DOES: Kind $86's start: the placed one ($07 bit 2 clear) makes
; its child ($07 bit 2 set, kept $2B above it), then forever: a hold of
; 16 frames, a drop ($20 frames, $14 with $07 bits 0-1 set) with sound
; $39 as it ends, a short bounce (a pixel up and back), $40 frames still,
; then the rise (sound $38) until it is back above where it started.
; The player standing on it is carried with it. The child only animates.
; HOW: Look $0600, $08 = 2, $09 = $90, $12 = 8. Child ($07 bit 2):
; AnimStart $0F, then a yield each frame for ever. Placed: $4F and $21
; cleared; ActorSpawnPlace, ActorSpawn (N clear: the child's kind and $07
; with bit 2 set, its speed cleared, its look; K86_PAIR = it); a yield;
; AnimStart $0E; K86_HOME_Y = its y. The cycle: each frame a yield,
; Kind86HeightKeep and Kind86StandCheck, then: 16 frames; ActorMotionReset
; $28 and K86_LEFT frames of ActorFall and Kind86Carry; SoundQueueAdd $39;
; 2 frames, a frame with y up one, 2 frames, a frame with y back, $40
; frames (each with Kind86Carry); SoundQueueAdd $38, ActorSpeedYSet $7B,
; and ActorStepYBack with Kind86Carry each frame while its y is at or
; below K86_HOME_Y.
; Entry: D = the actor's record; M 8-bit, X 8-bit; DB = $80; reached by
; JML from kind $86's start entry ($83:92FF). Exit: never returns.
; (The ten bytes at $84:968E-9697, after the BRA at $84:968C, are reached
; by nothing: no branch or jump in the routine targets them, and no JML,
; long pointer or table in the ROM names them.)

%at($84962D)
Kind86Start:
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    lda.b #$02
    sta.b !actor_08
    lda.b #$90
    sta.b !actor_09
    lda.b #$08
    sta.b !actor_12
    lda.b !actor_07
    and.b #$04
    bne .l9698
    stz.b !K86_4F
    stz.b !actor_spawn_at
    jsl.l ActorSpawnPlace
    jsl.l ActorSpawn
    bmi .l967A
    lda.b !actor_06
    sta.w !actor_06,x
    lda.b !actor_07
    and.b #$FB
    ora.b #$04
    sta.w !actor_07,x
    stz.w !actor_facing,x
    stz.w !actor_flip,x
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_look+1
    sta.w !actor_look+1,x
    rep.b #$20
    sep.b #$20
    stx.b !K86_PAIR
.l967A:
    sep.b #$10
    brk.b #$00
    lda.b #$0E
    jsl.l AnimStart
    rep.b #$20
    lda.b !actor_y+1
    sta.b !K86_HOME_Y
    sep.b #$20
    bra .cycle
    ; reached by nothing (see the header): sep #$10, lda #$8C, tsb $00, brk, bra
    db $E2,$10,$A9,$8C,$04,$00,$00,$00,$80,$FC
.l9698:
    lda.b #$0F
    jsl.l AnimStart
    jmp.w .child_wait
.cycle:
    lda.b #$10
    sta.b !K86_COUNT
    jsr.w Kind86HeightKeep
.l96A8:
    brk.b #$00
    jsr.w Kind86StandCheck
    dec.b !K86_COUNT
    bne .l96A8
    ldx.b #$20
    lda.b !actor_07
    and.b #$03
    beq .l96BB
    ldx.b #$14
.l96BB:
    stx.b !K86_LEFT
    lda.b #$28
    jsl.l ActorMotionReset
.l96C3:
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    jsl.l ActorFall
    jsr.w Kind86Carry
    dec.b !K86_LEFT
    bne .l96C3
    lda.b #$39
    jsl.l SoundQueueAdd
    lda.b #$02
    sta.b !K86_COUNT
.l96E0:
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    jsr.w Kind86Carry
    dec.b !K86_COUNT
    bne .l96E0
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    rep.b #$20
    dec.b !actor_y+1
    sep.b #$20
    jsr.w Kind86Carry
    lda.b #$02
    sta.b !K86_COUNT
.l9704:
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    jsr.w Kind86Carry
    dec.b !K86_COUNT
    bne .l9704
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    rep.b #$20
    inc.b !actor_y+1
    sep.b #$20
    jsr.w Kind86Carry
    lda.b #$40
    sta.b !K86_COUNT
    jsr.w Kind86HeightKeep
.l972B:
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    jsr.w Kind86Carry
    dec.b !K86_COUNT
    bne .l972B
    lda.b #$38
    jsl.l SoundQueueAdd
    ldy.b #$7B
    jsl.l ActorSpeedYSet
.l9746:
    brk.b #$00
    jsr.w Kind86HeightKeep
    jsr.w Kind86StandCheck
    jsl.l ActorStepYBack
    jsr.w Kind86Carry
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K86_HOME_Y
    sep.b #$20
    bcs .l9746
    jmp.w .cycle
.child_wait:
    brk.b #$00
    bra .child_wait

; @routine 84:9766 Kind86WaitWork
; WHAT IT DOES: What a kind $86 pair does each frame it waits: the child
; nothing; the placed one may hold the player, hurts him in its zone
; (which ends its drop), animates, keeps its child $2B above it, and once
; out of view frees the child and goes to its second state ($8C).
; HOW: $07 bit 2 set: RTL. K86_HOLD_AT 6; ActorHoldPlayerLong;
; ActorZoneHurtsPlayerLong with zone $5C; hurt (C clear): K86_LEFT 1.
; AnimStep; Kind86ChildAbove; Kind86OutOfView; out (C set): 16-bit X,
; ActorFreePlacedAtYLong with Y = K86_PAIR, X 8-bit, state bits $8C set
; (TSB); RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSL from ActorKind86Wait. Exit: M 8-bit, X 8-bit; as the last call
; leaves the rest.
%at($849766)
Kind86WaitWork:
    lda.b !actor_07
    and.b #$04
    bne .done
    lda.b #$06
    sta.b !K86_HOLD_AT
    jsl.l ActorHoldPlayerLong
    ldy.b #!K86_ZONE
    jsl.l ActorZoneHurtsPlayerLong
    bcs .anim
    lda.b #$01
    sta.b !K86_LEFT
.anim:
    jsl.l AnimStep
    jsr.w Kind86ChildAbove
    jsr.w Kind86OutOfView
    bcc .done
    rep.b #$10
    ldy.b !K86_PAIR
    jsl.l ActorFreePlacedAtYLong
    sep.b #$10
    lda.b #$8C
    tsb.b !actor_state
.done:
    rtl
%ends_at($84979B)

; @routine 84:979B Kind86HeightKeep
; WHAT IT DOES: Keeps a kind $86's height as the frame begins, and
; clears its "stood on".
; HOW: K86_LAST_Y = its y pixels; K86_STOOD = 0.
; Entry: D = the actor's record; M 8-bit; called by JSR. Exit: M 8-bit.

%at($84979B)
Kind86HeightKeep:
    rep.b #$20
    lda.b !actor_y+1
    sta.b !K86_LAST_Y
    sep.b #$20
    stz.b !K86_STOOD
    rts

; @routine 84:97A6 Kind86Carry
; WHAT IT DOES: Carries the player standing on a kind $86 with it.
; HOW: K86_STOOD set: the player's y pixels plus its y less K86_LAST_Y.
; Entry: D = the actor's record; M 8-bit; called by JSR. Exit: M 8-bit;
; not stood on: Z set.

Kind86Carry:
    lda.b !K86_STOOD
    beq .l97BA
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.b !K86_LAST_Y
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
.l97BA:
    rts

; @routine 84:97BB Kind86ChildAbove
; WHAT IT DOES: Keeps a kind $86's child $2B pixels above it.
; HOW: 16-bit: the child's (K86_PAIR's) y pixels = its own less $2B; M and
; X 8-bit.
; Entry: D = the placed one's record; DB a bank whose $0000-$1FFF is work
; RAM; called by JSR. Exit: M and X 8-bit; A = the child's new y; X =
; the child's record's low byte.
%at($8497BB)
Kind86ChildAbove:
    rep.b #$30
    ldx.b !K86_PAIR
    sec
    lda.b !actor_y+1
    sbc.w #!K86_ABOVE
    sta.w !actor_y+1,x
    sep.b #$30
    rts
%ends_at($8497CB)

; @routine 84:97CB Kind86StandCheck
; WHAT IT DOES: Lets the player stand on a kind $86 (4 pixels above its
; place), and marks it stood on when he does.
; HOW: Its y pixels less 4; ActorSolidForPlayerLong with box $24; C
; clear: K86_STOOD counted up; its y back.
; Entry: D = the actor's record; M 8-bit, X 8-bit; called by JSR. Exit:
; M 8-bit; Y = $24; A its y pixels.

%at($8497CB)
Kind86StandCheck:
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.w #$0004
    sta.b !actor_y+1
    sep.b #$20
    ldy.b #!K86_STAND_BOX
    jsl.l ActorSolidForPlayerLong
    bcs .l97E1
    inc.b !K86_STOOD
.l97E1:
    rep.b #$20
    clc
    lda.b !actor_y+1
    adc.w #$0004
    sta.b !actor_y+1
    sep.b #$20
    rts

; @routine 84:97EE Kind86OutOfView
; WHAT IT DOES: Tells whether a kind $86 is out of view: C set when it is
; more than $E0 across or $100 up or down from the range point.
; HOW: 16-bit: RANGE_X less its x, plus $E0, compared with $1C0; below:
; RANGE_Y less its y, plus $100, compared with $200. M 8-bit.
; Entry: D = the actor's record; called by JSR. Exit: M 8-bit; C set when
; out of view; A the last sum.
%at($8497EE)
Kind86OutOfView:
    rep.b #$20
    sec
    lda.w !RANGE_X
    sbc.b !actor_x+1
    clc
    adc.w #!K86_REACH_X
    cmp.w #!K86_SPAN_X
    bcs .out
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w #!K86_REACH_Y
    cmp.w #!K86_SPAN_Y
.out:
    sep.b #$20
    rts

; @routine 84:980F Kind86End
; @actor
; WHAT IT DOES: Kind $86's second state (the placed one, out of view, its
; child freed): it ends.
; HOW: JML to ActorEndPlaced.
; Entry: D = the actor's record; reached through kind $86's second-state
; entry ($83:9308) when the actor loop finds its state $8C. Exit: never
; returns: as ActorEndPlaced.
Kind86End:
    jml.l ActorEndPlaced
%ends_at($849813)

; @routine 84:968E Kind86Unused
; @actor
; WHAT IT DOES: Sets the actor's state bits $8C and waits for good. Code
; that nothing reaches (2026-11-18): it follows Kind86Start's BRA at
; 84:968C, and no JMP, JSR, JML, JSL, branch, or word $968E or $968D (a
; table entry or a pushed return address) in the ROM leads to it; no
; script's trace runs it. Filed with C as all code is.
; HOW: X 8-bit; state bits $8C set (TSB); each frame (BRK) nothing.
; Entry: D = the actor's record. Exit: never returns.
%at($84968E)
Kind86Unused:
    sep.b #$10
    lda.b #$8C
    tsb.b !actor_state
.wait:
    brk.b #$00
    bra .wait
%ends_at($849698)
