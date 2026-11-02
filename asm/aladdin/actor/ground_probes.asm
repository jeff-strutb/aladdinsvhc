; ground_probes.asm: tests of the ground round an actor of bank $A5: a
; wall in front, a ledge ahead with room above it, ground above a point
; toward the player.
; C: src/aladdin/actor/ground_probes.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!point_x = !scratch             ; the probe point PointInGround tests: x (word)
!point_y = !scratch+2           ; its y (word)
!tile_behaviour_byte = !scratch+6 ; PointInGround's tile behaviour; low 3 bits its kind
!probes_left = $002C            ; probes left to make (word)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!WALL_PROBE = 8                 ; the probe offset ActorOutOfWall8 gives
!TILE_STEP_UP = $FFF0           ; added to the point's y to go a tile up

; @routine A5:EB17 ActorOutOfWall8
; WHAT IT DOES: Moves the actor out of a wall at its probe 8 in front:
; C set when it was moved.
; HOW: ActorOutOfWallLong with A = 8.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
; ActorOutOfWall.
%at($A5EB17)
ActorOutOfWall8:
    lda.b #!WALL_PROBE
    jsl.l ActorOutOfWallLong
    rts


; @routine A5:EA2C ActorLedgeAhead
; WHAT IT DOES: Looks for ground 48 pixels ahead of the actor with room
; above it, from 32 pixels below its y up: C clear when found within four
; tiles.
; HOW: The point 48 pixels in front by the facing ($30, or -$30 facing
; left) and 32 below; up to 4 probes (PointInGroundLong, a kind in the
; behaviour's low 3 bits being ground), a tile up each time: first while
; no ground, then, once in ground, a tile up at a time until a probe finds
; none (C clear). C set when the probes run out.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
; M 8-bit, X 8-bit; C as above; the point at the last probe.
%at($A5EA2C)
ActorLedgeAhead:
    rep.b #$20
    lda.w #$0030
    ldx.b !actor_facing
    beq .ahead
    lda.w #$FFD0
.ahead:
    clc
    adc.b !actor_x+1
    sta.w !point_x
    clc
    lda.w #$0020
    adc.b !actor_y+1
    sta.w !point_y
    lda.w #$0004
    sta.w !probes_left
.air:
    php
    jsl.l PointInGroundLong
    plp
    sep.b #$30
    lda.w !tile_behaviour_byte
    and.b #$07
    bne .ground
    dec.w !probes_left
    beq .none
    rep.b #$20
    clc
    lda.w !point_y
    adc.w #!TILE_STEP_UP
    sta.w !point_y
    bra .air
.ground:
    rep.b #$20
    clc
    lda.w !point_y
    adc.w #!TILE_STEP_UP
    sta.w !point_y
    php
    jsl.l PointInGroundLong
    plp
    sep.b #$30
    lda.w !tile_behaviour_byte
    and.b #$07
    beq .room
    dec.w !probes_left
    bne .ground
.none:
    sec
    rts
.room:
    clc
    rts


; @routine A5:F242 ActorProbeUpTowardPlayer
; WHAT IT DOES: Probes up from a point offset from the actor toward the
; player, a tile at a time for X probes, for a tile whose behaviour has
; bit 3 or 6: C clear when one is found.
; HOW: The point's y = the actor's + A; its x = the actor's + Y when the
; player is not left of the actor, else - Y; X probes (PointInGroundLong,
; the behaviour byte tested with $48), the point a tile up after each.
; Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
; offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X
; 8-bit; C as above.
%at($A5F242)
ActorProbeUpTowardPlayer:
    rep.b #$20
    clc
    adc.b !actor_y+1
    sta.w !point_y
    txa
    sta.w !probes_left
    lda.w !player_x
    cmp.b !actor_x+1
    tya
    bcs .toward
    eor.w #$FFFF
    inc a
.toward:
    clc
    adc.b !actor_x+1
    sta.w !point_x
.probe:
    jsl.l PointInGroundLong
    sep.b #$30
    lda.w !tile_behaviour_byte
    bit.b #$48
    bne .found
    rep.b #$20
    clc
    lda.w !point_y
    adc.w #!TILE_STEP_UP
    sta.w !point_y
    dec.w !probes_left
    bne .probe
    sec
    rts
.found:
    rep.b #$20
    clc
    rts
