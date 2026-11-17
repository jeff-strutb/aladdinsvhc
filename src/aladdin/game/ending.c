/*
 * ending.c: the ending: its zoom scene's task, the stages task that shows
 * the levels played back, and the text pages with their extras.
 */
#include "machine.h"
#include "ops65.h"
#include "native.h"
#include "cpu_ops.h"
void abi_OamBuildBegin(void);
void abi_LayersScrollLong(void);
void abi_ActorsInView(void);
void abi_OamDrawActors(void);
void abi_BgFollowLayersLong(void);
void abi_ParallaxUpdateLong(void);
void abi_ActorCreate(void);
void abi_ActorRecordsInit(void);
void abi_DemoStartLong(void);
void abi_EndingStagesTask(void);
void abi_EndingTextTask(void);
void abi_KeptOptionsSeal(void);
void abi_Mode7Rotate(void);
void abi_NewGameStateLong(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_PlayStartCountsLong(void);
void abi_SceneSpinZoomTask(void);
void abi_SongPlay(void);
void abi_StageCountdownSet(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_TextBandsCommit(void);
void abi_TextBandsConceal(void);
void abi_TextBandsReveal(void);
void abi_TextLayerClear(void);
void abi_TextPageDraw(void);
void abi_TextPageExtra(void);
void abi_TextLayoutShow(void);
void abi_TextNumberTwoDigitsLong(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define WIPE_LEFT 0x0078u       /* the ending's window's left edge: fraction, then pixels (word), then a fourth byte */
#define WIPE_LEFT_PIXELS 0x0079u
#define WIPE_STEPS 0xDB91u      /* (data bank) per step, by X: the amount the edges close by (two words) */
#define WINDOW_LEFT 0x03A6u     /* WH0's shadow: the window's left */

/* A frame of the ending's play-back: the sprites and layers drawn and the
   uploads asked for, with or without the parallax bands. */
static void ending_frame(int bands)
{
    wr8(dba((uint16_t)TILE_BLOCKS_LENGTH), 0);
    wr8(dba((uint16_t)(TILE_BLOCKS_LENGTH + 1)), 0);
    wr8(dba((uint16_t)OAM_0857), 0);
    abi_OamBuildBegin();
    if (bands)
        abi_ParallaxUpdateLong();
    abi_LayersScrollLong();
    abi_ActorsInView();
    abi_OamDrawActors();
    abi_BgFollowLayersLong();
    (void)inc8_at(dba((uint16_t)OAM_0857));
    (void)inc8_at(dba((uint16_t)TILE_BLOCKS_PENDING));
    (void)inc8_at(dba((uint16_t)VRAM_QUEUE_PENDING));
}

/*
 * @routine 82:B511 EndingStageFrame
 * WHAT IT DOES: Draws a frame of a stage played back at the ending: the
 * sprites built, the layers scrolled, the actors in view drawn, the
 * backdrop following, and the uploads asked for.
 * HOW: TILE_BLOCKS_LENGTH and OAM_0857 0; OamBuildBegin, LayersScrollLong,
 * ActorsInView, OamDrawActors, BgFollowLayersLong; OAM_0857,
 * TILE_BLOCKS_PENDING and VRAM_QUEUE_PENDING up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR from the ending's stages task. Exit: N and Z from
 * VRAM_QUEUE_PENDING; else as BgFollowLayersLong leaves them.
 */
void abi_EndingStageFrame(void)
{
    ending_frame(0);
}

/*
 * @routine 82:B538 EndingStageFrameBands
 * WHAT IT DOES: As EndingStageFrame, with the backdrop's parallax bands
 * stepped too (the last stage, scrolled across).
 * HOW: As EndingStageFrame, ParallaxUpdateLong after OamBuildBegin.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR from the ending's stages task. Exit: as EndingStageFrame.
 */
void abi_EndingStageFrameBands(void)
{
    ending_frame(1);
}

/*
 * @routine 82:B4AF EndingWipeLeftStep
 * WHAT IT DOES: Closes the ending's window from the left by step X: its
 * left edge (WIPE_LEFT, 32-bit) less the step's amount, not below 0
 * pixels, into WH0's shadow.
 * HOW: 16-bit: WIPE_LEFT less WIPE_STEPS + X (two words, with the
 * borrow); WIPE_LEFT_PIXELS negative: 0. WINDOW_LEFT = its low byte.
 * Entry: M 8-bit; X = the step * 4; DB a bank whose $0000-$1FFF is work
 * RAM and that holds WIPE_STEPS; called by JSR. Exit: M 8-bit; A = the
 * left edge's pixels; N and Z from it.
 */
void abi_EndingWipeLeftStep(void)
{
    uint16_t lo, hi;

    op_rep(0x20);
    flag_set(P_C, 1);
    lo = op_sbc16(rd16(dba(WIPE_LEFT)), rd16(dba((uint16_t)(WIPE_STEPS + cpu.x))), &cpu.p);
    wr16(dba(WIPE_LEFT), lo);
    hi = op_sbc16(rd16(dba((uint16_t)(WIPE_LEFT + 2))), rd16(dba((uint16_t)(WIPE_STEPS + 2 + cpu.x))), &cpu.p);
    wr16(dba((uint16_t)(WIPE_LEFT + 2)), hi);
    a16_load(rd16(dba(WIPE_LEFT_PIXELS)));
    if (cpu.a & 0x8000)
        wr16(dba(WIPE_LEFT_PIXELS), 0);
    op_sep(0x20);
    a8_load(rd8(dba(WIPE_LEFT_PIXELS)));
    wr8(dba(WINDOW_LEFT), (uint8_t)cpu.a);
}

