; ground_slide.asm: ground that slides Aladdin along it: tiles whose
; behaviour has bit 4 set, in the levels where the slide replaces the push
; (ground_push.c). Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/ground_slide.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!GROUND_PUSHED = $1429  ; 2 while the player stands on ground that pushes, 1 while he slides on it, else 0

; @routine 81:CEF4 PlayerGroundSlide
; WHAT IT DOES: On ground that slides (tile behaviour bit 4), turns
; Aladdin the way the tile's bit 7 says and moves him along it by half a
; pixel a frame, marking him sliding.
; HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, out
; (the RTS at $81:CEF3). Else facing = 1 (bit 7 clear, C clear) or 0;
; GROUND_PUSHED = 1; speed x = $80 (24-bit); ActorStepX.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code (81:CD0A). Exit: as ActorStepX, or as
; GroundBehindBit4 with Z set.
%at($81CEF4)
PlayerGroundSlide:
    stz.w !GROUND_PUSHED
    lda.b #$18
    sta.b !actor_18
    jsl.l GroundBehindBit4
    beq PlayerGroundPush_lCEF3
    bcs .lCF09
    lda.b #$01
    sta.b !actor_facing
    bra .lCF0B
.lCF09:
    stz.b !actor_facing
.lCF0B:
    lda.b #$01
    sta.w !GROUND_PUSHED
    lda.b #$80
    sta.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    jsr.w ActorStepX
    rts
