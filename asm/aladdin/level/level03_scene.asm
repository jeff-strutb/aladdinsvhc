; level03_scene.asm: level 3's task, which its screen set-up starts
; (Level03ScreenSetup, $81:8908): it waits for Aladdin at a spot, takes him
; over and moves the camera's limits through the scene, as level $0D's and
; $10's (level0d_scene.asm, level10_scene.asm), with a copier check and a
; checksum of its own bytes.
; C: src/aladdin/level/level03_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!CUE_BD = $00BD                 ; the scene's cues, set by its actors
!CUE_BE = $00BE
!CUE_BF = $00BF
!SCENE_SUM = $00C0              ; the checksum of the task's bytes, less $EC (0 when untouched)
!LIMIT_1A40 = $1A40             ; the camera's limits the task moves on
!LIMIT_1A42 = $1A42
!V_083D = $083D
!SCENE_ON = $03FA               ; counted up as the scene ends
!V_141C = $141C
!HEALTH = $0367                 ; the player's health
!player_x = !player+!actor_x+1  ; Aladdin's x pixels (word; the low byte tested)
!player_y = !player+!actor_y+1  ; his y pixels
!scroll_x_high = !layer0+!LAYER_SCROLL_X+1  ; the play field's scroll x, high byte
!COPIER_COUNT = $700F10         ; a byte a copier's cartridge RAM keeps (a retail cartridge has none)
!SCENE_CODE = $CB3B             ; the player's code word for the scene
!COMPANION_CODE = $E554         ; the escort actor's code word for the scene
!SUM_BYTES = 15                 ; the bytes summed from Level03Scene_sum_from

; @routine 80:A12D Level03Scene
; WHAT IT DOES: Level 3's scene. LIMIT_1A42's high byte 4, its low 0, the
; cues and SCENE_SUM 0; each frame until the screen is past $04xx and
; Aladdin, alive, stands at y $B0 with x $F0 or more (low bytes): sound
; command $F6 2; then unless a copier's RAM keeps its count, every actor
; freed, his and the companion's code words set, his $07 8, theirs 4, the
; limit 5 and $083D 4. Each frame until the screen is past $05xx; the
; limit's high byte at $1A41 5; 48 frames; song $21; SCENE_SUM = the sum of
; its own 15 bytes from 80:A167, less $EC. Each frame until CUE_BD; sound
; command $F6 2; 128 frames; each frame until x $F0 or more, V_141C 0 and
; y $B0 or more: SCENE_ON up, the limit's high byte $7F, his code word
; again, his $07 $0D; each frame until CUE_BE; song $1E; TaskExit.
; HOW: As above; the copier check: COPIER_COUNT up (long) and compared:
; equal (the RAM kept it) skips to the second wait. The sum: X 0 to 14,
; from A = 0, CLC and ADC each byte (long, X); SEC, SBC #$EC.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
; task's entry (TaskCreate slot 3 at $81:8917). Exit: never returns
; (TaskExit).
%at($80A12D)
Level03Scene:
    lda.b #$04
    sta.w !LIMIT_1A42+1
    stz.w !LIMIT_1A42
    stz.w !CUE_BD
    stz.w !CUE_BE
    stz.w !CUE_BF
    stz.w !SCENE_SUM
.to_spot:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_x_high
    cmp.b #$04
    bcc .to_spot
    lda.w !player_y
    cmp.b #$B0
    bne .to_spot
    lda.w !player_x
    cmp.b #$F0
    bcc .to_spot
    lda.w !HEALTH
    beq .to_spot
    lda.b #$02
    jsl.l SoundQueueF6
.sum_from:
    lda.l !COPIER_COUNT
    inc
    sta.l !COPIER_COUNT
    cmp.l !COPIER_COUNT
    beq .screen_5
    jsl.l ActorsFreeAll
    rep.b #$20
    lda.w #!SCENE_CODE
    sta.w !player+!actor_03
    lda.w #!COMPANION_CODE
    sta.w !actor_escort+!actor_03
    sep.b #$20
    lda.b #$08
    sta.w !player+!actor_07
    lda.b #$04
    sta.w !actor_escort+!actor_07
    lda.b #$05
    sta.w !LIMIT_1A42+1
    lda.b #$04
    sta.w !V_083D
.screen_5:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_x_high
    cmp.b #$05
    bcc .screen_5
    lda.b #$05
    sta.w !LIMIT_1A40+1
    lda.b #$30
    jsl.l TaskWait
    lda.b #$21
    jsl.l SongPlay
    ldx.b #$00
    ldy.b #!SUM_BYTES
    lda.b #$00
.sum:
    clc
    adc.l .sum_from,x
    inx
    dey
    bne .sum
    sec
    sbc.b #$EC
    sta.w !SCENE_SUM
.until_bd:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BD
    beq .until_bd
    lda.b #$02
    jsl.l SoundQueueF6
    lda.b #$80
    jsl.l TaskWait
.to_end:
    lda.b #$01
    jsl.l TaskWait
    lda.w !player_x
    cmp.b #$F0
    bcc .to_end
    lda.w !V_141C
    bne .to_end
    lda.w !player_y
    cmp.b #$B0
    bcc .to_end
    inc.w !SCENE_ON
    lda.b #$7F
    sta.w !LIMIT_1A42+1
    rep.b #$20
    lda.w #!SCENE_CODE
    sta.w !player+!actor_03
    sep.b #$20
    lda.b #$0D
    sta.w !player+!actor_07
.until_be:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BE
    beq .until_be
    lda.b #$1E
    jsl.l SongPlay
    jml.l TaskExit
