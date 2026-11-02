; title_task.asm: the title: its start (the opening story first) and its
; task: the Mode 7 title, the demo when left alone, the menu and the level
; select.
; C: src/aladdin/game/title_task.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

; Places not yet named keep their address as their name.
!routine_818031 = $818031  ; not yet filed
!routine_818095 = $818095  ; not yet filed
!routine_81825b = $81825B  ; not yet filed
!routine_81830d = $81830D  ; not yet filed
!routine_8196b1 = $8196B1  ; not yet filed
!routine_81972d = $81972D  ; not yet filed
!routine_819dcd = $819DCD  ; not yet filed
!routine_819e8e = $819E8E  ; not yet filed
!routine_819eb9 = $819EB9  ; not yet filed
!routine_81eda2 = $81EDA2  ; not yet filed
!routine_81ee04 = $81EE04  ; not yet filed
!routine_838979 = $838979  ; not yet filed
!v_0000 = $0000
!v_002b = $002B
!v_0076 = $0076
!v_0087 = $0087
!v_00b7 = $00B7
!v_00ff = $00FF
!v_0345 = $0345
!v_0347 = $0347
!v_0358 = $0358
!v_0359 = $0359
!v_035a = $035A
!v_035d = $035D
!v_0360 = $0360
!v_0362 = $0362
!v_0369 = $0369
!v_036b = $036B
!v_036c = $036C
!v_036e = $036E
!v_0370 = $0370
!v_0373 = $0373
!v_0374 = $0374
!v_0375 = $0375
!v_0376 = $0376
!v_038d = $038D
!v_038e = $038E
!v_0393 = $0393
!v_0394 = $0394
!v_0395 = $0395
!v_0397 = $0397
!v_0399 = $0399
!v_039a = $039A
!v_03a0 = $03A0
!v_03a4 = $03A4
!v_0847 = $0847
!v_0848 = $0848
!v_0849 = $0849
!v_084a = $084A
!v_084b = $084B
!v_142e = $142E
!v_146b = $146B
!v_1773 = $1773
!v_1777 = $1777

; @routine 80:9C64 TitleStartTask
; WHAT IT DOES: Starts the title's task: frees the other tasks, stage 0,
; play mode 0, and runs the opening story, which ends by starting
; TitleTask; the jump to TitleTask's middle after it ($80:9C76) is never
; reached, since OpeningStory never returns.
; HOW: TaskSlotsFree1To6; $033F and the play mode cleared; OpeningStory;
; then a JMP to $80:9C9D (hand-written: no script reaches it).
; Entry: M 8-bit, X 8-bit; a task's entry. Exit: never returns.
%at($809C64)
TitleStartTask:
    lda.b #$80
    pha
    plb
    jsl.l TaskSlotsFree1To6
    stz.w !v_033f
    stz.w !play_mode
    jsl.l OpeningStory
    jmp.w TitleTask_l9C9D


; @routine 80:9C91 TitleTask
; WHAT IT DOES: The title screen: puts up the Mode 7 title, waits (playing
; the demo or the story after long idles), and runs the menu: game start,
; password, options; with $0009 set, the level select.
; HOW: The screen built (level map, layers, graphics, palette,
; Mode7ScreenSetup) and faded in. After a demo ($0345 set) it comes back
; at $80:9E7F and waits for a press (PadConfirmPressed) or the zoom task's
; end; else the menu runs until a press, the level select (LevelSelect,
; with $0009 set) or the countdown at $0343, which starts the demo
; ($81:8031) or the opening story. A choice ends the task by TaskExec:
; $83:8979, PasswordScreen or $81:8095.
; Entry: M 8-bit, X 8-bit; a task's entry. Exit: never returns.
%at($809C91)
TitleTask:
    jsl.l TaskSlotsFree1To6
    jsl.l LevelCountersClear
    jsl.l GameRamClear
TitleTask_l9C9D:
    stz.w !play_mode
    stz.w !v_0360
    jsl.l NmiDisable
    jsl.l !routine_8196b1
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    jsl.l TextLayerClear
    lda.b #$40
    sta.w !level
    jsl.l !routine_81825b
    lda.b #$FF
    sta.w !v_1a08
    rep.b #$20
    stz.w !v_1773
    stz.w !v_1777
    sep.b #$20
    jsl.l LevelMapBuildLong
    jsl.l LayersFillLong
    stz.w !v_0399
    stz.w !v_039a
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$04
    jsl.l UnpackSet
    ldx.b #$15
    jsl.l VramCopy
    ldx.b #$31
    jsl.l VramCopy
    lda.b #$2F
    jsl.l UnpackSet
    ldx.b #$23
    jsl.l VramCopy
    lda.b #$08
    jsl.l PaletteLoad
    jsl.l NmiEnable
    lda.b #$01
    jsl.l TaskWait
    lda.b #$00
    jsl.l Mode7ScreenSetup
    lda.b #$00
    sta.w !inidisp_shadow
    jsl.l !routine_81830d
    lda.b #$20
    jsl.l TaskWait
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819e8e
    lda.b #$5C
    sta.w !v_03a0
    lda.b #$55
    sta.w !v_03a4
    lda.b #$11
    sta.w !v_0399
    sta.w !v_039a
    lda.b #$01
    jsl.l TaskWait
    lda.w !v_0345
    beq .l9D59
    jmp.w .l9E7F
.l9D59:
    rep.b #$20
    lda.w #$0200
    sta.w !v_0343
    sep.b #$20
    jsl.l ActorsFreeAll
    stz.w !v_0399
    lda.b #$82
    jsl.l ActorCreate
    bcs .l9DC5
    lda.b #$0C
    sta.w !v_0000,x
    rep.b #$20
    lda.w #$0900
    sta.w !v_002b,x
    lda.w #$0080
    sta.w !scratch,x
    lda.w #$0080
    sta.w !scratch+3,x
    lda.w #$0080
    sta.w !v_0395
    lda.w #$00B0
    sta.w !v_0397
    sep.b #$20
    jsl.l NmiDisable
    jsl.l !routine_8196b1
    lda.b #$4C
    jsl.l PaletteLoad
    lda.b #$22
    jsl.l UnpackSet
    ldx.b #$FC
    jsl.l VramCopy
    jsl.l NmiEnable
    lda.w !inidisp_shadow
    and.b #$0F
    sta.w !inidisp_shadow
    lda.b #$14
    jsl.l SongPlay
.l9DC5:
    lda.b #$01
    jsl.l TaskWait
    lda.w !v_0087
    bne .l9DC5
    stz.w !cgwsel_shadow
    lda.b #$31
    sta.w !cgadsub_shadow
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
.l9DE1:
    lda.b #$01
    jsl.l TaskWait
    lda.w !coldata_red
    inc a
    sta.w !coldata_red
    sta.w !coldata_green
    sta.w !coldata_blue
    cmp.b #$1F
    bcc .l9DE1
    lda.b #$0C
    ldy.b #$01
    jsl.l PaletteSave
    lda.b #$E1
    sta.w !v_0076
    ldy.b #$01
    jsl.l !routine_819dcd
    lda.b #$11
    sta.w !v_0399
    rep.b #$20
    lda.w #$00D0
    sta.w !v_038d
    sta.w !v_0393
    sep.b #$20
.l9E1D:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    inc.w !v_038d
    inc.w !v_0393
    lda.w !coldata_red
    dec a
    sta.w !coldata_red
    sta.w !coldata_green
    sta.w !coldata_blue
    bne .l9E1D
    lda.b #$10
    sta.w !v_039a
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$21
    sta.w !cgadsub_shadow
.l9E4B:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_0076
    ldy.b #$01
    jsl.l !routine_819dcd
    inc.w !v_0076
    beq .l9E74
    lda.w !v_038d
    beq .l9E4B
    rep.b #$20
    inc.w !v_038d
    inc.w !v_0393
    sep.b #$20
    bra .l9E4B
.l9E74:
    ldx.b #$00
    lda.b #$22
    jsl.l PaletteCycleStartLong
    jmp.w .l9F08
.l9E7F:
    rep.b #$20
    lda.w #$1300
    sta.w !v_0343
    sep.b #$20
    lda.b #$12
    jsl.l SongPlay
.l9E8F:
    jsr.w PadConfirmPressed
    bne .l9EC8
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_146b
    beq .l9E8F
    jsl.l TaskCreate
    db 3*24, 0                ; slot 3, parameter 0
    dl Mode7ZoomInTask
.l9EAC:
    jsr.w PadConfirmPressed
    bne .l9EC8
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_146b
    bpl .l9EAC
    lda.b #$13
    jsl.l SongPlay
    bra .l9F08
.l9EC8:
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819eb9
.l9ED1:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_0087
    bne .l9ED1
    stz.w !v_00b7
    stz.w !v_00ff
    stz.w !hdmaen_shadow
    stz.w !HDMAEN
    jsl.l TaskCreate
    db 6*24, 6                ; slot 6, parameter 6
    dl WindowEffectTask
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819e8e
    jsl.l ActorsFreeAll
    lda.b #$13
    jsl.l SongPlay
.l9F08:
    lda.b #$01
    jsl.l TaskWait
    lda.b #$4A
    jsl.l ActorCreate
    bcs .l9F2B
    lda.b #$0C
    sta.w !v_0000,x
    rep.b #$20
    lda.w #$007C
    sta.w !scratch,x
    lda.w #$00C8
    sta.w !scratch+3,x
    sep.b #$20
.l9F2B:
    lda.b #$01
    sta.w !v_038e
    stz.w !v_038d
    sta.w !v_0394
    stz.w !v_0393
    jsl.l TaskCreate
    db 4*24, 8                ; slot 4, parameter 8
    dl WindowEffectTask
    jsl.l !routine_81ee04
    lda.b #$A6
    jsl.l !routine_81972d
    lda.b #$0E
    jsl.l !routine_81972d
    lda.b #$10
    jsl.l !routine_81972d
    lda.b #$12
    jsl.l !routine_81972d
    stz.w !v_0847
    stz.w !v_0848
    stz.w !v_0849
    stz.w !v_084a
    stz.w !v_084b
    stz.w !level_035c
    stz.w !level
    stz.w !v_035d
    lda.w !cheat_0009
    beq .l9F7E
    jsr.w LevelNumberText
.l9F7E:
    lda.b #$03
    sta.w !health_marks
    lda.b #$03
    sta.w !v_0362
    lda.b #$10
    sta.w !v_0369
    stz.w !v_036c
    stz.w !v_036e
    stz.w !v_036b
    stz.w !v_142e
    stz.w !v_0347
    stz.w !v_0370
    stz.w !v_0358
    stz.w !v_0373
    stz.w !v_0374
    stz.w !v_0375
    stz.w !v_0376
    stz.w !v_0359
    stz.w !v_035a
    lda.b #$03
    sta.w !continues
.l9FB9:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !cheat_0009
    beq .l9FCD
    jsr.w LevelSelect
    bcc .l9FD1
.l9FCD:
    jsl.l !routine_81eda2
.l9FD1:
    rep.b #$20
    dec.w !v_0343
    sep.b #$20
    bne .l9FF3
    lda.b #$04
    jsl.l SoundQueueF6
    lda.w !v_0345
    beq .l9FEC
    stz.w !v_0345
    jml.l OpeningStory
.l9FEC:
    inc.w !v_0345
    jml.l !routine_818031
.l9FF3:
    jsr.w PadConfirmPressed
    beq .l9FB9
    lda.b #$01
    jsl.l TaskWait
    lda.b #$4F
    jsl.l SoundQueueAdd
    lda.b #$01
    jsl.l TaskWait
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819eb9
    jsl.l SoundQueueF6With1
.lA017:
    lda.b #$01
    jsl.l TaskWait
    lda.w !v_0087
    bne .lA017
    stz.w !hdmaen_shadow
    stz.w !HDMAEN
    jsl.l TaskSlotsFree1To6
    lda.w !v_0849
    cmp.b #$02
    bne .lA03A
    jsl.l TaskExec
    dl !routine_838979
.lA03A:
    lda.w !v_0849
    cmp.b #$01
    bne .lA048
    jsl.l TaskExec
    dl PasswordScreen
.lA048:
    jsl.l TaskExec
    dl !routine_818095
