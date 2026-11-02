; player_scene_moves.asm: scripted figures of Aladdin (bank $A5: started at
; his place, their way picked by $07 through the table at $A5:E56D): the
; ways that run and leap, and that hop for good. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/game/player_scene_moves.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!SCENE_CUE = $146C  ; counted up by scripted actors as the scene goes on
!HOP_TIME = $42  ; frames left of its run

; @routine A5:EC45 PlayerSceneLeap
; @actor
; WHAT IT DOES: The scripted figure's way 2: it stands $20 frames, runs
; along the ground until it leaves it, then leaps; in the air until the
; scene's cue, then holds a pose until the next cue, and stays.
; HOW: Animation 0, COP $20; animation 2, ActorSpeedXFromA5 $0F; each
; frame ActorMoveX and ActorSnapToGround while Z is clear; SCENE_CUE
; cleared, animation $0C, gravity $38, ActorSpeedYFromA5 $12; each frame
; ActorFly until SCENE_CUE is set; it is cleared, animation $0D; BRK until
; SCENE_CUE is set; bit 7 of $09 cleared; COP $7F for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 2. Exit: never returns.
%at($A5EC45)
PlayerSceneLeap:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$0F
    jsr.w ActorSpeedXFromA5
.lEC5A:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    bne .lEC5A
    stz.w !SCENE_CUE
    lda.b #$0C
    jsl.l AnimStart
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$12
    jsr.w ActorSpeedYFromA5
.lEC78:
    brk.b #$00
    jsl.l ActorFly
    lda.w !SCENE_CUE
    beq .lEC78
    stz.w !SCENE_CUE
    lda.b #$0D
    jsl.l AnimStart
.lEC8C:
    brk.b #$00
    lda.w !SCENE_CUE
    beq .lEC8C
    lda.b #$80
    trb.b !actor_09
.lEC97:
    lda.b #$7F
    cop.b #$00
    bra .lEC97


; @routine A5:EC9D PlayerSceneHops
; @actor
; WHAT IT DOES: The scripted figure's way 3: it drops to the ground facing
; left, then for good: runs $30 frames along the ground, stands, holds a
; pose, and jumps up and lands again.
; HOW: $4D = 2, facing and flip 1, gravity $38, ActorSpeedYFromA5 6;
; ActorFall each frame until ActorOutOfGround. Then for good:
; ActorSpeedYFromA5 6, ActorSpeedXFromA5 $0C, animation 1, $30 frames of
; ActorMoveX and ActorSnapToGround; animation 0, COP $30; animation $0A,
; COP $10; animation 0, gravity $38, ActorSpeedYFromA5 $0C, ActorFall each
; frame until ActorOutOfGround.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 3. Exit: never returns.
%at($A5EC9D)
PlayerSceneHops:
    lda.b #$02
    sta.b !actor_4d
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
.lECB0:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .lECB0
.lECBC:
    ldy.b #$06
    jsr.w ActorSpeedYFromA5
    ldy.b #$0C
    jsr.w ActorSpeedXFromA5
    lda.b #$01
    jsl.l AnimStart
    lda.b #$30
    sta.b !HOP_TIME
.lECD0:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !HOP_TIME
    bne .lECD0
    lda.b #$00
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    lda.b #$0A
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    lda.b #$38
    sta.b !actor_accel_y
    ldy.b #$0C
    jsr.w ActorSpeedYFromA5
.lED01:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .lED01
    bra .lECBC
