; at_player.asm: an actor kept at Aladdin's place.
; C: src/aladdin/actor/at_player.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!PLAYER_2F = !player+$2F        ; picks the motion of an actor kept at his place
!AT_PLAYER_MOTIONS = $CA03      ; the motion presets, by his $2F (bytes, the DB bank)

; @routine 81:EC02 ActorAtPlayer
; WHAT IT DOES: Puts the actor at Aladdin's place and moves it by the
; motion his $2F picks.
; HOW: His x and y pixels copied into the actor's; the motion preset from
; the table at $CA03 by his $2F; then ActorMotionPreset and ActorMove.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the table's bank; called by JSR. Exit: as ActorMove.
%at($81EC02)
ActorAtPlayer:
    rep.b #$20
    lda.w !player+!actor_x+1
    sta.b !actor_x+1
    lda.w !player+!actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    ldx.w !PLAYER_2F
    ldy.w !AT_PLAYER_MOTIONS,x
    jsl.l ActorMotionPreset
    jsl.l ActorMove
    rts
