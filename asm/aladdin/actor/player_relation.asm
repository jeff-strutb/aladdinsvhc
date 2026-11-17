; player_relation.asm: where the player is from an actor: its direction,
; and whether it is inside a box in front of the actor.
; C: src/aladdin/actor/player_relation.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

!directions = $81BBE3           ; per quadrant (4 bytes) and steepness band, a direction (word)
!front_boxes = $81BC8F          ; per box: half-height, the x offset facing right, then facing left (words)

; @routine 81:ABEA ActorDirectionToPlayer
; WHAT IT DOES: The direction from the actor to the player, as one of the
; directions in the table at $BBE3.
; HOW: The distances across and up or down (made positive, the quadrant
; kept: 4 for the player above, 2 for left); the band is 0 when the smaller
; distance is under a quarter of the larger, 1 under three quarters, else
; 2; the quadrant * 4 plus the band picks the direction.
; Entry: D = the actor; DB = $81; called by JSL. Exit: M and X 8-bit; A =
; the direction (16 bits, low byte as M 8-bit); X = its index; Y = the
; quadrant; scratch $0C-$13 the distances.
%at($81ABEA)
ActorDirectionToPlayer:
    rep.b #$30
    ldx.w #$08DB
    lda.w !actor_x+1,x
    sta.w !scratch+4
    lda.w !actor_y+1,x
    sta.w !scratch+6
.to_point:
    ldy.w #$0000
    lda.w !scratch+6
    sec
    sbc.b !actor_y+1
    bcs .lAC0D
    eor.w #$FFFF
    inc a
    ldy.w #$0004
.lAC0D:
    sta.w !scratch
    lda.w !scratch+4
    sec
    sbc.b !scratch
    bcs .lAC1E
    eor.w #$FFFF
    inc a
    iny
    iny
.lAC1E:
    cmp.w !scratch
    bcs .lAC2B
    iny
    ldx.w !scratch
    sta.w !scratch
    txa
.lAC2B:
    ldx.w #$0000
    lsr a
    lsr a
    sta.w !scratch+2
    cmp.w !scratch
    bcs .lAC45
    inx
    adc.w !scratch+2
    adc.w !scratch+2
    cmp.w !scratch
    bcs .lAC45
    inx
.lAC45:
    tya
    asl a
    asl a
    sta.w !scratch
    txa
    clc
    adc.w !scratch
    tax
    lda.w !directions,x
    sep.b #$30
    rtl


; @routine 81:AEFD PlayerInFrontBox
; WHAT IT DOES: Says whether the player is inside box X in front of the
; actor: C clear when inside.
; HOW: Height: the actor's y plus the half-height less the player's y,
; below the height (twice the half-height). Width: the actor's x plus the
; box's offset for its facing less the player's x, below the box's width
; (the two offsets' sum), tested only when the height passes.
; Entry: M 8-bit, X 8-bit; D = the actor; X = the box's byte offset; DB =
; $81; called by JSL. Exit: M 8-bit; C as above; A = the last sum (16
; bits); Y = the facing less one when the height passed; scratch $0C-$11 the
; half-height, height and width.
%at($81AEFD)
PlayerInFrontBox:
    ldy.b !actor_facing
    bra .lAF03
; The same with the flip for the facing (an entry no script takes).
.by_flip:
    ldy.b !actor_flip
.lAF03:
    rep.b #$20
    lda.w !front_boxes,x
    sta.w !scratch
    asl a
    sta.w !scratch+2
    lda.w !front_boxes+2,x
    adc.w !front_boxes+4,x
    sta.w !scratch+4
    lda.b !actor_y+1
    clc
    adc.w !scratch
    sec
    sbc.w !player+!actor_y+1
    cmp.w !scratch+2
    bcs .lAF38
    dey
    bmi .lAF2C
    inx
    inx
.lAF2C:
    lda.b !scratch
    adc.w !front_boxes+2,x
    sec
    sbc.w !player+!actor_x+1
    cmp.w !scratch+4
.lAF38:
    sep.b #$20
    rtl

; Helpers no code refers to: each runs only in the unit calls of
; cov_helpers_unused.box.
!offset_points = $BBDF           ; (data bank) per entry: x offset along the facing, y offset (words)

; @routine 81:ABB4 ActorDirectionToPlayerOffset
; WHAT IT DOES: The direction from the actor to a point by the player
; (PlayerOffsetPoint), as ActorDirectionToPlayer gives it to the player.
; No code refers to it.
; HOW: PlayerOffsetPoint, then into ActorDirectionToPlayer after it puts
; the player's position at $10, $12 (81:ABFB).
; Entry: as PlayerOffsetPoint; called by JSL. Exit: as
; ActorDirectionToPlayer.
%at($81ABB4)
ActorDirectionToPlayerOffset:
    jsr.w PlayerOffsetPoint
    jmp.w ActorDirectionToPlayer_to_point

; @routine 81:ABBA ActorAimAtPlayerOffset
; WHAT IT DOES: The direction from the actor to a point by the player
; (PlayerOffsetPoint), as ActorAimAt gives it. No code refers to it.
; HOW: PlayerOffsetPoint, then on into ActorAimAt.
; Entry: as PlayerOffsetPoint; called by JSL. Exit: as ActorAimAt.
ActorAimAtPlayerOffset:
    jsr.w PlayerOffsetPoint
    jmp.w ActorAimAt

; @routine 81:ABC0 PlayerOffsetPoint
; WHAT IT DOES: The point at scratch $10, $12: the player's position
; offset by entry X of OFFSET_POINTS: across, added or taken away by the
; actor's flip, and down. No code refers to it.
; HOW: M and X 16-bit; the actor's flip bit 0 clear: the player's x plus
; the entry's x offset, set: less it; the player's y plus the entry's y
; offset.
; Entry: D = the actor; X = the entry's offset (high byte 0); DB = $81;
; called by JSR. Exit: M and X 16-bit; A = the point's y; N, V, Z, C from
; its ADC; X, Y unchanged.
PlayerOffsetPoint:
    rep.b #$30
    lda.b !actor_flip
    and.w #$0001
    bne .back
    clc
    lda.w !player+!actor_x+1
    adc.w !offset_points,x
    sta.w !scratch+4
    bra .y
.back:
    sec
    lda.w !player+!actor_x+1
    sbc.w !offset_points,x
    sta.w !scratch+4
.y:
    clc
    lda.w !player+!actor_y+1
    adc.w !offset_points+2,x
    sta.w !scratch+6
    rts

