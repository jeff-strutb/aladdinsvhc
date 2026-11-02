/*
 * screen_setup.c: the display's settings and the scroll state put back to
 * their defaults before a screen is built.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ScrollStateClear(void);
void abi_DisplayDefaults(void);
#include "hw.h"
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:96CB ScrollStateClear
 * WHAT IT DOES: Clears the scroll state: the camera's words at $1772-$1779
 * and $18C8-$18CF, the BG2 shifts and their neighbours at $1A1E-$1A25, and
 * the words at $19EC-$1A03.
 * HOW: A 16-bit STZ to each word.
 * Entry: DB = $00-$3F or $80-$BF; called by JSL. Exit: M 8-bit; A, X, Y
 * and flags but M as at entry.
 */
void abi_ScrollStateClear(void)
{
    /* the layers' scroll positions and their shifts, each a word */
    static const uint16_t words[24] = {
        0x1772, 0x1774, 0x1776, 0x1778, 0x18C8, 0x18CA, 0x18CC, 0x18CE,
        0x1A1E, BG2_X_SHIFT, 0x1A22, BG2_Y_SHIFT,
        0x19EC, 0x19EE, 0x19F0, 0x19F2, 0x19F4, 0x19F6, 0x19F8, 0x19FA,
        0x19FC, 0x19FE, 0x1A00, 0x1A02,
    };

    for (int i = 0; i < 24; i++)
        wr16(dba(words[i]), 0x0000);
    op_sep(P_M);
}

/*
 * @routine 81:9865 DisplayDefaults
 * WHAT IT DOES: Puts the display's settings back to their defaults:
 * sprites from VRAM $6000 (OBSEL = 3), colour math's shadows to their
 * start values, the fixed colour black, windows, HDMA and mode 7's
 * settings off, and only the NMI enabled.
 * HOW: Writes OBSEL, COLDATA, SETINI, HDMAEN and M7SEL, and the shadows
 * the NMI copies.
 * Entry: M 8-bit; DB = $00-$3F or $80-$BF; called by JSL. Exit: A low
 * byte = 1; X, Y unchanged.
 */
void abi_DisplayDefaults(void)
{
    /* the display's settings and their shadows, in the original's order */
    static const struct { uint16_t addr; uint8_t value; } settings[] = {
        { (uint16_t)OBSEL, 0x03 }, { CGWSEL_SHADOW, 0x30 }, { 0x039D, 0x09 },
        { 0x03A0, 0x49 }, { 0x03A1, 0x41 }, { 0x03A2, 0x5C }, { 0x03A4, 0x00 },
        { 0x03A5, 0x05 }, { COLDATA, 0xE0 }, { COLDATA_RED, 0 },
        { COLDATA_GREEN, 0 }, { COLDATA_BLUE, 0 }, { 0x038B, 0 }, { 0x038C, 0 },
        { 0x03AA, 0 }, { 0x03AB, 0 }, { WOBJSEL_SHADOW, 0 }, { 0x03AD, 0 },
        { 0x03B1, 0 }, { (uint16_t)SETINI, 0 }, { 0x03B6, 0 }, { (uint16_t)HDMAEN, 0 },
        { (uint16_t)M7SEL, 0 }, { 0x0829, 0 }, { NMITIMEN_SHADOW, 0x01 },
    };

    for (unsigned i = 0; i < sizeof settings / sizeof settings[0]; i++)
        wr8(dba(settings[i].addr), settings[i].value);
    a8_load(0x01);
}
