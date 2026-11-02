/*
 * level_screen_setups.c: levels' own screen set-ups, by level from the
 * table at $81:8802: palette cycles, the layers on the main and sub
 * screens, colour maths, and an HDMA wave on layer 2. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
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

/* PaletteCycleStart of cycle `a` at slot offset `x`. */
static void cycle(uint8_t x, uint8_t a)
{
    x8_load(x);
    a8_load(a);
    abi_PaletteCycleStart();
}

/* The layers on the main and sub screens, and the colour math. */
static void layers(uint8_t tm, uint8_t ts, uint8_t cgwsel, uint8_t cgadsub)
{
    wr8(dba(TM_SHADOW), tm);
    wr8(dba(TS_SHADOW), ts);
    wr8(dba(CGWSEL_SHADOW), cgwsel);
    a8_load(cgadsub);
    wr8(dba(CGADSUB_SHADOW), cgadsub);
}

/* HDMA channel 4 writing BG2's vertical scroll from $7E:F400 (the
   table named for the NMI too), and turned on. */
static void bg2_scroll_hdma(void)
{
    wr8(dba(DMAP4), 0x02);
    wr8(dba(BBAD4), 0x0F);
    wr8(dba(A1T4L), 0x00);
    wr8(dba(HDMA_TABLE_AT), 0x00);
    wr8(dba(A1T4H), 0xF4);
    wr8(dba(HDMA_TABLE_AT_HIGH), 0xF4);
    wr8(dba(A1B4), 0x7E);
    wr8(dba(DASB4), 0x00);
    a8_load(0x10);
    wr8(dba(HDMAEN_SHADOW), 0x10);
}

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
    cycle(0x54, 0x2D);
    cycle(0x00, 0x08);
    cycle(0x0E, 0x09);
    cycle(0x1C, 0x0E);
    cycle(0x2A, 0x0F);
    cycle(0x38, 0x10);
    layers(0x17, 0x10, 0x03, 0x53);
}


/* The eight cycles levels $0B and $0C share. */
static void cycles_0b(void)
{
    cycle(0x00, 0x00);
    cycle(0x0E, 0x01);
    cycle(0x1C, 0x03);
    cycle(0x2A, 0x0B);
    cycle(0x54, 0x18);
    cycle(0x64, 0x1B);
    cycle(0x74, 0x1E);
    cycle(0x84, 0x21);
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
    cycles_0b();
    layers(0x17, 0x15, 0x03, 0x73);
    a8_load(0x02);
    wr8(dba(V_083F), 0x02);
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
    wr8(dba(V_083F), 0x0E);
    wr8(dba(BG2_FOLLOWS), 0x40);
    wr8(V_7FF441, 0x08);
    wr8(V_7FF457, 0x06);
    wr8(dba(V_1C7D), 0x0C);
    bg2_scroll_hdma();
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
    wr8(dba(V_083F), 0x0A);
    wr8(dba(BG2_FOLLOWS), 0x40);
    cycle(0x54, 0x00);
    a8_load(0x18);
    wr8(V_7FF481, 0x18);
    wr8(V_7FF495, 0x18);
    cycle(0x54, 0x00);
    cycle(0x64, 0x03);
    cycle(0x74, 0x06);
    cycle(0x84, 0x09);
    bg2_scroll_hdma();
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
    cycles_0b();
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
    cycle(0x00, 0x19);
    cycle(0x0E, 0x1A);
    cycle(0x1C, 0x26);
    cycle(0x2A, 0x27);
    layers(0x17, 0x10, 0x03, 0x53);
}
