; guard_scene.asm: the scene code kind $92 sends a guard or escort to ($A5:E826, set in its resume address by the start at $A5:F5BA):
; by its $47 (the kind's $07) one of three walks, each ending in GuardStop. cov_escort_scene plays them.
; C: src/aladdin/actor/guard_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!SCENE_COUNTER = $146F          ; the scene's frame counter: $2C ends a wait

; @routine A5:E826 GuardSceneStart
; @actor
; WHAT IT DOES: Where the actor goes on after kind $92 set its resume
; address: by ACTOR_47 (the kind's $07, 0 to 3) to GuardSceneWalkOut (0),
; GuardSceneCharge (1) or GuardSceneEnd (2 and 3).
; HOW: A = ACTOR_47 times 2, X = A; JMP through the word table that
; follows (GuardSceneWalkOut, GuardSceneCharge, GuardSceneEnd,
; GuardSceneEnd).
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's RTI at the resume address. Exit: never returns.
%at($A5E826)
GuardSceneStart:
    lda.b !actor_47
    asl a
    tax
    jmp.w (.table,x)
.table:
    dw GuardSceneWalkOut, GuardSceneCharge, GuardSceneEnd, GuardSceneEnd

; @routine A5:E835 GuardSceneWalkOut
; @actor
; WHAT IT DOES: Scene 0: the actor stands (animation 0) until its counter
; reaches $2C or Aladdin is $90 ahead, then drops and walks out in two
; legs: animation $0B falling to y $BC, animation $16 by motion $85 to
; x $0D00, then by motions $7D, $7C, $7C and $7B through four falls, and
; again the wait, the drop to y $58, the walk to x $0F00 and past $A0 in
; its low x byte, and a last fall; then it ends as GuardStop with
; ACTOR_41 0 and ACTOR_4D 4.
; HOW: ACTOR_41 up; AnimStart 0; SCENE_COUNTER 0; each frame (BRK first)
; GuardSceneAhead until C is clear; ActorSpeedYFromA5 with Y = $12; facing
; and flip 0; AnimStart $0B; each frame ActorMoveY until y's high byte is
; 0 and its low byte under $BC is not (below it); AnimStart $16, motion $85,
; each frame ActorMove while x's top byte is under $0D; motion $7D, then
; a fall each: ActorAnimRiseFall, ActorFly, ActorOutOfGround (until it is
; on ground with ACTOR_SPEED_Y's top byte not negative) for motions $7D,
; $7C, $7C, then facing and flip 1 and $7B; the wait again, the drop to $58,
; x top byte $0F and low byte $A0, motion $7B and a last fall; ACTOR_41 0,
; ACTOR_4D 4, on at GuardStop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
; from GuardSceneStart. Exit: never returns.
%at($A5E835)
GuardSceneWalkOut:
    inc.b !actor_41
    lda.b #$00
    jsl.l AnimStart
    stz.w !SCENE_COUNTER
.lE840:
    brk.b #$00
    jsr.w GuardSceneAhead
    bcs .lE840
    ldy.b #$12
    jsr.w ActorSpeedYFromA5
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$0B
    jsl.l AnimStart
.lE856:
    brk.b #$00
    jsl.l ActorMoveY
    lda.b !actor_y+2
    bne .lE856
    lda.b !actor_y+1
    cmp.b #$BC
    bcs .lE856
    lda.b #$16
    jsl.l AnimStart
    ldy.b #$85
    jsl.l ActorMotionPreset
.lE872:
    brk.b #$00
    jsl.l ActorMove
    lda.b !actor_x+2
    cmp.b #$0D
    bcc .lE872
    ldy.b #$7D
    jsl.l ActorMotionPreset
.lE884:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE884
    jsl.l ActorOutOfGround
    beq .lE884
    ldy.b #$7C
    jsl.l ActorMotionPreset
.lE89D:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE89D
    jsl.l ActorOutOfGround
    beq .lE89D
    ldy.b #$7C
    jsl.l ActorMotionPreset
.lE8B6:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE8B6
    jsl.l ActorOutOfGround
    beq .lE8B6
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    ldy.b #$7B
    jsl.l ActorMotionPreset
.lE8D5:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE8D5
    jsl.l ActorOutOfGround
    beq .lE8D5
    lda.b #$00
    jsl.l AnimStart
    stz.w !SCENE_COUNTER
.lE8F1:
    brk.b #$00
    jsr.w GuardSceneAhead
    bcs .lE8F1
    ldy.b #$12
    jsr.w ActorSpeedYFromA5
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$0B
    jsl.l AnimStart
.lE907:
    brk.b #$00
    jsl.l ActorMoveY
    lda.b !actor_y+2
    bne .lE907
    lda.b !actor_y+1
    cmp.b #$58
    bcs .lE907
    lda.b #$16
    jsl.l AnimStart
    ldy.b #$85
    jsl.l ActorMotionPreset
.lE923:
    brk.b #$00
    jsl.l ActorMove
    lda.b !actor_x+2
    cmp.b #$0F
    bcc .lE923
    lda.b !actor_x+1
    cmp.b #$A0
    bcc .lE923
    ldy.b #$7B
    jsl.l ActorMotionPreset
.lE93B:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE93B
    jsl.l ActorOutOfGround
    beq .lE93B
    stz.b !actor_41
    lda.b #$04
    sta.b !actor_4d
    jmp.w GuardStop

; @routine A5:E957 GuardSceneAhead
; WHAT IT DOES: The scene's wait test: C clear when SCENE_COUNTER is $2C
; or the actor's x + $90 is below Aladdin's x.
; HOW: SCENE_COUNTER against $2C, equal: C clear, RTS; else (16-bit) the
; actor's x + $90 (carry as it stands) against the player's x: C clear
; when below.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; C as above; M 8-bit.
%at($A5E957)
GuardSceneAhead:
    lda.w !SCENE_COUNTER
    cmp.b #$2C
    clc
    beq .lE96B
    rep.b #$20
    lda.b !actor_x+1
    adc.w #$0090
    cmp.w !player_x
    sep.b #$20
.lE96B:
    rts

; @routine A5:E96C GuardSceneCharge
; @actor
; WHAT IT DOES: Scene 1: the actor charges left by motion $88 for $C0
; frames, flies by $87 while falling, flies against by $88 until it
; falls again, flies against 256 frames more, and goes on into
; GuardSceneEnd.
; HOW: ACTOR_08 low two bits set; ACTOR_41 up; AnimStart $1B; facing and
; flip 1; ACTOR_42 $C0; motion $88, each frame ActorMove, 192 times;
; motion $87, each frame ActorFly while ACTOR_SPEED_Y's top byte is
; negative; motion $88, each frame ActorFlyAgainst while it is not
; negative; ACTOR_42 0, each frame ActorFlyAgainst 256 times; then on into
; GuardSceneEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
; from GuardSceneStart. Exit: never returns.
%at($A5E96C)
GuardSceneCharge:
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    inc.b !actor_41
    lda.b #$1B
    jsl.l AnimStart
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$C0
    sta.b !actor_42
    ldy.b #$88
    jsl.l ActorMotionPreset
.lE98A:
    brk.b #$00
    jsl.l ActorMove
    dec.b !actor_42
    bne .lE98A
    ldy.b #$87
    jsl.l ActorMotionPreset
.lE99A:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .lE99A
    ldy.b #$88
    jsl.l ActorMotionPreset
.lE9AA:
    brk.b #$00
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .lE9AA
    stz.b !actor_42
.lE9B6:
    brk.b #$00
    jsl.l ActorFlyAgainst
    dec.b !actor_42
    bne .lE9B6


; @routine A5:E9C0 GuardSceneEnd
; @actor
; WHAT IT DOES: The end of the scene: the actor goes on as GuardStop with
; ACTOR_08's low bits 1, ACTOR_41 and ACTOR_4D 0.
; HOW: ACTOR_08 AND 3 OR 1; ACTOR_41 and ACTOR_4D 0; on at GuardStop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the jump
; from GuardSceneStart, or by GuardSceneCharge falling into it. Exit:
; never returns.
%at($A5E9C0)
GuardSceneEnd:
    lda.b !actor_08
    and.b #$03
    ora.b #$01
    sta.b !actor_08
    stz.b !actor_41
    stz.b !actor_4d
    jmp.w GuardStop
