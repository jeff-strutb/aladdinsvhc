; player_bounds.asm: where the player is against the screen and the level:
; kept on screen across, fallen out of the bottom, and the countdown of
; the time it cannot be hurt. Drafted by cdraft.py; named and described by
; hand.
; C: src/aladdin/player/player_bounds.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_video.inc"

arch 65816

; In bank $80: the least and most x pixel from the screen's left edge.
!SCREEN_X_LIMITS = $C9B9
!v_0347 = $0347

; @routine 81:E8EC PlayerFallenTest
; WHAT IT DOES: Tells whether the player is still above the bottom of the
; level: C clear once it has fallen 230 pixels below the scroll's lowest
; place.
; HOW: Its y pixels' top byte negative (above the level): C set. Else the
; scroll Y limit, or the scroll Y itself while an empty block stops the
; scroll up or down, plus $E6, compared with its y pixels.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: C set: above; C
; clear: below. M 8-bit, X 8-bit; X = CAMERA_STOPPED_Y's low byte (unless
; above).
%at($81E8EC)
PlayerFallenTest:
    sec
    lda.w !player+!actor_y+2
    bmi .lE908
    rep.b #$20
    lda.w !scroll_limit_y
    ldx.w !camera_stopped_y
    beq .lE8FF
    lda.w !layer0+!LAYER_SCROLL_Y
.lE8FF:
    clc
    adc.w #$00E6
    cmp.w !player+!actor_y+1
    sep.b #$20
.lE908:
    rts


; @routine 81:E95C PlayerHurtCountdown
; WHAT IT DOES: Counts down the time the player cannot be hurt; when it
; runs out, the player can be hit again.
; HOW: PLAYER_HURT_TIME, when nonzero, steps down; at 0, bit 4 of the
; player's $08, bit 7 of the actor's $08 and bit 2 of $0347 are cleared.
; Entry: M 8-bit; D = the player's record; called by JSR. Exit: M 8-bit;
; ran out: A low = the new $0347, N and Z from it; else A low = the count
; before its step.
%at($81E95C)
PlayerHurtCountdown:
    lda.w !player_hurt_time
    beq .lE97C
    dec.w !player_hurt_time
    bne .lE97C
    lda.w !player+!actor_08
    and.b #$EF
    sta.w !player+!actor_08
    lda.b !actor_08
    and.b #$7F
    sta.b !actor_08
    lda.w !v_0347
    and.b #$FB
    sta.w !v_0347
.lE97C:
    rts


; @routine 81:E990 PlayerKeepOnScreenX
; WHAT IT DOES: Keeps the actor's x on screen: between 16 and 240 pixels
; from the play field's left edge.
; HOW: The play field's scroll X plus each limit (from the table) compared
; with its x pixels, unsigned; past one, its x set to it.
; Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit: M
; 8-bit; A = the last limit computed; C from its comparison.
%at($81E990)
PlayerKeepOnScreenX:
    rep.b #$20
    clc
    lda.w !layer0+!LAYER_SCROLL_X
    adc.w !SCREEN_X_LIMITS
    cmp.b !actor_x+1
    bcs .lE9A8
    clc
    lda.w !layer0+!LAYER_SCROLL_X
    adc.w !SCREEN_X_LIMITS+2
    cmp.b !actor_x+1
    bcs .lE9AA
.lE9A8:
    sta.b !actor_x+1
.lE9AA:
    sep.b #$20
    rts
