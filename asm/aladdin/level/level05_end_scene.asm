; level05_end_scene.asm: level 5's closing scene, a task its screen set-up
; starts ($81:8954, through the table by level at $81:8802): once Aladdin
; gets far enough, the camera is taken over, actors play the scene out on
; cues, part of the map is rebuilt, the screen reddens and the level ends.
; C: src/aladdin/level/level05_end_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_video.inc"

arch 65816

!CUE_BD = $00BD                 ; the scene's cues (eight bytes, $BD-$C4), cleared as it starts
!CUE_BF = $00BF
!CUE_C0 = $00C0                 ; set by the task as the map is rebuilt
!CUE_C1 = $00C1                 ; set by the task at the second song
!CUE_C3 = $00C3
!CUE_C4 = $00C4
!scroll_x = !layer0+!LAYER_SCROLL_X  ; the play field's scroll x (word)
!scroll_y = !layer0+!LAYER_SCROLL_Y  ; and y (word)
!player_x = !player+!actor_x+1  ; Aladdin's x, whole pixels (word)
!HEALTH = $0367                 ; the player's health
!V_03FA = $03FA
!V_1A10 = $1A10
!V_1A12 = $1A12
!V_083D = $083D
!V_0087 = $0087
!V_0347 = $0347
!V_035D = $035D
!V_0360 = $0360
!V_1423 = $1423
!V_1454 = $1454
!WINDOW_03A6 = $03A6            ; the windows' settings, $03A6-$03AE
!SUB_LAYERS_0399 = $0399        ; layers the ending copies to the main and sub window masks
!SUB_LAYERS_039A = $039A
!PLACE_X = $002C                ; where ActorSpawnAt places the actor: x, then y (words)
!PLACE_Y = $002E
!FROM_X = $2C                   ; the map copy's (direct page, words): source x, y
!FROM_Y = $2E
!TO_X = $30                     ; destination x, y
!TO_Y = $32
!COLUMNS = $34                  ; tiles left in the row
!ROWS = $36                     ; rows left

; @routine 80:A2A8 Level05EndScene
; WHAT IT DOES: Level 5's closing scene. Once Aladdin, alive, is at x
; $0E20 or more with the play field's scroll y under 256, he is given
; scripted code, and on cue $C3 every actor goes and an actor of kind $2E
; comes; the camera is brought down to y $B8 (or cue $C4) and on to x
; $0E00, song $30; on cue $BF song $2F and the windows and colour maths
; set, two tasks started; on cue $BD three actors placed (kinds $2E, $70,
; $93) and an 8 by 7 block of the map's 16-pixel tiles copied from ($0A10,
; $0240) to ($0D60, $01E0); then the screen goes red over 29 steps and
; the level ends.
; HOW: CUE_BD-$C4 cleared; X 16-bit. Each frame (TaskWait 1) until
; SCROLL_Y's high byte is 0, PLAYER_X >= $0E20 and HEALTH nonzero: $03FA
; up, the player's $03 word $CB3B and $07 $0A. Each frame until CUE_C3:
; V_1A08 0, SoundQueueF6 2, ActorsFreeAll, ActorSpawnAt kind $2E ($07 2)
; at ($0E80, $0148). Each frame until SCROLL_Y = $B8 or CUE_C4: SCROLL_Y
; $B8, $083D 4, $1A10 and $1A12 up. Each frame until SCROLL_X >= $0E00:
; SongPlay $30. Each frame until CUE_BF: CUE_C1 up, SongPlay $2F, 16
; frames; TMW, TSW and CGADSUB $17, CGWSEL $20, $03AA $FF, $03AB and
; WOBJSEL $0F, $03AD $15, $03AE 1, $03A6 and $03A8 0, $03A7 and $03A9
; $FF; TaskCreate of WindowEffectTask twice: slot 1 ($18) with
; parameter $10, slot 6 ($90) with parameter $16. Each frame until CUE_BD: ActorSpawnAt
; kinds $2E ($07 3) at ($0E80, $0118), $70 ($07 1) at ($0DD0, $0228), $93
; ($07 0) at ($0DB0, $0220); the map copy by MapTileAt and MapTileSet,
; rows of 8 by 16 pixels, 7 rows. 128 frames; $0399 and $039A $13;
; $083D 8, 28 frames, $083D 0; 180 frames; CUE_C0 up; $0087 0; 100
; frames. CGADSUB $23, CGWSEL and the fixed colour's green and blue 0;
; its red from 3 up by one every 3 frames until it wraps to 0; $0360 and
; $035D 0, LEVEL_035C up, the player's facing and flip 0, $03 word
; $E7B1; $0347 bit 7 set; $1423 and $1454 up; TaskExit.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; D
; the task's direct page; a task's entry (TaskCreate slot $48 at
; $81:895F). Exit: never returns (TaskExit).
%at($80A2A8)
Level05EndScene:
    stz.w !CUE_BD
    stz.w !CUE_BD+1
    stz.w !CUE_BD+2
    stz.w !CUE_BD+3
    stz.w !CUE_BD+4
    stz.w !CUE_BD+5
    stz.w !CUE_BD+6
    stz.w !CUE_BD+7
    rep.b #$10
.until_far_in:
    lda.b #$01
    jsl.l TaskWait
    lda.w !scroll_y+1
    bne .until_far_in
    ldx.w !player_x
    cpx.w #$0E20
    bcc .until_far_in
    lda.w !HEALTH
    beq .until_far_in
    inc.w !V_03FA
    rep.b #$20
    lda.w #$CB3B
    sta.w !player+!actor_03
    sep.b #$30
    lda.b #$0A
    sta.w !player+!actor_07
.until_c3:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_C3
    beq .until_c3
    stz.w !v_1a08
    lda.b #$02
    jsl.l SoundQueueF6
    jsl.l ActorsFreeAll
    rep.b #$20
    lda.w #$0E80
    sta.w !PLACE_X
    lda.w #$0148
    sta.w !PLACE_Y
    sep.b #$20
    lda.b #$2E
    xba
    lda.b #$02
    jsr.w ActorSpawnAt
    sep.b #$20
.camera_down:
    lda.b #$01
    jsl.l TaskWait
    ldx.w #$00B8
    cpx.w !scroll_y
    beq .camera_set
    lda.w !CUE_C4
    beq .camera_down
.camera_set:
    ldx.w #$00B8
    stx.w !scroll_y
    lda.b #$04
    sta.w !V_083D
    inc.w !V_1A10
    inc.w !V_1A12
.camera_across:
    lda.b #$01
    jsl.l TaskWait
    ldx.w !scroll_x
    cpx.w #$0E00
    bcc .camera_across
    sep.b #$10
    lda.b #$30
    jsl.l SongPlay
.until_bf:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BF
    beq .until_bf
    inc.w !CUE_C1
    lda.b #$2F
    jsl.l SongPlay
    lda.b #$10
    jsl.l TaskWait
    lda.b #$17
    sta.w !tmw_shadow
    sta.w !tsw_shadow
    sta.w !cgadsub_shadow
    lda.b #$20
    sta.w !cgwsel_shadow
    lda.b #$FF
    sta.w !WINDOW_03A6+4
    lda.b #$0F
    sta.w !WINDOW_03A6+5
    sta.w !wobjsel_shadow
    lda.b #$15
    sta.w !WINDOW_03A6+7
    lda.b #$01
    sta.w !WINDOW_03A6+8
    lda.b #$00
    sta.w !WINDOW_03A6
    sta.w !WINDOW_03A6+2
    dec a
    sta.w !WINDOW_03A6+1
    sta.w !WINDOW_03A6+3
    jsl.l TaskCreate
    db 1*24, $10               ; slot 1, parameter $10
    dl WindowEffectTask
    jsl.l TaskCreate
    db 6*24, $16               ; slot 6, parameter $16
    dl WindowEffectTask
.until_bd:
    lda.b #$01
    jsl.l TaskWait
    lda.w !CUE_BD
    beq .until_bd
    rep.b #$20
    lda.w #$0E80
    sta.w !PLACE_X
    lda.w #$0118
    sta.w !PLACE_Y
    sep.b #$20
    lda.b #$2E
    xba
    lda.b #$03
    jsr.w ActorSpawnAt
    lda.w #$0DD0
    sta.w !PLACE_X
    lda.w #$0228
    sta.w !PLACE_Y
    sep.b #$30
    lda.b #$70
    xba
    lda.b #$01
    jsr.w ActorSpawnAt
    lda.w #$0DB0
    sta.w !PLACE_X
    lda.w #$0220
    sta.w !PLACE_Y
    sep.b #$30
    lda.b #$93
    xba
    lda.b #$00
    jsr.w ActorSpawnAt
    lda.w #$0240
    sta.b !FROM_Y
    lda.w #$01E0
    sta.b !TO_Y
    lda.w #$0007
    sta.b !ROWS
.row:
    lda.w #$0A10
    sta.b !FROM_X
    lda.w #$0D60
    sta.b !TO_X
    lda.w #$0008
    sta.b !COLUMNS
.tile:
    ldx.b !FROM_X
    ldy.b !FROM_Y
    jsl.l MapTileAt
    ldx.b !TO_X
    ldy.b !TO_Y
    jsl.l MapTileSet
    clc
    lda.b !FROM_X
    adc.w #$0010
    sta.b !FROM_X
    clc
    lda.b !TO_X
    adc.w #$0010
    sta.b !TO_X
    dec.b !COLUMNS
    bne .tile
    clc
    lda.b !FROM_Y
    adc.w #$0010
    sta.b !FROM_Y
    clc
    lda.b !TO_Y
    adc.w #$0010
    sta.b !TO_Y
    dec.b !ROWS
    bne .row
    sep.b #$30
    lda.b #$80
    jsl.l TaskWait
    lda.b #$13
    sta.w !SUB_LAYERS_0399
    sta.w !SUB_LAYERS_039A
    lda.b #$08
    sta.w !V_083D
    lda.b #$1C
    jsl.l TaskWait
    stz.w !V_083D
    lda.b #$B4
    jsl.l TaskWait
    inc.w !CUE_C0
    stz.w !V_0087
    lda.b #$64
    jsl.l TaskWait
    lda.b #$23
    sta.w !cgadsub_shadow
    stz.w !cgwsel_shadow
    stz.w !coldata_green
    stz.w !coldata_blue
    lda.b #$03
.redden:
    sta.w !coldata_red
    lda.b #$03
    jsl.l TaskWait
    lda.w !coldata_red
    inc a
    and.b #$1F
    bne .redden
    stz.w !V_0360
    stz.w !V_035D
    inc.w !level_035c
    stz.w !player+!actor_facing
    stz.w !player+!actor_flip
    rep.b #$20
    lda.w #$E7B1
    sta.w !player+!actor_03
    sep.b #$20
    lda.b #$80
    tsb.w !V_0347
    inc.w !V_1423
    inc.w !V_1454
    jml.l TaskExit
