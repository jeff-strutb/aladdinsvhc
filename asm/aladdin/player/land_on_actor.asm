; land_on_actor.asm: Aladdin landing on top of an actor he falls onto: he
; stands on it. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/land_on_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"

arch 65816

!V_1427 = $1427  ; nonzero: no landing on actors
!LAND_BOXES = $BE92  ; bank $82, by Y: x offset, width, y offset, height, then how far below his feet it may be (words)

; @routine 82:89E3 PlayerLandOnActor
; WHAT IT DOES: Lands Aladdin on the actor when he is falling (or still)
; onto its top, inside its landing box Y: his feet are put on it, his fall
; stops and he is marked standing on an actor. C clear when he landed, set
; when not.
; HOW: The actor can be hit (bit 6 of $09), V_1428 and V_1427 both zero
; and his speed y not negative, else out. Its x less his plus the box's x
; offset below its width, and its y less his plus the y offset below its
; height, else out; $000C up by one (a box matched); his y plus the box's
; fifth word below its y, else out; then his y = its y less the y offset
; plus 1, his speed y cleared, V_1428 up, V_141C cleared, C clear. Out: M
; and X 8-bit, C set.
; Entry: M 8-bit, X 16-bit; Y = the box's offset; D = the actor's record;
; DB = $82; reached by a jump (JML at $83:FBB1) from a routine called by
; JSL. Exit: by RTL; M and X 8-bit; C as above.
%at($8289E3)
PlayerLandOnActor:
    bit.b !actor_09
    bvc .l8A3F
    lda.w !v_1428
    ora.w !V_1427
    bne .l8A3F
    lda.w !player+!actor_speed_y+2
    bmi .l8A3F
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.w !player+!actor_x+1
    clc
    adc.w !LAND_BOXES,y
    cmp.w !LAND_BOXES+2,y
    bcs .l8A3F
    sec
    lda.b !actor_y+1
    sbc.w !player+!actor_y+1
    clc
    adc.w !LAND_BOXES+4,y
    cmp.w !LAND_BOXES+6,y
    bcs .l8A3F
    inc.w !scratch
    clc
    lda.w !player+!actor_y+1
    adc.w !LAND_BOXES+8,y
    cmp.b !actor_y+1
    bcs .l8A3F
    sec
    lda.b !actor_y+1
    sbc.w !LAND_BOXES+4,y
    inc a
    sta.w !player+!actor_y+1
    sep.b #$30
    stz.w !player+!actor_speed_y
    stz.w !player+!actor_speed_y+1
    stz.w !player+!actor_speed_y+2
    inc.w !v_1428
    stz.w !v_141c
    clc
    rtl
.l8A3F:
    sep.b #$31
    rtl
