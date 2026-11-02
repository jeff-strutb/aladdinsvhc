; at_player.asm: an actor kept at Aladdin's place, and actor kind $12,
; kept there until the player's box is one of three.
; C: src/aladdin/actor/at_player.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!PLAYER_2F = !player+$2F        ; picks the motion of an actor kept at his place
!AT_PLAYER_MOTIONS = $CA03      ; the motion presets, by his $2F (bytes, the DB bank)
!KIND12_11 = $11               ; kind $12: set to $FF at its start

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
; Kind12AtPlayer's end.
ActorAtPlayerEnd:
    jml.l ActorEnd

; @routine 81:EBDE Kind12AtPlayer
; @actor
; WHAT IT DOES: Actor kind $12: kept at Aladdin's place, facing as he
; does, until the box his code wants (player_box_wanted) is 3, 6 or $12;
; then it ends.
; HOW: ActorAtPlayer; $11 = $FF; facing and flip = his flip ($08F0).
; Each frame: ActorAtPlayer, BRK, then player_box_wanted compared with 3,
; 6 and $12; on a match ActorEnd (by the JML after ActorAtPlayer).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; the actor loop's jump for kind $12 in state $0C (the table at
; $83:8CAB, through the stub at $83:921A). Exit: never returns.
%at($81EBDE)
Kind12AtPlayer:
    jsr.w ActorAtPlayer
    lda.b #$FF
    sta.b !KIND12_11
    lda.w !player+!actor_flip
    sta.b !actor_facing
    sta.b !actor_flip
.frame:
    jsr.w ActorAtPlayer
    brk.b #$00
    lda.w !player_box_wanted
    cmp.b #$03
    beq ActorAtPlayerEnd
    cmp.b #$06
    beq ActorAtPlayerEnd
    cmp.b #$12
    beq ActorAtPlayerEnd
    bra .frame
