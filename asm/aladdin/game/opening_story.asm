; opening_story.asm: the story told before the title screen: six scenes,
; each a picture with its actors placed in it, held for a set number of
; frames (Start skips the rest), faded in and out; then the title screen's
; task. Its task runs in slot 0: the slot's parameter byte is the scene.
; C: src/aladdin/game/opening_story.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!SLOT1_STATE = !task_state+24   ; slot 1's state: the fades run there
!SCENE = !task_param            ; slot 0's parameter: the scene (0-5), $80 once Start skips
!SCENE_FRAMES_LEFT = $0076      ; slot 0's own data: frames the scene still runs (word)
!V_0079 = $0079                 ; slot 0's own data, cleared as a scene starts (word)
!V_007B = $007B                 ; the same (word)
!PAD1_PRESSED_HIGH = !joy_pressed+1 ; bit 4: Start newly pressed
!V_0399 = $0399                 ; set to $17 while the story runs
!V_03B6 = $03B6                 ; cleared with HDMAEN
!V_1A08 = $1A08                 ; set to $FF
!STORY_LEVEL = $35              ; the level number the story's set-up loads
!SPAWN_X = $002C                ; where ActorSpawnAt places the actor: x, then y (words)
!SPAWN_Y = $002E

; In the bank DB names ($80), by scene:
!SCENE_FRAMES = $DA9D           ; frames it runs (words)
!SCENE_GFX = $DAA9              ; its graphics set, 0: none of its own
!SCENE_COPY = $DAAF             ; its second-table VRAM copy
!SCENE_PALETTE = $DAB5          ; its palette
; 6 bytes an entry: kind, its byte $07, x, y (words)
!OPENING_ACTORS = $DABB

; Code not yet filed
!SCREEN_BLANK = $8196B1         ; forced blank on
!LAYER_POSITIONS_CLEAR = $8196CB ; the layers' scroll positions zeroed
!DISPLAY_SETUP = $819865        ; the PPU's display set-up
!LEVEL_LOAD = $81825B           ; sets up the level !level names
!GFX_LOAD = $81BAF1             ; unpacks graphics set A
!ACTORS_RESET = $819D82         ; every actor record freed
!SLOT_15F1_START = $81EC8A      ; starts entry X of the slots at $15F1 with A
!TITLE_TASK = $809C91           ; the title screen's task
!FADE_IN_TASK = $819E8E         ; the screen's fade in, a task
!FADE_OUT_TASK = $819EB9        ; its fade out
!SCENE_TASK = $81F332           ; a task the first, fifth and sixth scenes start
!SCENE_END_TASK = $828000       ; a task the third, fourth and sixth scenes start

; @routine 80:9C79 TitleAfterFadeOut
; WHAT IT DOES: A task's entry: ends the other tasks, fades the screen
; out and waits for the fade to end, then goes on as the title screen's
; task.
; HOW: TaskSlotsFree1To6; the fade-out task created in slot 1; a frame's
; wait at a time until slot 1 is free; then on into $80:9C91.
; Entry: started by the scheduler: M 8-bit, X 8-bit, D = 0; DB a bank
; whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($809C79)
TitleAfterFadeOut:
    jsl.l TaskSlotsFree1To6
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !FADE_OUT_TASK
.wait:
    lda.b #$01
    jsl.l TaskWait
    lda.w !SLOT1_STATE
    bne .wait
    ; on into the title screen's task

; @routine 80:AC87 OpeningStory
; WHAT IT DOES: The opening story: after the screen fades out, six scenes
; one after another (Start skips to the end), then the title screen.
; HOW: A frame's wait, a sound command ($F6, 1), the fade-out and 64
; frames; the screen set up for the story (level $35's set-up, graphics
; set 1, palette $5A). For each scene (the slot's parameter, 0 up): the
; screen blanked and cleared, the text and two tilemaps emptied, the
; story's common graphics and palette loaded, then the scene's own (when
; it has a graphics set); the fade-in, OpeningSceneRun, the fade-out and
; 64 frames. After the sixth (or a skip): the sound command again, and
; the task starts over at $80:9C91.
; Entry: M 8-bit, X 8-bit; called by JSL from the task in slot 0; DB = $80,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80AC87)
OpeningStory:
    lda.b #$01
    jsl.l TaskWait
    lda.b #$01
    xba
    jsl.l SoundQueueF6With1
    jsr.w ScreenFadeOutStart
    jsr.w Wait64Frames
    jsl.l NmiDisable
    jsl.l !SCREEN_BLANK
    jsl.l !LAYER_POSITIONS_CLEAR
    jsl.l OamBufferClear
    jsl.l !DISPLAY_SETUP
    lda.b #!STORY_LEVEL
    sta.w !level
    jsl.l !LEVEL_LOAD
    lda.b #$FF
    sta.w !V_1A08
    lda.b #$01
    jsl.l !GFX_LOAD
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$5A
    jsl.l PaletteLoad
    jsl.l NmiEnable
    lda.b #$17
    sta.w !V_0399
    stz.w !SCENE
.scene:
    jsl.l NmiDisable
    jsl.l !SCREEN_BLANK
    jsl.l OamBufferClear
    jsl.l !LAYER_POSITIONS_CLEAR
    jsl.l LevelTablesClear
    jsl.l !ACTORS_RESET
    stz.w !V_03B6
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
    lda.b #$0E
    jsl.l !GFX_LOAD
    ldx.b #$7E
    jsl.l VramCopy
    lda.b #$31
    jsl.l PaletteLoad
    ldx.w !SCENE
    lda.w !SCENE_GFX,x
    beq .shown
    jsl.l !GFX_LOAD
    ldx.b #$00
    jsl.l VramCopy2
    ldx.w !SCENE
    lda.w !SCENE_COPY,x
    tax
    jsl.l VramCopy2
    ldx.w !SCENE
    lda.w !SCENE_PALETTE,x
    jsl.l PaletteLoad
.shown:
    jsl.l NmiEnable
    jsr.w ScreenFadeInStart
    jsr.w OpeningSceneRun
    jsr.w ScreenFadeOutStart
    jsr.w Wait64Frames
    inc.w !SCENE
    lda.w !SCENE
    cmp.b #$06
    bcs .done
    jmp.w .scene
.done:
    lda.b #$01
    xba
    jsl.l SoundQueueF6With1
    jsl.l TaskExec
    dl !TITLE_TASK

; @routine 80:AD78 OpeningSceneRun
; WHAT IT DOES: Runs one scene of the opening story: sets it up, then
; lets it play for its frames, or until Start is pressed, which skips the
; rest of the story.
; HOW: The scene's frame count from its table; its set-up routine through
; a table of six; then FrameEnd and the count down each frame. Start:
; the scene number becomes $80, past the last.
; Entry: M 8-bit, X 8-bit; called by JSR from the story's task (slot 0);
; DB = $80, whose $0000-$1FFF is work RAM. Exit: as the C states.
OpeningSceneRun:
    lda.w !SCENE
    asl a
    tax
    rep.b #$20
    stz.w !V_007B
    stz.w !V_0079
    lda.w !SCENE_FRAMES,x
    sta.w !SCENE_FRAMES_LEFT
    sep.b #$20
    jsr.w (.starts,x)
.frame:
    lda.w !PAD1_PRESSED_HIGH
    bit.b #$10
    bne .skip
    jsr.w FrameEnd
    rep.b #$20
    dec.w !SCENE_FRAMES_LEFT
    sep.b #$20
    bne .frame
    rts
.skip:
    lda.b #$80
    sta.w !SCENE
    rts
.starts:
    dw OpeningScene1Start, OpeningScene2Start, OpeningScene3Start
    dw OpeningScene4Start, OpeningScene5Start, OpeningScene6Start

; @routine 80:ADB6 OpeningScene1Start
; WHAT IT DOES: Sets up the first scene of the opening story: its song,
; and the task at $81:F332 that runs with it.
; HOW: SongPlay $1C; the task at $81:F332 created in slot 4 with
; parameter 0.
; Entry: M 8-bit, X 8-bit; called by JSR; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
OpeningScene1Start:
    lda.b #$1C
    jsl.l SongPlay
    jsl.l TaskCreate
    db 4*24, 0                  ; slot 4, parameter 0
    dl !SCENE_TASK
    rts

; @routine 80:ADC6 OpeningScene2Start
; WHAT IT DOES: Sets up the second scene of the opening story: its song
; and four actors.
; HOW: SongPlay $1F; OpeningActorPlace with entries 0, 2, 3 and 1.
; Entry: M 8-bit, X 8-bit; called by JSR; DB = $80, whose $0000-$1FFF is
; work RAM. Exit: as the C states.
OpeningScene2Start:
    lda.b #$1F
    jsl.l SongPlay
    ldx.b #0*6
    jsr.w OpeningActorPlace
    ldx.b #2*6
    jsr.w OpeningActorPlace
    ldx.b #3*6
    jsr.w OpeningActorPlace
    ldx.b #1*6
    jsr.w OpeningActorPlace
    rts

; @routine 80:ADE1 OpeningScene3Start
; WHAT IT DOES: Sets up the third scene of the opening story: one actor,
; then the scenes' common end.
; HOW: OpeningActorPlace with entry 7, then the end at $80:AE43 (in
; OpeningScene6Start): the task at $82:8000 in slot 6, a frame
; (FrameEnd), palette $56, slot 0 at $15F1 started with $1F, the actor of
; entry 20.
; Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
OpeningScene3Start:
    ldx.b #7*6
    jsr.w OpeningActorPlace
    bra OpeningSceneEnd

; @routine 80:ADE8 OpeningScene4Start
; WHAT IT DOES: Sets up the fourth scene of the opening story: nine
; actors, then the scenes' common end.
; HOW: OpeningActorPlace with entries 10, 11, 13, 12, 4, 6, 8, 21 and 9,
; then the end at $80:AE43 as OpeningScene3Start.
; Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
OpeningScene4Start:
    ldx.b #10*6
    jsr.w OpeningActorPlace
    ldx.b #11*6
    jsr.w OpeningActorPlace
    ldx.b #13*6
    jsr.w OpeningActorPlace
    ldx.b #12*6
    jsr.w OpeningActorPlace
    ldx.b #4*6
    jsr.w OpeningActorPlace
    ldx.b #6*6
    jsr.w OpeningActorPlace
    ldx.b #8*6
    jsr.w OpeningActorPlace
    ldx.b #21*6
    jsr.w OpeningActorPlace
    ldx.b #9*6
    jsr.w OpeningActorPlace
    bra OpeningSceneEnd

; @routine 80:AE17 OpeningScene6Start
; WHAT IT DOES: Sets up the sixth scene of the opening story: seven
; actors, the sprite task, then the end it shares with the third and
; fourth scenes.
; HOW: OpeningActorPlace with entries 16, 17, 18, 5, 6, 14 and 15; the
; task at $81:F332 in slot 4 with parameter 5; then from $80:AE43 the
; task at $82:8000 in slot 6, a frame (FrameEnd), palette $56, slot 0 at
; $15F1 started with $1F and the actor of entry 20.
; Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
OpeningScene6Start:
    ldx.b #16*6
    jsr.w OpeningActorPlace
    ldx.b #17*6
    jsr.w OpeningActorPlace
    ldx.b #18*6
    jsr.w OpeningActorPlace
    ldx.b #5*6
    jsr.w OpeningActorPlace
    ldx.b #6*6
    jsr.w OpeningActorPlace
    ldx.b #14*6
    jsr.w OpeningActorPlace
    ldx.b #15*6
    jsr.w OpeningActorPlace
    jsl.l TaskCreate
    db 4*24, 5                  ; slot 4, parameter 5
    dl !SCENE_TASK
; The end the third, fourth and sixth scenes share.
OpeningSceneEnd:
    jsl.l TaskCreate
    db 6*24, 24                 ; slot 6, parameter 24
    dl !SCENE_END_TASK
    jsr.w FrameEnd
    lda.b #$56
    jsl.l PaletteLoad
    ldx.b #$00
    lda.b #$1F
    jsl.l !SLOT_15F1_START
    ldx.b #20*6
    jsr.w OpeningActorPlace
    rts

; @routine 80:AE63 OpeningScene5Start
; WHAT IT DOES: Sets up the fifth scene of the opening story: more
; graphics, an actor, the sprite task and two of the slots at $15F1.
; HOW: With the NMI off and the screen blanked, graphics set $27 and the
; second table's copy $54; the actor of entry 19; the task at
; $81:F332 in slot 4 with parameter 2; slot 0 at $15F1 started with $28
; and slot $0E with $29.
; Entry: M 8-bit, X 8-bit; called by JSR; DB = $80, whose $0000-$1FFF is
; work RAM. Exit: as the C states.
OpeningScene5Start:
    jsl.l NmiDisable
    jsl.l !SCREEN_BLANK
    lda.b #$27
    jsl.l !GFX_LOAD
    ldx.b #$54
    jsl.l VramCopy2
    jsl.l NmiEnable
    ldx.b #19*6
    jsr.w OpeningActorPlace
    jsl.l TaskCreate
    db 4*24, 2                  ; slot 4, parameter 2
    dl !SCENE_TASK
    ldx.b #$00
    lda.b #$28
    jsl.l !SLOT_15F1_START
    ldx.b #$0E
    lda.b #$29
    jsl.l !SLOT_15F1_START
    rts

; @routine 80:AEA5 ScreenFadeInStart
; WHAT IT DOES: Starts the screen fading in, over the next frames.
; HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4
; (frames a step, doubled).
; Entry: any widths; called by JSR; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
%at($80AEA5)
ScreenFadeInStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !FADE_IN_TASK
    rts

; @routine 80:AEAF ScreenFadeOutStart
; WHAT IT DOES: Starts the screen fading out, over the next frames.
; HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
; Entry: any widths; called by JSR; DB a bank whose $0000-$1FFF is work
; RAM. Exit: as the C states.
ScreenFadeOutStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !FADE_OUT_TASK
    rts

; @routine 80:AEC0 OpeningActorPlace
; WHAT IT DOES: Places one of the opening story's actors: entry X of its
; table gives the kind, the byte its code reads at $07, and the place.
; HOW: The entry's x and y in the spawn words (SPAWN_X, SPAWN_Y), its
; kind in A's high byte and the byte in its low; ActorSpawnAtLong; 8-bit
; registers after.
; Entry: M 8-bit, X 8-bit; X = the entry's offset (6 bytes an entry); DB =
; $80, the table's bank, whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80AEC0)
OpeningActorPlace:
    rep.b #$20
    lda.w !OPENING_ACTORS+2,x
    sta.w !SPAWN_X
    lda.w !OPENING_ACTORS+4,x
    sta.w !SPAWN_Y
    sep.b #$20
    lda.w !OPENING_ACTORS,x
    xba
    lda.w !OPENING_ACTORS+1,x
    jsl.l ActorSpawnAtLong
    sep.b #$30
    rts
