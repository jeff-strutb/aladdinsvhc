; shot_hit.asm: an actor hit by a shot: an actor of the second pool (the
; five records from $09CB) that can hit, found by the test at $83:8578.
; The shot is stopped, or turned back, and a small actor marks the hit.
; C: src/aladdin/actor/shot_hit.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_11 = $11                 ; a shot's: negative, it does not hit
!TURNED_BACK_AT = $EB7C         ; where a turned-back shot's code resumes (its own bank)

!PLAYER_2F = !player+$2F        ; $0B or more: the side a harmless shot turns the actor to flips

; @routine 83:80F8 ActorDownedByShotLong
; WHAT IT DOES: ActorDownedByShot for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorDownedByShot; called by JSL. Exit: as the C states.
%at($8380F8)
ActorDownedByShotLong:
    jsr.w ActorDownedByShot
    rtl

; @routine 83:80FC ActorDownedByShot
; WHAT IT DOES: When a shot reaches the actor, the actor is downed (state
; $8C): a shot that hits stops too (state $8C) and the actor turns the way
; it flew; a small actor (kind $11) marks the hit.
; HOW: ActorTouchesSecondPool; on a touch Spawn11AtY at the shot; the
; shot's $11 not negative: the shot's facing taken, ActorStateSet8C on the
; shot; negative (code the scripts have not run): its facing is bit 0 of
; Aladdin's flip byte plus 1 when his $2F is $0B or more, with bit 7 set.
; Then ActorStateSet8CSelf; C cleared.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit.
; Touched: C clear; A low = $8C, N set, Z clear; Y = the shot's record
; (low byte). No touch: C set; as ActorTouchesSecondPool leaves the rest.
;
ActorDownedByShot:
    jsr.w ActorTouchesSecondPool
    bcs .out
    jsr.w Spawn11AtY
    lda.w !ACTOR_11,y
    bpl .hits
    lda.w !PLAYER_2F
    cmp.b #$0B
    bcs .far
    lda.b #0
    bra .side
.far:
    lda.b #1
.side:
    clc
    adc.w !player+!actor_flip
    and.b #$01
    ora.b #$80
    sta.b !actor_facing
    bra .self
.hits:
    lda.w !actor_facing,y
    sta.b !actor_facing
    jsr.w ActorStateSet8C
.self:
    jsr.w ActorStateSet8CSelf
    clc
.out:
    sep.b #$30
    rts

; @routine 83:8135 ActorHitByShotCC
; WHAT IT DOES: When a shot reaches the actor: a shot that hits stops
; (state $8C), the actor turns the way it flew and goes into state $CC; a
; shot that does not hit downs the actor (state $8C); a small actor (kind
; $11) marks either.
; HOW: ActorTouchesSecondPool; on a touch Spawn11AtY at the shot; the
; shot's $11 not negative: the shot's facing taken, ActorStateSet8C on the
; shot, ActorStateSetCC; negative (code the scripts have not run): its
; facing is bit 0 of Aladdin's flip byte plus 1 when his $2F is $0B or
; more, with bit 7 set, then ActorStateSet8CSelf. C cleared.
; Entry: as ActorDownedByShot. Exit: as ActorDownedByShot, A low = $CC or
; $8C.
;
%at($838135)
ActorHitByShotCC:
    jsr.w ActorTouchesSecondPool
    bcs .out
    jsr.w Spawn11AtY
    lda.w !ACTOR_11,y
    bpl .hits
    lda.w !PLAYER_2F
    cmp.b #$0B
    bcs .far
    lda.b #0
    bra .side
.far:
    lda.b #1
.side:
    clc
    adc.w !player+!actor_flip
    and.b #$01
    ora.b #$80
    sta.b !actor_facing
    jsr.w ActorStateSet8CSelf
    bra .done
.hits:
    lda.w !actor_facing,y
    sta.b !actor_facing
    jsr.w ActorStateSet8C
    jsr.w ActorStateSetCC
.done:
    clc
.out:
    sep.b #$30
    rts

; @routine 83:816D ActorStopsShotLong
; WHAT IT DOES: ActorStopsShot for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorStopsShot; called by JSL. Exit: as ActorStopsShot.
;
ActorStopsShotLong:
    jsr.w ActorStopsShot
    rtl

; @routine 83:8171 ActorStopsShot
; WHAT IT DOES: When a shot reaches the actor, a small actor (kind $11)
; marks it; a shot that hits stops (state $8C); one that does not downs
; the actor (state $8C), facing Aladdin's flip with bit 7 set.
; HOW: ActorTouchesSecondPool; on a touch (code the scripts have not run)
; Spawn11AtY at the shot; the shot's $11 not negative: ActorStateSet8C on
; the shot; negative: the shot's facing stored, then Aladdin's flip byte
; with bit 7 set as the actor's, and ActorStateSet8CSelf. C cleared.
; Entry: as ActorDownedByShot. Exit: as ActorDownedByShot.
;
ActorStopsShot:
    jsr.w ActorTouchesSecondPool
    bcs .out
    jsr.w Spawn11AtY
    lda.w !ACTOR_11,y
    bpl .hits
    lda.w !actor_facing,y
    sta.b !actor_facing
    lda.w !player+!actor_flip
    ora.b #$80
    sta.b !actor_facing
    jsr.w ActorStateSet8CSelf
    bra .done
.hits:
    jsr.w ActorStateSet8C
.done:
    clc
.out:
    sep.b #$30
    rts

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
    jsr.w ActorTouchesSecondPool
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
    jsr.w ActorTouchesSecondPool
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
