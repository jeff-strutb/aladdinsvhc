; player_state.asm: putting Aladdin into state $8C, the state the bank-$83
; player code enters from several others.
; C: src/aladdin/player/player_state.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Counted by the state changes below; their purpose is not yet
; established.
!V_0347 = $0347                 ; bit 1 set on entering state $8C
!V_0367 = $0367                 ; counted down on entering state $8C
!V_141B = $141B                 ; counted up on entering state $8C
!V_1424 = $1424

; @routine 83:8008 PlayerEnter8C
; WHAT IT DOES: Puts Aladdin into state $8C.
; HOW: The state byte, then $141B counted up and $0367 down, bit 7 of the
; player's byte at $08 and bit 1 of $0347 set.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($838008)
PlayerEnter8C:
    lda.b #$8C
    sta.w !player+!actor_state
    inc.w !V_141B
    dec.w !V_0367
    lda.w !player+!actor_08
    ora.b #$80
    sta.w !player+!actor_08
    lda.w !V_0347
    ora.b #$02
    sta.w !V_0347
    rts

; @routine 83:8024 PlayerEnter8CFacing
; WHAT IT DOES: Unless bit 7 of the player's byte at $08 is already set,
; puts Aladdin into state $8C turned away from the actor (D).
; HOW: As PlayerEnter8C, then ActorSideOfPlayer's facing, flipped, is
; the player's.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
PlayerEnter8CFacing:
    lda.w !player+!actor_08
    bmi .done
    lda.b #$8C
    sta.w !player+!actor_state
    inc.w !V_141B
    dec.w !V_0367
    lda.w !player+!actor_08
    ora.b #$80
    sta.w !player+!actor_08
    lda.w !V_0347
    ora.b #$02
    sta.w !V_0347
    jsl.l ActorSideOfPlayer
    and.b #$01
    eor.b #$01
    sta.w !player+!actor_facing
    lda.b #$00
.done:
    rts

; @routine 83:8056 PlayerEnter8CFrom1
; WHAT IT DOES: Puts Aladdin into state $8C with $0367 set to 1 first
; (so it ends at 0) and $1424 counted up.
; HOW: The two stores, then on into PlayerEnter8C.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($838056)
PlayerEnter8CFrom1:
    lda.b #$01
    sta.w !V_0367
    inc.w !V_1424
    bra PlayerEnter8C
