; hurt_ground.asm: ground that hurts Aladdin: a tile whose behaviour has
; bit 5 set puts him into state $8C (hurt) and his code starts over.
; C: src/aladdin/player/hurt_ground.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/actor.inc"

arch 65816

!TILE_BEHAVIOUR = $0012         ; the last probed tile's behaviour (bit 5: it hurts)
!TILE_HURTS = $20
!V_0013 = $0013                 ; set to 1 instead while v_1428 is set
!PLAYER_CODE = $DADD            ; the player's code: where it starts over (this bank)
HurtTestBack = $81DD1D         ; the RTS ending 81:DD01, just before PlayerGroundHurtTest

; @routine 81:DCBC PlayerGroundSnapHurt
; WHAT IT DOES: Stands Aladdin on the ground just under him, and when that
; ground hurts him, puts him into state $8C and starts his code over.
; With V_1428 set, only sets V_0013 to 1.
; HOW: V_1428 nonzero: V_0013 = 1. Else ActorSnapToGround; ACTOR_08 bit 7
; or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back. Else
; PlayerEnter8CLong, the call's return address dropped (PLA PLA) and on
; at PLAYER_CODE.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
; JSR from the player's code at its own level (81:CCD4, 81:CD93). Exit: M
; and X 8-bit; A low, N and Z as the last test left them, or A = 1. Hurt:
; never returns.
%at($81DCBC)
PlayerGroundSnapHurt:
    lda.w !v_1428
    bne .set_0013
    jsl.l ActorSnapToGround
    lda.b !actor_08
    bmi .back
    bit.b #$10
    bne .back
    lda.w !TILE_BEHAVIOUR
    bit.b #!TILE_HURTS
    beq .back
    jsl.l PlayerEnter8CLong
    pla
    pla
    jmp.w !PLAYER_CODE
.back:
    rts
.set_0013:
    lda.b #$01
    sta.w !V_0013
    rts

; @routine 81:DD1E PlayerGroundHurtTest
; WHAT IT DOES: When the ground last probed hurts Aladdin, puts him into
; state $8C and starts his code over.
; HOW: ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR bit 5 clear: back
; (by the RTS that ends 81:DD01). Else PlayerEnter8CLong, the return addresses of this call and its
; caller's dropped (four PLAs) and on at PLAYER_CODE.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
; JSR from 81:D418, itself called by JSR from the player's code. Exit: M
; and X 8-bit; A low, N and Z as the last test left them. Hurt: never
; returns.
%at($81DD1E)
PlayerGroundHurtTest:
    lda.b !actor_08
    bmi HurtTestBack
    bit.b #$10
    bne HurtTestBack
    lda.w !TILE_BEHAVIOUR
    bit.b #!TILE_HURTS
    beq HurtTestBack
    jsl.l PlayerEnter8CLong
    pla
    pla
    pla
    pla
    jmp.w !PLAYER_CODE
