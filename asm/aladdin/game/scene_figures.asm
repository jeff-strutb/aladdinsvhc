; scene_figures.asm: scripted figures: actor kind $93, which walks in once
; the scene's cue $00C2 is given, and the player started with $07 = 15,
; who stands at (128, 176) for a scene. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/game/scene_figures.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"

arch 65816

!SCENE_CUE_C2 = $C2  ; counted up by a scripted actor when it has waited; this one waits for it
!FIGURE_29 = $29  ; word: its picture list ($891F, $8907)
!V_145B = $145B
!V_1430 = $1430

; @routine A5:F3F0 Kind93WalkIn
; @actor
; WHAT IT DOES: Actor kind $93: stands facing left until the scene's cue
; (SCENE_CUE_C2), then walks left until its x is below 162, takes its
; second pose and stays.
; HOW: $09 bits $90 set, $2C = 2, redraw marked, $29 = $891F, facing and
; flip 1, animation 0; BRK until SCENE_CUE_C2; ActorSpeedXSet $72; each
; frame ActorMoveX while its x pixels' low byte is 162 or more; animation
; 1; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
; the actor loop's jump for kind $93 in state $0C (through the stub at
; $83:9389). Exit: never returns.
%at($A5F3F0)
Kind93WalkIn:
    lda.b #$90
    tsb.b !actor_09
    lda.b #$02
    sta.b !actor_2c
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    rep.b #$20
    lda.w #$891F
    sta.b !FIGURE_29
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.lF411:
    brk.b #$00
    lda.w !SCENE_CUE_C2
    beq .lF411
    ldy.b #$72
    jsl.l ActorSpeedXSet
.lF41E:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_x+1
    cmp.b #$A2
    bcs .lF41E
    lda.b #$01
    jsl.l AnimStart
.lF430:
    brk.b #$00
    bra .lF430


; @routine 81:E23E PlayerSceneStand
; @actor
; WHAT IT DOES: The player started with $07 = 15: Aladdin set at (128,
; 176) facing right for a scene, standing; after $20 frames V_142D is
; counted up, and he stays.
; HOW: $29 = $8907, x 128, y 176; V_145B = 1; facing and flip 0;
; PlayerMoveStart 8; V_1430 up; $09 = $D0, redraw marked, $06 cleared,
; TASK_PARAM cleared; COP $20; V_142D up; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
; the jump through the player's start table by $07 at $81:CB61 (81:CB56).
; Exit: never returns.
%at($81E23E)
PlayerSceneStand:
    rep.b #$20
    lda.w #$8907
    sta.b !FIGURE_29
    lda.w #$0080
    sta.b !actor_x+1
    lda.w #$00B0
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$01
    sta.w !V_145B
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$08
    jsr.w PlayerMoveStart
    inc.w !V_1430
    lda.b #$D0
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    stz.b !actor_06
    stz.w !task_param
    lda.b #$20
    cop.b #$00
    inc.w !v_142d
.lE276:
    brk.b #$00
    bra .lE276
