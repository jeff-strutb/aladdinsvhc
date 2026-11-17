; player_frame.asm: the player's work each frame his actor waits: his
; move's animation started when the move changes, the hurt time counted
; down, the view-edge rule run, his picture sent, the fall out of the
; level tested, and the test box set. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/player/player_frame.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"

arch 65816

!PFW_1435 = $1435               ; cleared at the start of the player's frame work
; Nonzero: a scene moves the player, and only his animation and picture
; are kept up; bit 6 set: not his animation; bit 7 set: not while
; PFW_081E is 0.
!PFW_SCENE = $1430
!PFW_081E = $081E
!PFW_MOVE_WAS = $141F           ; the move whose animation was last started
!PFW_1A0A = $1A0A               ; nonzero: an index into PFW_03F1_VALUES
!PFW_03F1 = $03F1               ; set from PFW_03F1_VALUES by PFW_1A0A
!PFW_03F1_VALUES = $C7E8        ; in bank $80: bytes
!PFW_FLAGS = $0347              ; nonzero: bit 7 of the player's $08 set
!PFW_142C = $142C               ; cleared each frame with v_1428 and v_1434
!PFW_NO_FALL_1 = $07            ; levels where falling out of the bottom does not hurt
!PFW_NO_FALL_2 = $12

; PlayerFrameWork's path while a scene moves the player (it branches here
; from its third instruction).
%at($81E84D)
PlayerFrameWork_scene:
    bpl .animate
    lda.w !PFW_081E
    beq .picture
.animate:
    bit.w !PFW_SCENE
    bvs .picture
    jsl.l AnimStep
.picture:
    jsl.l ActorPictureSend
    jmp.w TestBoxPlace

; @routine 81:E864 PlayerFrameWork
; WHAT IT DOES: The player's work each frame his actor waits: starts his
; move's animation when the move has changed, counts down the time he
; cannot be hurt, steps his animation, runs the view-edge rule, sends his
; picture, hurts him when he falls out of the bottom of the level, and
; sets the test box from his box. While a scene moves him (PFW_SCENE
; nonzero), only his animation and picture are kept up and the test box
; placed.
; HOW: PFW_1435 cleared. PFW_SCENE nonzero (the code at $81:E84D, before
; the entry): bit 7 set with PFW_081E 0, or bit 6 set, skips AnimStep;
; then ActorPictureSend and a jump to TestBoxPlace. Otherwise:
; PLAYER_MOVE unlike PFW_MOVE_WAS: kept there, PlayerMoveStart with it;
; PlayerHurtCountdown; AnimStep unless V_142D; ViewEdgeRuleRun;
; ActorPictureSend; PFW_1A0A nonzero: PFW_03F1 from PFW_03F1_VALUES by it.
; With health left, on a level but 7 and $12, PlayerFallenTest clear (he
; has fallen): bit 7 of his $09 cleared, PlayerEnter8CFrom1Long. Bit 7 of
; his $08 set from PFW_FLAGS nonzero, else cleared; V_1428, V_1434 and
; PFW_142C cleared; PlayerBoxUpdate, and on into TestBoxPlace. No script
; runs bit 7 of PFW_SCENE set (the read of PFW_081E): that path is
; written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSL (ActorKind00Wait). Exit: as TestBoxPlace leaves them (M and X 8-bit;
; A = the test box's y; its RTL is this routine's).
PlayerFrameWork:
    stz.w !PFW_1435
    lda.w !PFW_SCENE
    bne PlayerFrameWork_scene
    lda.w !player_move
    cmp.w !PFW_MOVE_WAS
    beq .moveKept
    sta.w !PFW_MOVE_WAS
    lda.w !player_move
    jsr.w PlayerMoveStart
.moveKept:
    jsr.w PlayerHurtCountdown
    lda.w !v_142d
    bne .animated
    jsl.l AnimStep
.animated:
    jsr.w ViewEdgeRuleRun
    jsl.l ActorPictureSend
    ldx.w !PFW_1A0A
    beq .fallTest
    lda.w !PFW_03F1_VALUES,x
    sta.w !PFW_03F1
.fallTest:
    lda.w !health
    beq .flags
    lda.w !level
    cmp.b #!PFW_NO_FALL_1
    beq .flags
    cmp.b #!PFW_NO_FALL_2
    beq .flags
    jsr.w PlayerFallenTest
    bcs .flags
    lda.b !actor_09
    and.b #$7F
    sta.b !actor_09
    jsl.l PlayerEnter8CFrom1Long
.flags:
    lda.b !actor_08
    and.b #$7F
    ldx.w !PFW_FLAGS
    beq .keep08
    lda.b #$80
    ora.b !actor_08
.keep08:
    sta.b !actor_08
    stz.w !v_1428
    stz.w !v_1434
    stz.w !PFW_142C
    jsr.w PlayerBoxUpdate
