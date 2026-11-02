; actor_anim_tile.asm: a bank $A5 actor's animation chosen by the tile it
; stands on and its speed.
; C: src/aladdin/actor/actor_anim_tile.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

!actor_anim = $2D               ; direct page (the actor): its animation

; Places not yet named keep their address as their name.
!v_0019 = $0019
!v_001d = $001D
!v_002c = $002C
!v_002d = $002D

; @routine A5:E9D2 ActorAnimByTile
; WHAT IT DOES: Picks the actor's animation: $16 on a tile with behaviour
; bit 5, else by its speed: 0 standing, 1 or 2 moving by the speed's sign
; once it is far from $1C0, else 1 unless already 2 or 4.
; HOW: Bit 5 of $0012: ActorAnimChange $16 (A5:EA13). Speed 0:
; ActorAnimChange 0. Else the speed less $1C0 at $002C: outside -16 to 15,
; 1 when its high byte is negative, else 2; inside, nothing when the
; animation ($2D) is 4 or 2, else 1.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
; as ActorAnimChange, or M 8-bit.
%at($A5E9D2)
ActorAnimByTile:
    lda.w !v_0012
    bit.b #$20
    bne .lEA13
    rep.b #$20
    lda.b !actor_speed_x
    beq .lEA0D
    sec
    sbc.w #$01C0
    sta.w !v_002c
    clc
    adc.w #$0010
    cmp.w #$0020
    bcc .lE9FC
    sep.b #$20
    lda.b #$01
    ldx.w !v_002d
    bmi ActorAnimChange
    lda.b #$02
    bra ActorAnimChange
.lE9FC:
    sep.b #$20
    ldx.b #$04
    cpx.b !actor_anim
    beq .lEA0C
    lda.b #$01
    ldx.b #$02
    cpx.b !actor_anim
    bne ActorAnimChange
.lEA0C:
    rts
.lEA0D:
    sep.b #$20
    lda.b #$00
    bra ActorAnimChange
.lEA13:
    lda.b #$16
