; camera_trigger.asm: a level's task (created at $80:8943) that waits for
; the camera to reach two places: at the first it frees the actors, at
; the second (the player alive) it sets the player and the escort going
; again and starts an actor of kind $6B.
; C: src/aladdin/level/camera_trigger.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!v_00bd = $00BD                 ; three bytes cleared as the task starts
!v_03fa = $03FA                 ; stepped up at the second place
!v_1a10 = $1A10                 ; stepped up at the second place
!HEALTH = $0367                 ; the player's health
!SPAWN_X = $002C                ; where ActorSpawnAt places the actor: x, then y (words)
!SPAWN_Y = $002E
!scroll_x_high = !layer0+!LAYER_SCROLL_X+1
!scroll_y_high = !layer0+!LAYER_SCROLL_Y+1

; @routine 80:A22D CameraTriggerTask
; WHAT IT DOES: Waits for the camera to reach x $0Bxx, y $02xx and frees
; every actor; then waits for x $0Fxx with the player alive, gives the
; escort and the player new code, starts an actor of kind $6B at ($0F38,
; $03B0) and queues sound command $F6 with parameter 2, and ends.
; HOW: $00BD-$00BF cleared. Each frame (TaskWait 1) until the high bytes
; of the scroll x and y are $0B and 2: ActorsFreeAll. Each frame until
; the scroll x's high byte is $0F and the health nonzero: $03FA up; the
; escort's $03 word $E554 and $07 5; $1A10 up; ActorsFreeAll; the
; player's $07 9 and $03 word $CB3B; ActorSpawnAt of kind $6B (its $07
; 1) at the spawn words; SoundQueueF6 2; TaskExit.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
; task's entry. Exit: never returns (TaskExit).
%at($80A22D)
CameraTriggerTask:
    stz.w !v_00bd
    stz.w !v_00bd+1
    stz.w !v_00bd+2
.first:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_x_high
    cmp.b #$0B
    bne .first
    lda.w !scroll_y_high
    cmp.b #$02
    bne .first
    jsl.l ActorsFreeAll
.second:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_x_high
    cmp.b #$0F
    bne .second
    lda.w !HEALTH
    beq .second
    inc.w !v_03fa
    rep.b #$20
    lda.w #$E554
    sta.w !actor_escort+!actor_03
    sep.b #$20
    lda.b #$05
    sta.w !actor_escort+!actor_07
    inc.w !v_1a10
    jsl.l ActorsFreeAll
    lda.b #$09
    sta.w !player+!actor_07
    rep.b #$20
    lda.w #$CB3B
    sta.w !player+!actor_03
    lda.w #$0F38
    sta.w !SPAWN_X
    lda.w #$03B0
    sta.w !SPAWN_Y
    sep.b #$20
    lda.b #$6B
    xba
    lda.b #$01
    jsr.w ActorSpawnAt
    sep.b #$30
    lda.b #$02
    jsl.l SoundQueueF6
    jml.l TaskExit
