/*
 * logo_screen.c: the logo screen the game shows at power on, before the
 * title.
 */
#include "machine.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_pad.h"

void abi_DisplayDefaults(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);

#define TM_SHADOW 0x0399u           /* TM's shadow: the main screen's layers */
#define TS_SHADOW 0x039Au           /* TS's shadow: the sub screen's layers */
#define FADE_SLOT_STATE (TASK_STATE + 1 * 24)    /* slot 1's state: the fades run there */
#define SHINE_SLOT_STATE (TASK_STATE + 2 * 24)   /* slot 2's state: PaletteSequenceTask runs there */
#define LOGO_KEYS 0x002Cu           /* scratch: pad 1's A, X, L and R held, as the high nibble */

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine A5:E000 LogoScreen
 * WHAT IT DOES: The logo screen at power on: the logo shown with its tune,
 * faded in, its palettes played through, held for two seconds or until a
 * button, and faded out.
 * HOW: With the NMI off and the screen blanked: scroll and sprites
 * cleared, the display's defaults, graphics set 3 unpacked and VRAM copies
 * 7 and $0E, layer 1 alone on the main and sub screens, palette 3; NMI on,
 * song $10. After 60 frames ColourFadeInTask in slot 1 (parameter 2); when
 * it has ended, INIDISP full bright and 31 frames; PaletteSequenceTask in
 * slot 2, waited out; then up to 120 frames (TASK_PARAM counts) or until
 * pad 1 holds B, Y, Select, Start, A, X, L or R; ColourFadeOutTask in
 * slot 1 (parameter 1), waited out.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSL from a task (it waits). Exit: M 8-bit, X 8-bit; A = slot 1's
 * state (0), Z set; TASK_PARAM (slot 0's parameter, the count) changed.
 */
void abi_LogoScreen(void)
{
    jsl_call(0xA5E000u, abi_NmiDisable);  /* A5E000 jsl */
    jsl_call(0xA5E004u, abi_ScreenBlank);  /* A5E004 jsl */
    jsl_call(0xA5E008u, abi_ScrollStateClear);  /* A5E008 jsl */
    jsl_call(0xA5E00Cu, abi_OamBufferClear);  /* A5E00C jsl */
    jsl_call(0xA5E010u, abi_DisplayDefaults);  /* A5E010 jsl */
    lda8_v(0x03);  /* A5E014 lda */
    jsl_call(0xA5E016u, abi_UnpackSet);  /* A5E016 jsl */
    ldx8_v(0x07);  /* A5E01A ldx */
    jsl_call(0xA5E01Cu, abi_VramCopy);  /* A5E01C jsl */
    ldx8_v(0x0E);  /* A5E020 ldx */
    jsl_call(0xA5E022u, abi_VramCopy);  /* A5E022 jsl */
    lda8_v(0x01);  /* A5E026 lda */
    sta8(ea_abs(TM_SHADOW));  /* A5E028 sta */
    sta8(ea_abs(TS_SHADOW));  /* A5E02B sta */
    lda8_v(0x03);  /* A5E02E lda */
    jsl_call(0xA5E030u, abi_PaletteLoad);  /* A5E030 jsl */
    jsl_call(0xA5E034u, abi_NmiEnable);  /* A5E034 jsl */
    lda8_v(0x10);  /* A5E038 lda */
    jsl_call(0xA5E03Au, abi_SongPlay);  /* A5E03A jsl */
    lda8_v(0x3C);  /* A5E03E lda */
    jsl_frame(0xA5E040u); abi_TaskWait();  /* A5E040 jsl */
    /* ColourFadeInTask, slot 1, parameter 2 */
    jsl_frame(0xA5E044u); abi_TaskCreate(); cpu.unwound = 0;  /* A5E044 jsl */
    do {
        lda8_v(0x01);  /* A5E04D lda */
        jsl_frame(0xA5E04Fu); abi_TaskWait();  /* A5E04F jsl */
        lda8(ea_abs((uint16_t)FADE_SLOT_STATE));  /* A5E053 lda */
    } while (!IF_Z);  /* A5E056 bne */
    lda8_v(0x0F);  /* A5E058 lda */
    sta8(ea_abs(INIDISP_SHADOW));  /* A5E05A sta */
    lda8_v(0x1F);  /* A5E05D lda */
    jsl_frame(0xA5E05Fu); abi_TaskWait();  /* A5E05F jsl */
    /* PaletteSequenceTask, slot 2, parameter 0 */
    jsl_frame(0xA5E063u); abi_TaskCreate(); cpu.unwound = 0;  /* A5E063 jsl */
    do {
        lda8_v(0x01);  /* A5E06C lda */
        jsl_frame(0xA5E06Eu); abi_TaskWait();  /* A5E06E jsl */
        lda8(ea_abs((uint16_t)SHINE_SLOT_STATE));  /* A5E072 lda */
    } while (!IF_Z);  /* A5E075 bne */
    lda8_v(0x78);  /* A5E077 lda */
    sta8(ea_abs((uint16_t)TASK_PARAM));  /* A5E079 sta */
    do {
        lda8_v(0x01);  /* A5E07C lda */
        jsl_frame(0xA5E07Eu); abi_TaskWait();  /* A5E07E jsl */
        lda8(ea_abs((uint16_t)(PAD1_HELD + 1)));  /* A5E082 lda */
        and8_v(0xF0);  /* A5E085 and */
        sta8(ea_abs(LOGO_KEYS));  /* A5E087 sta */
        lda8(ea_abs((uint16_t)PAD1_HELD));  /* A5E08A lda */
        and8_v(0xF0);  /* A5E08D and */
        ora8_v(rd8(ea_abs(LOGO_KEYS)));  /* A5E08F ora */
        if (!IF_Z)  /* A5E092 bne */
            break;
        MEM8(dec, ea_abs((uint16_t)TASK_PARAM));  /* A5E094 dec */
    } while (!IF_Z);  /* A5E097 bne */
    /* ColourFadeOutTask, slot 1, parameter 1 */
    jsl_frame(0xA5E099u); abi_TaskCreate(); cpu.unwound = 0;  /* A5E099 jsl */
    do {
        lda8_v(0x01);  /* A5E0A2 lda */
        jsl_frame(0xA5E0A4u); abi_TaskWait();  /* A5E0A4 jsl */
        lda8(ea_abs((uint16_t)FADE_SLOT_STATE));  /* A5E0A8 lda */
    } while (!IF_Z);  /* A5E0AB bne */
}
