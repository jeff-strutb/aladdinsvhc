; kind3d.asm: actor kind $3D, a thing the player can stand on and ride:
; its contact with him each frame it waits. Written from the bytes where
; no script runs them; named and described by hand.
; C: src/aladdin/actor/kind3d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K3D_X_WAS = $41                 ; K3D_X_COUNT as it was the frame before (word)
!K3D_Y_WAS = $43                 ; K3D_Y_COUNT as it was (word)
!K3D_X_COUNT = $45               ; nonzero: the player within its box in x (word)
!K3D_Y_COUNT = $47               ; in y (word)
!K3D_RIDDEN = $49                ; nonzero: the player rides it
!K3D_4B = $4B                    ; nonzero: touching it from below does not hurt
!K3D_BELOW = $003D               ; his y below it, pushed under
!K3D_ON = $003B                  ; his y above it, standing on it
!K3D_LEFT = $0010                ; his x left of it, pushed off
!K3D_RIGHT = $0011               ; right of it

; @routine 83:C3E4 Kind3DPlayerContact
; WHAT IT DOES: Actor kind $3D's contact with the player: once he comes
; within its box from above he stands on it and rides it (it carries him
; with its moves); from below he is pushed under it, and hurt when its $07
; is nonzero and its $4B zero; touching it from the side he is pushed
; 16 pixels off to its left or 17 to its right.
; HOW: K3D_X_COUNT and K3D_Y_COUNT (counted by ActorPlayerBoxCount) moved
; to K3D_X_WAS and K3D_Y_WAS and cleared. Not ridden (K3D_RIDDEN 0):
; ActorPlayerBoxCount; within on both axes: unless within in y the frame
; before, its y pixels below his: his y = its y + $3D, PlayerEnter8CFacing
; as above, then (V_141C nonzero) K3D_Y_COUNT cleared, or on to the side
; push; not below: his y = its y - $3B, V_1428 and K3D_RIDDEN 1, V_141C 0.
; The side push: his x = its x - 16 when its x is not below his, else its
; x + 17; K3D_Y_COUNT up. Ridden: PlayerMoveWithActor,
; PlayerEnter8CFacing, ActorPlayerBoxCount; V_141C zero and within on both
; axes: V_1428 up; else K3D_RIDDEN 0. No script runs the push, the landing
; or the ride: written from their bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: M 8-bit, X 8-bit; as the
; last step leaves A and the flags.
%at($83C3E4)
Kind3DPlayerContact:
    rep.b #$20
    lda.b !K3D_X_COUNT
    sta.b !K3D_X_WAS
    lda.b !K3D_Y_COUNT
    sta.b !K3D_Y_WAS
    stz.b !K3D_X_COUNT
    stz.b !K3D_Y_COUNT
    ldx.b !K3D_RIDDEN
    bne .ridden
    jsr.w ActorPlayerBoxCount
    lda.b !K3D_X_COUNT
    beq .out
    lda.b !K3D_Y_COUNT
    beq .out
    ldx.b !K3D_Y_WAS
    bne .side
    rep.b #$20
    lda.b !actor_y+1
    cmp.w !player+!actor_y+1
    bcs .on
    adc.w #!K3D_BELOW
    sta.w !player+!actor_y+1
    sep.b #$20
    lda.b !actor_07
    beq .hurtDone
    lda.b !K3D_4B
    bne .hurtDone
    jsr.w PlayerEnter8CFacing
.hurtDone:
    rep.b #$20
    ldx.w !v_141c
    beq .side
    stz.b !K3D_Y_COUNT
    bra .out
.on:
    sec
    sbc.w #!K3D_ON
    sta.w !player+!actor_y+1
    ldx.b #$01
    stx.w !v_1428
    stx.b !K3D_RIDDEN
    ldx.b #$00
    stx.w !v_141c
    bra .out
.side:
    lda.b !actor_x+1
    cmp.w !player+!actor_x+1
    bcc .right
    sbc.w #!K3D_LEFT
    bra .pushed
.right:
    adc.w #!K3D_RIGHT
.pushed:
    sta.w !player+!actor_x+1
    inc.b !K3D_Y_COUNT
.out:
    sep.b #$20
    rts
.ridden:
    sep.b #$20
    jsl.l PlayerMoveWithActor
    jsr.w PlayerEnter8CFacing
    rep.b #$20
    jsr.w ActorPlayerBoxCount
    ldy.w !v_141c
    bne .off
    lda.b !K3D_X_COUNT
    beq .off
    lda.b !K3D_Y_COUNT
    beq .off
    sep.b #$20
    inc.w !v_1428
    rts
.off:
    sep.b #$20
    stz.b !K3D_RIDDEN
    rts

; @routine 83:C4A4 Kind3DWait
; WHAT IT DOES: What actor kind $3D runs each frame it waits: its contact
; with the player, then a shot that hits it turned back.
; HOW: Kind3DPlayerContact, ActorTurnsShotBack.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table by kind ($83:8F4F). Exit: as ActorTurnsShotBack.
%at($83C4A4)
Kind3DWait:
    jsr.w Kind3DPlayerContact
    jsr.w ActorTurnsShotBack
    rts
