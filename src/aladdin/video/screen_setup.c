/*
 * screen_setup.c: the display's settings and the scroll state put back to
 * their defaults before a screen is built.
 */
#include "machine.h"
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
    op_rep(0x20);  /* 8196CB rep */
    stz16(ea_abs(0x1772));  /* 8196CD stz */
    stz16(ea_abs(0x1774));  /* 8196D0 stz */
    stz16(ea_abs(0x1776));  /* 8196D3 stz */
    stz16(ea_abs(0x1778));  /* 8196D6 stz */
    stz16(ea_abs(0x18C8));  /* 8196D9 stz */
    stz16(ea_abs(0x18CA));  /* 8196DC stz */
    stz16(ea_abs(0x18CC));  /* 8196DF stz */
    stz16(ea_abs(0x18CE));  /* 8196E2 stz */
    stz16(ea_abs(0x1A1E));  /* 8196E5 stz */
    stz16(ea_abs(BG2_X_SHIFT));  /* 8196E8 stz */
    stz16(ea_abs(0x1A22));  /* 8196EB stz */
    stz16(ea_abs(BG2_Y_SHIFT));  /* 8196EE stz */
    stz16(ea_abs(0x19EC));  /* 8196F1 stz */
    stz16(ea_abs(0x19EE));  /* 8196F4 stz */
    stz16(ea_abs(0x19F0));  /* 8196F7 stz */
    stz16(ea_abs(0x19F2));  /* 8196FA stz */
    stz16(ea_abs(0x19F4));  /* 8196FD stz */
    stz16(ea_abs(0x19F6));  /* 819700 stz */
    stz16(ea_abs(0x19F8));  /* 819703 stz */
    stz16(ea_abs(0x19FA));  /* 819706 stz */
    stz16(ea_abs(0x19FC));  /* 819709 stz */
    stz16(ea_abs(0x19FE));  /* 81970C stz */
    stz16(ea_abs(0x1A00));  /* 81970F stz */
    stz16(ea_abs(0x1A02));  /* 819712 stz */
    op_sep(0x20);  /* 819715 sep */
    return;  /* 819717 rtl */
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
    lda8_v(0x03);  /* 819865 lda */
    sta8(ea_abs(OBSEL));  /* 819867 sta */
    lda8_v(0x30);  /* 81986A lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 81986C sta */
    lda8_v(0x09);  /* 81986F lda */
    sta8(ea_abs(0x039D));  /* 819871 sta */
    lda8_v(0x49);  /* 819874 lda */
    sta8(ea_abs(0x03A0));  /* 819876 sta */
    lda8_v(0x41);  /* 819879 lda */
    sta8(ea_abs(0x03A1));  /* 81987B sta */
    lda8_v(0x5C);  /* 81987E lda */
    sta8(ea_abs(0x03A2));  /* 819880 sta */
    lda8_v(0x00);  /* 819883 lda */
    sta8(ea_abs(0x03A4));  /* 819885 sta */
    lda8_v(0x05);  /* 819888 lda */
    sta8(ea_abs(0x03A5));  /* 81988A sta */
    lda8_v(0xE0);  /* 81988D lda */
    sta8(ea_abs(COLDATA));  /* 81988F sta */
    stz8(ea_abs(COLDATA_RED));  /* 819892 stz */
    stz8(ea_abs(COLDATA_GREEN));  /* 819895 stz */
    stz8(ea_abs(COLDATA_BLUE));  /* 819898 stz */
    stz8(ea_abs(0x038B));  /* 81989B stz */
    stz8(ea_abs(0x038C));  /* 81989E stz */
    stz8(ea_abs(0x03AA));  /* 8198A1 stz */
    stz8(ea_abs(0x03AB));  /* 8198A4 stz */
    stz8(ea_abs(WOBJSEL_SHADOW));  /* 8198A7 stz */
    stz8(ea_abs(0x03AD));  /* 8198AA stz */
    stz8(ea_abs(0x03B1));  /* 8198AD stz */
    stz8(ea_abs(SETINI));  /* 8198B0 stz */
    stz8(ea_abs(0x03B6));  /* 8198B3 stz */
    stz8(ea_abs(HDMAEN));  /* 8198B6 stz */
    stz8(ea_abs(M7SEL));  /* 8198B9 stz */
    stz8(ea_abs(0x0829));  /* 8198BC stz */
    lda8_v(0x01);  /* 8198BF lda */
    sta8(ea_abs(NMITIMEN_SHADOW));  /* 8198C1 sta */
    return;  /* 8198C4 rtl */
}
