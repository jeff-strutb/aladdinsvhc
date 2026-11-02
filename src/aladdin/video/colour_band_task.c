/*
 * colour_band_task.c: the colour band task: the fixed colour set line by
 * line by HDMA channel 7, the lower bands pulsing dimmer and back. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "hw.h"
#include "wram_video.h"

void abi_ColourBandsWrite(void);
void abi_TaskWait(void);

#define BAND_TASK_STEP 0x06u  /* the task's own data (D = its slot): how much the lower bands are dimmed */
#define BAND_TASK_BUFFER 0x07u  /* the buffer ColourBandsWrite writes next (0 or 1); with $08 the bands left (word) */
#define COLOUR_BAND_HDMA_AT 0x0356u  /* the buffer the HDMA reads: its offset, then $0357 its page */

/* The task waits `frames` frames, returning at `site`. */
static void band_wait(uint8_t frames, uint32_t site)
{
    a8_load(frames);
    jsl_frame(site);
    abi_TaskWait();
}

/*
 * @routine 81:A00C ColourBandTask
 * WHAT IT DOES: A task that shows colour bands down the screen through the
 * fixed colour (HDMA channel 7 on COLDATA), its lower bands pulsing for
 * good: dimmed one step every 4 frames to 16 steps, held about half a
 * second, brightened back the same way, held again.
 * HOW: D = its slot (Y + $6F); step and buffer cleared. COLDATA red $0C;
 * channel 7: mode 0 to COLDATA ($32), source bank $7E, indirect bank 0;
 * the buffer page $F5; colour maths shadows: CGADSUB 2, CGWSEL 0, WOBJSEL
 * $80. ColourBandsWrite, HDMA channel 7 on in the shadow, TaskWait 1. Then
 * for good: the step up by one, ColourBandsWrite, TaskWait 4, until 16;
 * TaskWait $1F; the step down by one, ColourBandsWrite, TaskWait 4, until
 * 0; TaskWait $1F.
 * Entry: M 8-bit, X 8-bit; Y = its slot's offset; DB = $80; started by the
 * scheduler (a task entry). Exit: never returns.
 */
void abi_ColourBandTask(void)
{
    uint8_t step;

    /* D = its slot's data */
    op_rep(P_M);
    flag_set(P_C, 0);
    cpu.a = op_adc16((uint16_t)cpu.y, 0x006F, &cpu.p);
    cpu.d = cpu.a;
    dp16_set(BAND_TASK_BUFFER, 0);
    op_sep(P_M);
    /* HDMA channel 7 to COLDATA from $7E:F5xx; colour math on */
    wr8(dba((uint16_t)COLDATA_RED), 0x0C);
    wr8(dba((uint16_t)DMAP7), 0x00);
    wr8(dba((uint16_t)BBAD7), 0x32);
    wr8(dba((uint16_t)A1B7), 0x7E);
    wr8(dba((uint16_t)DASB7), 0x00);
    wr8(dba(COLOUR_BAND_HDMA_AT + 1), 0xF5);
    wr8(dba(CGADSUB_SHADOW), 0x02);
    wr8(dba(CGWSEL_SHADOW), 0x00);
    a8_load(0x80);
    wr8(dba(WOBJSEL_SHADOW), 0x80);
    abi_ColourBandsWrite();
    a8_load((uint8_t)(rd8(dba(HDMAEN_SHADOW)) | 0x80));
    wr8(dba(HDMAEN_SHADOW), (uint8_t)cpu.a);
    band_wait(0x01, 0x81A04Eu);
    for (;;) {
        /* the lower bands dim a step every 4 frames to 16, then back */
        do {
            flag_set(P_C, 0);
            step = op_adc8(rd8(dpa(BAND_TASK_STEP)), 0x01, &cpu.p);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | step);
            wr8(dpa(BAND_TASK_STEP), step);
            abi_ColourBandsWrite();
            band_wait(0x04, 0x81A05Eu);
            step = rd8(dpa(BAND_TASK_STEP));
            a8_load(step);
            cmp8_of(step, 0x10);
        } while (step != 0x10);
        band_wait(0x1F, 0x81A06Au);
        do {
            flag_set(P_C, 1);
            step = op_sbc8(rd8(dpa(BAND_TASK_STEP)), 0x01, &cpu.p);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | step);
            wr8(dpa(BAND_TASK_STEP), step);
            abi_ColourBandsWrite();
            band_wait(0x04, 0x81A07Au);
            step = rd8(dpa(BAND_TASK_STEP));
            a8_load(step);
        } while (step);
        band_wait(0x1F, 0x81A084u);
    }
}
