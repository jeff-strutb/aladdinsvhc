/*
 * colour_band_task.c: the colour band task: the fixed colour set line by
 * line by HDMA channel 7, the lower bands pulsing dimmer and back. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "hw.h"
#include "wram_video.h"

void abi_ColourBandsWrite(void);
void abi_TaskWait(void);

#define BAND_TASK_STEP 0x06u  /* the task's own data (D = its slot): how much the lower bands are dimmed */
#define BAND_TASK_BUFFER 0x07u  /* the buffer ColourBandsWrite writes next (0 or 1); with $08 the bands left (word) */
#define COLOUR_BAND_HDMA_AT 0x0356u  /* the buffer the HDMA reads: its offset, then $0357 its page */

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
    op_rep(0x20);  /* 81A00C rep */
    clc();  /* 81A00E clc */
    tya16();  /* 81A00F tya */
    adc16_v(0x006F);  /* 81A010 adc */
    tcd();  /* 81A013 tcd */
    stz16(ea_dp(BAND_TASK_BUFFER));  /* 81A014 stz */
    op_sep(0x20);  /* 81A016 sep */
    lda8_v(0x0C);  /* 81A018 lda */
    sta8(ea_abs(COLDATA_RED));  /* 81A01A sta */
    lda8_v(0x00);  /* 81A01D lda */
    sta8(ea_abs(DMAP7));  /* 81A01F sta */
    lda8_v(0x32);  /* 81A022 lda */
    sta8(ea_abs(BBAD7));  /* 81A024 sta */
    lda8_v(0x7E);  /* 81A027 lda */
    sta8(ea_abs(A1B7));  /* 81A029 sta */
    stz8(ea_abs(DASB7));  /* 81A02C stz */
    lda8_v(0xF5);  /* 81A02F lda */
    sta8(ea_abs(COLOUR_BAND_HDMA_AT + 1));  /* 81A031 sta */
    lda8_v(0x02);  /* 81A034 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 81A036 sta */
    stz8(ea_abs(CGWSEL_SHADOW));  /* 81A039 stz */
    lda8_v(0x80);  /* 81A03C lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 81A03E sta */
    abi_ColourBandsWrite();  /* 81A041 jsr */
    lda8(ea_abs(HDMAEN_SHADOW));  /* 81A044 lda */
    ora8_v(0x80);  /* 81A047 ora */
    sta8(ea_abs(HDMAEN_SHADOW));  /* 81A049 sta */
    lda8_v(0x01);  /* 81A04C lda */
    jsl_frame(0x81A04Eu); abi_TaskWait();  /* 81A04E jsl */
L_81A052:
    clc();  /* 81A052 clc */
    lda8(ea_dp(BAND_TASK_STEP));  /* 81A053 lda */
    adc8_v(0x01);  /* 81A055 adc */
    sta8(ea_dp(BAND_TASK_STEP));  /* 81A057 sta */
    abi_ColourBandsWrite();  /* 81A059 jsr */
    lda8_v(0x04);  /* 81A05C lda */
    jsl_frame(0x81A05Eu); abi_TaskWait();  /* 81A05E jsl */
    lda8(ea_dp(BAND_TASK_STEP));  /* 81A062 lda */
    cmp8_v(0x10);  /* 81A064 cmp */
    if (!IF_Z) goto L_81A052;  /* 81A066 bne */
    lda8_v(0x1F);  /* 81A068 lda */
    jsl_frame(0x81A06Au); abi_TaskWait();  /* 81A06A jsl */
L_81A06E:
    sec();  /* 81A06E sec */
    lda8(ea_dp(BAND_TASK_STEP));  /* 81A06F lda */
    sbc8_v(0x01);  /* 81A071 sbc */
    sta8(ea_dp(BAND_TASK_STEP));  /* 81A073 sta */
    abi_ColourBandsWrite();  /* 81A075 jsr */
    lda8_v(0x04);  /* 81A078 lda */
    jsl_frame(0x81A07Au); abi_TaskWait();  /* 81A07A jsl */
    lda8(ea_dp(BAND_TASK_STEP));  /* 81A07E lda */
    if (!IF_Z) goto L_81A06E;  /* 81A080 bne */
    lda8_v(0x1F);  /* 81A082 lda */
    jsl_frame(0x81A084u); abi_TaskWait();  /* 81A084 jsl */
    goto L_81A052;  /* 81A088 bra */
}
