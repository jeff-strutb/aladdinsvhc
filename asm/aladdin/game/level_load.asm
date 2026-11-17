; level_load.asm: loading a level: its graphics, its RAM and VRAM, its
; screen, then Aladdin and the copier check; and a card of text shown on
; its own. Drafted by asmdraft.py; named and described by hand.
; C: src/aladdin/game/level_load.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!LL_SET_A = $80B7D8             ; by level: a graphics set of its own (negative: none)
!LL_SET_B = $80B7F5             ; by level: a second one
!LL_PLAYER_06 = $80B764         ; by level: Aladdin's !actor_06
!LL_PLAYER_07 = $80B781         ; by level: Aladdin's !actor_07
!LL_WINDOW_OPTIONS = $80B9E2    ; by option (words): !LL_COLOUR_WINDOWS + 2, then + 0, + 4, + 6, in tables 8 bytes apart
!LL_COLOUR_WINDOWS = $0348      ; four words the colour-math windows take
!LL_V_03FA = $03FA
!LL_V_03E9 = $03E9
!LL_V_083F = $083F
!LL_TM_SHADOW = $0399
!LL_TS_SHADOW = $039A
!LL_FADE_BUSY = $0087           ; nonzero while a colour fade runs

; @routine 81:8664 LevelGraphicsLoadAllLong
; WHAT IT DOES: LevelGraphicsLoadAll for callers in other banks.
; HOW: JSR LevelGraphicsLoadAll, RTL.
; Entry: as LevelGraphicsLoadAll; called by JSL. Exit: as
; LevelGraphicsLoadAll.
%at($818664)
LevelGraphicsLoadAllLong:
    jsr.w LevelGraphicsLoadAll
    rtl

; @routine 81:8668 LevelGraphicsLoadAll
; WHAT IT DOES: Loads the level's graphics into VRAM: the sets every level uses
; (1, $0C, $2D), up to two of its own, then its tiles and its actors'.
; HOW: UnpackSet and VramCopy for sets 1 (X 0) and $0C (X $1C), set $2D by
; VramCopy2 (X $62); for levels below $1F, the set at LEVEL_SET_A by
; level (X $23) and the one at LEVEL_SET_B (X $2A), unless negative;
; LevelGraphicsLoad, LevelActorGraphicsLoad.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: as LevelActorGraphicsLoad.
%at($818668)
LevelGraphicsLoadAll:
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$0C
    jsl.l UnpackSet
    ldx.b #$1C
    jsl.l VramCopy
    lda.b #$2D
    jsl.l UnpackSet
    ldx.b #$62
    jsl.l VramCopy2
    lda.w !level
    sec
    cmp.b #$1F
    bcs .l86B6
    tax
    lda.w !LL_SET_A,x
    bmi .l86B6
    jsl.l UnpackSet
    ldx.b #$23
    jsl.l VramCopy
    ldx.w !level
    lda.w !LL_SET_B,x
    bmi .l86B6
    jsl.l UnpackSet
    ldx.b #$2A
    jsl.l VramCopy
.l86B6:
    jsl.l LevelGraphicsLoad
    jsl.l LevelActorGraphicsLoad
    rts

; @routine 81:825B LevelRamSetupLong
; WHAT IT DOES: LevelRamSetup for callers in other banks.
; HOW: JSR LevelRamSetup, RTL.
; Entry: as LevelRamSetup; called by JSL. Exit: as LevelRamSetup.
%at($81825B)
LevelRamSetupLong:
    jsr.w LevelRamSetup
    rtl

; @routine 81:825F LevelRamSetup
; WHAT IT DOES: Sets up everything in RAM and VRAM a level starts from: game RAM
; cleared, the tasks but the game's freed, the text upload size, the live
; actors' counts, display defaults, animation sets, graphics, actor
; records, palettes, the text layer, scroll limits, the start place, the
; placements and the level's palettes.
; HOW: GameRamClear, TaskSlotsFree1To6; TEXT_UPLOAD_SIZE $0160 (levels
; below $14) or $0800; V_03FA counted up; KIND_COUNT's 256 bytes 0;
; DisplayDefaults, AnimSetsLoad, LevelGraphicsLoadAll, ActorRecordsInit;
; PaletteLoad 2, 0, $0C, 9; TextLayerClear, LevelScrollLimits, V_083F 0,
; LevelStartPlace, PlacementsStart, LevelPalettesLoad.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: as LevelPalettesLoad.
%at($81825F)
LevelRamSetup:
    jsl.l GameRamClear
    jsl.l TaskSlotsFree1To6
    sec
    lda.w !level
    cmp.b #$14
    bcs .l827B
    rep.b #$20
    lda.w #$0160
    sta.w !text_upload_size
    sep.b #$20
    bra .l8285
.l827B:
    rep.b #$20
    lda.w #$0800
    sta.w !text_upload_size
    sep.b #$20
.l8285:
    inc.w !LL_V_03FA
    ldx.b #$00
    lda.b #$00
.l828C:
    sta.l !kind_count,x
    inx
    bne .l828C
    jsl.l DisplayDefaults
    jsl.l AnimSetsLoad
    jsr.w LevelGraphicsLoadAll
    jsl.l ActorRecordsInit
    lda.b #$02
    jsl.l PaletteLoad
    lda.b #$00
    jsl.l PaletteLoad
    lda.b #$0C
    jsl.l PaletteLoad
    lda.b #$09
    jsl.l PaletteLoad
    jsl.l TextLayerClear
    jsr.w LevelScrollLimits
    stz.w !LL_V_083F
    jsr.w LevelStartPlace
    jsl.l PlacementsStart
    jsr.w LevelPalettesLoad
    rts

; @routine 81:81A3 LevelLoadLong
; WHAT IT DOES: LevelLoad for callers in other banks.
; HOW: JSR LevelLoad, RTL.
; Entry: as LevelLoad; called by JSL. Exit: as LevelLoad.
%at($8181A3)
LevelLoadLong:
    jsr.w LevelLoad
    rtl

; @routine 81:81A7 LevelLoad
; WHAT IT DOES: Loads and starts the level: its colour-math windows by the
; options (fixed ones in a demo), the screen blanked while LevelRamSetup,
; the HUD, the map, the screen set-up, the layers, spots, tile behaviours
; and backdrop are made, then the screen back on, its song, Aladdin's
; start and the copier check's task.
; HOW: COLOUR_WINDOWS (four words) $4000, $8000, $80, $10 with PLAY_MODE
; set, else by KEPT_OPTIONS' low byte from the tables at $80:B9E2-B9FA;
; TextLayerClearFront, TaskWait 1, NmiDisable, ScreenBlank, LevelRamSetup,
; V_03E9 0, DisplayDefaults, HudRedrawAll (levels below $14),
; LevelMapBuild, LevelScreenSetup, LayersFill, LevelSpotsSet,
; TileBehaviourLoad, LevelBackdropStart, NmiEnable; with no PLAY_MODE,
; TaskWait 1 and LevelSongPlay; Aladdin's state $0C and his ACTOR_06 and
; ACTOR_07 by level ($80:B764, $80:B781); EscortStart,
; LevelCompanionStart; TaskCreate slot 7 MirrorCheckTask.
; Entry: M 8-bit, X 8-bit; DB = $80; in the game's task (it waits).
; Exit: as TaskCreate leaves the registers, M 8-bit.
%at($8181A7)
LevelLoad:
    nop
    nop
    nop
    rep.b #$20
    ldy.w !play_mode
    beq .l81CB
    lda.w #$4000
    sta.w !LL_COLOUR_WINDOWS+2
    lda.w #$8000
    sta.w !LL_COLOUR_WINDOWS
    lda.w #$0080
    sta.w !LL_COLOUR_WINDOWS+4
    lda.w #$0010
    sta.w !LL_COLOUR_WINDOWS+6
    bra .l81EB
.l81CB:
    lda.w !kept_options
    and.w #$00FF
    asl a
    tax
    lda.w !LL_WINDOW_OPTIONS,x
    sta.w !LL_COLOUR_WINDOWS+2
    lda.w !LL_WINDOW_OPTIONS+8,x
    sta.w !LL_COLOUR_WINDOWS
    lda.w !LL_WINDOW_OPTIONS+16,x
    sta.w !LL_COLOUR_WINDOWS+4
    lda.w !LL_WINDOW_OPTIONS+24,x
    sta.w !LL_COLOUR_WINDOWS+6
.l81EB:
    sep.b #$20
    jsl.l TextLayerClearFront
    lda.b #$01
    jsl.l TaskWait
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsr.w LevelRamSetup
    stz.w !LL_V_03E9
    jsl.l DisplayDefaults
    lda.w !level
    cmp.b #$14
    bcs .l8213
    jsr.w HudRedrawAll
.l8213:
    jsr.w LevelMapBuild
    jsr.w LevelScreenSetup
    jsr.w LayersFill
    jsr.w LevelSpotsSet
    jsr.w TileBehaviourLoad
    jsr.w LevelBackdropStart
    jsl.l NmiEnable
    lda.w !play_mode
    bne .l8237
    lda.b #$01
    jsl.l TaskWait
    jsr.w LevelSongPlay
.l8237:
    lda.b #$0C
    sta.w !player
    ldx.w !level
    lda.w !LL_PLAYER_06,x
    sta.w !player+!actor_06
    lda.w !LL_PLAYER_07,x
    sta.w !player+!actor_07
    jsr.w EscortStart
    jsr.w LevelCompanionStart
    jsl.l TaskCreate
    db 7*24, 0                ; slot 7, parameter 0
    dl MirrorCheckTask
    rts

; @routine 81:ED33 TextCardScreen
; WHAT IT DOES: Shows a card of text on its own (text layout 6, layer 3 alone):
; the screen made with NMI off, faded in, held $78 frames, faded out,
; then the text layer cleared.
; HOW: NmiDisable, ScreenBlank, ScrollStateClear, TextLayerClear,
; OamBufferClear, DisplayDefaults; UnpackSet 1 and VramCopy (X 0);
; PaletteLoad 2; TM_SHADOW and TS_SHADOW 4; TextLayoutShow 6; NmiEnable;
; TaskCreate slot 1 ColourFadeInTask (4), TaskWait 1 until FADE_BUSY is
; 0; TaskWait $78; TaskCreate slot 1 ColourFadeOutTask (4), the same
; wait; TextLayerClear.
; Entry: M 8-bit, X 8-bit; DB = $80; in the game's task (it waits).
; Exit: as TextLayerClear.
%at($81ED33)
TextCardScreen:
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l TextLayerClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$02
    jsl.l PaletteLoad
    lda.b #$04
    sta.w !LL_TM_SHADOW
    sta.w !LL_TS_SHADOW
    lda.b #$06
    jsl.l TextLayoutShow
    jsl.l NmiEnable
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl ColourFadeInTask
.lED78:
    lda.b #$01
    jsl.l TaskWait
    lda.w !LL_FADE_BUSY
    bne .lED78
    lda.b #$78
    jsl.l TaskWait
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl ColourFadeOutTask
.lED92:
    lda.b #$01
    jsl.l TaskWait
    lda.w !LL_FADE_BUSY
    bne .lED92
    jsl.l TextLayerClear
    rts

