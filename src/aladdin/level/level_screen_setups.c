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
void abi_TaskCreate(void);
void abi_Bits035DExpand(void);

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

/* More of the levels' state their set-ups write */
#define SCROLL_KIND 0x083Du         /* the level's scroll handler (video/scroll_update.c), times 2 */
#define V_1A09 0x1A09u              /* added to V_1A08 for the camera's window */
#define V_145B 0x145Bu
#define V_1C75 0x1C75u              /* word */
#define SCROLL_Y_HELD 0x1A12u       /* nonzero: no camera follow up and down */
#define RESTART_POINT 0x0360u       /* the level's restart point the player starts from */
#define V_7FF043 0x7FF043u
#define LEVEL06_BANDS 0xB991u       /* level 6's band count, then each band's lines (bank DB, $80) */

/* TaskCreate from the JSL at site, its five inline bytes after it. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/*
 * @routine 81:8943 Level04ScreenSetup
 * WHAT IT DOES: Level 4's screen set-up: its camera trigger task and one
 * palette cycle.
 * HOW: TaskCreate slot 3, CameraTriggerTask; PaletteCycleStart (0, 2).
 * Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
 * at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
 * $81:87EF); as PaletteCycleStart leaves the registers.
 */
void abi_Level04ScreenSetup(void)
{
    task_create(0x818943u);
    cycle(0x00, 0x02);
}

/*
 * @routine 81:8954 Level05ScreenSetup
 * WHAT IT DOES: Level 5's screen set-up: the camera's window moved on, its
 * end scene's task and a window effect, three palette cycles, layers 1-3
 * and sprites on the main screen with sprites alone on the sub screen,
 * colour maths adding the sub screen at half.
 * HOW: V_1A09 $28; TaskCreate slot 3 Level05EndScene, slot 6 (parameter
 * 20) WindowEffectTask; PaletteCycleStart (0, $0A), ($0E, $0C), ($1C,
 * $25); TM_SHADOW $17, TS_SHADOW $10; CGWSEL $03, CGADSUB $53 (shadows).
 * Entry: as Level04ScreenSetup. Exit: by RTS; A = $53.
 */
void abi_Level05ScreenSetup(void)
{
    a8_load(0x28);
    wr8(dba(V_1A09), 0x28);
    task_create(0x818959u);
    task_create(0x818962u);
    cycle(0x00, 0x0A);
    cycle(0x0E, 0x0C);
    cycle(0x1C, 0x25);
    layers(0x17, 0x10, 0x03, 0x53);
}

/*
 * @routine 81:8995 Level06ScreenSetup
 * WHAT IT DOES: Level 6's screen set-up: two tasks (the bands' speed-up
 * and a colour band), eight palette cycles, the camera's scroll kind 2
 * and parallax kind $0C, BG2 following the backdrop's y only, and two
 * HDMA channels: 1 writing COLDATA from $80:B99E, 2 the backdrop bands'
 * BG2 scroll from $7E:F400, whose line counts it copies in.
 * HOW: TaskCreate slot 3 BandsSpeedUpTask, slot 6 ColourBandTask;
 * PaletteCycleStart ($54, $0C), ($64, $0F), ($74, $12), ($84, $15), (0,
 * 4), ($0E, 5), ($1C, 6), ($2A, 7); V_145B 3, SCROLL_KIND 2, V_083F $0C,
 * BG2_FOLLOWS $40; channel 1: DMAP 0, BBAD $2C, A1T $80:B99E; channel 2:
 * DMAP 2, BBAD $0F, A1T $7E:F400; LEVEL06_BANDS' count of line bytes to
 * every third byte of BAND_HDMA; HDMAEN_SHADOW 6.
 * Entry: as Level04ScreenSetup; DB = $80. Exit: by RTS; A = 6; X = 3 per
 * band, Y = the bands (8-bit); SCRATCH 0.
 */
void abi_Level06ScreenSetup(void)
{
    uint8_t n, i;

    task_create(0x818995u);
    task_create(0x81899Eu);
    cycle(0x54, 0x0C);
    cycle(0x64, 0x0F);
    cycle(0x74, 0x12);
    cycle(0x84, 0x15);
    cycle(0x00, 0x04);
    cycle(0x0E, 0x05);
    cycle(0x1C, 0x06);
    cycle(0x2A, 0x07);
    wr8(dba(V_145B), 0x03);
    wr8(dba(SCROLL_KIND), 0x02);
    wr8(dba(V_083F), 0x0C);
    wr8(dba((uint16_t)BG2_FOLLOWS), 0x40);
    wr8(dba((uint16_t)DMAP1), 0x00);
    wr8(dba((uint16_t)BBAD1), 0x2C);
    wr8(dba((uint16_t)A1T1L), 0x9E);
    wr8(dba((uint16_t)A1T1H), 0xB9);
    wr8(dba((uint16_t)A1B1), 0x80);
    wr8(dba((uint16_t)DASB1), 0x00);
    wr8(dba((uint16_t)DMAP2), 0x02);
    wr8(dba((uint16_t)BBAD2), 0x0F);
    wr8(dba((uint16_t)A1T2L), 0x00);
    wr8(dba((uint16_t)A1T2H), 0xF4);
    wr8(dba((uint16_t)A1B2), 0x7E);
    wr8(dba((uint16_t)DASB2), 0x00);
    n = rd8(dba(LEVEL06_BANDS));
    wr8(dba((uint16_t)SCRATCH), n);
    for (i = 0; i < n; i++)
        wr8(BAND_HDMA + 3 * i, rd8(dba((uint16_t)(LEVEL06_BANDS + 1 + i))));
    wr8(dba((uint16_t)SCRATCH), 0);
    cpu.x = (uint8_t)(3 * n);
    cpu.y = n;
    a8_load(0x06);
    wr8(dba((uint16_t)HDMAEN_SHADOW), 0x06);
}

/*
 * @routine 81:8AE5 Level0AScreenSetup
 * WHAT IT DOES: Level $0A's screen set-up: the camera's window moved on,
 * the climb's top task, three palette cycles; V_1C75 $00BD; started from
 * a restart point, $7F:F043 = $FE (no script starts it there).
 * HOW: V_1A09 $50; TaskCreate slot 3 ClimbTopTask; PaletteCycleStart (0,
 * $1D), ($0E, $1E), ($1C, $23); V_1C75 $BD, then 0; RESTART_POINT nonzero:
 * V_7FF043 $FE.
 * Entry: as Level04ScreenSetup. Exit: by RTS; A = RESTART_POINT, N and Z
 * from it, or $FE.
 */
void abi_Level0AScreenSetup(void)
{
    uint8_t r;

    a8_load(0x50);
    wr8(dba(V_1A09), 0x50);
    task_create(0x818AEAu);
    cycle(0x00, 0x1D);
    cycle(0x0E, 0x1E);
    cycle(0x1C, 0x23);
    a8_load(0xBD);
    wr8(dba(V_1C75), 0xBD);
    wr8(dba(V_1C75 + 1), 0x00);
    r = rd8(dba(RESTART_POINT));
    a8_load(r);
    if (r) {
        a8_load(0xFE);
        wr8(V_7FF043, 0xFE);
    }
}

/*
 * @routine 81:8BA7 Level0DScreenSetup
 * WHAT IT DOES: Level $0D's screen set-up: its scene's task, two palette
 * cycles, parallax kind $14.
 * HOW: TaskCreate slot 3 Level0DScene; PaletteCycleStart ($0E, 1), ($1C,
 * 3); V_083F $14.
 * Entry: as Level04ScreenSetup. Exit: by RTS; A = $14.
 */
void abi_Level0DScreenSetup(void)
{
    task_create(0x818BA7u);
    cycle(0x0E, 0x01);
    cycle(0x1C, 0x03);
    a8_load(0x14);
    wr8(dba(V_083F), 0x14);
}

/*
 * @routine 81:8BC4 Level0EScreenSetup
 * WHAT IT DOES: Level $0E's screen set-up: the grey past task, three
 * palette cycles, layers 1-3 and sprites on both screens, parallax kind
 * 8, then the level's bits expanded.
 * HOW: TaskCreate slot 6 GreyPastTask; PaletteCycleStart (0, $0D), ($0E,
 * $1B), ($54, $15); TM_SHADOW and TS_SHADOW $17; V_083F 8;
 * Bits035DExpand.
 * Entry: as Level04ScreenSetup. Exit: by RTS; as Bits035DExpand leaves
 * the registers.
 */
void abi_Level0EScreenSetup(void)
{
    task_create(0x818BC4u);
    cycle(0x00, 0x0D);
    cycle(0x0E, 0x1B);
    cycle(0x54, 0x15);
    a8_load(0x17);
    wr8(dba(TM_SHADOW), 0x17);
    wr8(dba(TS_SHADOW), 0x17);
    a8_load(0x08);
    wr8(dba(V_083F), 0x08);
    abi_Bits035DExpand();
}

/*
 * @routine 81:8CF5 AutoScrollScreenSetup
 * WHAT IT DOES: The screen set-up of the levels that scroll on their own:
 * the scroll kind A, no camera up and down, no parallax, layers 1-3 and
 * sprites on the main screen with sprites alone on the sub screen, colour
 * maths adding it at half, a window effect and four palette cycles.
 * HOW: SCROLL_KIND = A; V_145B 1, SCROLL_Y_HELD 1, V_083F 0; TM_SHADOW
 * $17, TS_SHADOW $10; CGWSEL $03, CGADSUB $53; TaskCreate slot 6
 * (parameter 18) WindowEffectTask; PaletteCycleStart (0, $15), ($0E,
 * $16), ($1C, $17), ($2A, $18).
 * Entry: M 8-bit, X 8-bit; A = the scroll kind. Exit: as
 * PaletteCycleStart leaves the registers.
 */
void abi_AutoScrollScreenSetup(void)
{
    wr8(dba(SCROLL_KIND), (uint8_t)cpu.a);
    a8_load(0x01);
    wr8(dba(V_145B), 0x01);
    wr8(dba(SCROLL_Y_HELD), 0x01);
    a8_load(0x00);
    wr8(dba(V_083F), 0x00);
    layers(0x17, 0x10, 0x03, 0x53);
    task_create(0x818D1Bu);
    cycle(0x00, 0x15);
    cycle(0x0E, 0x16);
    cycle(0x1C, 0x17);
    cycle(0x2A, 0x18);
}

/*
 * @routine 81:8CF3 Level12ScreenSetup
 * WHAT IT DOES: Level $12's screen set-up: it scrolls on its own, scroll
 * kind $0A (a set speed, the camera following up and down).
 * HOW: A = $0A, on into AutoScrollScreenSetup.
 * Entry: as Level04ScreenSetup. Exit: as AutoScrollScreenSetup.
 */
void abi_Level12ScreenSetup(void)
{
    a8_load(0x0A);
    abi_AutoScrollScreenSetup();
}

/*
 * @routine 81:8D61 Level1CScreenSetup
 * WHAT IT DOES: Level $1C's screen set-up: it scrolls on its own, scroll
 * kind $0E (the faster set speed), and the windows mask layer 3.
 * HOW: AutoScrollScreenSetup with A = $0E; TMW_SHADOW 4.
 * Entry: as Level04ScreenSetup. Exit: by RTS; A = 4.
 */
void abi_Level1CScreenSetup(void)
{
    a8_load(0x0E);
    abi_AutoScrollScreenSetup();
    a8_load(0x04);
    wr8(dba((uint16_t)TMW_SHADOW), 0x04);
}

/*
 * @routine 81:8D6C Level14ScreenSetup
 * WHAT IT DOES: Levels $14 and $1D's screen set-up: level 0's.
 * HOW: Level00ScreenSetup.
 * Entry: as Level04ScreenSetup. Exit: as Level00ScreenSetup.
 */
void abi_Level14ScreenSetup(void)
{
    abi_Level00ScreenSetup();
}

/*
 * @routine 81:8D42 Level15ScreenSetup
 * WHAT IT DOES: Level $15's screen set-up: level 2's.
 * HOW: Level02ScreenSetup.
 * Entry: as Level04ScreenSetup. Exit: as Level02ScreenSetup.
 */
void abi_Level15ScreenSetup(void)
{
    abi_Level02ScreenSetup();
}
