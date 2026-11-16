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
!TILE_THROUGH = $40             ; behaviour bit: he passes through it
!ACTOR_PROBE_AT = $18           ; the probe the ground tests use
!V_0013 = $0013                 ; set to 1 instead while v_1428 is set
!PLAYER_CODE = $DADD            ; the player's code: where it starts over (this bank)
!PLAYER_CODE_LONG = $81DADD     ; the same, by a long jump

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

; @routine 81:DD01 PlayerOutOfWallsHurt
; WHAT IT DOES: Moves Aladdin out of a wall in front of him; when the
; ground last probed hurts him, puts him into state $8C and starts his
; code over.
; HOW: ActorOutOfWalls; ACTOR_08 bit 7 or bit 4 set, or TILE_BEHAVIOUR
; bit 5 clear: back. Else, a path no script runs: PlayerEnter8CLong, the
; call's return address dropped (PLA PLA) and on at PLAYER_CODE.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code at its own level (81:CD90, 81:D3E4). Exit:
; M and X as ActorOutOfWalls left them; A low, N and Z as the last test
; left them. Hurt: never returns.
%at($81DD01)
PlayerOutOfWallsHurt:
    jsl.l ActorOutOfWalls
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
    bmi PlayerOutOfWallsHurt_back
    bit.b #$10
    bne PlayerOutOfWallsHurt_back
    lda.w !TILE_BEHAVIOUR
    bit.b #!TILE_HURTS
    beq PlayerOutOfWallsHurt_back
    jsl.l PlayerEnter8CLong
    pla
    pla
    pla
    pla
    jmp.w !PLAYER_CODE

; @routine 81:DD50 PlayerLandOrHurt
; WHAT IT DOES: Stands Aladdin on a floor under him and stops his fall,
; unless the ground in front of him at probe $1C is ground he passes
; through; a floor that hurts him puts him into state $8C and starts his
; code over.
; HOW: ACTOR_PROBE_AT = $1C, GroundInFrontLong; in ground and
; TILE_BEHAVIOUR bit 6 set: A = 0. Else ActorSettleOnFloor; Z set (not
; settled), or ACTOR_08 bit 7 set: back. TILE_BEHAVIOUR bit 5 set, a path
; no script runs: PlayerEnter8CLong, this call's return address and its
; caller's dropped (four PLAs), and on at PLAYER_CODE. Else the low word
; of the vertical speed cleared and A = 1.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code (81:DB37) and from 81:D411. Exit: M 8-bit; A
; low = 1 when he landed, 0 when the ground in front is passed through,
; else as ActorSettleOnFloor or ACTOR_08 left it; N and Z from it; X as
; ActorSettleOnFloor left it. Hurt: never returns.
%at($81DD50)
PlayerLandOrHurt:
    lda.b #$1C
    sta.b !ACTOR_PROBE_AT
    jsl.l GroundInFrontLong
    bcc .settle
    lda.w !TILE_BEHAVIOUR
    bit.b #!TILE_THROUGH
    bne .through
.settle:
    jsl.l ActorSettleOnFloor
    beq .back
    lda.b !actor_08
    bmi .back
    lda.w !TILE_BEHAVIOUR
    bit.b #!TILE_HURTS
    beq .landed
    jsl.l PlayerEnter8CLong
    pla
    pla
    pla
    pla
    jml.l !PLAYER_CODE_LONG
.landed:
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    lda.b #$01
.back:
    rts
.through:
    lda.b #$00
    rts
