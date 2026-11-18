; options_task.asm: the options screen's task: the screen set up, then each
; frame the cheat code, the cursor's moves and the items' steps.
; C: src/aladdin/game/options_task.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/actor.inc"
incsrc "../include/hw.inc"

arch 65816

!OPTION_ITEM = $1B14            ; the cursor's item, times 2 (0 setting, 2 sound test, 4 output, 6 exit)
!SOUND_TEST = $1B15             ; the sound test's number, less one (0-34)
!OPTION_SETTING = $1B17         ; the setting the options screen shows (0-3)
!CODE_WAIT = $1B19              ; frames since the cheat code's last press
!CODE_AT = $1B1A                ; presses of the cheat code made (0: none yet)
!PRESSED_HIGH = $037C           ; this frame's presses, high byte: B Y Select Start Up Down Left Right
!CODE_PRESSES_NOW = $037D       ; the presses the cheat code compares (word)
!V_0399 = $0399                 ; set to $16 here; purpose not yet established
; In the bank DB names: the cheat code's presses (a word each, from the second).
!CODE_PRESSES = $CE63

; @routine 83:8979 OptionsTask
; WHAT IT DOES: The options screen's task. Sets the screen up: the
; screen's graphics and palettes, its figure (an actor of kind $82), the
; task that animates it, its text, the colour math, the cursor on the
; first item and each item's setting shown. Then every frame: the cheat
; code (Select, then the presses at CODE_PRESSES one after another, each
; within 32 frames, sets CHEAT_0009 and plays sound $28); A, B, X or Y on
; the sound test plays its song, on the exit item leaves; Start leaves;
; up and down move the cursor (wrapping), with sounds $F2 and $0C; left
; and right run the item's step through OptionSteps (C clear left, set
; right), with sound $0C.
; HOW: NmiDisable, ScreenBlank, ScrollStateClear, OamBufferClear,
; DisplayDefaults, TextLayerClear; the text's upload ($5C00, $0800
; bytes); LEVEL = $2B, LevelRamSetupLong; V_1A08 = $FF; unpacked sets $01,
; $0D, $02, $22 to VRAM at $00, $70, $77, $FC; NmiEnable; V_0399 = $16;
; palettes $08, $0B, $57. ActorCreate kind $82: state $0C, look $0900, x
; and y $0080. TaskCreate (slot $18, $04, the task at $81:9E8E); text
; layout $2E; CGWSEL clear, CGADSUB $90, COLDATA red 2, green 1, blue 6.
; The item 0, the sound test 0, the setting from KEPT_OPTIONS; the cursor
; cleared and drawn; the setting, sound test and output shown; the code
; cleared. Each frame (TaskWait 1, FrameStepLong): the code unless
; CHEAT_0009 is set; the presses as above.
; Entry: a task (TaskExec from the title task); M 8-bit, X 8-bit; DB a
; bank whose $0000-$1FFF is work RAM and that holds CODE_PRESSES and
; OptionSteps ($83). Exit: by OptionsLeave.
%at($838979)
OptionsTask:
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    jsl.l TextLayerClear
    rep.b #$20
    lda.w #$5C00
    sta.w !text_upload_vram
    lda.w #$0800
    sta.w !text_upload_size
    sep.b #$20
    lda.b #$2B
    sta.w !level
    jsl.l LevelRamSetupLong
    lda.b #$FF
    sta.w !v_1a08
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$0D
    jsl.l UnpackSet
    ldx.b #$70
    jsl.l VramCopy
    lda.b #$02
    jsl.l UnpackSet
    ldx.b #$77
    jsl.l VramCopy
    lda.b #$22
    jsl.l UnpackSet
    ldx.b #$FC
    jsl.l VramCopy
    jsl.l NmiEnable
    lda.b #$16
    sta.w !V_0399
    lda.b #$08
    jsl.l PaletteLoad
    lda.b #$0B
    jsl.l PaletteLoad
    lda.b #$57
    jsl.l PaletteLoad
    lda.b #$82
    jsl.l ActorCreate
    bcs .l8A1B
    lda.b #$0C
    sta.w !actor_state,x
    rep.b #$20
    lda.w #$0900
    sta.w !actor_look,x
    lda.w #$0080
    sta.w !actor_x+1,x
    lda.w #$0080
    sta.w !actor_y+1,x
.l8A1B:
    sep.b #$20
    jsl.l TaskCreate
    db $18,$04,$8E,$9E,$81
    lda.b #$2E
    jsl.l TextLayoutShow
    stz.w !cgwsel_shadow
    lda.b #$90
    sta.w !cgadsub_shadow
    lda.b #$02
    sta.w !coldata_red
    lda.b #$01
    sta.w !coldata_green
    lda.b #$06
    sta.w !coldata_blue
    stz.w !OPTION_ITEM
    stz.w !SOUND_TEST
    lda.w !kept_options
    sta.w !OPTION_SETTING
    jsr.w OptionCursorClear
    ldy.w !OPTION_ITEM
    jsr.w OptionCursorDraw
    lda.w !OPTION_SETTING
    jsr.w OptionSettingShow
    jsr.w SoundTestShow
    jsr.w OptionStereoShow
    stz.w !CODE_WAIT
    stz.w !CODE_AT
.l8A6A:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    lda.w !cheat_0009
    bne .l8ACC
    lda.w !CODE_AT
    bne .l8A8D
    lda.w !CODE_PRESSES_NOW
    and.b #$20
    beq .l8ACC
    inc.w !CODE_AT
    stz.w !CODE_WAIT
    bra .l8ACC
.l8A8D:
    inc.w !CODE_WAIT
    lda.w !CODE_WAIT
    cmp.b #$20
    bcc .l8A9F
.l8A97:
    stz.w !CODE_WAIT
    stz.w !CODE_AT
    bra .l8ACC
.l8A9F:
    lda.w !CODE_AT
    asl a
    tax
    rep.b #$20
    lda.w !CODE_PRESSES_NOW
    beq .l8ACA
    cmp.w !CODE_PRESSES,x
    sep.b #$20
    bne .l8A97
    stz.w !CODE_WAIT
    inc.w !CODE_AT
    lda.w !CODE_AT
    cmp.b #$08
    bne .l8ACC
    lda.b #$01
    sta.w !cheat_0009
    lda.b #$28
    jsl.l SoundQueueAdd
.l8ACA:
    sep.b #$20
.l8ACC:
    lda.w !PRESSED_HIGH
    ora.w !joy_pressed
    bit.b #$C0
    beq .l8AF4
    lda.w !OPTION_ITEM
    cmp.b #$02
    bne .l8AEA
    lda.w !SOUND_TEST
    clc
    adc.b #$10
    jsl.l SongPlay
    jmp.w .l8A6A
.l8AEA:
    cmp.b #$06
    beq .l8AF1
    jmp.w .l8A6A
.l8AF1:
    jmp.w OptionsLeave
.l8AF4:
    lda.w !PRESSED_HIGH
    bit.b #$10
    beq .l8AFE
    jmp.w OptionsLeave
.l8AFE:
    bit.b #$08
    beq .l8B0E
    dec.w !OPTION_ITEM
    dec.w !OPTION_ITEM
    bpl .l8B24
    lda.b #$06
    bra .l8B21
.l8B0E:
    bit.b #$04
    beq .l8B39
    inc.w !OPTION_ITEM
    inc.w !OPTION_ITEM
    lda.w !OPTION_ITEM
    cmp.b #$08
    bcc .l8B24
    lda.b #$00
.l8B21:
    sta.w !OPTION_ITEM
.l8B24:
    jsl.l SoundQueueF2
    lda.b #$0C
    jsl.l SoundQueueAdd
    jsr.w OptionCursorClear
    ldy.w !OPTION_ITEM
    jsr.w OptionCursorDraw
    bra .l8B47
.l8B39:
    bit.b #$02
    beq .l8B40
    clc
    bra .l8B4A
.l8B40:
    bit.b #$01
    beq .l8B47
    sec
    bra .l8B4A
.l8B47:
    jmp.w .l8A6A
.l8B4A:
    ldx.w !OPTION_ITEM
    jsr.w (OptionSteps,x)
    lda.b #$0C
    jsl.l SoundQueueAdd
    jmp.w .l8A6A

; Each item's step, by the item (OPTION_ITEM): left with C clear, right
; with C set.
OptionSteps:
    dw OptionSettingStep, SoundTestStep, OptionStereoToggle, OptionExitStep
