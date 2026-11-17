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
void abi_EndingStageTextTask(void);
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
    uint16_t lo, hi, w;

    op_rep(0x20);
    flag_set(P_C, 1);
    w = rd16(dba(WIPE_LEFT));
    lo = op_sbc16(w, rd16(dba((uint16_t)(WIPE_STEPS + cpu.x))), &cpu.p);
    wr16(dba(WIPE_LEFT), lo);
    w = rd16(dba((uint16_t)(WIPE_LEFT + 2)));
    hi = op_sbc16(w, rd16(dba((uint16_t)(WIPE_STEPS + 2 + cpu.x))), &cpu.p);
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
    uint16_t lo, hi, w;

    op_rep(0x20);
    flag_set(P_C, 1);
    w = rd16(dba(WIPE_RIGHT));
    lo = op_sbc16(w, rd16(dba((uint16_t)(WIPE_STEPS + cpu.x))), &cpu.p);
    wr16(dba(WIPE_RIGHT), lo);
    w = rd16(dba((uint16_t)(WIPE_RIGHT + 2)));
    hi = op_sbc16(w, rd16(dba((uint16_t)(WIPE_STEPS + 2 + cpu.x))), &cpu.p);
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
 * from both sides; then the stage's caption (EndingStageTextTask) is started and the task
 * waits 63 frames as many times as the stage's hold says, and the stage
 * count goes on.
 * HOW: TASK_PARAM $29; each frame: TaskWait 1; EndingStageFrame; X =
 * ENDING_STEP * 4; SCROLL_X and BG_SCROLL_X_18C9 + ENDING_SCROLLS + X;
 * EndingWipeLeftStep, EndingWipeRightStep; TASK_PARAM down. TaskCreate
 * (slot 5, parameter 0, EndingStageTextTask); ENDING_EE = ENDING_STAGE; Y =
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

/* EndingTextTask's page loop, from its palettes on (82:B0BB): the set-up,
   then each page drawn, revealed and (timed) counted down and concealed;
   EndingStageTextTask goes into it too. Never returns. */
static void ending_text_pages(void)
{
    lda8_v(0x61);  /* 82B0BB lda */
    abi_PaletteLoad();  /* 82B0BD jsl */
    lda8_v(0x5F);  /* 82B0C1 lda */
    abi_PaletteLoad();  /* 82B0C3 jsl */
    stz8(ea_dp(0xF9));  /* 82B0C7 stz */
    lda8_v(0x40);  /* 82B0C9 lda */
    sta8(ea_abs(DMAP5));  /* 82B0CB sta */
    lda8_v(0x24);  /* 82B0CE lda */
    sta8(ea_abs(BBAD5));  /* 82B0D0 sta */
    lda8_v(0xEF);  /* 82B0D3 lda */
    sta8(ea_abs(A1T5L));  /* 82B0D5 sta */
    lda8_v(0xCE);  /* 82B0D8 lda */
    sta8(ea_abs(A1T5H));  /* 82B0DA sta */
    lda8_v(0x80);  /* 82B0DD lda */
    sta8(ea_abs(A1B5));  /* 82B0DF sta */
    stz8(ea_abs(DASB5));  /* 82B0E2 stz */
    ldx8_v(0x07);  /* 82B0E5 ldx */
L_82B0E7:
    stz8(ea_absx(TEXT_BANDS));  /* 82B0E7 stz */
    dex8();  /* 82B0EA dex */
    if (!IF_N) goto L_82B0E7;  /* 82B0EB bpl */
    abi_TextBandsCommit();  /* 82B0ED jsr */
    lda8_v(0x20);  /* 82B0F0 lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 82B0F2 tsb */
    lda8_v(0x00);  /* 82B0F5 lda */
    sta8(ea_abs(0x03A8));  /* 82B0F7 sta */
    lda8_v(0xFF);  /* 82B0FA lda */
    sta8(ea_abs(0x03A9));  /* 82B0FC sta */
    stz8(ea_abs(0x0005));  /* 82B0FF stz */
    abi_KeptOptionsSeal();  /* 82B102 jsl */
    lda8(ea_dp(0xF4));  /* 82B106 lda */
    if (IF_Z) goto L_82B115;  /* 82B108 beq */
L_82B10A:
    op_rep(0x20);  /* 82B10A rep */
    MEM16(dec, ea_dp(0xEF));  /* 82B10C dec */
    op_sep(0x20);  /* 82B10E sep */
    if (!IF_Z) goto L_82B187;  /* 82B110 bne */
    push16(0xB114); abi_TextBandsConceal(); cpu.s = (uint16_t)(cpu.s + 2);  /* 82B112 jsr */
L_82B115:
    abi_TextLayerClear();  /* 82B115 jsl */
    ldx8_v(0x07);  /* 82B119 ldx */
L_82B11B:
    lda8_v(0x08);  /* 82B11B lda */
    sta8(ea_absx(TEXT_BANDS));  /* 82B11D sta */
    sta8(ea_absx(0x1C9E));  /* 82B120 sta */
    dex8();  /* 82B123 dex */
    if (!IF_N) goto L_82B11B;  /* 82B124 bpl */
    abi_TextBandsCommit();  /* 82B126 jsr */
    lda8(ea_dp(0xEE));  /* 82B129 lda */
    ACC8(asl);  /* 82B12B asl */
    adc8_v(rd8(ea_dp(0xF4)));  /* 82B12C adc */
    tax8();  /* 82B12E tax */
    phb();  /* 82B12F phb */
    lda8_v(0xA5);  /* 82B130 lda */
    pha8();  /* 82B132 pha */
    plb();  /* 82B133 plb */
    op_rep(0x10);  /* 82B134 rep */
    ldy16(ea_absx(0xE433));  /* 82B136 ldy */
    sty16(ea_dp(0xF5));  /* 82B139 sty */
    ldy16(ea_absx(0xE0AE));  /* 82B13B ldy */
    abi_TextPageDraw();  /* 82B13E jsr */
    op_sep(0x10);  /* 82B141 sep */
    plb();  /* 82B143 plb */
    abi_TextPageExtra();  /* 82B144 jsr */
    lda8_v(0x01);  /* 82B147 lda */
    jsl_frame(0x82B149u); abi_TaskWait();  /* 82B149 jsl */
    push16(0xB14F); abi_TextBandsReveal(); cpu.s = (uint16_t)(cpu.s + 2);  /* 82B14D jsr */
    lda8(ea_dp(0xF4));  /* 82B150 lda */
    if (IF_Z) goto L_82B17F;  /* 82B152 beq */
    lda8_v(0x94);  /* 82B154 lda */
    sta8(ea_dp(0xEF));  /* 82B156 sta */
    lda8_v(0x02);  /* 82B158 lda */
    sta8(ea_dp(0xF0));  /* 82B15A sta */
    MEM8(inc, ea_dp(0xEE));  /* 82B15C inc */
    lda8(ea_dp(0xEE));  /* 82B15E lda */
    cmp8_v(0x0D);  /* 82B160 cmp */
    if (!IF_Z) goto L_82B187;  /* 82B162 bne */
    lda8_v(0xFF);  /* 82B164 lda */
    jsl_frame(0x82B166u); abi_TaskWait();  /* 82B166 jsl */
    lda8_v(0xFF);  /* 82B16A lda */
    jsl_frame(0x82B16Cu); abi_TaskWait();  /* 82B16C jsl */
    lda8_v(0xFF);  /* 82B170 lda */
    jsl_frame(0x82B172u); abi_TaskWait();  /* 82B172 jsl */
    lda8_v(0xFF);  /* 82B176 lda */
    jsl_frame(0x82B178u); abi_TaskWait();  /* 82B178 jsl */
    push16(0xB17E); abi_TextBandsConceal(); cpu.s = (uint16_t)(cpu.s + 2);  /* 82B17C jsr */
L_82B17F:
    lda8_v(0x01);  /* 82B17F lda */
    jsl_frame(0x82B181u); abi_TaskWait();  /* 82B181 jsl */
    goto L_82B17F;  /* 82B185 bra */
L_82B187:
    lda8_v(0x01);  /* 82B187 lda */
    jsl_frame(0x82B189u); abi_TaskWait();  /* 82B189 jsl */
    goto L_82B10A;  /* 82B18D jmp */
}

/*
 * @routine 82:B0A9 EndingStageTextTask
 * WHAT IT DOES: The ending play-back's caption for the stage just reached:
 * its page of text drawn, revealed and left up; the task then waits a frame
 * at a time for good, until the next stage's caption takes its slot.
 * HOW: $F4 = 0 (untimed: the captions' entries in the page tables), then
 * EndingTextTask's set-up and page loop from its palettes on with the page
 * $EE that EndingStageScroll set: the first pass draws it at once, with no
 * count down or conceal, and after its reveal the loop waits forever.
 * Entry: M 8-bit, X 8-bit; D = 0; a task's entry (TaskCreate in
 * EndingStageScroll, slot 5). Exit: never returns.
 */
void abi_EndingStageTextTask(void)
{
    stz8(ea_dp(0xF4));  /* 82B0A9 stz */
    ending_text_pages();  /* 82B0AB bra */
}

/*
 * @routine 82:B0AD EndingTextTask
 * WHAT IT DOES: Runs the ending's text: 13 pages, each drawn, its bands
 * revealed, shown for 660 frames and concealed; after the last, long waits
 * and the text masked for good.
 * HOW: Page 0, timed ($F4 = 14, the timed pages' place in the page
 * tables); then the page loop: HDMA channel 5 on the bands' window bytes
 * (TextBandsCommit), the screen set up. Per page: TextPageDraw from the
 * page tables in bank $A5, TextPageExtra, TaskWait, TextBandsReveal; the
 * count $EF from $294; at 0 TextBandsConceal and the next page. After page
 * 13: four waits of 255 frames, TextBandsConceal, then a frame's wait
 * forever.
 * Entry: M 8-bit, X 8-bit; D = 0; a task's entry. Exit: never returns.
 */
void abi_EndingTextTask(void)
{
    lda8_v(0x0E);  /* 82B0AD lda */
    sta8(ea_dp(0xF4));  /* 82B0AF sta */
    stz8(ea_dp(0xEE));  /* 82B0B1 stz */
    lda8_v(0x00);  /* 82B0B3 lda */
    sta8(ea_dp(0xEF));  /* 82B0B5 sta */
    lda8_v(0x02);  /* 82B0B7 lda */
    sta8(ea_dp(0xF0));  /* 82B0B9 sta */
    ending_text_pages();
}

/*
 * @routine 82:B1CE TextPageExtra
 * WHAT IT DOES: After a page of the ending's text, adds the item count to
 * the two pages that show it: page 12 of the timed text, with layouts $6C
 * and $6E, and page 6 of the untimed captions, with layouts $76 and $78 and
 * then $7C when the count ($0376) is 70, else $7A.
 * HOW: $F4 nonzero (timed) and page ($EE) 12, or $F4 zero and page 6: the
 * count drawn as two digits (TextNumberTwoDigitsLong, offset $0522 or
 * $0530), then the layouts by TextLayoutShow. Any other page: nothing.
 * Entry: M 8-bit, X 8-bit; D = 0; DB a bank whose $0000-$1FFF is work RAM
 * and $4200-$421F the CPU's; called by JSR. Exit: M 8-bit, X 8-bit; at a
 * page with no count, A = the page and the flags from its compare; else as
 * TextLayoutShow leaves them.
 */
void abi_TextPageExtra(void)
{
    lda8(ea_dp(0xF4));  /* 82B1CE lda */
    if (IF_Z) goto L_82B1D9;  /* 82B1D0 beq */
    lda8(ea_dp(0xEE));  /* 82B1D2 lda */
    cmp8_v(0x0C);  /* 82B1D4 cmp */
    if (IF_Z) goto L_82B1E0;  /* 82B1D6 beq */
    return;  /* 82B1D8 rts */
L_82B1D9:
    lda8(ea_dp(0xEE));  /* 82B1D9 lda */
    cmp8_v(0x06);  /* 82B1DB cmp */
    if (IF_Z) goto L_82B1F8;  /* 82B1DD beq */
    return;  /* 82B1DF rts */
L_82B1E0:
    ldx8(ea_abs(0x0376));  /* 82B1E0 ldx */
    lda8_v(0x05);  /* 82B1E3 lda */
    ldy8_v(0x22);  /* 82B1E5 ldy */
    abi_TextNumberTwoDigitsLong();  /* 82B1E7 jsl */
    lda8_v(0x6C);  /* 82B1EB lda */
    abi_TextLayoutShow();  /* 82B1ED jsl */
    lda8_v(0x6E);  /* 82B1F1 lda */
    abi_TextLayoutShow();  /* 82B1F3 jsl */
    return;  /* 82B1F7 rts */
L_82B1F8:
    ldx8(ea_abs(0x0376));  /* 82B1F8 ldx */
    lda8_v(0x05);  /* 82B1FB lda */
    ldy8_v(0x30);  /* 82B1FD ldy */
    abi_TextNumberTwoDigitsLong();  /* 82B1FF jsl */
    lda8_v(0x76);  /* 82B203 lda */
    abi_TextLayoutShow();  /* 82B205 jsl */
    lda8_v(0x78);  /* 82B209 lda */
    abi_TextLayoutShow();  /* 82B20B jsl */
    lda8(ea_abs(0x0376));  /* 82B20F lda */
    cmp8_v(0x46);  /* 82B212 cmp */
    if (IF_Z) goto L_82B21D;  /* 82B214 beq */
    lda8_v(0x7A);  /* 82B216 lda */
    abi_TextLayoutShow();  /* 82B218 jsl */
    return;  /* 82B21C rts */
L_82B21D:
    lda8_v(0x7C);  /* 82B21D lda */
    abi_TextLayoutShow();  /* 82B21F jsl */
    return;  /* 82B223 rts */
}
