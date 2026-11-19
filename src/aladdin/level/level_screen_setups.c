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
void abi_UnpackSet(void);
void abi_LevelStartPlace(void);
void abi_Level04ScreenSetup(void);
void abi_Level07ScreenSetup(void);
void abi_Level08ScreenSetup(void);
void abi_Level0AScreenSetup(void);
void abi_Level0BScreenSetup(void);
void abi_Level0FScreenSetup(void);

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
 * @routine 81:88C4 Level01ScreenSetup
 * WHAT IT DOES: Level 1's screen set-up: nothing (an RTS).
 * HOW: RTS.
 * Entry: reached by the jump through the table by level at $81:8802
 * (81:87FF). Exit: every register and flag unchanged.
 */
void abi_Level01ScreenSetup(void)
{
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
 * @routine 81:888A LevelNoScreenSetup
 * WHAT IT DOES: The screen set-up of the levels with none of their own
 * (levels $1E and $20 on, the Genie's screen $2F among them): nothing (an
 * RTS).
 * HOW: RTS.
 * Entry: reached by the jump through the table by level at $81:8802
 * (81:87FF). Exit: every register and flag unchanged.
 */
void abi_LevelNoScreenSetup(void)
{
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
    jsl_frame(0x818BEFu);
    abi_Bits035DExpand();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    call_frame(0x8D65, abi_AutoScrollScreenSetup);
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
    call_frame(0x8D6E, abi_Level00ScreenSetup);
}

/*
 * @routine 81:8D42 Level15ScreenSetup
 * WHAT IT DOES: Level $15's screen set-up: level 2's.
 * HOW: Level02ScreenSetup.
 * Entry: as Level04ScreenSetup. Exit: as Level02ScreenSetup.
 */
void abi_Level15ScreenSetup(void)
{
    call_frame(0x8D44, abi_Level02ScreenSetup);
}

#define V_03A1 0x03A1u
#define BAND_FILL 0x7EDF80u         /* level $11's table of 64 words $4000 (its HDMA scroll) */
#define LEVEL11_MODES 0xB9C3u       /* level $11's HDMA tables in bank $80: BGMODE, BG3 y, TM */
#define LEVEL11_BG3Y 0xB9CAu
#define LEVEL11_LAYERS 0xB9D4u
#define LEVEL_SETUPS 0x818802u      /* the set-ups by level (words, this bank) */

/*
 * @routine 81:8A4A Level07ScreenSetup
 * WHAT IT DOES: Level 7's screen set-up: its red-out task, three palette
 * cycles, scroll kind 6, layers 1-3 and sprites on the main screen, its
 * graphics set $1A unpacked, parallax kind $12.
 * HOW: TaskCreate slot 6 RedOutTask; PaletteCycleStart (0, $11), ($0E,
 * $12), ($1C, $13); V_145B 1, SCROLL_KIND 6, TM_SHADOW $17;
 * UnpackSet $1A; V_083F $12; V_03A1 $3B.
 * Entry: as Level04ScreenSetup. Exit: by RTS; A = $3B.
 */
void abi_Level07ScreenSetup(void)
{
    task_create(0x818A4Au);
    cycle(0x00, 0x11);
    cycle(0x0E, 0x12);
    cycle(0x1C, 0x13);
    a8_load(0x01);
    wr8(dba(V_145B), 0x01);
    a8_load(0x06);
    wr8(dba(SCROLL_KIND), 0x06);
    a8_load(0x17);
    wr8(dba(TM_SHADOW), 0x17);
    a8_load(0x1A);
    jsl_frame(0x818A79u);
    abi_UnpackSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x12);
    wr8(dba(V_083F), 0x12);
    a8_load(0x3B);
    wr8(dba(V_03A1), 0x3B);
}

/*
 * @routine 81:8C62 Level11ScreenSetup
 * WHAT IT DOES: Level $11's screen set-up: parallax kind 2, its graphics
 * set $21 unpacked, a table of 64 words $4000 at BAND_FILL, three HDMA
 * channels from bank $80's tables (BGMODE, BG3's y, the main screen's
 * layers), layers 2 and 3 and sprites on the main screen, three palette
 * cycles.
 * HOW: V_083F 2; UnpackSet $21; SCROLL_LIMIT_X's high byte 1; BAND_FILL
 * from its last word down: 0, $40; channel 1: DMAP 0, BBAD $05, A1T
 * $80:B9C3; channel 2: DMAP 2, BBAD $12, A1T $80:B9CA; channel 3: DMAP 0,
 * BBAD $2C, A1T $80:B9D4; TM_SHADOW $16; PaletteCycleStart ($64, $24),
 * ($74, $27), ($84, $2A).
 * Entry: as Level04ScreenSetup. Exit: by RTS; as PaletteCycleStart leaves
 * the registers.
 */
void abi_Level11ScreenSetup(void)
{
    int x;

    a8_load(0x02);
    wr8(dba(V_083F), 0x02);
    a8_load(0x21);
    jsl_frame(0x818C69u);
    abi_UnpackSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x01);
    wr8(dba((uint16_t)(SCROLL_LIMIT_X + 1)), 0x01);
    for (x = 0x7E; x >= 0; x -= 2) {
        wr8(BAND_FILL + x, 0x00);
        wr8(BAND_FILL + 1 + x, 0x40);
    }
    cpu.x = 0xFE;
    a8_load(0x00);
    wr8(dba((uint16_t)DMAP1), 0x00);
    wr8(dba((uint16_t)BBAD1), 0x05);
    wr8(dba((uint16_t)A1T1L), (uint8_t)LEVEL11_MODES);
    wr8(dba((uint16_t)A1T1H), (uint8_t)(LEVEL11_MODES >> 8));
    wr8(dba((uint16_t)A1B1), 0x80);
    wr8(dba((uint16_t)DASB1), 0x00);
    wr8(dba((uint16_t)DMAP2), 0x02);
    wr8(dba((uint16_t)BBAD2), 0x12);
    wr8(dba((uint16_t)A1T2L), (uint8_t)LEVEL11_BG3Y);
    wr8(dba((uint16_t)A1T2H), (uint8_t)(LEVEL11_BG3Y >> 8));
    wr8(dba((uint16_t)A1B2), 0x80);
    wr8(dba((uint16_t)DASB2), 0x00);
    wr8(dba((uint16_t)DMAP3), 0x00);
    wr8(dba((uint16_t)BBAD3), 0x2C);
    wr8(dba((uint16_t)A1T3L), (uint8_t)LEVEL11_LAYERS);
    wr8(dba((uint16_t)A1T3H), (uint8_t)(LEVEL11_LAYERS >> 8));
    wr8(dba((uint16_t)A1B3), 0x80);
    wr8(dba((uint16_t)DASB3), 0x00);
    a8_load(0x16);
    wr8(dba(TM_SHADOW), 0x16);
    cycle(0x64, 0x24);
    cycle(0x74, 0x27);
    cycle(0x84, 0x2A);
}

/*
 * @routine 81:87EF LevelScreenSetup
 * WHAT IT DOES: Starts the level's screen: V_145B and V_1C7D cleared,
 * NMI, IRQ by V and auto joypad read on, then the level's own set-up.
 * HOW: V_145B 0, NMITIMEN_SHADOW $31, V_1C7D 0; JMP through LEVEL_SETUPS
 * by LEVEL (byte, times 2).
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR from the level's start (81:8216). Exit: as the level's
 * set-up.
 */
void abi_LevelScreenSetup(void)
{
    uint8_t level;

    wr8(dba(V_145B), 0);
    a8_load(0x31);
    wr8(dba((uint16_t)NMITIMEN_SHADOW), 0x31);
    wr8(dba(V_1C7D), 0);
    level = rd8(dba((uint16_t)LEVEL));
    flag_set(P_C, level & 0x80);
    a8_load((uint8_t)(level << 1));
    x8_load((uint8_t)cpu.a);
    jump_routine(0x810000u | rd16(LEVEL_SETUPS + cpu.x));
}

/* The set-ups of the levels no script plays, and a colour split no code
   reaches. Each runs only in the unit calls of cov_level_setups.box. */
#define V_7FF489 0x7FF489u
#define BACKDROP_STAGE 0x0841u      /* PlayField3Stages's stage, times 2 (word) */
#define SPLIT_TABLE 0x1C7Eu         /* HDMA channel 6's table: three entries of lines and four bytes */
#define SPLIT_LAYERS 0xB9DBu        /* (bank $80) HDMA channel 7's table: TM by lines */

/*
 * @routine 81:8908 Level03ScreenSetup
 * WHAT IT DOES: Level 3's screen set-up: layer set-up mode $0A, layer 2
 * following $40, a task in slot 3, four palette cycles, and HDMA channel 4
 * writing layer 2's horizontal scroll from the table at $7E:F400 (as
 * Level02ScreenSetup). No script plays level 3.
 * HOW: V_083F = $0A, BG2_FOLLOWS = $40, V_7FF481 and V_7FF489 = 2;
 * TaskCreate (slot 3, parameter 0, $80:A12D); PaletteCycleStart ($54, 0),
 * ($64, 3), ($74, 6), ($84, 9); then into Level02ScreenSetup's HDMA
 * channel 4 set-up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * reached by the jump through the table by level at $81:8802 (81:87FF).
 * Exit: as Level02ScreenSetup.
 */
void abi_Level03ScreenSetup(void)
{
    a8_load(0x0A);
    wr8(dba(V_083F), 0x0A);
    a8_load(0x40);
    wr8(dba(BG2_FOLLOWS), 0x40);
    a8_load(0x02);
    wr8(V_7FF481, 0x02);
    wr8(V_7FF489, 0x02);
    jsl_frame(0x81891Cu);
    abi_TaskCreate();
    cpu.unwound = 0;
    cycle(0x54, 0x00);
    cycle(0x64, 0x03);
    cycle(0x74, 0x06);
    cycle(0x84, 0x09);
    bg2_scroll_hdma();
}

/*
 * @routine 81:8BF4 Level0FScreenSetup
 * WHAT IT DOES: Level $0F's (and level $1B's) screen set-up: two palette
 * cycles, layers 1-3 and sprites on the main and the sub screen, layer
 * set-up mode 8, and the level's bits expanded. No script plays it.
 * HOW: PaletteCycleStart (0, $1C), ($54, $15); TM_SHADOW and TS_SHADOW
 * $17; V_083F = 8; Bits035DExpand.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * reached by the jump through the table by level at $81:8802 (81:87FF),
 * or called by JSR (81:8D5D). Exit: as Bits035DExpand leaves them.
 */
void abi_Level0FScreenSetup(void)
{
    cycle(0x00, 0x1C);
    cycle(0x54, 0x15);
    a8_load(0x17);
    wr8(dba(TM_SHADOW), 0x17);
    wr8(dba(TS_SHADOW), 0x17);
    a8_load(0x08);
    wr8(dba(V_083F), 0x08);
    jsl_frame(0x818C0Fu);
    abi_Bits035DExpand();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 81:8C14 Level10ScreenSetup
 * WHAT IT DOES: Level $10's screen set-up: layer set-up mode $10, the
 * backdrop's stage 4 past the first restart point, a task in slot 3,
 * seven palette cycles, and the level's bits expanded. No script plays
 * it from its start.
 * HOW: V_083F = $10; with RESTART_POINT nonzero, BACKDROP_STAGE's low
 * byte 4; TaskCreate (slot 3, parameter 0, $80:A5AC); PaletteCycleStart
 * (0, $0D), ($0E, $2A), ($1C, $2B), ($54, $15), ($64, $24), ($74, $27),
 * ($84, $2A); Bits035DExpand.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * reached by the jump through the table by level at $81:8802 (81:87FF).
 * Exit: as Bits035DExpand leaves them.
 */
void abi_Level10ScreenSetup(void)
{
    uint8_t point;

    a8_load(0x10);
    wr8(dba(V_083F), 0x10);
    point = rd8(dba(RESTART_POINT));
    a8_load(point);
    if (point) {
        a8_load(0x04);
        wr8(dba(BACKDROP_STAGE), 0x04);
    }
    jsl_frame(0x818C23u);
    abi_TaskCreate();
    cpu.unwound = 0;
    cycle(0x00, 0x0D);
    cycle(0x0E, 0x2A);
    cycle(0x1C, 0x2B);
    cycle(0x54, 0x15);
    cycle(0x64, 0x24);
    cycle(0x74, 0x27);
    cycle(0x84, 0x2A);
    jsl_frame(0x818C5Du);
    abi_Bits035DExpand();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 81:8D46 Level16ScreenSetup
 * WHAT IT DOES: Level $16's screen set-up: level 4's. No script plays it.
 * HOW: Level04ScreenSetup.
 * Entry: as Level04ScreenSetup. Exit: as Level04ScreenSetup.
 */
void abi_Level16ScreenSetup(void)
{
    call_frame(0x8D48, abi_Level04ScreenSetup);
}

/*
 * @routine 81:8D4A Level17ScreenSetup
 * WHAT IT DOES: Level $17's screen set-up: level 7's. No script plays it.
 * HOW: Level07ScreenSetup.
 * Entry: as Level07ScreenSetup. Exit: as Level07ScreenSetup.
 */
void abi_Level17ScreenSetup(void)
{
    call_frame(0x8D4C, abi_Level07ScreenSetup);
}

/*
 * @routine 81:8D4E Level18ScreenSetup
 * WHAT IT DOES: Level $18's screen set-up: level 8's. No script plays it.
 * HOW: Level08ScreenSetup.
 * Entry: as Level08ScreenSetup. Exit: as Level08ScreenSetup.
 */
void abi_Level18ScreenSetup(void)
{
    call_frame(0x8D50, abi_Level08ScreenSetup);
}

/*
 * @routine 81:8D52 Level19ScreenSetup
 * WHAT IT DOES: Level $19's screen set-up: level $0A's. No script plays
 * it.
 * HOW: Level0AScreenSetup.
 * Entry: as Level0AScreenSetup. Exit: as Level0AScreenSetup.
 */
void abi_Level19ScreenSetup(void)
{
    call_frame(0x8D54, abi_Level0AScreenSetup);
}

/*
 * @routine 81:8D56 Level1AScreenSetup
 * WHAT IT DOES: Level $1A's screen set-up: level $0B's, then the player
 * and the camera put at the level's start. No script plays it.
 * HOW: Level0BScreenSetup, then LevelStartPlace.
 * Entry: as Level0BScreenSetup, DB = $80 (for LevelStartPlace). Exit: as
 * LevelStartPlace.
 */
void abi_Level1AScreenSetup(void)
{
    call_frame(0x8D58, abi_Level0BScreenSetup);
    call_frame(0x8D5B, abi_LevelStartPlace);
}

/*
 * @routine 81:8D5D Level1BScreenSetup
 * WHAT IT DOES: Level $1B's screen set-up: level $0F's. No script plays
 * it.
 * HOW: Level0FScreenSetup.
 * Entry: as Level0FScreenSetup. Exit: as Level0FScreenSetup.
 */
void abi_Level1BScreenSetup(void)
{
    call_frame(0x8D5F, abi_Level0FScreenSetup);
}

/*
 * @routine 81:8D70 BackdropSplitSetup
 * WHAT IT DOES: Splits the screen's backdrop colour: the top 104 lines
 * keep colour 0 as it is in the palette buffer, the rest are black, and
 * HDMA channel 7 sets the main screen's layers by lines. No code refers
 * to it.
 * HOW: SPLIT_TABLE: 1 line, 103 lines, 1 line, then the end; each entry
 * CGRAM address 0 (twice) and a colour: colour 0 of PALETTE_BUFFER for
 * the first two, black for the third; colour 0 in the buffer then black.
 * Channel 6: mode 3 to CGADD/CGDATA from $00:1C7E (also in HDMA_TABLE_AT
 * + 4); channel 7: mode 0 to TM from SPLIT_LAYERS in bank $80 (also in
 * HDMA_TABLE_AT + 6); HDMAEN_SHADOW bits 6 and 7 set.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and $4300-$437F
 * the DMA registers; called by JSR (it ends RTS). Exit: A = the new
 * HDMAEN_SHADOW, N set, Z clear; X, Y unchanged.
 */
void abi_BackdropSplitSetup(void)
{
    uint8_t lo, hi;

    a8_load(0x01);
    wr8(dba(SPLIT_TABLE + 0), 0x01);
    a8_load(0x67);
    wr8(dba(SPLIT_TABLE + 5), 0x67);
    a8_load(0x01);
    wr8(dba(SPLIT_TABLE + 10), 0x01);
    wr8(dba(SPLIT_TABLE + 15), 0x00);
    wr8(dba(SPLIT_TABLE + 1), 0x00);
    wr8(dba(SPLIT_TABLE + 6), 0x00);
    wr8(dba(SPLIT_TABLE + 11), 0x00);
    wr8(dba(SPLIT_TABLE + 2), 0x00);
    wr8(dba(SPLIT_TABLE + 7), 0x00);
    wr8(dba(SPLIT_TABLE + 12), 0x00);
    lo = rd8(PALETTE_BUFFER);
    a8_load(lo);
    wr8(dba(SPLIT_TABLE + 3), lo);
    wr8(dba(SPLIT_TABLE + 8), lo);
    wr8(dba(SPLIT_TABLE + 13), 0x00);
    hi = rd8(PALETTE_BUFFER + 1);
    a8_load(hi);
    wr8(dba(SPLIT_TABLE + 4), hi);
    wr8(dba(SPLIT_TABLE + 9), hi);
    wr8(dba(SPLIT_TABLE + 14), 0x00);
    a8_load(0x00);
    wr8(PALETTE_BUFFER, 0x00);
    wr8(PALETTE_BUFFER + 1, 0x00);
    a8_load(0x03);
    wr8(dba(DMAP6), 0x03);
    a8_load(0x21);
    wr8(dba(BBAD6), 0x21);
    a8_load(0x7E);
    wr8(dba(A1T6L), 0x7E);
    wr8(dba(HDMA_TABLE_AT + 4), 0x7E);
    a8_load(0x1C);
    wr8(dba(A1T6H), 0x1C);
    wr8(dba(HDMA_TABLE_AT + 5), 0x1C);
    a8_load(0x00);
    wr8(dba(A1B6), 0x00);
    wr8(dba(DASB6), 0x00);
    a8_load(0x00);
    wr8(dba(DMAP7), 0x00);
    a8_load(0x2C);
    wr8(dba(BBAD7), 0x2C);
    a8_load(0xDB);
    wr8(dba(A1T7L), 0xDB);
    wr8(dba(HDMA_TABLE_AT + 6), 0xDB);
    a8_load(0xB9);
    wr8(dba(A1T7H), 0xB9);
    wr8(dba(HDMA_TABLE_AT + 7), 0xB9);
    a8_load(0x80);
    wr8(dba(A1B7), 0x80);
    wr8(dba(DASB7), 0x00);
    a8_load((uint8_t)(rd8(dba(HDMAEN_SHADOW)) | 0xC0));
    wr8(dba(HDMAEN_SHADOW), (uint8_t)cpu.a);
}

#define BAND_MODES 0x16E5u          /* HDMA channel 2's table: BGMODE by lines (9 bytes) */
#define BAND_LAYERS 0x16F5u         /* HDMA channel 3's table: TM by lines (9 bytes) */
#define BAND_SETS 0xB9A7u           /* (bank $80) 7 bytes a set: top mode, lines, lines, bottom mode, top, middle and bottom layers */

/*
 * @routine 81:8EC1 ModeBandsSetup
 * WHAT IT DOES: Splits the screen into bands by lines: a line on top, two
 * bands in Mode 7, and a line below, each with its own BG mode and main
 * screen layers, from the set at BAND_SETS + X. No code refers to it.
 * HOW: HDMA channel 2: mode 0 to BGMODE from $00:BAND_MODES; channel 3:
 * mode 0 to TM from $00:BAND_LAYERS. BAND_MODES: 1 line of the set's top
 * mode, its first lines of mode 7, its second lines of mode 7, 1 line of
 * its bottom mode, the end; BAND_LAYERS: the same lines with its top,
 * middle (both bands) and bottom layers.
 * Entry: M 8-bit; X = the set's offset (a multiple of 7; X at either
 * width); DB = $80; called by JSR (ModeBandsSetupLong). Exit: A = the
 * set's bottom layers, N and Z from it; X, Y unchanged.
 */
void abi_ModeBandsSetup(void)
{
    uint8_t v;
    uint16_t x = cpu.x;

    wr8(dba(DMAP2), 0x00);
    a8_load(0x05);
    wr8(dba(BBAD2), 0x05);
    a8_load(0xE5);
    wr8(dba(A1T2L), 0xE5);
    a8_load(0x16);
    wr8(dba(A1T2H), 0x16);
    a8_load(0x00);
    wr8(dba(A1B2), 0x00);
    wr8(dba(DASB2), 0x00);
    a8_load(0x01);
    wr8(dba(BAND_MODES + 0), 0x01);
    wr8(dba(BAND_LAYERS + 0), 0x01);
    v = rd8(dba((uint16_t)(BAND_SETS + x)));
    a8_load(v);
    wr8(dba(BAND_MODES + 1), v);
    v = rd8(dba((uint16_t)(BAND_SETS + 1 + x)));
    a8_load(v);
    wr8(dba(BAND_MODES + 2), v);
    wr8(dba(BAND_LAYERS + 2), v);
    a8_load(0x07);
    wr8(dba(BAND_MODES + 3), 0x07);
    wr8(dba(BAND_MODES + 5), 0x07);
    v = rd8(dba((uint16_t)(BAND_SETS + 2 + x)));
    a8_load(v);
    wr8(dba(BAND_MODES + 4), v);
    wr8(dba(BAND_LAYERS + 4), v);
    a8_load(0x01);
    wr8(dba(BAND_MODES + 6), 0x01);
    wr8(dba(BAND_LAYERS + 6), 0x01);
    v = rd8(dba((uint16_t)(BAND_SETS + 3 + x)));
    a8_load(v);
    wr8(dba(BAND_MODES + 7), v);
    wr8(dba(BAND_MODES + 8), 0x00);
    wr8(dba(BAND_LAYERS + 8), 0x00);
    wr8(dba(DMAP3), 0x00);
    a8_load(0x2C);
    wr8(dba(BBAD3), 0x2C);
    a8_load(0xF5);
    wr8(dba(A1T3L), 0xF5);
    a8_load(0x16);
    wr8(dba(A1T3H), 0x16);
    a8_load(0x00);
    wr8(dba(A1B3), 0x00);
    wr8(dba(DASB3), 0x00);
    v = rd8(dba((uint16_t)(BAND_SETS + 4 + x)));
    a8_load(v);
    wr8(dba(BAND_LAYERS + 1), v);
    v = rd8(dba((uint16_t)(BAND_SETS + 5 + x)));
    a8_load(v);
    wr8(dba(BAND_LAYERS + 3), v);
    wr8(dba(BAND_LAYERS + 5), v);
    v = rd8(dba((uint16_t)(BAND_SETS + 6 + x)));
    a8_load(v);
    wr8(dba(BAND_LAYERS + 7), v);
}

/*
 * @routine 81:8EBD ModeBandsSetupLong
 * WHAT IT DOES: ModeBandsSetup for a caller in another bank. No code
 * refers to it.
 * HOW: JSR to it, then RTL.
 * Entry: as ModeBandsSetup; called by JSL. Exit: as ModeBandsSetup.
 */
void abi_ModeBandsSetupLong(void)
{
    call_frame(0x8EBF, abi_ModeBandsSetup);
}
