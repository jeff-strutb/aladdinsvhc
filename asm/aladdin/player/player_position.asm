; player_position.asm: Aladdin's position as objects set and test it.
; C: src/aladdin/player/player_position.c.

incsrc "../include/macros.inc"
incsrc "../include/object.inc"
incsrc "../include/wram_player.inc"

arch 65816

; @routine 83:8865 ObjectNearPlayer
; WHAT IT DOES: Tells whether Aladdin is within reach of the object: a
; box 26 pixels wide and 31 high placed by his position.
; HOW: Unsigned compares, so a difference below 0 counts as far: player
; X plus 18 (8 when $08F0 is set) less the object's X must be below 26,
; then player Y plus 2 less the object's Y below 31.
; Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL. Exit: as the C states.
%at($838865)
ObjectNearPlayer:
    rep.b #$21
    lda.w #18
    ldx.w !player_08f0
    beq .reach
    lda.w #8
.reach:
    adc.w !player_x
    sec
    sbc.b !obj_0c
    cmp.w #26
    bcs .done
    lda.w !player_y
    adc.w #2
    sec
    sbc.b !obj_0f
    cmp.w #31
.done:
    sep.b #$20
    rtl

; @routine 83:C8DD PlayerToObject
; WHAT IT DOES: Puts Aladdin at the object's position, 12 pixels below
; its Y, as an object that carries him does.
; HOW: The record's X and Y words to the player's, 12 added to Y.
; Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($83C8DD)
PlayerToObject:
    rep.b #$21
    lda.b !obj_0c
    sta.w !player_x
    lda.b !obj_0f
    clc
    adc.w #12
    sta.w !player_y
    sep.b #$20
    rts

; @routine 83:C8F0 Point145DToObject
; WHAT IT DOES: Sets the point at $145D to the object's position, 12
; pixels below its Y.
; HOW: The record's X and Y words to the point's, 12 added to Y.
; Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
Point145DToObject:
    rep.b #$21
    lda.b !obj_0c
    sta.w !point_145d
    lda.b !obj_0f
    clc
    adc.w #12
    sta.w !point_145f
    sep.b #$20
    rts
