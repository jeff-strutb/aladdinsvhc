; player_near.asm: where an actor is against the player, or a point.
;
; The actor is the record the direct page is; the player's is !player.
; Coordinates are the positions' whole pixels.
; C: src/aladdin/actor/player_near.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

; Boxes round an actor: a half-width (and a half-height) per entry.
!near_box_table = $81BC43       ; half-width, half-height: 4 bytes an entry
!near_point_x = !scratch+8      ; ActorNearPoint's point (words)
!near_point_y = !scratch+10
!aim_point_x = !scratch+4       ; ActorAimAt's point (words)
!aim_point_y = !scratch+6
!aim_steep = !scratch+2         ; the larger difference / 8
!aim_directions = $BC03         ; per octant 8 bytes, the direction of each step
!near_span_table = $81BC7B      ; half-width: 2 bytes an entry

; @routine 81:AB85 ActorSideOfPlayer
; WHAT IT DOES: Says which way the actor must face to look at the player.
; HOW: 0 (right) when the player's x is at or right of the actor's, else
; 1 (left).
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: A low byte and X = the facing; A high byte = the
; player's x high byte; N clear, Z from the facing; C from the compare;
; Y, V unchanged.
%at($81AB85)
ActorSideOfPlayer:
    rep.b #$20
    ldx.b #0
    sec
    lda.w !player+!actor_x+1
    cmp.b !actor_x+1
    bcs .right
    inx
.right:
    sep.b #$20
    txa
    rtl

; @routine 81:ACDE ActorNearPlayer
; WHAT IT DOES: Says whether the player is inside a box round the actor:
; within a half-width across and a half-height up or down.
; HOW: Takes box Y's half-width and half-height and keeps them and their
; doubles in scratch; x - player's x + half-width is below the width
; (unsigned) when within it across; the same for y, tested only when x
; passes.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the box's byte
; offset. Exit: C clear when inside; A = the last sum (16 bits); N, Z, C
; from its compare; scratch $0C-$13 = half-width, width, half-height,
; height; M 8-bit; X, Y, V unchanged.
%at($81ACDE)
ActorNearPlayer:
    rep.b #$20
    lda.w !near_box_table,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    lda.w !near_box_table+2,y
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    sec
    lda.b !actor_x+1
    sbc.w !player+!actor_x+1
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .out
    sec
    lda.b !actor_y+1
    sbc.w !player+!actor_y+1
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
.out:
    sep.b #$20
    rtl

; @routine 81:AD68 ActorNearPlayerX
; WHAT IT DOES: Says whether the player is within a half-width of the
; actor across, whatever the height.
; HOW: Takes span Y's half-width and keeps it and its double in scratch;
; x - player's x + half-width is below the width (unsigned) when within.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the span's byte
; offset. Exit: C clear when within; A = the sum (16 bits); N, Z, C from
; its compare; scratch $0C-$0F = half-width, width; M 8-bit; X, Y, V
; unchanged.
%at($81AD68)
ActorNearPlayerX:
    rep.b #$20
    lda.w !near_span_table,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    sec
    lda.b !actor_x+1
    sbc.w !player+!actor_x+1
    clc
    adc.w !scratch
    cmp.w !scratch+2
    sep.b #$20
    rtl

; @routine 81:AD17 ActorNearPoint
; WHAT IT DOES: Says whether the actor is inside a box round the point
; in scratch $14, $16: within a half-width across and a half-height up or
; down.
; HOW: Takes box Y's half-width and half-height and keeps them and their
; doubles in scratch; the actor's x - the point's x + half-width is below
; the width (unsigned) when within it across; the same for y, tested only
; when x passes.
; Entry: M 16-bit; D = the actor; DB = $81; Y = the box's byte offset;
; $14, $16 = the point (words); called by JSL. Exit: C clear when inside;
; A = the last sum; N, Z, C from its compare; scratch $0C-$13 =
; half-width, width, half-height, height; M 16-bit; X, Y, V unchanged.
%at($81AD17)
ActorNearPoint:
    lda.w !near_box_table,y
    sta.w !scratch
    asl a
    sta.w !scratch+2
    lda.w !near_box_table+2,y
    sta.w !scratch+4
    asl a
    sta.w !scratch+6
    sec
    lda.b !actor_x+1
    sbc.w !near_point_x
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .out
    sec
    lda.b !actor_y+1
    sbc.w !near_point_y
    clc
    adc.w !scratch+4
    cmp.w !scratch+6
.out:
    rtl

; @routine 81:AC68 ActorAimAt
; WHAT IT DOES: The direction from the actor to the point in scratch $10,
; $12: an entry of the direction table at $BC03.
; HOW: The differences across (point x - actor's x) and up or down, made
; positive; the octant from their signs (+4 the point above, +2 left) and
; which is larger (+1 up or down); the step in it from the smaller against
; the larger (under 1/8, 3/8, 5/8, 7/8 of it, else 4, the larger / 8 kept
; at $0E); the table's byte at the octant * 8 + the step.
; Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; $10, $12 = the point
; (words); called by JSL. Exit: M and X 8-bit; A low byte = the direction;
; X = the table offset, Y = the octant (low bytes); scratch $0C = the
; octant * 8, $0E = the larger difference / 8.
%at($81AC68)
ActorAimAt:
    ldy.w #$0000
    lda.w !aim_point_y
    sec
    sbc.b !actor_y+1
    bcs .down
    eor.w #$FFFF
    inc a
    ldy.w #$0004
.down:
    sta.w !scratch
    lda.w !aim_point_x
    sec
    sbc.b !actor_x+1
    bcs .right
    eor.w #$FFFF
    inc a
    iny
    iny
.right:
    cmp.w !scratch
    bcs .wide
    iny
    ldx.w !scratch
    sta.w !scratch
    txa
.wide:
    ldx.w #$0000
    lsr a
    lsr a
    lsr a
    sta.w !aim_steep
    cmp.w !scratch
    bcs .step
    inx
    adc.w !aim_steep
    adc.w !aim_steep
    cmp.w !scratch
    bcs .step
    inx
    adc.w !aim_steep
    adc.w !aim_steep
    cmp.w !scratch
    bcs .step
    inx
    adc.w !aim_steep
    adc.w !aim_steep
    cmp.w !scratch
    bcs .step
    inx
.step:
    tya
    asl a
    asl a
    asl a
    sta.w !scratch
    txa
    clc
    adc.w !scratch
    tax
    lda.w !aim_directions,x
    sep.b #$30
    rtl
