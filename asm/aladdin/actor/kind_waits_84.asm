; kind_waits_84.asm: the bank $84 work of nine actor kinds' waits (what each
; runs every frame it waits, by JSL from its stub in the actor loop's
; wait table): nothing, its animation, or its animation, a touch that
; hurts the player, and its end when out of range.
; C: src/aladdin/actor/kind_waits_84.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

!K9A_BOUNCES = $41             ; nonzero: Aladdin bounces off it (else touching it hurts him)
!K7E_BOUNCE = $42              ; frames left of a bounce (the player held on it)
!K7E_STAYS = $47               ; nonzero: never freed out of range
!K9E_FRAME = $27               ; its animation frame: 7, 8, 9, $0C and $0D are drawn in front

arch 65816

; @routine 84:951C Kind81WaitWork
; WHAT IT DOES: What actors of kind $81 do each frame they wait: nothing.
; HOW: RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as entered.
%at($84951C)
Kind81WaitWork:
    rtl

; @routine 84:95E3 Kind84Start
; @actor
; WHAT IT DOES: Kind $84's start (GrabberStart spawns it): shows its first
; animation for 16 frames, then ends.
; HOW: Bit 1 of $08 and bits $90 of $09 set; AnimStart 0; a wait of 16
; frames (COP, Kind84WaitWork each frame); JML ActorEndPlaced.
; Entry: D = the actor's record; M 8-bit, X 8-bit; reached by JML from
; kind $84's start entry ($83:92E5). Exit: never returns: as
; ActorEndPlaced.
%at($8495E3)
Kind84Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    jml.l ActorEndPlaced

; @routine 84:95FD Kind84WaitWork
; WHAT IT DOES: What actors of kind $84 do each frame they wait: their
; animation steps.
; HOW: AnimStep, RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as AnimStep leaves them.
%at($8495FD)
Kind84WaitWork:
    jsl.l AnimStep
    rtl

; @routine 84:99E7 Kind8CWaitWork
; WHAT IT DOES: What actors of kind $8C do each frame they wait: their
; animation steps.
; HOW: AnimStep, RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as AnimStep leaves them.
%at($8499E7)
Kind8CWaitWork:
    jsl.l AnimStep
    rtl

; @routine 84:AB97 KindA4WaitWork
; WHAT IT DOES: What actors of kind $A4 do each frame they wait: their
; animation steps.
; HOW: AnimStep, RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as AnimStep leaves them.
%at($84AB97)
KindA4WaitWork:
    jsl.l AnimStep
    rtl

; @routine 84:A438 Kind99WaitWork
; WHAT IT DOES: What actors of kind $99 do each frame they wait: their
; animation steps, Aladdin touching them is hurt, and they are freed
; once out of range by box $18.
; HOW: AnimStep, ActorHurtsPlayerOnTouchLong; X = $18; on into
; ActorFreeOutOfRangeLong (JML), whose RTL is this routine's.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
%at($84A438)
Kind99WaitWork:
    jsl.l AnimStep
    jsl.l ActorHurtsPlayerOnTouchLong
    ldx.b #$18
    jml.l ActorFreeOutOfRangeLong

; @routine 84:AC31 KindA5WaitWork
; WHAT IT DOES: What actors of kind $A5 do each frame they wait: their
; animation steps, Aladdin touching them is hurt, and they are freed
; once out of range by box $18.
; HOW: AnimStep, ActorHurtsPlayerOnTouchLong; X = $18; on into
; ActorFreeOutOfRangeLong (JML), whose RTL is this routine's.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
%at($84AC31)
KindA5WaitWork:
    jsl.l AnimStep
    jsl.l ActorHurtsPlayerOnTouchLong
    ldx.b #$18
    jml.l ActorFreeOutOfRangeLong

; @routine 83:840B ActorBouncesPlayerLong
; WHAT IT DOES: ActorBouncesPlayer for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorBouncesPlayer; called by JSL. Exit: as ActorBouncesPlayer.
%at($83840B)
ActorBouncesPlayerLong:
    jsr.w ActorBouncesPlayer
    rtl

; @routine 84:A4E3 Kind9AWaitWork
; WHAT IT DOES: What actors of kind $9A do each frame they wait: their
; animation steps; Aladdin touching one is hurt, or, with K9A_BOUNCES
; set, coming down on it bounces him (sound $12, animation $18); freed
; once out of range by box $18.
; HOW: AnimStep. K9A_BOUNCES zero: ActorHurtsPlayerOnTouchLong. Else
; ActorBouncesPlayerLong, bounced (Z clear): SoundQueueAdd $12, AnimStart
; $18. X = $18; on into ActorFreeOutOfRangeLong (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
; actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
%at($84A4E3)
Kind9AWaitWork:
    jsl.l AnimStep
    lda.b !K9A_BOUNCES
    bne .bounces
    jsl.l ActorHurtsPlayerOnTouchLong
    bra .range
.bounces:
    jsl.l ActorBouncesPlayerLong
    beq .range
    lda.b #$12
    jsl.l SoundQueueAdd
    lda.b #$18
    jsl.l AnimStart
.range:
    ldx.b #$18
    jml.l ActorFreeOutOfRangeLong

; @routine 84:A8BD Kind9EWaitWork
; WHAT IT DOES: What actors of kind $9E do each frame they wait: their
; animation steps, and they are drawn in front (ACTOR_09 bits 4-5) on the
; frames 7, 8, 9, $0C and $0D.
; HOW: AnimStep; ACTOR_09 bit 5 cleared; K9E_FRAME one of those: ACTOR_09
; bits $30 set. RTL.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL from the
; kind's stub through the actor loop's wait table. Exit: M 8-bit; A =
; K9E_FRAME or $30; N, Z and C from the last test.
%at($84A8BD)
Kind9EWaitWork:
    jsl.l AnimStep
    lda.b #$20
    trb.b !actor_09
    lda.b !K9E_FRAME
    cmp.b #$07
    beq .front
    cmp.b #$08
    beq .front
    cmp.b #$09
    beq .front
    cmp.b #$0C
    beq .front
    cmp.b #$0D
    bne .done
.front:
    lda.b #$30
    tsb.b !actor_09
.done:
    rtl

; @routine 84:931C Kind7EWaitWork
; WHAT IT DOES: What actors of kind $7E (a platform Aladdin bounces on) do
; each frame they wait: for 8 frames after a bounce the player is held
; 38 pixels above it; else coming down on it bounces him (sound $12);
; unless K7E_STAYS, freed once out of range by box 8.
; HOW: K7E_BOUNCE nonzero: down by 1; 16-bit (REP #$21: carry clear) the
; player's y = its y - $25 - 1. Else ActorBouncesPlayerLong, bounced (Z
; clear): SoundQueueAdd $12, K7E_BOUNCE 8. K7E_STAYS set: RTL; else X = 8
; and on into ActorFreeOutOfRangeLong (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub ($83:92AC)
; through the actor loop's wait table. Exit: M 8-bit; as
; ActorFreeOutOfRangeLong leaves them, or A = K7E_STAYS.
%at($84931C)
Kind7EWaitWork:
    lda.b !K7E_BOUNCE
    beq .test
    dec.b !K7E_BOUNCE
    rep.b #$21
    lda.b !actor_y+1
    sbc.w #$0025
    sta.w !player+!actor_y+1
    sep.b #$20
    bra .range
.test:
    jsl.l ActorBouncesPlayerLong
    beq .range
    lda.b #$12
    jsl.l SoundQueueAdd
    lda.b #$08
    sta.b !K7E_BOUNCE
.range:
    lda.b !K7E_STAYS
    bne .stays
    ldx.b #$08
    jml.l ActorFreeOutOfRangeLong
.stays:
    rtl
