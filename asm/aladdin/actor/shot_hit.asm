; shot_hit.asm: an actor hit by a shot: an actor of the second pool (the
; five records from $09CB) that can hit, found by the test at $83:8578.
; The shot is stopped, or turned back, and a small actor marks the hit.
; C: src/aladdin/actor/shot_hit.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_11 = $11                 ; a shot's: negative, it does not hit
!TURNED_BACK_AT = $EB7C         ; where a turned-back shot's code resumes (its own bank)

; Not yet filed
!SHOT_TEST = $838578            ; C clear: a shot overlaps the actor (D); Y = its record, X 16-bit
!SHOT_HIT_80FC = $8380FC        ; the shot test, then (code no script has run)

; @routine 83:80F8 ActorShotHit80FCLong
; WHAT IT DOES: The routine at $83:80FC, which starts with the shot test,
; for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as $83:80FC: M 8-bit, X 8-bit; D = the actor; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: as the C states.
%at($8380F8)
ActorShotHit80FCLong:
    jsr.w !SHOT_HIT_80FC
    rtl

; @routine 83:8196 ActorTurnsShotBackLong
; WHAT IT DOES: ActorTurnsShotBack for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorTurnsShotBack; called by JSL. Exit: as the C states.
%at($838196)
ActorTurnsShotBackLong:
    jsr.w ActorTurnsShotBack
    rtl

; @routine 83:819A ActorTurnsShotBack
; WHAT IT DOES: When a shot hits the actor, turns it back: it flies the
; other way, harmless, and a small actor (kind $13) marks the hit.
; HOW: The shot test; on a hit, kind $13 started at the shot's place, the
; shot's facing flipped, its code sent on at $EB7C (in its bank), bit 7
; of its $08 set and of its $09 cleared.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
ActorTurnsShotBack:
    jsr.w !SHOT_TEST
    bcs .none
    jsr.w Spawn13AtY
    lda.w !actor_facing,y
    eor.b #$01
    sta.w !actor_facing,y
    lda.b #!TURNED_BACK_AT&255
    sta.w !actor_03,y
    lda.b #!TURNED_BACK_AT>>8
    sta.w !actor_04,y
    lda.b #$80
    ora.w !actor_08,y
    sta.w !actor_08,y
    lda.w !actor_09,y
    and.b #$7F
    sta.w !actor_09,y
    sep.b #$30
    clc
    rts
.none:
    sep.b #$31
    rts

; @routine 83:81E8 ActorTakesShot
; WHAT IT DOES: When a shot that hits (its $11 not negative) reaches the
; actor: the shot stops (state $8C), a small actor (kind $11) marks the
; hit, and the actor turns the way the shot flew.
; HOW: The shot test; then the shot's $11; kind $11 started at the shot's
; place; the shot's facing in the actor's facing and flip bytes;
; ActorStateSet8C on the shot.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($8381E8)
ActorTakesShot:
    jsr.w !SHOT_TEST
    bcs .done
    sec
    lda.w !ACTOR_11,y
    bmi .done
    jsr.w Spawn11AtY
    lda.w !actor_facing,y
    sta.b !actor_facing
    sta.b !actor_flip
    jsr.w ActorStateSet8C
    clc
.done:
    sep.b #$30
    rts

; @routine 83:82B1 Spawn13AtYLong
; WHAT IT DOES: The routine at $83:82B5, which starts an actor of kind
; $13 at record Y's place, for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as $83:82B5: M 8-bit, X 16-bit; Y = the record; called by JSL.
; Exit: as the C states.
%at($8382B1)
Spawn13AtYLong:
    jsr.w Spawn13AtY
    rtl
