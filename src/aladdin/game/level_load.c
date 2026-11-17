/*
 * level_load.c: loading a level: its graphics, its RAM and VRAM, its
 * screen, then Aladdin and the copier check; and a card of text shown on
 * its own. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"
void abi_ActorRecordsInit(void);
void abi_AnimSetsLoad(void);
void abi_DisplayDefaults(void);
void abi_EscortStart(void);
void abi_GameRamClear(void);
void abi_HudRedrawAll(void);
void abi_LayersFill(void);
void abi_LevelActorGraphicsLoad(void);
void abi_LevelBackdropStart(void);
void abi_LevelCompanionStart(void);
void abi_LevelGraphicsLoad(void);
void abi_LevelMapBuild(void);
void abi_LevelPalettesLoad(void);
void abi_LevelScreenSetup(void);
void abi_LevelScrollLimits(void);
void abi_LevelSongPlay(void);
void abi_LevelSpotsSet(void);
void abi_LevelStartPlace(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_PlacementsStart(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_TaskCreate(void);
void abi_TaskSlotsFree1To6(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_TextLayerClearFront(void);
void abi_TextLayoutShow(void);
void abi_TileBehaviourLoad(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_LevelGraphicsLoadAll(void);
void abi_LevelRamSetup(void);
void abi_LevelLoad(void);


#define LEVEL_SET_A 0xB7D8u        /* by level: a graphics set of its own (negative: none) */
#define LEVEL_SET_B 0xB7F5u        /* by level: a second one */
#define LEVEL_PLAYER_06 0xB764u    /* by level: Aladdin's ACTOR_06 */
#define LEVEL_PLAYER_07 0xB781u    /* by level: Aladdin's ACTOR_07 */
#define WINDOW_OPTIONS 0xB9E2u     /* by option (words): COLOUR_WINDOWS + 2, then + 0, + 4, + 6, in tables 8 bytes apart */
#define COLOUR_WINDOWS 0x0348u     /* four words the colour-math windows take */
#define V_03FA 0x03FAu
#define V_03E9 0x03E9u
#define V_083F 0x083Fu
#define TM_SHADOW 0x0399u
#define TS_SHADOW 0x039Au
#define FADE_BUSY 0x0087u          /* nonzero while a colour fade runs */

/*
 * @routine 81:8664 LevelGraphicsLoadAllLong
 * WHAT IT DOES: LevelGraphicsLoadAll for callers in other banks.
 * HOW: JSR LevelGraphicsLoadAll, RTL.
 * Entry: as LevelGraphicsLoadAll; called by JSL. Exit: as
 * LevelGraphicsLoadAll.
 */
void abi_LevelGraphicsLoadAllLong(void)
{
    abi_LevelGraphicsLoadAll();
    return;
}

/*
 * @routine 81:8668 LevelGraphicsLoadAll
 * WHAT IT DOES: Loads the level's graphics into VRAM: the sets every level uses
 * (1, $0C, $2D), up to two of its own, then its tiles and its actors'.
 * HOW: UnpackSet and VramCopy for sets 1 (X 0) and $0C (X $1C), set $2D by
 * VramCopy2 (X $62); for levels below $1F, the set at LEVEL_SET_A by
 * level (X $23) and the one at LEVEL_SET_B (X $2A), unless negative;
 * LevelGraphicsLoad, LevelActorGraphicsLoad.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: as LevelActorGraphicsLoad.
 */
void abi_LevelGraphicsLoadAll(void)
{
    lda8_v(0x01);
    abi_UnpackSet();
    ldx8_v(0x00);
    abi_VramCopy();
    lda8_v(0x0C);
    abi_UnpackSet();
    ldx8_v(0x1C);
    abi_VramCopy();
    lda8_v(0x2D);
    abi_UnpackSet();
    ldx8_v(0x62);
    abi_VramCopy2();
    lda8(ea_abs(LEVEL));
    sec();
    cmp8_v(0x1F);
    if (IF_C) goto L_8186B6;
    tax8();
    lda8(ea_absx(LEVEL_SET_A));
    if (IF_N) goto L_8186B6;
    abi_UnpackSet();
    ldx8_v(0x23);
    abi_VramCopy();
    ldx8(ea_abs(LEVEL));
    lda8(ea_absx(LEVEL_SET_B));
    if (IF_N) goto L_8186B6;
    abi_UnpackSet();
    ldx8_v(0x2A);
    abi_VramCopy();
L_8186B6:
    abi_LevelGraphicsLoad();
    abi_LevelActorGraphicsLoad();
    return;
}

/*
 * @routine 81:825B LevelRamSetupLong
 * WHAT IT DOES: LevelRamSetup for callers in other banks.
 * HOW: JSR LevelRamSetup, RTL.
 * Entry: as LevelRamSetup; called by JSL. Exit: as LevelRamSetup.
 */
void abi_LevelRamSetupLong(void)
{
    abi_LevelRamSetup();
    return;
}

/*
 * @routine 81:825F LevelRamSetup
 * WHAT IT DOES: Sets up everything in RAM and VRAM a level starts from: game RAM
 * cleared, the tasks but the game's freed, the text upload size, the live
 * actors' counts, display defaults, animation sets, graphics, actor
 * records, palettes, the text layer, scroll limits, the start place, the
 * placements and the level's palettes.
 * HOW: GameRamClear, TaskSlotsFree1To6; TEXT_UPLOAD_SIZE $0160 (levels
 * below $14) or $0800; V_03FA counted up; KIND_COUNT's 256 bytes 0;
 * DisplayDefaults, AnimSetsLoad, LevelGraphicsLoadAll, ActorRecordsInit;
 * PaletteLoad 2, 0, $0C, 9; TextLayerClear, LevelScrollLimits, V_083F 0,
 * LevelStartPlace, PlacementsStart, LevelPalettesLoad.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: as LevelPalettesLoad.
 */
void abi_LevelRamSetup(void)
{
    abi_GameRamClear();
    abi_TaskSlotsFree1To6();
    sec();
    lda8(ea_abs(LEVEL));
    cmp8_v(0x14);
    if (IF_C) goto L_81827B;
    op_rep(0x20);
    lda16_v(0x0160);
    sta16(ea_abs(TEXT_UPLOAD_SIZE));
    op_sep(0x20);
    goto L_818285;
L_81827B:
    op_rep(0x20);
    lda16_v(0x0800);
    sta16(ea_abs(TEXT_UPLOAD_SIZE));
    op_sep(0x20);
L_818285:
    MEM8(inc, ea_abs(V_03FA));
    ldx8_v(0x00);
    lda8_v(0x00);
L_81828C:
    sta8(ea_longx(KIND_COUNT));
    inx8();
    if (!IF_Z) goto L_81828C;
    abi_DisplayDefaults();
    abi_AnimSetsLoad();
    abi_LevelGraphicsLoadAll();
    abi_ActorRecordsInit();
    lda8_v(0x02);
    abi_PaletteLoad();
    lda8_v(0x00);
    abi_PaletteLoad();
    lda8_v(0x0C);
    abi_PaletteLoad();
    lda8_v(0x09);
    abi_PaletteLoad();
    abi_TextLayerClear();
    abi_LevelScrollLimits();
    stz8(ea_abs(V_083F));
    abi_LevelStartPlace();
    abi_PlacementsStart();
    abi_LevelPalettesLoad();
    return;
}

/*
 * @routine 81:81A3 LevelLoadLong
 * WHAT IT DOES: LevelLoad for callers in other banks.
 * HOW: JSR LevelLoad, RTL.
 * Entry: as LevelLoad; called by JSL. Exit: as LevelLoad.
 */
void abi_LevelLoadLong(void)
{
    abi_LevelLoad();
    return;
}

/*
 * @routine 81:81A7 LevelLoad
 * WHAT IT DOES: Loads and starts the level: its colour-math windows by the
 * options (fixed ones in a demo), the screen blanked while LevelRamSetup,
 * the HUD, the map, the screen set-up, the layers, spots, tile behaviours
 * and backdrop are made, then the screen back on, its song, Aladdin's
 * start and the copier check's task.
 * HOW: COLOUR_WINDOWS (four words) $4000, $8000, $80, $10 with PLAY_MODE
 * set, else by KEPT_OPTIONS' low byte from the tables at $80:B9E2-B9FA;
 * TextLayerClearFront, TaskWait 1, NmiDisable, ScreenBlank, LevelRamSetup,
 * V_03E9 0, DisplayDefaults, HudRedrawAll (levels below $14),
 * LevelMapBuild, LevelScreenSetup, LayersFill, LevelSpotsSet,
 * TileBehaviourLoad, LevelBackdropStart, NmiEnable; with no PLAY_MODE,
 * TaskWait 1 and LevelSongPlay; Aladdin's state $0C and his ACTOR_06 and
 * ACTOR_07 by level ($80:B764, $80:B781); EscortStart,
 * LevelCompanionStart; TaskCreate slot 7 MirrorCheckTask.
 * Entry: M 8-bit, X 8-bit; DB = $80; in the game's task (it waits).
 * Exit: as TaskCreate leaves the registers, M 8-bit.
 */
void abi_LevelLoad(void)
{
    ;
    ;
    ;
    op_rep(0x20);
    ldy8(ea_abs(PLAY_MODE));
    if (IF_Z) goto L_8181CB;
    lda16_v(0x4000);
    sta16(ea_abs(COLOUR_WINDOWS + 2));
    lda16_v(0x8000);
    sta16(ea_abs(COLOUR_WINDOWS));
    lda16_v(0x0080);
    sta16(ea_abs(COLOUR_WINDOWS + 4));
    lda16_v(0x0010);
    sta16(ea_abs(COLOUR_WINDOWS + 6));
    goto L_8181EB;
L_8181CB:
    lda16(ea_abs(KEPT_OPTIONS));
    and16_v(0x00FF);
    ACC16(asl);
    tax8();
    lda16(ea_absx(WINDOW_OPTIONS));
    sta16(ea_abs(COLOUR_WINDOWS + 2));
    lda16(ea_absx(WINDOW_OPTIONS + 8));
    sta16(ea_abs(COLOUR_WINDOWS));
    lda16(ea_absx(WINDOW_OPTIONS + 16));
    sta16(ea_abs(COLOUR_WINDOWS + 4));
    lda16(ea_absx(WINDOW_OPTIONS + 24));
    sta16(ea_abs(COLOUR_WINDOWS + 6));
L_8181EB:
    op_sep(0x20);
    abi_TextLayerClearFront();
    lda8_v(0x01);
    jsl_frame(0x8181F3u); abi_TaskWait();
    abi_NmiDisable();
    abi_ScreenBlank();
    abi_LevelRamSetup();
    stz8(ea_abs(V_03E9));
    abi_DisplayDefaults();
    lda8(ea_abs(LEVEL));
    cmp8_v(0x14);
    if (IF_C) goto L_818213;
    abi_HudRedrawAll();
L_818213:
    abi_LevelMapBuild();
    abi_LevelScreenSetup();
    abi_LayersFill();
    abi_LevelSpotsSet();
    abi_TileBehaviourLoad();
    abi_LevelBackdropStart();
    abi_NmiEnable();
    lda8(ea_abs(PLAY_MODE));
    if (!IF_Z) goto L_818237;
    lda8_v(0x01);
    jsl_frame(0x818230u); abi_TaskWait();
    abi_LevelSongPlay();
L_818237:
    lda8_v(0x0C);
    sta8(ea_abs((uint16_t)PLAYER));
    ldx8(ea_abs(LEVEL));
    lda8(ea_absx(LEVEL_PLAYER_06));
    sta8(ea_abs((uint16_t)(PLAYER + ACTOR_06)));
    lda8(ea_absx(LEVEL_PLAYER_07));
    sta8(ea_abs((uint16_t)(PLAYER + ACTOR_07)));
    abi_EscortStart();
    abi_LevelCompanionStart();
    jsl_frame(0x818251u); abi_TaskCreate(); cpu.unwound = 0;
    return;
}

/*
 * @routine 81:ED33 TextCardScreen
 * WHAT IT DOES: Shows a card of text on its own (text layout 6, layer 3 alone):
 * the screen made with NMI off, faded in, held $78 frames, faded out,
 * then the text layer cleared.
 * HOW: NmiDisable, ScreenBlank, ScrollStateClear, TextLayerClear,
 * OamBufferClear, DisplayDefaults; UnpackSet 1 and VramCopy (X 0);
 * PaletteLoad 2; TM_SHADOW and TS_SHADOW 4; TextLayoutShow 6; NmiEnable;
 * TaskCreate slot 1 ColourFadeInTask (4), TaskWait 1 until FADE_BUSY is
 * 0; TaskWait $78; TaskCreate slot 1 ColourFadeOutTask (4), the same
 * wait; TextLayerClear.
 * Entry: M 8-bit, X 8-bit; DB = $80; in the game's task (it waits).
 * Exit: as TextLayerClear.
 */
void abi_TextCardScreen(void)
{
    abi_NmiDisable();
    abi_ScreenBlank();
    abi_ScrollStateClear();
    abi_TextLayerClear();
    abi_OamBufferClear();
    abi_DisplayDefaults();
    lda8_v(0x01);
    abi_UnpackSet();
    ldx8_v(0x00);
    abi_VramCopy();
    lda8_v(0x02);
    abi_PaletteLoad();
    lda8_v(0x04);
    sta8(ea_abs(TM_SHADOW));
    sta8(ea_abs(TS_SHADOW));
    lda8_v(0x06);
    abi_TextLayoutShow();
    abi_NmiEnable();
    jsl_frame(0x81ED6Fu); abi_TaskCreate(); cpu.unwound = 0;
L_81ED78:
    lda8_v(0x01);
    jsl_frame(0x81ED7Au); abi_TaskWait();
    lda8(ea_abs(FADE_BUSY));
    if (!IF_Z) goto L_81ED78;
    lda8_v(0x78);
    jsl_frame(0x81ED85u); abi_TaskWait();
    jsl_frame(0x81ED89u); abi_TaskCreate(); cpu.unwound = 0;
L_81ED92:
    lda8_v(0x01);
    jsl_frame(0x81ED94u); abi_TaskWait();
    lda8(ea_abs(FADE_BUSY));
    if (!IF_Z) goto L_81ED92;
    abi_TextLayerClear();
    return;
}

