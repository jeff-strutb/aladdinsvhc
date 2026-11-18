; scene_figures_more.asm: two more of the ways bank $A5's start (A5:E515)
; sends an actor by its $07: the scripted figure's way 4 (dropped in,
; then a scene played to its cues), and way 11 (a guard placed at a spot,
; holding back until the player passes).
; C: src/aladdin/game/scene_figures_more.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!SF_SCENE_CUE = $146C           ; counted up by scripted actors as the scene goes on
!SF_CUE_BD = $00BD              ; the scene's cues
!SF_CUE_BE = $00BE
!SF_HURT_COUNT = $141B          ; nonzero while the player is hurt (his state $8C)
!SF_PLAYER_07 = !player+!actor_07 ; the player's $07
!SF_COUNT = $42                 ; frames left of a walk
!SF_STATE = $4D                 ; a guard's state (GuardWait's table)

; @routine A5:ED0F PlayerSceneDropIn
; @actor
; WHAT IT DOES: The scripted figure's way 4: it drops to the ground and
; waits, is put at its spot and leaps in, then plays its part of the
; scene by the cues: posed while the player is hurt, a leap and fall
; once cue $BD comes, a wait for the player's $07 to be $0D, a short
; walk, a pose until cue $BE, and walks off for good. No script meets
; it: written from its bytes.
; HOW: Each frame (BRK, animation rise or fall) ActorFall until
; ActorOutOfGround is nonzero. Animation 0, COP $80; ActorPlaceAtSpot
; with X = $24; facing and flip 0; ACTOR_08's low bits 3; animation $0C,
; preset $86; each frame (BRK) ActorAnimRiseFall and ActorFly until
; ActorOutOfGround nonzero; animation 0, COP $30. Then animation $17 each
; frame (BRK) while SF_HURT_COUNT is 0 (animation $18 each frame while it
; is not), flip = SF_SCENE_CUE, until SF_CUE_BD is set; facing and flip 0,
; preset $7B; each frame ActorFly until falling; ACTOR_08's low bits 1;
; each frame ActorFly until ActorOutOfGround nonzero; animation $19, COP
; $50; animation 0, BRK until the player's $07 is $0D; animation 1,
; ActorSpeedXFromA5 0, $40 frames of ActorMoveX; animation 0, BRK until
; SF_CUE_BE bit 7; animation $1A, COP $80; ActorSpeedXFromA5 $0F,
; animation 2, $28 frames of ActorMoveX; ActorSpeedXFromA5 $0C,
; animation 1, ActorMoveX each frame for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 4. Exit: never returns.
%at($A5ED0F)
PlayerSceneDropIn:
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    bne .down
    brk.b #$00
    jsr.w ActorAnimRiseFall
    bra PlayerSceneDropIn
.down:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$80
    cop.b #$00
    ldx.b #$24
    jsr.w ActorPlaceAtSpot
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_08
    and.b #$FC
    ora.b #$03
    sta.b !actor_08
    lda.b #$0C
    jsl.l AnimStart
    ldy.b #$86
    jsl.l ActorMotionPreset
.leap:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .leap
    lda.b #$00
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    bra .ready
.hurt:
    lda.b #$18
    jsl.l AnimStart
.hurt_wait:
    brk.b #$00
    lda.w !SF_HURT_COUNT
    bne .hurt_wait
.ready:
    lda.b #$17
    jsl.l AnimStart
.cue:
    brk.b #$00
    lda.w !SF_HURT_COUNT
    bne .hurt
    lda.w !SF_SCENE_CUE
    sta.b !actor_flip
    lda.w !SF_CUE_BD
    beq .cue
    stz.b !actor_facing
    stz.b !actor_flip
    ldy.b #$7B
    jsl.l ActorMotionPreset
.rise:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .rise
    lda.b !actor_08
    and.b #$FC
    ora.b #$01
    sta.b !actor_08
.fall:
    brk.b #$00
    jsr.w ActorAnimRiseFall
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .fall
    lda.b #$19
    jsl.l AnimStart
    lda.b #$50
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
.player:
    brk.b #$00
    lda.w !SF_PLAYER_07
    cmp.b #$0D
    bne .player
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$00
    jsr.w ActorSpeedXFromA5
    lda.b #$40
    sta.b !SF_COUNT
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !SF_COUNT
    bne .walk
    lda.b #$00
    jsl.l AnimStart
.cue_be:
    brk.b #$00
    lda.w !SF_CUE_BE
    bpl .cue_be
    lda.b #$1A
    jsl.l AnimStart
    lda.b #$80
    cop.b #$00
    ldy.b #$0F
    jsr.w ActorSpeedXFromA5
    lda.b #$02
    jsl.l AnimStart
    lda.b #$28
    sta.b !SF_COUNT
.run:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !SF_COUNT
    bne .run
    ldy.b #$0C
    jsr.w ActorSpeedXFromA5
    lda.b #$01
    jsl.l AnimStart
.off:
    brk.b #$00
    jsl.l ActorMoveX
    bra .off
%ends_by($A5EE29)

; @routine A5:F00E GuardPlacedHold
; @actor
; WHAT IT DOES: Bank $A5's start for $07 = 11: a guard put at spot $20,
; in state 4 (holding back while the player is to its left), stopped.
; No script meets it: written from its bytes.
; HOW: SF_STATE 4; ActorPlaceAtSpot with X = $20; AnimStart 0; on at
; GuardStop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the jump through the table at $A5:E56D for $07 = 11. Exit: never
; returns.
%at($A5F00E)
GuardPlacedHold:
    lda.b #$04
    sta.b !SF_STATE
    ldx.b #$20
    jsr.w ActorPlaceAtSpot
    lda.b #$00
    jsl.l AnimStart
    jmp.w GuardStop
%ends_by($A5F020)
