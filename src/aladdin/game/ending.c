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
void abi_EndingStageFrame(void);
void abi_EndingWipeLeftStep(void);
void abi_EndingWipeRightStep(void);
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
#define WINDOW_RIGHT 0x03A7u    /* WH1's shadow: the window's right */
#define WIPE_RIGHT 0x007Cu      /* the window's right edge: fraction, then pixels (word), then a fourth byte */
#define WIPE_RIGHT_PIXELS 0x007Du
#define ENDING_STAGE 0x0076u    /* the stage being played back (its index in ENDING_HOLDS) */
#define ENDING_STEP 0x0077u     /* the play-back's step (0 or 1): its scroll and wipe entries */
#define ENDING_EE 0x00EEu       /* the stage, kept for the task started at the end */
#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define BG_SCROLL_X_18C9 0x18C9u /* a second layer's scroll x, moved with it */
#define ENDING_SCROLLS 0xDB92u  /* (data bank) per step, by X: the scroll's change (word) */
#define ENDING_HOLDS 0xDBB3u    /* (data bank) per stage: how many waits of 63 frames follow */

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

/*
 * @routine 82:B4D5 EndingWipeRightStep
 * WHAT IT DOES: Closes the ending's window from the right by step X: its
 * right edge (WIPE_RIGHT, 32-bit) less the step's amount, at most 255
 * pixels, into WH1's shadow.
 * HOW: 16-bit: WIPE_RIGHT less WIPE_STEPS + X (two words, with the
 * borrow); WIPE_RIGHT_PIXELS $0100 or more: $00FF (no script's play-back
 * reaches it: $82:B4F2). WINDOW_RIGHT = its low byte.
 * Entry: M 8-bit; X = the step * 4; DB a bank whose $0000-$1FFF is work
 * RAM and that holds WIPE_STEPS; called by JSR. Exit: M 8-bit; A = the
 * right edge's pixels; N and Z from it.
 */
void abi_EndingWipeRightStep(void)
{
    uint16_t lo, hi;

    op_rep(0x20);
    flag_set(P_C, 1);
    lo = op_sbc16(rd16(dba(WIPE_RIGHT)), rd16(dba((uint16_t)(WIPE_STEPS + cpu.x))), &cpu.p);
    wr16(dba(WIPE_RIGHT), lo);
    hi = op_sbc16(rd16(dba((uint16_t)(WIPE_RIGHT + 2))), rd16(dba((uint16_t)(WIPE_STEPS + 2 + cpu.x))), &cpu.p);
    wr16(dba((uint16_t)(WIPE_RIGHT + 2)), hi);
    a16_load(rd16(dba(WIPE_RIGHT_PIXELS)));
    cmp16_of(cpu.a, 0x0100);
    if (cpu.a >= 0x0100) {
        a16_load(0x00FF);
        wr16(dba(WIPE_RIGHT_PIXELS), 0x00FF);
    }
    op_sep(0x20);
    a8_load(rd8(dba(WIPE_RIGHT_PIXELS)));
    wr8(dba(WINDOW_RIGHT), (uint8_t)cpu.a);
}

/*
 * @routine 82:B458 EndingStageScroll
 * WHAT IT DOES: The play-back's move to the next stage: for 41 frames the
 * stage is drawn while the view scrolls by its step and the window closes
 * from both sides; then the task at $82:B0A9 is started and the task
 * waits 63 frames as many times as the stage's hold says, and the stage
 * count goes on.
 * HOW: TASK_PARAM $29; each frame: TaskWait 1; EndingStageFrame; X =
 * ENDING_STEP * 4; SCROLL_X and BG_SCROLL_X_18C9 + ENDING_SCROLLS + X;
 * EndingWipeLeftStep, EndingWipeRightStep; TASK_PARAM down. TaskCreate
 * (slot 5, parameter 0, $82:B0A9); ENDING_EE = ENDING_STAGE; Y =
 * ENDING_HOLDS by it; Y times TaskWait $3F; ENDING_STAGE up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * that holds the tables; called by JSR from the ending's stages task.
 * Exit: N and Z from ENDING_STAGE.
 */
void abi_EndingStageScroll(void)
{
    uint8_t stage, holds;
    uint16_t step;

    a8_load(0x29);
    wr8(dba(TASK_PARAM), 0x29);
    do {
        a8_load(0x01);
        jsl_frame(0x82B45Fu);
        abi_TaskWait();
        abi_EndingStageFrame();
        a8_load((uint8_t)(rd8(dba(ENDING_STEP)) << 2));
        flag_set(P_C, 0);
        x8_load((uint8_t)cpu.a);
        op_rep(0x20);
        step = rd16(dba((uint16_t)(ENDING_SCROLLS + cpu.x)));
        flag_set(P_C, 0);
        a16_load(op_adc16(rd16(dba((uint16_t)SCROLL_X)), step, &cpu.p));
        wr16(dba((uint16_t)SCROLL_X), cpu.a);
        flag_set(P_C, 0);
        a16_load(op_adc16(rd16(dba(BG_SCROLL_X_18C9)), step, &cpu.p));
        wr16(dba(BG_SCROLL_X_18C9), cpu.a);
        op_sep(0x20);
        abi_EndingWipeLeftStep();
        abi_EndingWipeRightStep();
    } while (dec8_at(dba(TASK_PARAM)));
    jsl_frame(0x82B48Fu); abi_TaskCreate(); cpu.unwound = 0;
    stage = rd8(dba(ENDING_STAGE));
    a8_load(stage);
    wr8(dba(ENDING_EE), stage);
    x8_load(stage);
    holds = rd8(dba((uint16_t)(ENDING_HOLDS + stage)));
    y8_load(holds);
    do {
        a8_load(0x3F);
        jsl_frame(0x82B4A4u);
        abi_TaskWait();
        cpu.y = (uint8_t)(cpu.y - 1);
        nz8_of((uint8_t)cpu.y);
    } while (cpu.y);
    (void)inc8_at(dba(ENDING_STAGE));
}

/*
 * @routine 80:B1D8 SceneSpinZoomTask
 * WHAT IT DOES: Spins and zooms a Mode 7 scene: the angle steps on by a
 * speed that grows every 16 frames and the zoom grows every 24, the zoom
 * taken as its negative past $7F.
 * HOW: D = the task's slot page. The centre ($80, $50); Mode 7 scaled
 * (nmi_ppu_mode 1); angle 0, speed 4, zoom 8, counts 16 and 16. Each
 * frame: the counts step (speed up every 16, zoom up every 24); the zoom
 * (negated when negative) into the four scales; the matrix shadows
 * shifted left 4 (C = -B); TaskWait 1; Mode7Rotate by the angle, then the
 * angle plus the speed.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
 * never returns.
 */
void abi_SceneSpinZoomTask(void)
{
    op_rep(0x20);  /* 80B1D8 rep */
    clc();  /* 80B1DA clc */
    tya16();  /* 80B1DB tya */
    adc16_v(0x006F);  /* 80B1DC adc */
    tcd();  /* 80B1DF tcd */
    lda16_v(0x0080);  /* 80B1E0 lda */
    sta16(ea_abs(0x0395));  /* 80B1E3 sta */
    lda16_v(0x0050);  /* 80B1E6 lda */
    sta16(ea_abs(0x0397));  /* 80B1E9 sta */
    op_sep(0x20);  /* 80B1EC sep */
    lda8_v(0x01);  /* 80B1EE lda */
    sta8(ea_abs(0x0829));  /* 80B1F0 sta */
    stz8(ea_dp(0x06));  /* 80B1F3 stz */
    lda8_v(0x04);  /* 80B1F5 lda */
    sta8(ea_dp(0x07));  /* 80B1F7 sta */
    lda8_v(0x08);  /* 80B1F9 lda */
    sta8(ea_dp(0x08));  /* 80B1FB sta */
    lda8_v(0x10);  /* 80B1FD lda */
    sta8(ea_dp(0x09));  /* 80B1FF sta */
    lda8_v(0x10);  /* 80B201 lda */
    sta8(ea_dp(0x0A));  /* 80B203 sta */
L_80B205:
    MEM8(dec, ea_dp(0x09));  /* 80B205 dec */
    if (!IF_Z) goto L_80B20F;  /* 80B207 bne */
    lda8_v(0x10);  /* 80B209 lda */
    sta8(ea_dp(0x09));  /* 80B20B sta */
    MEM8(inc, ea_dp(0x07));  /* 80B20D inc */
L_80B20F:
    MEM8(dec, ea_dp(0x0A));  /* 80B20F dec */
    if (!IF_Z) goto L_80B219;  /* 80B211 bne */
    lda8_v(0x18);  /* 80B213 lda */
    sta8(ea_dp(0x0A));  /* 80B215 sta */
    MEM8(inc, ea_dp(0x08));  /* 80B217 inc */
L_80B219:
    lda8(ea_dp(0x08));  /* 80B219 lda */
    if (!IF_N) goto L_80B220;  /* 80B21B bpl */
    eor8_v(0xFF);  /* 80B21D eor */
    ACC8(inc);  /* 80B21F inc */
L_80B220:
    sta8(ea_abs(0x0827));  /* 80B220 sta */
    sta8(ea_abs(0x0828));  /* 80B223 sta */
    sta8(ea_abs(0x0825));  /* 80B226 sta */
    sta8(ea_abs(0x0826));  /* 80B229 sta */
    op_rep(0x20);  /* 80B22C rep */
    lda16(ea_abs(0x038D));  /* 80B22E lda */
    ACC16(asl);  /* 80B231 asl */
    ACC16(asl);  /* 80B232 asl */
    ACC16(asl);  /* 80B233 asl */
    ACC16(asl);  /* 80B234 asl */
    sta16(ea_abs(0x038D));  /* 80B235 sta */
    sta16(ea_abs(0x0393));  /* 80B238 sta */
    lda16(ea_abs(0x038F));  /* 80B23B lda */
    ACC16(asl);  /* 80B23E asl */
    ACC16(asl);  /* 80B23F asl */
    ACC16(asl);  /* 80B240 asl */
    ACC16(asl);  /* 80B241 asl */
    sta16(ea_abs(0x038F));  /* 80B242 sta */
    eor16_v(0xFFFF);  /* 80B245 eor */
    ACC16(inc);  /* 80B248 inc */
    sta16(ea_abs(0x0391));  /* 80B249 sta */
    op_sep(0x20);  /* 80B24C sep */
    lda8_v(0x01);  /* 80B24E lda */
    jsl_frame(0x80B250u); abi_TaskWait();  /* 80B250 jsl */
    lda8(ea_dp(0x06));  /* 80B254 lda */
    abi_Mode7Rotate();  /* 80B256 jsl */
    clc();  /* 80B25A clc */
    lda8(ea_dp(0x06));  /* 80B25B lda */
    adc8_v(rd8(ea_dp(0x07)));  /* 80B25D adc */
    sta8(ea_dp(0x06));  /* 80B25F sta */
    goto L_80B205;  /* 80B261 bra */
}
