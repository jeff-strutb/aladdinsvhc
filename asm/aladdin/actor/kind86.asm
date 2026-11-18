; kind86.asm: actor kind $86 (stage a5 s0), a pair that drops and rises
; again: the placed one ($07 0) and its child ($07 4) kept $2B above it.
; Here its wait (the hold, the hurting zone, the child kept above, the end
; out of view) and its second state's end. Its start (84:962D) is not
; filed yet.
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
