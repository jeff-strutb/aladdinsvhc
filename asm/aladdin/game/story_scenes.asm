; story_scenes.asm: steps of the story that runs at $80:AF89, six scenes
; by slot 0's parameter (each set up by an entry of the table at
; $80:B092), faded in and out, with 64 frames between scenes. A scene may
; leave a sound command for its end.
; C: src/aladdin/game/story_scenes.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"

arch 65816

; Slot 0's own data, read as the scene ends ($80:B02F): when nonzero, the
; parameter of sound command $F6 queued then, and the frames waited after
; it.
!SCENE_END_SOUND = $007C
!SCENE_END_WAIT = $007D
!STORY_SCENE_TASK = $81F344     ; the task the story's scenes start in slot 4
!STORY_FADE_IN_TASK = $819E8E   ; the screen's fade in, a task
!STORY_FADE_OUT_TASK = $819EB9  ; its fade out
!SLOT1_STATE = !task_state+24   ; slot 1's state: the fades run there
!V_0399 = $0399                 ; set to $17 while the story runs; purpose not yet established
!STORY_FRAMES_LEFT = $0076      ; slot 0's own data: frames the scene still runs (word)
!V_0078 = $0078                 ; slot 0's own data, cleared as the fifth scene starts
!V_0079 = $0079
!V_007A = $007A
!V_007B = $007B
; In the bank DB names ($80), by scene:
!STORY_FRAMES = $DB3F           ; the frames it runs (words)
!STORY_GFX = $DB4B              ; its graphics set, 0: none of its own
!STORY_COPY = $DB51             ; its second-table VRAM copy
!STORY_PALETTE = $DB57          ; its palette

; @routine 80:AF42 EndingStory
; WHAT IT DOES: The ending's story: after the screen fades out, six scenes
; one after another, each set up by StorySceneStarts and run for its
; frames (a scene may leave a sound command for its end), faded in and
; out; then the iris task, a sound command, the fade-out, and the ending's
; stages task.
; HOW: Sound $F6 with 1; StoryFadeOutStart, StoryWait64Frames; the screen
; blanked and cleared, level $39 set up, V_1A08 = $FF, graphics set 1,
; palette $5A. For each scene (slot 0's parameter, 0 up): V_0399 = $17;
; the screen blanked and cleared, the actors freed, HDMA off, both
; tilemaps filled, the text cleared, the first colour black; set $0E,
; palette $31; the scene's graphics set (STORY_GFX: 0 none, else copies 0,
; $4D and STORY_COPY, palette STORY_PALETTE, SCENE_END_SOUND and
; SCENE_END_WAIT cleared); StoryFadeInStart, StorySceneRun; after it the
; end sound and wait when set, StoryFadeOutStart, StoryWait64Frames. After
; the sixth: IrisCloseTask in slot 3; sound $F6 with 1; the fade-out task;
; frames until slot 1 is free; TaskExec EndingStagesTask.
; Entry: M 8-bit, X 8-bit; a task's start; DB = $80, whose $0000-$1FFF is
; work RAM. Exit: never returns.
%at($80AF42)
EndingStory:
    lda.b #$01
    jsl.l SoundQueueF6
    jsr.w StoryFadeOutStart
    jsr.w StoryWait64Frames
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    lda.b #$39
    sta.w !level
    jsl.l LevelRamSetupLong
    lda.b #$FF
    sta.w !v_1a08
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$5A
    jsl.l PaletteLoad
    jsl.l NmiEnable
    stz.w !task_param
.lAF89:
    lda.b #$17
    sta.w !V_0399
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l OamBufferClear
    jsl.l ScrollStateClear
    jsl.l LevelTablesClear
    jsl.l ActorsFreeAll
    jsl.l DisplayDefaults
    stz.w !hdmaen_shadow
    stz.w !HDMAEN
    rep.b #$10
    ldx.w #$4000
    ldy.w #$0000
    jsl.l VramFillMap
    ldx.w #$4800
    ldy.w #$0000
    jsl.l VramFillMap
    sep.b #$10
    jsl.l TextLayerClear
    lda.b #$00
    sta.l !palette_buffer
    sta.l !palette_buffer+1
    lda.b #$0E
    jsl.l UnpackSet
    ldx.b #$7E
    jsl.l VramCopy
    lda.b #$31
    jsl.l PaletteLoad
    ldx.w !task_param
    lda.w !STORY_GFX,x
    beq .lB01B
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy2
    ldx.b #$4D
    jsl.l VramCopy2
    ldx.w !task_param
    lda.w !STORY_COPY,x
    tax
    jsl.l VramCopy2
    ldx.w !task_param
    lda.w !STORY_PALETTE,x
    jsl.l PaletteLoad
    stz.w !SCENE_END_SOUND
    stz.w !SCENE_END_WAIT
.lB01B:
    jsl.l NmiEnable
    jsr.w StoryFadeInStart
    jsr.w StorySceneRun
    inc.w !task_param
    lda.w !task_param
    cmp.b #$06
    bcs .lB048
    lda.w !SCENE_END_SOUND
    beq .lB03F
    jsl.l SoundQueueF6
    lda.w !SCENE_END_WAIT
    jsl.l TaskWait
.lB03F:
    jsr.w StoryFadeOutStart
    jsr.w StoryWait64Frames
    jmp.w .lAF89
.lB048:
    jsl.l TaskCreate
    db 3*24, 0                  ; slot 3, parameter 0
    dl IrisCloseTask
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 1*24, $18                ; slot 1, parameter $18
    dl BrightnessDownTask
.lB05E:
    lda.b #$01
    jsl.l TaskWait
    lda.w !SLOT1_STATE
    bne .lB05E
    jsl.l TaskExec
    dl EndingStagesTask

; @routine 80:B073 StorySceneRun
; WHAT IT DOES: Runs one scene of the ending's story: its set-up, then a
; frame at a time for its frames.
; HOW: STORY_FRAMES_LEFT from STORY_FRAMES by the scene (slot 0's
; parameter); the set-up through StorySceneStarts; StoryFrameEnd until the
; count is 0.
; Entry: M 8-bit, X 8-bit; called by JSR from EndingStory; DB = $80, whose
; $0000-$1FFF is work RAM. Exit: Z set; as the last StoryFrameEnd leaves
; A, X and Y.
%at($80B073)
StorySceneRun:
    lda.w !task_param
    asl a
    tax
    rep.b #$20
    lda.w !STORY_FRAMES,x
    sta.w !STORY_FRAMES_LEFT
    sep.b #$20
    jsr.w (StorySceneStarts,x)
.lB085:
    jsr.w StoryFrameEnd
    rep.b #$20
    dec.w !STORY_FRAMES_LEFT
    sep.b #$20
    bne .lB085
    rts

; Each scene's set-up, by the scene.
StorySceneStarts:
    dw StoryScene1Start, StoryScene2Start, StoryScene3Start
    dw StoryScene4Start, StoryScene5Start, StoryScene6Start

; @routine 80:B09E StoryScene1Start
; WHAT IT DOES: Sets up the story's first scene: song $32, and the scene's
; task at $81:F344 started.
; HOW: SongPlay $32; the task created in slot 4 with parameter 0.
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at
; $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B09E)
StoryScene1Start:
    lda.b #$32
    jsl.l SongPlay
    jsl.l TaskCreate
    db 4*24, 0                  ; slot 4, parameter 0
    dl !STORY_SCENE_TASK
    rts

; @routine 80:B0AE StoryScene2Start
; WHAT IT DOES: Sets up the story's second scene: sound command $F6 with
; parameter 2 to be queued as it ends, then a wait of 64 frames, and the
; scene's task at $81:F344 started.
; HOW: The two slot 0 bytes; the task created in slot 4 with parameter 3.
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at
; $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B0AE)
StoryScene2Start:
    lda.b #$02
    sta.w !SCENE_END_SOUND
    lda.b #$40
    sta.w !SCENE_END_WAIT
    jsl.l TaskCreate
    db 4*24, 3                  ; slot 4, parameter 3
    dl !STORY_SCENE_TASK
    rts

; @routine 80:B0C2 StoryScene3Start
; WHAT IT DOES: Sets up the story's third scene: song $34, its actors
; from the first entry, the scene's task with scene 4, and palette cycle
; $14 in slot 0.
; HOW: SongPlay $34; StoryActorPlace with X = 0; TaskCreate (slot 4,
; parameter 4, $81:F344); PaletteCycleStartLong with X = 0, A = $14.
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at
; $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as
; PaletteCycleStartLong leaves them.
%at($80B0C2)
StoryScene3Start:
    lda.b #$34
    jsl.l SongPlay
    ldx.b #$00
    jsr.w StoryActorPlace
    jsl.l TaskCreate
    db 4*24, 4                  ; slot 4, parameter 4
    dl !STORY_SCENE_TASK
    ldx.b #$00
    lda.b #$14
    jsl.l PaletteCycleStartLong
    rts

; @routine 80:B0DF StoryScene4Start
; WHAT IT DOES: Sets up the story's fourth scene: song $23, its actors
; from entry $24, a frame, the Mode 7 screen (set $20), the spinning zoom
; task, and the first two palette bytes $2048 uploaded.
; HOW: SongPlay $23; StoryActorPlace with X = $24; TaskWait 1;
; Mode7ScreenSetup $20; TaskCreate (slot 4, parameter 0,
; SceneSpinZoomTask); the palette buffer's first word's bytes $48, $20;
; PALETTE_UPLOAD up.
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at $80:B092)
; from the story's task; DB a bank whose $0000-$1FFF is work RAM. Exit:
; when the task has resumed; N and Z from PALETTE_UPLOAD.
%at($80B0DF)
StoryScene4Start:
    lda.b #$23
    jsl.l SongPlay
    ldx.b #$24
    jsr.w StoryActorPlace
    lda.b #$01
    jsl.l TaskWait
    lda.b #$20
    jsl.l Mode7ScreenSetup
    jsl.l TaskCreate
    db 4*24, 0                  ; slot 4, parameter 0
    dl SceneSpinZoomTask
    lda.b #$48
    sta.l !palette_buffer
    lda.b #$20
    sta.l !palette_buffer+1
    inc.w !palette_upload
    rts

; @routine 80:B10F StoryScene5Start
; WHAT IT DOES: Sets up the story's fifth scene: five actors, the window
; effect task, song $24, a frame, palette $5B, the scene's text task, and
; the sound and wait at its end.
; HOW: V_0078-V_007B cleared; StoryActorPlace with entries 1-5;
; WindowEffectTask in slot 6 with parameter $18; SongPlay $24;
; StoryFrameEnd; palette $5B; StoryTextTask1 in slot 4 with parameter 6;
; SCENE_END_SOUND = 2, SCENE_END_WAIT = $40.
; Entry: M 8-bit, X 8-bit; called by JSR (through StorySceneStarts); DB a
; bank whose $0000-$1FFF is work RAM. Exit: A low = $40.
%at($80B10F)
StoryScene5Start:
    stz.w !V_0078
    stz.w !V_0079
    stz.w !V_007A
    stz.w !V_007B
    ldx.b #$06
    jsr.w StoryActorPlace
    ldx.b #$0C
    jsr.w StoryActorPlace
    ldx.b #$12
    jsr.w StoryActorPlace
    ldx.b #$18
    jsr.w StoryActorPlace
    ldx.b #$1E
    jsr.w StoryActorPlace
    jsl.l TaskCreate
    db 6*24, $18                ; slot 6, parameter $18
    dl WindowEffectTask
    lda.b #$24
    jsl.l SongPlay
    jsr.w StoryFrameEnd
    lda.b #$5B
    jsl.l PaletteLoad
    jsl.l TaskCreate
    db 4*24, 6                  ; slot 4, parameter 6
    dl StoryTextTask1
    lda.b #$02
    sta.w !SCENE_END_SOUND
    lda.b #$40
    sta.w !SCENE_END_WAIT
    rts

; @routine 80:B160 StoryScene6Start
; WHAT IT DOES: Sets up the story's sixth scene: song $13 and the scene's
; task with scene $13.
; HOW: SongPlay $13; TaskCreate (slot 4, parameter $13, $81:F344).
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at
; $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as TaskCreate
; leaves them.
%at($80B160)
StoryScene6Start:
    lda.b #$13
    jsl.l SongPlay
    jsl.l TaskCreate
    db 4*24, $13                ; slot 4, parameter $13
    dl !STORY_SCENE_TASK
    rts

; @routine 80:B170 StoryFrameEnd
; WHAT IT DOES: Ends a frame of the ending's story: the frame's update,
; then the wait for the next frame.
; HOW: FrameStepLong; TaskWait with A = 1.
; Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB a bank
; whose $0000-$1FFF is work RAM. Exit: as the wait restores the
; registers (A low = 1).
%at($80B170)
StoryFrameEnd:
    jsl.l FrameStepLong
    lda.b #$01
    jsl.l TaskWait
    rts

; @routine 80:B17B StoryFadeInStart
; WHAT IT DOES: Starts the screen fading in, over the next frames, as a
; scene of the story begins.
; HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4.
; Entry: any widths; called by JSR from the story's task; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B17B)
StoryFadeInStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !STORY_FADE_IN_TASK
    rts

; @routine 80:B185 StoryFadeOutStart
; WHAT IT DOES: Starts the screen fading out, over the next frames, as a
; scene of the story ends.
; HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
; Entry: any widths; called by JSR from the story's task; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
StoryFadeOutStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !STORY_FADE_OUT_TASK
    rts

; @routine 80:B18F StoryWait64Frames
; WHAT IT DOES: Makes the story's task wait 64 frames between scenes,
; letting the other tasks run.
; HOW: TaskWait with A = 64.
; Entry: M 8-bit; called by JSR from the story's task. Exit: as the C states.
StoryWait64Frames:
    lda.b #$40
    jsl.l TaskWait
    rts


; The scenes' actors: per entry (6 bytes), the kind (0: the player), the
; byte for its $07, then x and y (words).
!scene_actors = $DB5D           ; data bank $80
!SPAWN_X = $002C                ; where ActorSpawnAt places the actor: x, then y (words)
!SPAWN_Y = $002E

; @routine 80:B196 StoryActorPlace
; WHAT IT DOES: Places entry X of the story scenes' actors ($80:DB5D): a
; new actor of its kind, or, for kind 0, the player.
; HOW: The entry's x and y into the spawn words. A kind: in A's high
; byte, the $07 byte in its low, ActorSpawnAtLong. Kind 0: the player's
; $06 cleared, its $07 the byte, the low bytes of x and y its whole
; pixels' low bytes with their fractions cleared, and its state $0C (new).
; Entry: M 8-bit, X 8-bit; X = the entry's offset; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit.
%at($80B196)
StoryActorPlace:
    rep.b #$20
    lda.w !scene_actors+2,x
    sta.w !SPAWN_X
    lda.w !scene_actors+4,x
    sta.w !SPAWN_Y
    sep.b #$20
    lda.w !scene_actors,x
    beq .player
    xba
    lda.w !scene_actors+1,x
    jsl.l ActorSpawnAtLong
.done:
    sep.b #$30
    rts
.player:
    stz.w !player+!actor_06
    lda.w !scene_actors+1,x
    sta.w !player+!actor_07
    ldx.w !SPAWN_X
    stx.w !player+!actor_x+1
    stz.w !player+!actor_x
    ldx.w !SPAWN_Y
    stx.w !player+!actor_y+1
    stz.w !player+!actor_y
    lda.b #$0C
    sta.w !player+!actor_state
    bra .done
