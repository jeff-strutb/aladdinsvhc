/*
 * level_screen_setups.c: levels' own screen set-ups, by level from the
 * table at $81:8802: palette cycles, the layers on the main and sub
 * screens, colour maths, and an HDMA wave on layer 2. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "hw.h"
#include "wram_video.h"

void abi_PaletteCycleStart(void);

#define TM_SHADOW 0x0399u  /* the layers on the main screen (TM, for the NMI) */
#define TS_SHADOW 0x039Au  /* the layers on the sub screen (TS) */
#define V_083F 0x083Fu  /* the level's layer set-up mode */
#define V_1C7D 0x1C7Du
#define HDMA_TABLE_AT_HIGH 0x0351u  /* the page of the HDMA table HDMA_TABLE_AT names */
#define V_7FF441 0x7FF441u
#define V_7FF457 0x7FF457u
#define V_7FF481 0x7FF481u
#define V_7FF495 0x7FF495u

/*
 * @routine 81:8A88 Level08ScreenSetup
 * WHAT IT DOES: Level 8's screen set-up: six palette cycles, layers 1-3
 * and sprites on the main screen with sprites alone on the sub screen,
 * colour maths adding the sub screen to layers 1 and 2, the backdrop and
 * sprites, at half.
 * HOW: PaletteCycleStart for (slot, cycle) ($54, $2D), (0, 8), ($0E, 9),
 * ($1C, $0E), ($2A, $0F), ($38, $10); TM_SHADOW $17, TS_SHADOW $10; CGWSEL
 * $03, CGADSUB $53 (shadows).
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
 * $81:87EF); A = $53.
 */
void abi_Level08ScreenSetup(void)
{
    ldx8_v(0x54);  /* 818A88 ldx */
    lda8_v(0x2D);  /* 818A8A lda */
    abi_PaletteCycleStart();  /* 818A8C jsr */
    ldx8_v(0x00);  /* 818A8F ldx */
    lda8_v(0x08);  /* 818A91 lda */
    abi_PaletteCycleStart();  /* 818A93 jsr */
    ldx8_v(0x0E);  /* 818A96 ldx */
    lda8_v(0x09);  /* 818A98 lda */
    abi_PaletteCycleStart();  /* 818A9A jsr */
    ldx8_v(0x1C);  /* 818A9D ldx */
    lda8_v(0x0E);  /* 818A9F lda */
    abi_PaletteCycleStart();  /* 818AA1 jsr */
    ldx8_v(0x2A);  /* 818AA4 ldx */
    lda8_v(0x0F);  /* 818AA6 lda */
    abi_PaletteCycleStart();  /* 818AA8 jsr */
    ldx8_v(0x38);  /* 818AAB ldx */
    lda8_v(0x10);  /* 818AAD lda */
    abi_PaletteCycleStart();  /* 818AAF jsr */
    lda8_v(0x17);  /* 818AB2 lda */
    sta8(ea_abs(TM_SHADOW));  /* 818AB4 sta */
    lda8_v(0x10);  /* 818AB7 lda */
    sta8(ea_abs(TS_SHADOW));  /* 818AB9 sta */
    lda8_v(0x03);  /* 818ABC lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 818ABE sta */
    lda8_v(0x53);  /* 818AC1 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 818AC3 sta */
    return;  /* 818AC6 rts */
}


/*
 * @routine 81:8B1C Level0BScreenSetup
 * WHAT IT DOES: Level $0B's screen set-up: eight palette cycles, layers
 * 1-3 and sprites on the main screen, layers 1 and 3 on the sub screen,
 * colour maths on all but layer 3, at half; layer set-up mode 2.
 * HOW: PaletteCycleStart for (slot, cycle) (0, 0), ($0E, 1), ($1C, 3),
 * ($2A, $0B), ($54, $18), ($64, $1B), ($74, $1E), ($84, $21); TM_SHADOW
 * $17, TS_SHADOW $15; CGWSEL $03, CGADSUB $73 (shadows); V_083F = 2.
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
 * $81:87EF); A = 2.
 */
void abi_Level0BScreenSetup(void)
{
    ldx8_v(0x00);  /* 818B1C ldx */
    lda8_v(0x00);  /* 818B1E lda */
    abi_PaletteCycleStart();  /* 818B20 jsr */
    ldx8_v(0x0E);  /* 818B23 ldx */
    lda8_v(0x01);  /* 818B25 lda */
    abi_PaletteCycleStart();  /* 818B27 jsr */
    ldx8_v(0x1C);  /* 818B2A ldx */
    lda8_v(0x03);  /* 818B2C lda */
    abi_PaletteCycleStart();  /* 818B2E jsr */
    ldx8_v(0x2A);  /* 818B31 ldx */
    lda8_v(0x0B);  /* 818B33 lda */
    abi_PaletteCycleStart();  /* 818B35 jsr */
    ldx8_v(0x54);  /* 818B38 ldx */
    lda8_v(0x18);  /* 818B3A lda */
    abi_PaletteCycleStart();  /* 818B3C jsr */
    ldx8_v(0x64);  /* 818B3F ldx */
    lda8_v(0x1B);  /* 818B41 lda */
    abi_PaletteCycleStart();  /* 818B43 jsr */
    ldx8_v(0x74);  /* 818B46 ldx */
    lda8_v(0x1E);  /* 818B48 lda */
    abi_PaletteCycleStart();  /* 818B4A jsr */
    ldx8_v(0x84);  /* 818B4D ldx */
    lda8_v(0x21);  /* 818B4F lda */
    abi_PaletteCycleStart();  /* 818B51 jsr */
    lda8_v(0x17);  /* 818B54 lda */
    sta8(ea_abs(TM_SHADOW));  /* 818B56 sta */
    lda8_v(0x15);  /* 818B59 lda */
    sta8(ea_abs(TS_SHADOW));  /* 818B5B sta */
    lda8_v(0x03);  /* 818B5E lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 818B60 sta */
    lda8_v(0x73);  /* 818B63 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 818B65 sta */
    lda8_v(0x02);  /* 818B68 lda */
    sta8(ea_abs(V_083F));  /* 818B6A sta */
    return;  /* 818B6D rts */
}


/*
 * @routine 81:88C5 Level02ScreenSetup
 * WHAT IT DOES: Level 2's (and level $15's) screen set-up: layer set-up
 * mode $0E, layer 2 following $40, and HDMA channel 4 writing layer 2's
 * horizontal scroll from the table at $7E:F400.
 * HOW: V_083F = $0E, BG2_FOLLOWS = $40, V_7FF441 = 8, V_7FF457 = 6, V_1C7D
 * = $0C; channel 4: mode 2 (two writes) to BG2HOFS ($0F), the table at
 * $7E:F400 (also in HDMA_TABLE_AT), indirect bank 0; HDMAEN shadow = $10
 * (channel 4 alone).
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF), or called by JSR (81:8D42, level $15's entry).
 * Exit: A = $10; X, Y unchanged.
 */
void abi_Level02ScreenSetup(void)
{
    lda8_v(0x0E);  /* 8188C5 lda */
    sta8(ea_abs(V_083F));  /* 8188C7 sta */
    lda8_v(0x40);  /* 8188CA lda */
    sta8(ea_abs(BG2_FOLLOWS));  /* 8188CC sta */
    lda8_v(0x08);  /* 8188CF lda */
    sta8(V_7FF441);  /* 8188D1 sta */
    lda8_v(0x06);  /* 8188D5 lda */
    sta8(V_7FF457);  /* 8188D7 sta */
    lda8_v(0x0C);  /* 8188DB lda */
    sta8(ea_abs(V_1C7D));  /* 8188DD sta */
    lda8_v(0x02);  /* 8188E0 lda */
    sta8(ea_abs(DMAP4));  /* 8188E2 sta */
    lda8_v(0x0F);  /* 8188E5 lda */
    sta8(ea_abs(BBAD4));  /* 8188E7 sta */
    lda8_v(0x00);  /* 8188EA lda */
    sta8(ea_abs(A1T4L));  /* 8188EC sta */
    sta8(ea_abs(HDMA_TABLE_AT));  /* 8188EF sta */
    lda8_v(0xF4);  /* 8188F2 lda */
    sta8(ea_abs(A1T4H));  /* 8188F4 sta */
    sta8(ea_abs(HDMA_TABLE_AT_HIGH));  /* 8188F7 sta */
    lda8_v(0x7E);  /* 8188FA lda */
    sta8(ea_abs(A1B4));  /* 8188FC sta */
    stz8(ea_abs(DASB4));  /* 8188FF stz */
    lda8_v(0x10);  /* 818902 lda */
    sta8(ea_abs(HDMAEN_SHADOW));  /* 818904 sta */
    return;  /* 818907 rts */
}


/*
 * @routine 81:888B Level00ScreenSetup
 * WHAT IT DOES: Level 0's (and levels $14 and $1D's) screen set-up: layer
 * set-up mode $0A, layer 2 following $40, five palette cycles, and HDMA
 * channel 4 writing layer 2's horizontal scroll from the table at $7E:F400
 * (as Level02ScreenSetup).
 * HOW: V_083F = $0A, BG2_FOLLOWS = $40; PaletteCycleStart ($54, 0);
 * V_7FF481 and V_7FF495 = $18; PaletteCycleStart ($54, 0), ($64, 3), ($74,
 * 6), ($84, 9); then into Level02ScreenSetup's HDMA channel 4 set-up.
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF), or called by JSR (81:8D6C). Exit: as
 * Level02ScreenSetup.
 */
void abi_Level00ScreenSetup(void)
{
    lda8_v(0x0A);  /* 81888B lda */
    sta8(ea_abs(V_083F));  /* 81888D sta */
    lda8_v(0x40);  /* 818890 lda */
    sta8(ea_abs(BG2_FOLLOWS));  /* 818892 sta */
    ldx8_v(0x54);  /* 818895 ldx */
    lda8_v(0x00);  /* 818897 lda */
    abi_PaletteCycleStart();  /* 818899 jsr */
    lda8_v(0x18);  /* 81889C lda */
    sta8(V_7FF481);  /* 81889E sta */
    sta8(V_7FF495);  /* 8188A2 sta */
    ldx8_v(0x54);  /* 8188A6 ldx */
    lda8_v(0x00);  /* 8188A8 lda */
    abi_PaletteCycleStart();  /* 8188AA jsr */
    ldx8_v(0x64);  /* 8188AD ldx */
    lda8_v(0x03);  /* 8188AF lda */
    abi_PaletteCycleStart();  /* 8188B1 jsr */
    ldx8_v(0x74);  /* 8188B4 ldx */
    lda8_v(0x06);  /* 8188B6 lda */
    abi_PaletteCycleStart();  /* 8188B8 jsr */
    ldx8_v(0x84);  /* 8188BB ldx */
    lda8_v(0x09);  /* 8188BD lda */
    abi_PaletteCycleStart();  /* 8188BF jsr */
    goto L_8188E0;  /* 8188C2 bra */
L_8188E0:
    lda8_v(0x02);  /* 8188E0 lda */
    sta8(ea_abs(DMAP4));  /* 8188E2 sta */
    lda8_v(0x0F);  /* 8188E5 lda */
    sta8(ea_abs(BBAD4));  /* 8188E7 sta */
    lda8_v(0x00);  /* 8188EA lda */
    sta8(ea_abs(A1T4L));  /* 8188EC sta */
    sta8(ea_abs(HDMA_TABLE_AT));  /* 8188EF sta */
    lda8_v(0xF4);  /* 8188F2 lda */
    sta8(ea_abs(A1T4H));  /* 8188F4 sta */
    sta8(ea_abs(HDMA_TABLE_AT_HIGH));  /* 8188F7 sta */
    lda8_v(0x7E);  /* 8188FA lda */
    sta8(ea_abs(A1B4));  /* 8188FC sta */
    stz8(ea_abs(DASB4));  /* 8188FF stz */
    lda8_v(0x10);  /* 818902 lda */
    sta8(ea_abs(HDMAEN_SHADOW));  /* 818904 sta */
    return;  /* 818907 rts */
}


/*
 * @routine 81:8B6E Level0CScreenSetup
 * WHAT IT DOES: Level $0C's screen set-up: the eight palette cycles of
 * level $0B, nothing else.
 * HOW: PaletteCycleStart for (slot, cycle) (0, 0), ($0E, 1), ($1C, 3),
 * ($2A, $0B), ($54, $18), ($64, $1B), ($74, $1E), ($84, $21).
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF). Exit: by RTS; as the last PaletteCycleStart left
 * them.
 */
void abi_Level0CScreenSetup(void)
{
    ldx8_v(0x00);  /* 818B6E ldx */
    lda8_v(0x00);  /* 818B70 lda */
    abi_PaletteCycleStart();  /* 818B72 jsr */
    ldx8_v(0x0E);  /* 818B75 ldx */
    lda8_v(0x01);  /* 818B77 lda */
    abi_PaletteCycleStart();  /* 818B79 jsr */
    ldx8_v(0x1C);  /* 818B7C ldx */
    lda8_v(0x03);  /* 818B7E lda */
    abi_PaletteCycleStart();  /* 818B80 jsr */
    ldx8_v(0x2A);  /* 818B83 ldx */
    lda8_v(0x0B);  /* 818B85 lda */
    abi_PaletteCycleStart();  /* 818B87 jsr */
    ldx8_v(0x54);  /* 818B8A ldx */
    lda8_v(0x18);  /* 818B8C lda */
    abi_PaletteCycleStart();  /* 818B8E jsr */
    ldx8_v(0x64);  /* 818B91 ldx */
    lda8_v(0x1B);  /* 818B93 lda */
    abi_PaletteCycleStart();  /* 818B95 jsr */
    ldx8_v(0x74);  /* 818B98 ldx */
    lda8_v(0x1E);  /* 818B9A lda */
    abi_PaletteCycleStart();  /* 818B9C jsr */
    ldx8_v(0x84);  /* 818B9F ldx */
    lda8_v(0x21);  /* 818BA1 lda */
    abi_PaletteCycleStart();  /* 818BA3 jsr */
    return;  /* 818BA6 rts */
}

/*
 * @routine 81:8AC7 Level09ScreenSetup
 * WHAT IT DOES: Level 9's screen set-up: four palette cycles, then the
 * layers and colour maths of level 8.
 * HOW: PaletteCycleStart for (slot, cycle) (0, $19), ($0E, $1A), ($1C,
 * $26), ($2A, $27); then into Level08ScreenSetup's layers and colour maths.
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF). Exit: by RTS; A = $53.
 */
void abi_Level09ScreenSetup(void)
{
    goto L_818AC7;
L_818AB2:
    lda8_v(0x17);  /* 818AB2 lda */
    sta8(ea_abs(TM_SHADOW));  /* 818AB4 sta */
    lda8_v(0x10);  /* 818AB7 lda */
    sta8(ea_abs(TS_SHADOW));  /* 818AB9 sta */
    lda8_v(0x03);  /* 818ABC lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 818ABE sta */
    lda8_v(0x53);  /* 818AC1 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 818AC3 sta */
    return;  /* 818AC6 rts */
L_818AC7:
    ldx8_v(0x00);  /* 818AC7 ldx */
    lda8_v(0x19);  /* 818AC9 lda */
    abi_PaletteCycleStart();  /* 818ACB jsr */
    ldx8_v(0x0E);  /* 818ACE ldx */
    lda8_v(0x1A);  /* 818AD0 lda */
    abi_PaletteCycleStart();  /* 818AD2 jsr */
    ldx8_v(0x1C);  /* 818AD5 ldx */
    lda8_v(0x26);  /* 818AD7 lda */
    abi_PaletteCycleStart();  /* 818AD9 jsr */
    ldx8_v(0x2A);  /* 818ADC ldx */
    lda8_v(0x27);  /* 818ADE lda */
    abi_PaletteCycleStart();  /* 818AE0 jsr */
    goto L_818AB2;  /* 818AE3 bra */
}
