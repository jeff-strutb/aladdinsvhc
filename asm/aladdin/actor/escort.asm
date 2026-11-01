; escort.asm: the actor some levels start beside the player.
; C: src/aladdin/actor/escort.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Per level: 0, no escort; else its $07 value + 1.
!level_escorts = $81B79E
!ESCORT_HIT_BOX = 8
!ESCORT_LOOK = $0220
!ACTOR_NEW = $0C

; @routine 81:85A1 EscortStart
; WHAT IT DOES: Starts the level's escort actor beside the player, when
; the level has one.
; HOW: The level's entry in the escort table, less one, is the escort's
; $07; its record is made new, with hit box 8, sprite attributes $0220
; and the player's x and y.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: no escort: X =
; the level; A low byte = 0; Z set. Else: M 8-bit; A = the player's y
; (16 bits); N, Z from it; X = the level. Y, C, V unchanged.
%at($8185A1)
EscortStart:
    ldx.w !level
    lda.w !level_escorts,x
    beq .done
    dec a
    sta.w !actor_escort+!actor_07
    lda.b #!ACTOR_NEW
    sta.w !actor_escort+!actor_state
    lda.b #!ESCORT_HIT_BOX
    sta.w !actor_escort+!actor_06
    rep.b #$20
    lda.w #!ESCORT_LOOK
    sta.w !actor_escort+!actor_look
    lda.w !player+!actor_x+1
    sta.w !actor_escort+!actor_x+1
    lda.w !player+!actor_y+1
    sta.w !actor_escort+!actor_y+1
    sep.b #$20
.done:
    rts
