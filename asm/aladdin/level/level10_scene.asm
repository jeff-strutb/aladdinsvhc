; level10_scene.asm: level $10's task, which its screen set-up starts
; (Level10ScreenSetup, $81:8C14): it waits for Aladdin to reach a spot,
; then takes him over and moves the camera's limits on through a short
; scene, as level $0D's (level0d_scene.asm).
; C: src/aladdin/level/level10_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CUE_BD = $00BD                 ; the scene's cue, set by an actor
!LIMIT_1A40 = $1A40             ; two words the task sets as the scene goes on
!LIMIT_1A42 = $1A42
!V_083D = $083D
!V_1A10 = $1A10
!player_x = !player+!actor_x+1  ; Aladdin's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; and y (word)
!SCENE_CODE = $CB3B             ; the player's code word for the scene
!SCENE_SONG = $21

; @routine 80:A5AC Level10Scene
; WHAT IT DOES: Level $10's scene. Each frame until Aladdin is at x $1190
; or more with his y's low byte $A8 or more; then sound command $F6 $10
; is queued, every actor freed, he is given scripted code, the camera's
; limits move on and song $21 plays; on cue $BD the task queues $F6 $10
; again, waits 32 frames, sets the limits for the scene's end and ends.
; HOW: LIMIT_1A42 = $10A0 (byte by byte), CUE_BD 0. Each frame (TaskWait
; 1): until PLAYER_X >= $1190 (16-bit) and PLAYER_Y's low byte >= $A8.
; SoundQueueF6 $10; ActorsFreeAll; the player's $03 word SCENE_CODE, his
; $07 8; LIMIT_1A42 = $11A0; $083D 4, V_1A10 1; 64 frames; SongPlay
; SCENE_SONG. Each frame until CUE_BD; SoundQueueF6 $10; 32 frames;
; LIMIT_1A40 = $11A0, LIMIT_1A42's high byte $7F; V_1A10 and $083D 0;
; TaskExit.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
; task's entry (TaskCreate slot 3 at $81:8C25). Exit: never returns
; (TaskExit).
%at($80A5AC)
Level10Scene:
    lda.b #$A0
    sta.w !LIMIT_1A42
    lda.b #$10
    sta.w !LIMIT_1A42+1
    stz.w !CUE_BD
.until_spot:
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    lda.w !player_x
    cmp.w #$1190
    sep.b #$20
    bcc .until_spot
    lda.w !player_y
    cmp.b #$A8
    bcc .until_spot
    lda.b #$10
    jsl.l SoundQueueF6
    jsl.l ActorsFreeAll
    rep.b #$20
    lda.w #!SCENE_CODE
    sta.w !player+!actor_03
    sep.b #$20
    lda.b #$08
    sta.w !player+!actor_07
    lda.b #$A0
    sta.w !LIMIT_1A42
    lda.b #$11
    sta.w !LIMIT_1A42+1
    lda.b #$04
    sta.w !V_083D
    lda.b #$01
    sta.w !V_1A10
    lda.b #$40
    jsl.l TaskWait
    lda.b #!SCENE_SONG
    jsl.l SongPlay
.until_bd:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BD
    beq .until_bd
    lda.b #$10
    jsl.l SoundQueueF6
    lda.b #$20
    jsl.l TaskWait
    lda.b #$A0
    sta.w !LIMIT_1A40
    lda.b #$11
    sta.w !LIMIT_1A40+1
    lda.b #$7F
    sta.w !LIMIT_1A42+1
    stz.w !V_1A10
    stz.w !V_083D
    jml.l TaskExit
