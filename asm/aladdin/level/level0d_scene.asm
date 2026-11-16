; level0d_scene.asm: level $0D's task, which its screen set-up starts
; ($81:8BA7, through the table by level at $81:8802): it keeps Aladdin
; from rising too high until he reaches a spot, then takes him over and
; moves the camera's limits on through a short scene.
; C: src/aladdin/level/level0d_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!CUE_BD = $00BD                 ; the scene's cue, set by an actor
!CUE_BE = $00BE                 ; set by the task once the camera has moved on
!LIMIT_1A40 = $1A40             ; two words the task sets as the scene goes on ($0A00, then $0900 and $7FFF at $1A42)
!LIMIT_1A42 = $1A42
!V_083D = $083D
!scroll_x_high = !layer0+!LAYER_SCROLL_X+1  ; the play field's scroll x, high byte
!player_x = !player+!actor_x+1  ; Aladdin's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; and y (word)
!HEALTH = $0367                 ; the player's health

; @routine 80:A501 Level0DScene
; WHAT IT DOES: Level $0D's scene. Each frame Aladdin is kept from going
; above y $18 (when his y's low byte is $18 or less it is set to $0018)
; until he, alive, is at x $09F0 or more and y $78 or more; then he is given
; scripted code, sound command $F6 2 is queued, the camera moves on until
; the scroll x reaches $0Axx, song $25 plays, and on cue $BD the task
; waits 240 frames, queues $F6 2 again, waits 192 frames, opens the
; limits ($1A42 = $7FFF) and ends.
; HOW: X 16-bit; LIMIT_1A42 = $0900, CUE_BD and CUE_BE 0. Each frame
; (TaskWait 1): PLAYER_Y's low byte $18 or less: PLAYER_Y = $0018; until
; PLAYER_X >= $09F0, PLAYER_Y >= $0078 and HEALTH nonzero. X 8-bit; the
; player's $03 word $CB3B and $07 5; SoundQueueF6 2; $1A43 $0A; $083D 4.
; Each frame until SCROLL_X_HIGH >= $0A: CUE_BE up, $083D 0, LIMIT_1A40 =
; $0A00; 1 frame; SongPlay $25. Each frame until CUE_BD: 240 frames,
; SoundQueueF6 2, 192 frames; LIMIT_1A42 = $7FFF; TaskExit.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
; task's entry (TaskCreate slot $48 at $81:8BA7). Exit: never returns
; (TaskExit).
%at($80A501)
Level0DScene:
    rep.b #$10
    lda.b #$09
    sta.w !LIMIT_1A42+1
    stz.w !LIMIT_1A42
    stz.w !CUE_BD
    stz.w !CUE_BE
.kept_low:
    lda.b #$01
    jsl.l TaskWait
    lda.b #$18
    cmp.w !player_y
    bcc .at_spot
    sta.w !player_y
    stz.w !player_y+1
.at_spot:
    ldx.w !player_x
    cpx.w #$09F0
    bcc .kept_low
    ldx.w !player_y
    cpx.w #$0078
    bcc .kept_low
    lda.w !HEALTH
    beq .kept_low
    sep.b #$10
    rep.b #$20
    lda.w #$CB3B
    sta.w !player+!actor_03
    sep.b #$20
    lda.b #$05
    sta.w !player+!actor_07
    lda.b #$02
    jsl.l SoundQueueF6
    lda.b #$0A
    sta.w !LIMIT_1A42+1
    lda.b #$04
    sta.w !V_083D
.camera_on:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_x_high
    cmp.b #$0A
    bcc .camera_on
    inc.w !CUE_BE
    stz.w !V_083D
    lda.b #$0A
    sta.w !LIMIT_1A40+1
    stz.w !LIMIT_1A40
    lda.b #$01
    jsl.l TaskWait
    lda.b #$25
    jsl.l SongPlay
.until_bd:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BD
    beq .until_bd
    lda.b #$F0
    jsl.l TaskWait
    lda.b #$02
    jsl.l SoundQueueF6
    lda.b #$C0
    jsl.l TaskWait
    lda.b #$FF
    sta.w !LIMIT_1A42
    lda.b #$7F
    sta.w !LIMIT_1A42+1
    jml.l TaskExit
