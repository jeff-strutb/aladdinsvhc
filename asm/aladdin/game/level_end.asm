; level_end.asm: the screen at a level's end (81:F61D) and its pieces: 2x2-tile
; blocks queued for VRAM, the four columns its picture grows in step by
; step. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/game/level_end.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

!BLANK_TILE = $080E             ; the tilemap word a blanked block gets
!END_STEPS_DONE = $AD           ; in the direct page: counts the pieces placed
; In the data bank: per column the tile's high byte; per step the VRAM
; word address of column 0 (words); per level (level_035c less 1) four
; picture indices, a byte per column; per index the picture's first tile.
!END_COLUMN_ATTR = $FDF7
!END_STEP_VRAM = $FDC5
!END_LEVEL_PICTURES = $FDD5
!END_PICTURE_TILE = $FDF1
; In the data bank: the VRAM word addresses of the 16 blocks blanked (words).
!END_BLANK_BLOCKS = $FE02
; LevelEndTask: what it sets and reads in work RAM, the direct page (D = 0)
; and the data bank.
!V_005D = $005D                 ; nonzero on a PAL console (RegionCheck)
!v_0005 = $0005                 ; set to LEVEL_035C
!v_0376 = $0376                 ; GEM_SETS added up at each level end
!GEM_SETS_FOUND = $0373         ; a bit per set gem found (word)
!GEM_SETS = $0375               ; set gems found (the screen's figures, kind $A8, pose by it)
!PAUSED = $0405                 ; counted up while paused
!SCENE_WAIT = $0087             ; nonzero while the screen's fade runs
!tm_shadow = $0399              ; TM's shadow: the main screen's layers
!ts_shadow = $039A              ; TS's shadow: the sub screen's layers
!END_COUNT = $A5                ; the count the picture grows by: $A5, then $A6
!END_NEXT_AT = $A7              ; the count at which the next block is placed
!END_NEXT_BLOCK = $A8           ; the next block of the picture
!END_COLUMN_STEP = $A9          ; per column (4 bytes), its step
!CUE_AE = $AE                   ; the figures' cue (as kind $A8's)
!END_FRAMES = $AF               ; frames left of a pause
!END_COPIER_COUNT = $701399     ; a byte a copier's cartridge RAM keeps (a retail cartridge has none): counted up here
; In the data bank: per part (LEVEL_035C), its block's offset (a byte) and
; its four palettes (bytes, 4 a part); per column, the count its growth
; stops at.
!END_PART_BLOCKS = $FDFA
!END_PART_PALETTES = $FDD1
!END_COLUMN_LIMIT = $FDCD

; @routine 81:F61D LevelEndTask
; WHAT IT DOES: The screen between a level's acts: on a PAL console a
; screen of its own; else the level-end picture set up (level $24's RAM,
; its graphics, palettes and text), faded in, its four columns grown step
; by step as the counts at $A5-$A6 go, a pause, then the wait for a
; button; faded out, and on to the story card (or, for part 7, the task at
; $81:8181).
; HOW: $005D set: JML $84:AFCC. $0005 = LEVEL_035C; the display shut and
; cleared (KeptOptionsSeal, NmiDisable, ScreenBlank, ScrollStateClear,
; OamBufferClear, DisplayDefaults); $AD, $AE 0; $0376 += $0375; LEVEL
; $24, LevelRamSetupLong; sets $0E and $23 unpacked and nine VRAM copies,
; VramCopy2 $70, palette $4D, VramFillMap $4000 from 0, LevelEndBlanks;
; a frame; palette cycles $20 and $21, $0405 1, set $19, VramCopy $23,
; palette $3D. Part 5: five tile blocks (TileBlockQueueOne) over two
; frames. The part's block from $FDFA (TileBlockQueueRun), text layout
; $6A, the part's four palettes ($FDD1); BG3 x $FC, $0399/$039A $17,
; colour 0 $1806; a frame; TaskCreate (slot 1, parameter 2,
; BrightnessUpTask); frames of FrameStepLong until $0087 is 0;
; brightness 15. Then frames until $A6 is 0 and $A5 reaches $E0: each
; time $A5 passes $A7 ($20, then 16 more), $AD up and the next block of
; the picture ($A8) queued. Then ($0405 0, $A9-$AC 0) each frame, while
; $A6 is 0 or 1, for the four columns ($000C) whose step $A9,x is even
; and under 8 while $A5 is below the column's limit ($FDCD): the step's
; tile (LevelEndColumnPlace, LevelEndColumnTile) queued, the step up.
; $A6 past 1: 255 frames, 16 frames, $AE up, 72 frames; frames until A,
; B, X, Y or START is newly pressed; TaskCreate (slot 1, parameter 1,
; BrightnessDownTask), frames until $0087 is 0; forced blank. The copier
; RAM's count at $70:1399 up: when it keeps it, LEVEL_035C becomes 1 at
; or past $0003, else 6. $0373-$0375 0; part 7: TaskExec $81:8181,
; else TaskExec StoryCardTask.
; Entry: M 8-bit, X 8-bit; DB = $81; D = 0; a task's entry (TaskExec).
; Exit: never returns.
%at($81F61D)
LevelEndTask:
    lda.w !V_005D
    beq .lF626
    jml.l !pal_screen
.lF626:
    lda.w !level_035c
    sta.w !v_0005
    jsl.l KeptOptionsSeal
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    stz.b !END_STEPS_DONE
    stz.b !CUE_AE
    lda.w !v_0376
    clc
    adc.w !GEM_SETS
    sta.w !v_0376
    lda.b #$24
    sta.w !level
    jsl.l LevelRamSetupLong
    lda.b #$0E
    jsl.l UnpackSet
    ldx.b #$7E
    jsl.l VramCopy
    lda.b #$23
    jsl.l UnpackSet
    ldx.b #$69
    jsl.l VramCopy
    ldx.b #$93
    jsl.l VramCopy
    ldx.b #$A1
    jsl.l VramCopy
    ldx.b #$AF
    jsl.l VramCopy
    ldx.b #$BD
    jsl.l VramCopy
    ldx.b #$CB
    jsl.l VramCopy
    ldx.b #$D9
    jsl.l VramCopy
    ldx.b #$E7
    jsl.l VramCopy
    ldx.b #$70
    jsl.l VramCopy2
    lda.b #$4D
    jsl.l PaletteLoad
    rep.b #$10
    ldx.w #$4000
    ldy.w #$0000
    jsl.l VramFillMap
    sep.b #$10
    jsr.w LevelEndBlanks
    jsl.l NmiEnable
    lda.b #$01
    jsl.l TaskWait
    jsl.l NmiDisable
    ldx.b #$00
    lda.b #$20
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$21
    jsr.w PaletteCycleStart
    lda.b #$01
    sta.w !PAUSED
    lda.b #$19
    jsl.l UnpackSet
    ldx.b #$23
    jsl.l VramCopy
    lda.b #$3D
    jsl.l PaletteLoad
    jsl.l NmiEnable
    ldx.w !level_035c
    cpx.b #$05
    bne .lF746
    rep.b #$30
    ldy.w #$0435
    lda.w #$488C
    jsl.l TileBlockQueueOne
    rep.b #$30
    ldy.w #$043B
    lda.w #$4886
    jsl.l TileBlockQueueOne
    rep.b #$30
    ldy.w #$0454
    lda.w #$4888
    jsl.l TileBlockQueueOne
    lda.b #$01
    jsl.l TaskWait
    rep.b #$30
    ldy.w #$0572
    lda.w #$488A
    jsl.l TileBlockQueueOne
    rep.b #$30
    ldy.w #$0570
    lda.w #$4884
    jsl.l TileBlockQueueOne
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    bra .lF759
.lF746:
    rep.b #$31
    lda.w !END_PART_BLOCKS,x
    and.w #$00FF
    adc.w #$04C0
    tay
    lda.w #$488F
    jsl.l TileBlockQueueRun
.lF759:
    jsl.l NmiDisable
    lda.b #$6A
    jsl.l TextLayoutShow
    stz.w !scratch+2
    ldx.b #$00
.lF768:
    lda.w !level_035c
    asl a
    asl a
    adc.w !scratch+2
    tay
    lda.w !END_PART_PALETTES,y
    ldx.w !scratch+2
    jsl.l PaletteFromRom
    inc.w !scratch+2
    lda.w !scratch+2
    cmp.b #$04
    bne .lF768
    lda.b #$FC
    sta.w !bg3_x
    lda.b #$17
    sta.w !tm_shadow
    sta.w !ts_shadow
    lda.b #$06
    sta.l !palette_buffer
    lda.b #$18
    sta.l !palette_buffer+1
    jsl.l NmiEnable
    lda.b #$01
    jsl.l TaskWait
    jsl.l TaskCreate
    db 1*24, 2                  ; slot 1, parameter 2
    dl BrightnessUpTask
.lF7B1:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    lda.w !SCENE_WAIT
    bne .lF7B1
    lda.b #$0F
    sta.w !inidisp_shadow
    lda.b #$20
    sta.b !END_NEXT_AT
    stz.b !END_NEXT_BLOCK
.lF7CB:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    lda.b !END_COUNT+1
    bne .lF7CB
    lda.b !END_COUNT
    cmp.b #$E0
    bcc .lF7E2
    jmp.w .lF808
.lF7E2:
    cmp.b !END_NEXT_AT
    bcc .lF7CB
    inc.b !END_STEPS_DONE
    lda.b !END_NEXT_AT
    adc.b #$0F
    sta.b !END_NEXT_AT
    rep.b #$30
    lda.b !END_NEXT_BLOCK
    and.w #$00FF
    asl a
    pha
    asl a
    adc.w #$0890
    tay
    pla
    adc.w #$4AE4
    jsl.l TileBlockQueueRun
    inc.b !END_NEXT_BLOCK
    bra .lF7CB
.lF808:
    stz.w !PAUSED
    stz.b !END_COLUMN_STEP
    stz.b !END_COLUMN_STEP+1
    stz.b !END_COLUMN_STEP+2
    stz.b !END_COLUMN_STEP+3
.lF813:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    stz.w !scratch
    ldx.b #$00
.lF822:
    lda.w !END_COUNT+1
    beq .lF82D
    dec a
    beq .lF813
    jmp.w .lF8B0
.lF82D:
    lda.w !END_COUNT
    cmp.w !END_COLUMN_LIMIT,x
    bcs .lF8A0
    lda.b !END_COLUMN_STEP,x
    cmp.b #$00
    bne .lF84D
    ldy.b #$00
    jsr.w LevelEndColumnPlace
    jsr.w LevelEndColumnTile
    tay
    lda.w !scratch+2
    jsl.l TileBlockQueueOne
    bra .lF89B
.lF84D:
    cmp.b #$02
    bne .lF867
    ldy.b #$02
    jsr.w LevelEndColumnPlace
    jsr.w LevelEndColumnTile
    clc
    adc.w #$0006
    tay
    lda.w !scratch+2
    jsl.l TileBlockQueueOne
    bra .lF89B
.lF867:
    cmp.b #$04
    bne .lF881
    ldy.b #$04
    jsr.w LevelEndColumnPlace
    jsr.w LevelEndColumnTile
    clc
    adc.w #$0002
    tay
    lda.w !scratch+2
    jsl.l TileBlockQueueOne
    bra .lF89B
.lF881:
    cmp.b #$06
    bne .lF89B
    ldy.b #$06
    jsr.w LevelEndColumnPlace
    jsr.w LevelEndColumnTile
    clc
    adc.w #$0004
    tay
    lda.w !scratch+2
    jsl.l TileBlockQueueOne
    bra .lF89B
.lF89B:
    ldx.w !scratch
    inc.b !END_COLUMN_STEP,x
.lF8A0:
    inc.w !scratch
    ldx.w !scratch
    cpx.b #$04
    beq .lF8AD
    jmp.w .lF822
.lF8AD:
    jmp.w .lF813
.lF8B0:
    lda.b #$FF
    sta.b !END_FRAMES
.lF8B4:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    dec.b !END_FRAMES
    bne .lF8B4
    lda.b #$10
    sta.b !END_FRAMES
.lF8C6:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    dec.b !END_FRAMES
    bne .lF8C6
    inc.b !CUE_AE
    lda.b #$48
    sta.b !END_FRAMES
.lF8DA:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    dec.b !END_FRAMES
    bne .lF8DA
.lF8E8:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    lda.w !joy_pressed
    and.b #$C0
    ora.w !joy_pressed+1
    bit.b #$D0
    beq .lF8E8
    jsl.l TaskCreate
    db 1*24, 1                  ; slot 1, parameter 1
    dl BrightnessDownTask
.lF907:
    lda.b #$01
    jsl.l TaskWait
    lda.w !SCENE_WAIT
    bne .lF907
    lda.b #$80
    sta.w !inidisp_shadow
    lda.l !END_COPIER_COUNT
    inc a
    sta.l !END_COPIER_COUNT
    cmp.l !END_COPIER_COUNT
    bne .lF93A
    lda.w !level_035c
    cmp.w !kept_options
    bcc .lF935
    lda.b #$01
    sta.w !level_035c
    bra .lF93A
.lF935:
    lda.b #$06
    sta.w !level_035c
.lF93A:
    stz.w !GEM_SETS
    stz.w !GEM_SETS_FOUND
    stz.w !GEM_SETS_FOUND+1
    lda.w !level_035c
    cmp.b #$07
    beq .last_part
    jsl.l TaskExec
    dl StoryCardTask
.last_part:
    jsl.l TaskExec
    dl $818181

; @routine 81:F958 LevelEndBlanks
; WHAT IT DOES: Queues 16 blank 2x2-tile blocks for VRAM, at the addresses
; of a table, and has the NMI send them.
; HOW: From the queue's length, TileBlockBlank for each word of the table
; at $FE02 (the last first); the new length kept, then the queue marked
; ready (TILE_BLOCKS_PENDING counted up).
; Entry: DB = $81 (the table's bank); called by JSR from the level-end
; screen's set-up (81:F6B7). Exit: M 8-bit, X 8-bit; Y = $FE; A low = the
; last address's low byte.
%at($81F958)
LevelEndBlanks:
    rep.b #$30
    ldx.w !tile_blocks_length
    ldy.w #$001E
.lF960:
    lda.w !END_BLANK_BLOCKS,y
    jsr.w TileBlockBlank
    dey
    dey
    bpl .lF960
    stx.w !tile_blocks_length
    sep.b #$30
    inc.w !tile_blocks_pending
    rts

; @routine 81:F973 TileBlockBlank
; WHAT IT DOES: Writes a blank 2x2-tile block record at X in the tile-
; block queue: VRAM address A, its four tilemap words $080E.
; HOW: Five word stores in the 10-byte record; X moves on by 10.
; Entry: M 16-bit, X 16-bit; A = the VRAM word address; X = the record's
; offset in TILE_BLOCKS; called by JSR (by 81:F958, for the 16 blocks of a
; table). Exit: M 16-bit, X 16-bit; X = A = the next record's offset; C
; from the add.
%at($81F973)
TileBlockBlank:
    sta.w !tile_blocks,x
    lda.w #!BLANK_TILE
    sta.w !tile_blocks+2,x
    sta.w !tile_blocks+4,x
    sta.w !tile_blocks+6,x
    sta.w !tile_blocks+8,x
    txa
    clc
    adc.w #$000A
    tax
    rts

; @routine 81:F98C LevelEndColumnTile
; WHAT IT DOES: Gives the tilemap word of the picture piece column $000C
; shows for the level, and counts the piece.
; HOW: END_STEPS_DONE steps up; the column's attribute byte as the high
; byte; the level's picture index for the column (LEVEL_035C less 1, times
; 4, plus the column) picks the picture's first tile, doubled, as the low
; bits.
; Entry: M 8-bit, X 8-bit; $000C = the column (0-3); D the direct page of
; END_STEPS_DONE; called by JSR. Exit: M 16-bit, X 16-bit; A = the tilemap
; word; $0010-$0011 = the attribute word; X = the picture index; Y = its
; offset in the level table.
%at($81F98C)
LevelEndColumnTile:
    inc.b !END_STEPS_DONE
    ldx.w !scratch
    lda.w !END_COLUMN_ATTR,x
    sta.w !scratch+5
    stz.w !scratch+4
    lda.w !level_035c
    dec a
    asl a
    asl a
    adc.w !scratch
    tay
    ldx.w !END_LEVEL_PICTURES,y
    lda.w !END_PICTURE_TILE,x
    rep.b #$30
    and.w #$00FF
    asl a
    ora.w !scratch+4
    rts

; @routine 81:F9B4 LevelEndColumnPlace
; WHAT IT DOES: Gives the VRAM address of column X's piece at step Y of
; the level-end picture: the step's address for column 0, less 6 words a
; column.
; HOW: X times 6 (16-bit, by a double and an add) taken from the step's
; word in END_STEP_VRAM; kept at $000E.
; Entry: M 8-bit; X = the column; Y = the step (0, 2, 4, 6); called by
; JSR. Exit: M 8-bit; $000E = A = the VRAM word address; C from the
; subtraction.
%at($81F9B4)
LevelEndColumnPlace:
    txa
    rep.b #$20
    and.w #$00FF
    asl a
    sta.w !scratch+2
    asl a
    adc.w !scratch+2
    sta.w !scratch+2
    lda.w !END_STEP_VRAM,y
    sec
    sbc.w !scratch+2
    sta.w !scratch+2
    sep.b #$20
    rts
