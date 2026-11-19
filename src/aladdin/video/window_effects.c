/*
 * window_effects.c: the windows some screens draw shapes with by HDMA:
 * the task that runs an effect, the effects (a channel started with its
 * table at the table area's base or a place in it, shapes, waves, boxes
 * and spots), the window and colour math shadows set for a frame, a shape
 * narrowed round its middle, a half turn of sines, and a window box round
 * a point in the level.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_HdmaEffect3(void);
void abi_HdmaEffect6(void);
void abi_WindowWavesLoop(void);
void abi_HdmaChannelFree(void);
void abi_HdmaEffect10(void);
void abi_HdmaEffect11(void);
void abi_HdmaEffect12(void);
void abi_HdmaEffect0(void);
void abi_HdmaEffect1(void);
void abi_HdmaEffect5(void);
void abi_HdmaEffect7(void);
void abi_HdmaEffect2(void);
void abi_HdmaEffect4(void);
void abi_HdmaEffect8(void);
void abi_HdmaEffect9(void);
void abi_HdmaStart(void);
void abi_TaskExit(void);
void abi_WindowEffectTask(void);
void abi_HalfSineBuild(void);
void abi_HdmaStartAtBase(void);
void abi_TaskWait(void);
void abi_WindowBoxAt(void);
void abi_WindowColoursWait(void);
void abi_WindowLinesFromShape(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"



#define WINDOW_SHAPE 0xBBAEu    /* (bank $82) the offset of its last pair (word), then left, right pairs */
#define WINDOW_EDGES 0x7EF800u  /* the window's edges per line, left then right */
#define WINDOW_SHIFT 0x0030u    /* WindowLinesFromShape's inputs: added to both edges */
#define WINDOW_HALF 0x0032u     /* the most either side of a line's middle */
#define WINDOW_LINES 0x0033u    /* lines left to write */
#define WINDOW_03A6 0x03A6u     /* the windows' settings, $03A6-$03AE (WH0-WH3 first) */
#define SUB_LAYERS_0399 0x0399u
#define CUE_BD 0x00BDu          /* counted up as effect 8 starts */
#define CUE_C1 0x00C1u          /* set: effect 10's window closes */
#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)
#define WAVES_PHASE 0x06u       /* the waves (direct page): the sine's start (word) */
#define WAVES_STEP 0x08u        /* lines a step of the sine lasts */
#define WAVES_COUNT 0x09u       /* lines left of the step */
#define WAVE_SINES 0x5600u      /* (bank $7F) the sine bytes */
#define WAVES_TABLE 0x5300u     /* (bank $7F) the window table while the waves are built */
#define OPEN_LINES 0x08u        /* effect 6 (direct page): lines open */
#define OPEN_HALF 0x09u         /* effect 6: the half-height */
#define BOX_SLOT 0x06u          /* effect 8 (direct page): its channel's slot in HDMA_TABLE_AT */
#define BOX_TABLE 0x07u         /* effect 8: the table being shown ($F400 or $F410, word) */
#define BOX_X (SCRATCH + 0)     /* WindowBoxAt's point (words) */
#define BOX_Y (SCRATCH + 2)
#define BOX_TABLE_BUILT 0x7F5600u /* the table WindowBoxAt builds */
#define SPOT_CHANNEL 0x06u      /* effect 10 (direct page): its channel's bit */
#define SPOT_ACROSS 0x002Cu     /* effect 10: the spot on the screen (words) */
#define SPOT_DOWN 0x002Eu
#define WRAM_MOVES_END 0x1705u  /* the WRAM moves queued (wram_moves.c): their length */
#define WRAM_MOVES 0x1707u      /* the moves: from, to, count (words) */

/* The table area: HDMA tables are built from $7E:F400 on. */
#define HDMA_TABLES 0x7EF400u
#define HDMA_VALUE_ADD 0x0030u  /* added to each entry's value (word) */
#define HDMA_CHANNEL_BIT 0x002Cu /* the channel's HDMAEN bit */
#define HDMA_CHANNEL_REGS 0x002Du /* the channel's register offset (channel * 16) */
#define HDMA_TABLE_SET 0x002Eu  /* the table set being started */
#define HDMA_TABLE_PLACE 0x0032u /* the table's place from $7E:F400 (word) */
#define HDMA_RESERVED 0x03B5u   /* channels kept by other code (high nibble) */
#define HDMA_TABLE_HIGH 0x0351u /* per channel 4-7, its table address's high byte */
#define CHANNEL_BITS 0xBCA1u    /* (bank $82) per channel: its HDMAEN bit */
#define REGISTER_SETS 0xBCA9u   /* (bank $82) per register set: DMAP, BBAD */
#define TABLE_SETS 0xBCBBu      /* (bank $82) per set: its entries' offset (word); entries: count, word */
#define EFFECTS 0x8017u         /* (bank $82) the effects, by the task's parameter */
#define EFFECT_PARAM 0x06u      /* the task's parameter: the effect (direct page) */
#define EFFECT0_STEP 0x07u      /* effect 0 (direct page): added to the last word each round (word) */
#define EFFECT5_VALUE 0x08u     /* effect 5 (direct page): the word being written (word) */
#define EFFECT5_ROUNDS 0x0Au    /* effect 5: rounds left (word) */

/*
 * @routine 82:871B HdmaChannelFree
 * WHAT IT DOES: Finds a free HDMA channel among 4-7, from 7 down: C clear
 * and X = the channel, or C set when all four are in use.
 * HOW: The high nibble of HDMAEN's shadow ORed with HDMA_RESERVED; all
 * set: C set. Else shifted left until a clear bit, X stepping down from 7.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: X = the channel (or 3 or 4 when none); A shifted.
 */
void abi_HdmaChannelFree(void)
{
    uint8_t used, top;

    x8_load(0x07);
    used = rd8(dba(HDMAEN_SHADOW));
    used = (uint8_t)((used | rd8(dba(HDMA_RESERVED))) & 0xF0);
    a8_load(used);
    cmp8_of(used, 0xF0);
    if (used != 0xF0) {
        for (;;) {
            top = used & 0x80;
            used = (uint8_t)(used << 1);
            a8_load(used);
            if (!top) {
                flag_set(P_C, 0);
                return;
            }
            cpu.x = (uint8_t)(cpu.x - 1);
            cmp8_of((uint8_t)cpu.x, 0x04);
            if ((uint8_t)cpu.x < 0x04)
                break;
        }
    }
    flag_set(P_C, 1);
}

/*
 * @routine 82:873B HdmaStart
 * WHAT IT DOES: Starts HDMA table set X on register set Y in a free
 * channel, its table built at $7E:F400 plus HDMA_TABLE_PLACE: C clear,
 * HDMA_CHANNEL_BIT the channel's HDMAEN bit; C set when no channel is
 * free.
 * HOW: HDMA_VALUE_ADD cleared, HDMA_TABLE_SET = X; HdmaChannelFree. The
 * channel's bit (CHANNEL_BITS) at HDMA_CHANNEL_BIT, its register base
 * (channel * 16) at HDMA_CHANNEL_REGS; the table address ($F400 + the
 * place) into HDMA_TABLE_AT for it; DMAP and BBAD from the register set
 * (REGISTER_SETS), bank $7E. The set's entries (TABLE_SETS: count, then a
 * word) copied to the table, HDMA_VALUE_ADD added to each word, until a
 * count of 0.
 * Entry: M 8-bit, X 8-bit; X = the table set; Y = the register set;
 * HDMA_TABLE_PLACE = the table's place; DB = $82; called by JSR. Exit: M
 * 8-bit, X 8-bit; C as above; A = 0 after a start.
 */
void abi_HdmaStart(void)
{
    uint8_t ch, regs, slot, reg_set, v;
    uint16_t at, to;

    wr8(dba(HDMA_VALUE_ADD), 0);
    wr8(dba(HDMA_VALUE_ADD + 1), 0);
    wr8(dba(HDMA_TABLE_SET), (uint8_t)cpu.x);
    reg_set = (uint8_t)cpu.y;
    call_frame(0x8746, abi_HdmaChannelFree);
    if (cpu.p & P_C)
        return;
    ch = (uint8_t)cpu.x;
    regs = (uint8_t)(ch << 4);
    a8_load(regs);
    flag_set(P_C, 0);                   /* the fourth ASL's: bit 4 of a channel below 8 */
    wr8(dba(HDMA_CHANNEL_REGS), regs);
    a8_load(rd8(dba((uint16_t)(CHANNEL_BITS + ch))));
    wr8(dba(HDMA_CHANNEL_BIT), (uint8_t)cpu.a);
    slot = (uint8_t)((ch - 4) << 1);
    x8_load(slot);
    flag_set(P_C, 0);
    a8_load((uint8_t)op_adc8(rd8(dba(HDMA_TABLE_PLACE)), 0x00, &cpu.p));
    wr8(dba(HDMA_TABLE_PLACE), (uint8_t)cpu.a);
    wr8(dba((uint16_t)(HDMA_TABLE_AT + slot)), (uint8_t)cpu.a);
    a8_load((uint8_t)op_adc8(rd8(dba(HDMA_TABLE_PLACE + 1)), 0xF4, &cpu.p));
    wr8(dba(HDMA_TABLE_PLACE + 1), (uint8_t)cpu.a);
    wr8(dba((uint16_t)(HDMA_TABLE_HIGH + slot)), (uint8_t)cpu.a);
    x8_load(regs);
    a8_load(rd8(dba((uint16_t)(REGISTER_SETS + reg_set))));
    wr8(dba((uint16_t)(DMAP0 + regs)), (uint8_t)cpu.a);
    a8_load(rd8(dba((uint16_t)(REGISTER_SETS + 1 + reg_set))));
    wr8(dba((uint16_t)(BBAD0 + regs)), (uint8_t)cpu.a);
    a8_load(0x7E);
    wr8(dba((uint16_t)(A1B0 + regs)), 0x7E);
    wr8(dba((uint16_t)(DASB0 + regs)), 0x7E);
    x8_load(rd8(dba(HDMA_TABLE_SET)));
    op_rep(0x10);
    cpu.y = rd16(dba((uint16_t)(TABLE_SETS + cpu.x)));
    cpu.x = rd16(dba(HDMA_TABLE_PLACE));
    nz16_of((uint16_t)cpu.x);
    for (at = cpu.y, to = cpu.x;; at += 3, to += 3) {
        uint8_t count = rd8(dba((uint16_t)(TABLE_SETS + at)));

        cpu.x = to;
        cpu.y = at;
        a8_load(count);
        wr8(0x7E0000u + to, count);
        if (!count)
            break;
        flag_set(P_C, 0);
        v = rd8(dba((uint16_t)(TABLE_SETS + 1 + at)));
        a8_load((uint8_t)op_adc8(v, rd8(dba(HDMA_VALUE_ADD)), &cpu.p));
        wr8(0x7E0000u + (uint16_t)(to + 1), (uint8_t)cpu.a);
        v = rd8(dba((uint16_t)(TABLE_SETS + 2 + at)));
        a8_load((uint8_t)op_adc8(v, rd8(dba(HDMA_VALUE_ADD + 1)), &cpu.p));
        wr8(0x7E0000u + (uint16_t)(to + 2), (uint8_t)cpu.a);
    }
    op_sep(0x10);
    flag_set(P_C, 0);
}

/*
 * @routine 82:8735 HdmaStartAtBase
 * WHAT IT DOES: Starts HDMA table set X on register set Y with its table
 * at the base of the table area, $7E:F400.
 * HOW: HDMA_TABLE_PLACE cleared, then on into HdmaStart (it follows).
 * Entry: M 8-bit, X 8-bit; X = the table set; Y = the register set; DB =
 * $82; called by JSR. Exit: as HdmaStart.
 */
void abi_HdmaStartAtBase(void)
{
    wr8(dba(HDMA_TABLE_PLACE), 0);
    wr8(dba(HDMA_TABLE_PLACE + 1), 0);
    abi_HdmaStart();
}

/* The window effects' table entries (by the parameter) that are code. */
static void effect_by_param(uint8_t x)
{
    switch (x) {
    case 0x04: abi_HdmaEffect2(); break;
    case 0x06: abi_HdmaEffect3(); break;
    case 0x08: abi_HdmaEffect4(); break;
    case 0x0C: abi_HdmaEffect6(); break;
    case 0x10: abi_HdmaEffect8(); break;
    case 0x12: abi_HdmaEffect9(); break;
    case 0x14: abi_HdmaEffect10(); break;
    case 0x16: abi_HdmaEffect11(); break;
    case 0x18: abi_HdmaEffect12(); break;
    case 0x00: abi_HdmaEffect0(); break;
    case 0x02: abi_HdmaEffect1(); break;
    case 0x0A: abi_HdmaEffect5(); break;
    case 0x0E: abi_HdmaEffect7(); break;
    default:                            /* past the table: what its words say */
        call_routine(0x820000u | rd16(0x820000u | (uint16_t)(EFFECTS + x)), 0);
        break;
    }
}

/*
 * @routine 82:8000 WindowEffectTask
 * WHAT IT DOES: Runs window effect EFFECT_PARAM / 2 of its slot (its
 * parameter) from the table at $82:8017, retrying each frame while the
 * effect cannot start.
 * HOW: D = the slot's page ($6F + Y); JSR through EFFECTS by the
 * parameter; when it returns (no HDMA channel was free), TaskWait 1 and
 * again.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
 * never returns (the effects end the task).
 */
void abi_WindowEffectTask(void)
{
    op_rep(0x20);
    flag_set(P_C, 0);
    cpu.a = op_adc16((uint16_t)cpu.y, 0x006F, &cpu.p);
    cpu.d = cpu.a;
    op_sep(0x20);
    for (;;) {
        x8_load(rd8(dpa(EFFECT_PARAM)));
        push16(0x800E);                 /* JSR (EFFECTS,X): an effect can wait */
        effect_by_param((uint8_t)cpu.x);
        cpu.s = (uint16_t)(cpu.s + 2);
        a8_load(0x01);
        jsl_frame(0x828011u);
        abi_TaskWait();
    }
}

/* An effect's start: HdmaStart (or HdmaStartAtBase) with X, Y; nonzero
   when no channel was free (the effect then returns to retry). */
static int start(void (*how)(void), uint8_t set, uint8_t regs)
{
    x8_load(set);
    y8_load(regs);
    how();
    return (cpu.p & P_C) != 0;
}

/* The table's place for HdmaStart. */
static void place(uint8_t lo, uint8_t hi)
{
    if (lo) {
        a8_load(lo);
        wr8(dba(HDMA_TABLE_PLACE), lo);
    } else
        wr8(dba(HDMA_TABLE_PLACE), 0);
    if (hi) {
        a8_load(hi);
        wr8(dba(HDMA_TABLE_PLACE + 1), hi);
    } else
        wr8(dba(HDMA_TABLE_PLACE + 1), 0);
}

/* The started channel on in HDMAEN's shadow. */
static void channel_on(void)
{
    a8_load(rd8(dba(HDMA_CHANNEL_BIT)));
    tsb_at(dba(HDMAEN_SHADOW), (uint8_t)cpu.a);
}

/* DB = $82 by PHA, PLB with A = $82. */
static void db82(void)
{
    a8_load(0x82);
    op_push8(0x82);
    cpu.db = op_pull8();
    nz8_of(cpu.db);
}

/*
 * @routine 82:80E2 HdmaEffect2
 * WHAT IT DOES: Window effect 2: HDMA table set 4 on register set 6 from
 * the table area's base, then the task ends.
 * HOW: HdmaStartAtBase; no channel: back (to retry); else the channel on
 * in HDMAEN's shadow and TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect2(void)
{
    if (start(abi_HdmaStartAtBase, 0x04, 0x06))
        return;
    channel_on();
    abi_TaskExit();
}

/* The window's edges for the waves: the shape's pairs (WINDOW_SHAPE, from
   its last) copied to the window table. */
static void waves_edges_from_shape(void)
{
    uint16_t x;

    op_rep(0x10);
    db82();
    x = rd16(dba(WINDOW_SHAPE));
    cpu.x = x;
    nz16_of(x);
    do {
        a8_load(rd8(dba((uint16_t)(WINDOW_SHAPE + x))));
        wr8(WINDOW_EDGES + x, (uint8_t)cpu.a);
        a8_load(rd8(dba((uint16_t)(WINDOW_SHAPE + 1 + x))));
        wr8(WINDOW_EDGES + 1 + x, (uint8_t)cpu.a);
        x = (uint16_t)(x - 2);
        cpu.x = x;
        nz16_of(x);
    } while (x);
}

/*
 * @routine 82:80F6 HdmaEffect3
 * WHAT IT DOES: Window effect 3: HDMA table set $0A on register set 6 at
 * place $30, the window's edges taken from the shape, then the waves
 * (WindowWavesLoop) for good.
 * HOW: HDMA_TABLE_PLACE $0030; HdmaStart; no channel: back (C set).
 * HalfSineBuild; X 16-bit, DB $82; WINDOW_SHAPE's pairs, from its last,
 * to WINDOW_EDGES; the channel on; on into WindowWavesLoop.
 * Entry: M 8-bit, X 8-bit; DB = $82; D the task's page; called by JSR
 * from WindowEffectTask. Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect3(void)
{
    place(0x30, 0x00);
    if (start(abi_HdmaStart, 0x0A, 0x06))
        return;
    abi_HalfSineBuild();
    waves_edges_from_shape();
    channel_on();
    abi_WindowWavesLoop();
}

/*
 * @routine 82:812C WindowWavesLoop
 * WHAT IT DOES: The waves the wave effects end in, for good: each frame
 * the window's lines are the shape's edges moved by a sine that runs down
 * the screen and on in time.
 * HOW: The $200-byte window table moved from $7E:F800 to $7F:5300
 * (MVN, DB kept). Phase (direct page $06, word) 0, step $08 1. Each
 * frame: WindowColoursWait; count $09 = step; the phase up by one; Y =
 * the phase, X = WINDOW_SHAPE's last offset, DB $7F; for each pair from
 * the last: the sine byte at $5600+Y added to the shape's left and right
 * into $7F:5300; from offset $60 down Y down by one each pair; when the
 * count runs out the step up by one (from offset $60 down, $FF instead),
 * the count again; then the table moved back to $7E:F800.
 * Entry: M 8-bit, X 16-bit; D the task's page; reached by JMP from
 * effect 6, and from effect 3 running on into it. Exit: never returns.
 */
void abi_WindowWavesLoop(void)
{
    uint16_t x;
    uint8_t v, sine;

    phb();
    cpu.a = 0x01FF;
    cpu.x = 0xF800;
    cpu.y = 0x5300;
    mvn(0x7F, 0x7E);                    /* to $7F:5300 from $7E:F800 */
    plb();
    cpu.y = 0x0000;
    nz16_of(0);
    dp16_set(WAVES_PHASE, 0x0000);
    a8_load(0x01);
    wr8(dpa(WAVES_STEP), 0x01);
    for (;;) {
        push16(0x8147);                 /* JSR WindowColoursWait (it waits) */
        abi_WindowColoursWait();
        cpu.s = (uint16_t)(cpu.s + 2);
        a8_load(rd8(dpa(WAVES_STEP)));
        wr8(dpa(WAVES_COUNT), (uint8_t)cpu.a);
        (void)inc8_at(dpa(WAVES_PHASE));
        cpu.y = dp16(WAVES_PHASE);
        nz16_of((uint16_t)cpu.y);
        x = rd16(dba(WINDOW_SHAPE));
        cpu.x = x;
        nz16_of(x);
        phb();
        a8_load(0x7F);
        op_push8(0x7F);
        cpu.db = op_pull8();
        nz8_of(cpu.db);
        for (;;) {
            flag_set(P_C, 0);
            sine = rd8(dba((uint16_t)(WAVE_SINES + cpu.y)));
            a8_load((uint8_t)op_adc8(sine, rd8(0x820000u + (uint16_t)(WINDOW_SHAPE + x)), &cpu.p));
            wr8(dba((uint16_t)(WAVES_TABLE + x)), (uint8_t)cpu.a);
            flag_set(P_C, 0);
            sine = rd8(dba((uint16_t)(WAVE_SINES + cpu.y)));
            a8_load((uint8_t)op_adc8(sine, rd8(0x820000u + (uint16_t)(WINDOW_SHAPE + 1 + x)), &cpu.p));
            wr8(dba((uint16_t)(WAVES_TABLE + 1 + x)), (uint8_t)cpu.a);
            cmp16_of(x, 0x0060);
            if (x >= 0x0060) {
                /* TYA, DEC, TAY with A 8-bit, X 16-bit: Y = B:A, A's low byte one down */
                v = (uint8_t)((uint8_t)cpu.y - 1);
                a8_load(v);
                cpu.y = cpu.a;
                nz16_of((uint16_t)cpu.y);
            }
            x = (uint16_t)(x - 2);
            cpu.x = x;
            nz16_of(x);
            if (!x)
                break;
            if (dec8_at(dpa(WAVES_COUNT)))
                continue;
            (void)inc8_at(dpa(WAVES_STEP));
            cmp16_of(x, 0x0060);
            if (x >= 0x0060)
                break;
            a8_load(0xFF);
            wr8(dpa(WAVES_STEP), 0xFF);
            break;
        }
        cpu.a = 0x01FF;
        cpu.x = 0x5300;
        cpu.y = 0xF800;
        mvn(0x7E, 0x7F);                /* to $7E:F800 from $7F:5300 */
        plb();
    }
}

/*
 * @routine 82:82DB HdmaEffect4
 * WHAT IT DOES: Window effect 4: HDMA table set 6 on register set 8 with
 * its table $20 into the area, the value at $7E:F480 7, and the task left
 * waiting for good.
 * HOW: HDMA_TABLE_PLACE $0020; HdmaStart; no channel: back. $7E:F480 =
 * 7, $7E:F481 = 0; the channel on; TaskWait 2 forever.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect4(void)
{
    place(0x20, 0x00);
    if (start(abi_HdmaStart, 0x06, 0x08))
        return;
    a8_load(0x07);
    wr8(HDMA_TABLES + 0x80, 0x07);
    a8_load(0x00);
    wr8(HDMA_TABLES + 0x81, 0x00);
    channel_on();
    for (;;) {
        a8_load(0x02);
        jsl_frame(0x828301u);
        abi_TaskWait();
    }
}

/*
 * @routine 82:8031 HdmaEffect0
 * WHAT IT DOES: Window effect 0: HDMA table set 0 on register set 2 from
 * the table area's base, its $40 bytes at $7E:F500 cleared, then every 5
 * frames the table's words moved down one and the last one's count
 * stepped by one, the step's direction turned when the count reaches 0
 * or $20.
 * HOW: HdmaStartAtBase; no channel: back. X 16-bit; $7E:F500-F53F 0;
 * the channel on; the step (direct page $07, word) $0100. Each round:
 * TaskWait 5; 16-bit: the words at $7E:F504 on moved to $7E:F500 (30 of
 * them); the word after them ($7E:F53E) plus the step, AND $1F00: nonzero
 * is stored there, zero turns the step (its negative).
 * Entry: M 8-bit, X 8-bit; DB = $82; D the task's page; called by JSR
 * from WindowEffectTask (no TaskCreate names it). Exit: no channel: C
 * set; else never returns.
 */
void abi_HdmaEffect0(void)
{
    uint16_t x, y, v, step;

    if (start(abi_HdmaStartAtBase, 0x00, 0x02))
        return;
    op_rep(0x10);
    x = 0x0000;
    y = 0x0040;
    do {
        a8_load(0x00);
        wr8(HDMA_TABLES + 0x100 + x, 0x00);
        x = (uint16_t)(x + 1);
        y = (uint16_t)(y - 1);
    } while (y);
    cpu.x = x;
    cpu.y = y;
    nz16_of(y);
    channel_on();
    a8_load(0x01);
    wr8(dpa(EFFECT0_STEP + 1), 0x01);
    wr8(dpa(EFFECT0_STEP), 0x00);
    for (;;) {
        op_sep(0x30);
        a8_load(0x05);
        jsl_frame(0x82805Du);
        abi_TaskWait();
        op_rep(0x30);
        x = 0x0000;
        y = 0x001E;
        do {
            a16_load(rd16(HDMA_TABLES + 0x104 + x));
            wr16(HDMA_TABLES + 0x100 + x, (uint16_t)cpu.a);
            x = (uint16_t)(x + 2);
            y = (uint16_t)(y - 1);
        } while (y);
        cpu.x = x;
        cpu.y = y;
        nz16_of(y);
        flag_set(P_C, 0);
        v = rd16(HDMA_TABLES + 0x102 + x);
        step = dp16(EFFECT0_STEP);
        v = op_adc16(v, step, &cpu.p);
        a16_load((uint16_t)(v & 0x1F00));
        if (cpu.a) {
            wr16(HDMA_TABLES + 0x102 + x, (uint16_t)cpu.a);
            continue;
        }
        a16_load(dp16(EFFECT0_STEP));
        a16_load((uint16_t)(cpu.a ^ 0xFFFF));
        a16_load((uint16_t)(cpu.a + 1));
        dp16_set(EFFECT0_STEP, (uint16_t)cpu.a);
    }
}

/*
 * @routine 82:8092 HdmaEffect1
 * WHAT IT DOES: Window effect 1: HDMA table set 2 on register set 4 from
 * the table area's base, $80 words at $7E:F500 from $0080 down to 1, then
 * every 2 frames each word that is not yet 0 taken down by one.
 * HOW: HdmaStartAtBase; no channel: back. 16-bit: $7E:F600 = 0; the
 * words $0080, $007F ... $0001 at $7E:F500 on; 8-bit; the channel on.
 * Each round: TaskWait 2; 16-bit, each of the $80 words, nonzero, one
 * less.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask
 * (no TaskCreate names it). Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect1(void)
{
    uint16_t x, y;

    if (start(abi_HdmaStartAtBase, 0x02, 0x04))
        return;
    op_rep(0x30);
    a16_load(0x0000);
    wr16(HDMA_TABLES + 0x200, 0x0000);
    a16_load(0x0080);
    x = 0x0000;
    y = 0x0080;
    do {
        wr16(HDMA_TABLES + 0x100 + x, (uint16_t)cpu.a);
        a16_load((uint16_t)(cpu.a - 1));
        x = (uint16_t)(x + 2);
        y = (uint16_t)(y - 1);
    } while (y);
    cpu.x = x;
    cpu.y = y;
    nz16_of(y);
    op_sep(0x30);
    channel_on();
    for (;;) {
        op_sep(0x30);
        a8_load(0x02);
        jsl_frame(0x8280C4u);
        abi_TaskWait();
        op_rep(0x30);
        x = 0x0000;
        y = 0x0080;
        do {
            a16_load(rd16(HDMA_TABLES + 0x100 + x));
            if (cpu.a) {
                a16_load((uint16_t)(cpu.a - 1));
                wr16(HDMA_TABLES + 0x100 + x, (uint16_t)cpu.a);
            }
            x = (uint16_t)(x + 2);
            y = (uint16_t)(y - 1);
        } while (y);
        cpu.x = x;
        cpu.y = y;
        nz16_of(y);
    }
}

/*
 * @routine 82:8307 HdmaEffect7
 * WHAT IT DOES: Window effect 7: HDMA table set $0C on register set 8
 * with its table $20 into the area, the values at $7E:F480 1 and 7, and
 * the task left waiting for good.
 * HOW: HDMA_TABLE_PLACE $0020; HdmaStart; no channel: back. $7E:F480 =
 * 1, $7E:F481 = 7; the channel on; TaskWait 2 forever.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask
 * (its TaskCreate is at $80:A8BB). Exit: no channel: C set; else never
 * returns.
 */
void abi_HdmaEffect7(void)
{
    place(0x20, 0x00);
    if (start(abi_HdmaStart, 0x0C, 0x08))
        return;
    a8_load(0x01);
    wr8(HDMA_TABLES + 0x80, 0x01);
    a8_load(0x07);
    wr8(HDMA_TABLES + 0x81, 0x07);
    channel_on();
    for (;;) {
        a8_load(0x02);
        jsl_frame(0x82832Du);
        abi_TaskWait();
    }
}

/*
 * @routine 82:8333 HdmaEffect5
 * WHAT IT DOES: Window effect 5: HDMA table set 8 on register set $0A
 * with its table $10 into the area; then $80 rounds, 2 frames each, of
 * the words at $7E:F700: in round n (from $80 down to 1) n words rising
 * from $0101 - n by one, held at $0100; then the task waits for good.
 * HOW: HDMA_TABLE_PLACE $0010; HdmaStart; no channel: back. The channel
 * on; 16-bit; rounds (direct page $0A, word) $80. Each round: Y = the
 * rounds; the value ($08, word) $0101 less the rounds; Y words of it to
 * $7E:F700 on, each one more than the last until $0100; TaskWait 2
 * (A 16-bit); the rounds down. Then TaskWait 1 forever.
 * Entry: M 8-bit, X 8-bit; DB = $82; D the task's page; called by JSR
 * from WindowEffectTask (no TaskCreate names it). Exit: no channel: C
 * set; else never returns.
 */
void abi_HdmaEffect5(void)
{
    uint16_t x, y;

    place(0x10, 0x00);
    if (start(abi_HdmaStart, 0x08, 0x0A))
        return;
    channel_on();
    op_rep(0x30);
    a16_load(0x0080);
    dp16_set(EFFECT5_ROUNDS, 0x0080);
    do {
        cpu.x = 0x0000;
        nz16_of(0x0000);
        x = 0x0000;
        y = dp16(EFFECT5_ROUNDS);
        cpu.y = y;
        nz16_of(y);
        flag_set(P_C, 1);
        a16_load(op_sbc16(0x0101, dp16(EFFECT5_ROUNDS), &cpu.p));
        dp16_set(EFFECT5_VALUE, (uint16_t)cpu.a);
        do {
            a16_load(dp16(EFFECT5_VALUE));
            wr16(HDMA_TABLES + 0x300 + x, (uint16_t)cpu.a);
            x = (uint16_t)(x + 2);
            a16_load(dp16(EFFECT5_VALUE));
            cmp16_of((uint16_t)cpu.a, 0x0100);
            if (cpu.a != 0x0100)
                a16_load((uint16_t)(cpu.a + 1));
            dp16_set(EFFECT5_VALUE, (uint16_t)cpu.a);
            y = (uint16_t)(y - 1);
            cpu.y = y;
            nz16_of(y);
        } while (y);
        cpu.x = x;
        a16_load(0x0002);
        jsl_frame(0x828377u);
        abi_TaskWait();
    } while (dec16_at(dpa(EFFECT5_ROUNDS)));
    for (;;) {
        a16_load(0x0001);
        jsl_frame(0x828382u);
        abi_TaskWait();
    }
}

/*
 * @routine 82:819A HdmaEffect6
 * WHAT IT DOES: Window effect 6: a circle that opens from the middle of
 * the screen (half its height a step, 3 lines a frame), then grows past
 * the shape line by line, then the waves (WindowWavesLoop) for good.
 * HOW: HDMA_TABLE_PLACE $0030; HdmaStart with set $0A on register set 6;
 * no channel: back. $03A6 and $03A7 0; HalfSineBuild; X 16-bit; the
 * window table at $7E:F7FE filled with $FF, 0 pairs ($200 bytes); DB
 * $82; the channel on. Lines (direct page $08) 0, half-height ($09) 4.
 * Each frame, lines + 3 while under $80: WindowLinesFromShape with
 * window_lines the lines and window_half the half-height, X $0100, shift
 * 0; WindowColoursWait. Then per shape pair from X $0100 up to
 * WINDOW_SHAPE's last: window_lines the lines, the half-height up by one
 * (to $80 at most) and halved into window_half, shift 0;
 * WindowLinesFromShape and WindowColoursWait (X kept); lines up by one.
 * Then WindowWavesLoop (JMP).
 * Entry: M 8-bit, X 8-bit; DB = $82; D the task's page; called by JSR
 * from WindowEffectTask. Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect6(void)
{
    uint16_t x;
    uint8_t v;

    place(0x30, 0x00);
    if (start(abi_HdmaStart, 0x0A, 0x06))
        return;
    wr8(dba(WINDOW_03A6), 0);
    wr8(dba(WINDOW_03A6 + 1), 0);
    abi_HalfSineBuild();
    op_rep(0x10);
    x = 0x01FE;
    do {
        a8_load(0xFF);
        wr8(WINDOW_EDGES - 2 + x, 0xFF);
        a8_load(0x00);
        wr8(WINDOW_EDGES - 1 + x, 0x00);
        x = (uint16_t)(x - 2);
        cpu.x = x;
        nz16_of(x);
    } while (!(x & 0x8000));
    db82();
    channel_on();
    wr8(dpa(OPEN_LINES), 0);
    a8_load(0x04);
    wr8(dpa(OPEN_HALF), 0x04);
    /* opening from the middle, 3 lines a frame */
    for (;;) {
        cpu.x = 0x0100;
        nz16_of(0x0100);
        flag_set(P_C, 0);
        v = op_adc8(rd8(dpa(OPEN_LINES)), 0x03, &cpu.p);
        a8_load(v);
        wr8(dpa(OPEN_LINES), v);
        cmp8_of(v, 0x80);
        if (v >= 0x80)
            break;
        wr8(dba(WINDOW_LINES), v);
        a8_load(rd8(dpa(OPEN_HALF)));
        wr8(dba(WINDOW_HALF), (uint8_t)cpu.a);
        wr8(dba(WINDOW_SHIFT), 0);
        abi_WindowLinesFromShape();
        push16(0x81F8);                 /* JSR WindowColoursWait (it waits) */
        abi_WindowColoursWait();
        cpu.s = (uint16_t)(cpu.s + 2);
    }
    /* then past the shape, a pair a frame */
    do {
        a8_load(rd8(dpa(OPEN_LINES)));
        wr8(dba(WINDOW_LINES), (uint8_t)cpu.a);
        v = (uint8_t)(rd8(dpa(OPEN_HALF)) + 1);
        a8_load(v);
        cmp8_of(v, 0x80);
        if (v >= 0x80)
            a8_load(0x80);
        wr8(dpa(OPEN_HALF), (uint8_t)cpu.a);
        flag_set(P_C, cpu.a & 0x01);    /* the LSR's */
        a8_load((uint8_t)((uint8_t)cpu.a >> 1));
        wr8(dba(WINDOW_HALF), (uint8_t)cpu.a);
        wr8(dba(WINDOW_SHIFT), 0);
        push16((uint16_t)cpu.x);        /* PHX */
        abi_WindowLinesFromShape();
        push16(0x8218);                 /* JSR WindowColoursWait (it waits) */
        abi_WindowColoursWait();
        cpu.s = (uint16_t)(cpu.s + 2);
        (void)inc8_at(dpa(OPEN_LINES));
        cpu.x = pull16();               /* PLX */
        cpu.x = (uint16_t)(cpu.x + 2);
        cmp16_of((uint16_t)cpu.x, rd16(dba(WINDOW_SHAPE)));
    } while (!(cpu.p & P_C));
    abi_WindowWavesLoop();
}

/*
 * @routine 82:8388 HdmaEffect8
 * WHAT IT DOES: Window effect 8: windows round two places in the level
 * ($0E40, $00F8 and $0D60, $01D8), rebuilt each frame and swapped between
 * two tables.
 * HOW: HdmaStartAtBase with set $0E on register set $0E; $00BD up; the
 * channel on; its table address slot ((the channel's registers / 16 - 4)
 * * 2) kept at direct page $06 and the table at $07 ($F400). Each frame:
 * TaskWait 1; the table address set; WindowBoxAt for each place; an entry
 * of 1 line of no window and the end; WH0's shadow from the first entry;
 * the 16 bytes moved from $7F:5600 to the table, and the table swapped
 * ($10 apart).
 * Entry: M 8-bit, X 8-bit; DB = $82; D the task's page; called by JSR
 * from WindowEffectTask. Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect8(void)
{
    uint8_t slot, x;

    if (start(abi_HdmaStartAtBase, 0x0E, 0x0E))
        return;
    (void)inc8_at(dba(CUE_BD));
    channel_on();
    flag_set(P_C, 0);
    slot = op_adc8((uint8_t)(rd8(dba(HDMA_CHANNEL_REGS)) >> 4), 0xFC, &cpu.p);
    a8_load((uint8_t)(slot << 1));
    flag_set(P_C, slot & 0x80);
    slot = (uint8_t)(slot << 1);
    wr8(dpa(BOX_SLOT), slot);
    x8_load(slot);
    op_rep(0x20);
    a16_load(0xF400);
    dp16_set(BOX_TABLE, 0xF400);
    for (;;) {
        op_sep(0x30);
        a8_load(0x01);
        jsl_frame(0x8283B4u);
        abi_TaskWait();
        op_rep(0x20);
        x8_load(rd8(dpa(BOX_SLOT)));
        a16_load(dp16(BOX_TABLE));
        wr16(dba((uint16_t)(HDMA_TABLE_AT + cpu.x)), cpu.a);
        x8_load(0x00);
        a16_load(0x0E40);
        wr16(dba(BOX_X), 0x0E40);
        a16_load(0x00F8);
        wr16(dba(BOX_Y), 0x00F8);
        abi_WindowBoxAt();
        a16_load(0x0D60);
        wr16(dba(BOX_X), 0x0D60);
        a16_load(0x01D8);
        wr16(dba(BOX_Y), 0x01D8);
        abi_WindowBoxAt();
        x = (uint8_t)cpu.x;
        a16_load(0x0001);
        wr16(BOX_TABLE_BUILT + x, 0x0001);
        a16_load(0x00FF);
        wr16(BOX_TABLE_BUILT + 1 + x, 0x00FF);
        a16_load(0x0000);
        wr16(BOX_TABLE_BUILT + 3 + x, 0x0000);
        a16_load(rd16(BOX_TABLE_BUILT + 1));
        wr16(dba(WINDOW_03A6), cpu.a);
        op_rep(0x10);
        phb();
        cpu.x = 0x5600;
        cpu.y = dp16(BOX_TABLE);
        nz16_of((uint16_t)cpu.y);
        a16_load(0x000F);
        mvn(0x7E, 0x7F);                /* to $7E:table from $7F:5600 */
        plb();
        a16_load((uint16_t)(dp16(BOX_TABLE) ^ 0x0010));
        dp16_set(BOX_TABLE, cpu.a);
    }
}

/*
 * @routine 82:867B HdmaEffect9
 * WHAT IT DOES: Window effect 9: a window below the line $0200 in the
 * level less the scroll (at most 240 lines down), kept each frame.
 * HOW: HdmaStartAtBase with set $16 on register set $10; the table's
 * count at $7E:F500 $10, then 0; the channel on. Each frame: TaskWait 1;
 * $0200 less the scroll y: past the screen or 0 lines, the count 0; else
 * half of it (at most 240) as the first two counts ($7E:F400, F403) and
 * $10 at $7E:F500.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect9(void)
{
    uint16_t lines;

    if (start(abi_HdmaStartAtBase, 0x16, 0x10))
        return;
    a8_load(0x10);
    wr8(HDMA_TABLES + 0x100, 0x10);
    a8_load(0x00);
    wr8(HDMA_TABLES + 0x101, 0x00);
    channel_on();
    for (;;) {
        a8_load(0x01);
        jsl_frame(0x828699u);
        abi_TaskWait();
        op_rep(0x20);
        flag_set(P_C, 1);
        lines = op_sbc16(0x0200, rd16(dba((uint16_t)SCROLL_Y)), &cpu.p);
        a16_load(lines);
        if (cpu.p & P_C) {
            cmp16_of(lines, 0x00F0);
            if (lines >= 0x00F0)
                lines = 0x00F0;
            flag_set(P_C, lines & 0x01);
            lines = (uint16_t)(lines >> 1);
            a16_load(lines);
            if (lines) {
                op_sep(0x20);
                wr8(HDMA_TABLES, (uint8_t)lines);
                wr8(HDMA_TABLES + 3, (uint8_t)lines);
                a8_load(0x10);
                wr8(HDMA_TABLES + 0x100, 0x10);
                continue;
            }
        }
        op_sep(0x20);
        a8_load(0x00);
        wr8(HDMA_TABLES + 0x100, 0x00);
    }
}

/* Effect 10's colour math off: no fixed colour, the window's left at
   $FF then $00 (as the original writes both to WH2's shadow), CGWSEL 2,
   CGADSUB $53. */
static void spot_colours_off(void)
{
    wr8(dba(COLDATA_RED), 0);
    wr8(dba(COLDATA_GREEN), 0);
    wr8(dba(COLDATA_BLUE), 0);
    a8_load(0x02);
    wr8(dba(CGWSEL_SHADOW), 0x02);
    a8_load(0x53);
    wr8(dba(CGADSUB_SHADOW), 0x53);
}

/*
 * @routine 82:84CB HdmaEffect10
 * WHAT IT DOES: Window effect 10: a window and colour math round the
 * place $0E78, $0144 in the level while it is on the screen; when $00C1
 * is set, the window closes in from both sides and the task ends.
 * HOW: HdmaStart with set $10 on register set $0C at place $100; the
 * channel on (its bit kept at direct page $06); $7E:F510 $FF, $F511 0;
 * the masks cleared, WOBJSEL $C0; the table's 32 bytes at $7E:F500 kept
 * at $F520. Each frame: the place less the scroll and the layer's shift:
 * on the screen, the window's left and right (right + $10, clamped to
 * 255) and its first line (half the y, rounded up) into the kept table
 * and WH2/WH3's shadows, the fixed colour (8, 8, 16) added (CGWSEL $20,
 * CGADSUB $33), a WRAM move queued to restore the table; off, the table's
 * count 0, the colour math off. With $00C1 set, WH2 up and WH3 down a
 * step a frame (into $7E:F512/F513) until they cross, then the colour
 * math off, the channel off and TaskExit.
 * Entry: M 8-bit, X 8-bit; D the task's page; DB = $82; called by JSR
 * from WindowEffectTask. Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect10(void)
{
    uint16_t down, across;
    uint8_t q, right;

    place(0x00, 0x01);
    if (start(abi_HdmaStart, 0x10, 0x0C))
        return;
    channel_on();
    wr8(dpa(SPOT_CHANNEL), (uint8_t)cpu.a);
    a8_load(0xFF);
    wr8(HDMA_TABLES + 0x110, 0xFF);
    a8_load(0x00);
    wr8(HDMA_TABLES + 0x111, 0x00);
    wr8(dba(TMW_SHADOW), 0);
    wr8(dba(TSW_SHADOW), 0);
    wr8(dba(WINDOW_03A6 + 4), 0);
    wr8(dba(WINDOW_03A6 + 5), 0);
    a8_load(0xC0);
    wr8(dba(WOBJSEL_SHADOW), 0xC0);
    phb();
    op_rep(0x10);
    cpu.x = 0x001F;
    a8_load(0x1F);                      /* TXA: A's low byte the count */
    cpu.x = 0xF500;
    cpu.y = 0xF520;
    mvn(0x7E, 0x7E);                    /* the table kept: $7E:F500 to $7E:F520 */
    op_sep(0x10);
    plb();
    for (;;) {
        op_rep(0x20);
        flag_set(P_C, 1);
        down = op_sbc16(0x0144, rd16(dba((uint16_t)SCROLL_Y)), &cpu.p);
        flag_set(P_C, 1);
        down = op_sbc16(down, rd16(dba((uint16_t)LAYER_OFFSET_Y)), &cpu.p);
        a16_load(down);
        cmp16_of(down, 0x00E0);
        if (down < 0x00E0) {
            wr16(dba(SPOT_DOWN), down);
            flag_set(P_C, 1);
            across = op_sbc16(0x0E78, rd16(dba((uint16_t)SCROLL_X)), &cpu.p);
            flag_set(P_C, 1);
            across = op_sbc16(across, rd16(dba((uint16_t)BG1_X_SHIFT)), &cpu.p);
            a16_load(across);
            cmp16_of(across, 0x0100);
            if (across < 0x0100) {
                wr16(dba(SPOT_ACROSS), across);
                op_sep(0x20);
                flag_set(P_C, 0);
                right = op_adc8((uint8_t)across, 0x10, &cpu.p);
                a8_load(right);
                if (cpu.p & P_C)
                    a8_load(0xFF);
                wr8(HDMA_TABLES + 0x133, (uint8_t)cpu.a);
                wr8(dba(WINDOW_03A6 + 3), (uint8_t)cpu.a);
                a8_load(rd8(dba(SPOT_ACROSS)));
                wr8(HDMA_TABLES + 0x132, (uint8_t)cpu.a);
                wr8(dba(WINDOW_03A6 + 2), (uint8_t)cpu.a);
                q = rd8(dba(SPOT_DOWN));
                flag_set(P_C, q & 0x01);
                a8_load((uint8_t)op_adc8((uint8_t)(q >> 1), 0x00, &cpu.p));
                wr8(HDMA_TABLES + 0x120, (uint8_t)cpu.a);
                wr8(HDMA_TABLES + 0x123, (uint8_t)cpu.a);
                a8_load(0x08);
                wr8(dba(COLDATA_RED), 0x08);
                wr8(dba(COLDATA_GREEN), 0x08);
                flag_set(P_C, 0);
                a8_load(0x10);
                wr8(dba(COLDATA_BLUE), 0x10);
                a8_load(0x20);
                wr8(dba(CGWSEL_SHADOW), 0x20);
                a8_load(0x33);
                wr8(dba(CGADSUB_SHADOW), 0x33);
                /* a WRAM move queued: the kept table back over the live one */
                op_rep(0x20);
                y8_load(rd8(dba(WRAM_MOVES_END)));
                a16_load(0xF520);
                wr16(dba((uint16_t)(WRAM_MOVES + cpu.y)), 0xF520);
                a16_load(0xF500);
                wr16(dba((uint16_t)(WRAM_MOVES + 2 + cpu.y)), 0xF500);
                a16_load(0x001F);
                wr16(dba((uint16_t)(WRAM_MOVES + 4 + cpu.y)), 0x001F);
                op_sep(0x20);
                flag_set(P_C, 0);
                a8_load((uint8_t)op_adc8((uint8_t)cpu.y, 0x06, &cpu.p));
                wr8(dba(WRAM_MOVES_END), (uint8_t)cpu.a);
                a8_load(rd8(dba(CUE_C1)));
                if ((uint8_t)cpu.a)
                    break;
                goto next;
            }
        }
        /* off the screen */
        op_sep(0x20);
        a8_load(0x00);
        wr8(HDMA_TABLES + 0x100, 0x00);
        wr8(dba(COLDATA_RED), 0);
        wr8(dba(COLDATA_GREEN), 0);
        wr8(dba(COLDATA_BLUE), 0);
        a8_load(0xFF);
        wr8(dba(WINDOW_03A6 + 2), 0xFF);
        a8_load(0x00);
        wr8(dba(WINDOW_03A6 + 2), 0x00);
        a8_load(0x02);
        wr8(dba(CGWSEL_SHADOW), 0x02);
        a8_load(0x53);
        wr8(dba(CGADSUB_SHADOW), 0x53);
next:
        a8_load(0x01);
        jsl_frame(0x8285C7u);
        abi_TaskWait();
    }
    /* closing in from both sides */
    do {
        a8_load(0x01);
        jsl_frame(0x8285D0u);
        abi_TaskWait();
        a8_load((uint8_t)(rd8(dba(WINDOW_03A6 + 2)) + 1));
        wr8(dba(WINDOW_03A6 + 2), (uint8_t)cpu.a);
        wr8(HDMA_TABLES + 0x112, (uint8_t)cpu.a);
        a8_load((uint8_t)(rd8(dba(WINDOW_03A6 + 3)) - 1));
        wr8(dba(WINDOW_03A6 + 3), (uint8_t)cpu.a);
        wr8(HDMA_TABLES + 0x113, (uint8_t)cpu.a);
        cmp8_of((uint8_t)cpu.a, rd8(dba(WINDOW_03A6 + 2)));
    } while (cpu.p & P_C);
    spot_colours_off();
    a8_load(rd8(dpa(SPOT_CHANNEL)));
    trb_at(dba(HDMAEN_SHADOW), (uint8_t)cpu.a);
    abi_TaskExit();
}

/*
 * @routine 82:860B HdmaEffect11
 * WHAT IT DOES: Window effect 11: a window that narrows over the lines
 * above $00F8 in the level, a step a frame, then the task ends.
 * HOW: HdmaStart with set $12 on register set $0C at place $80; the
 * channel on; WH2/WH3's shadows and the table's window $00FF/$FF00; the
 * line count ($00F8 less the scroll y, + $30) at $7E:F483 and F486. Each
 * frame: TaskWait 1, the left edge ($7E:F480) up, the right edge
 * ($7E:F492) up and its complement at F493, the count down; at 0
 * TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect11(void)
{
    uint16_t d;
    uint8_t v;

    place(0x80, 0x00);
    if (start(abi_HdmaStart, 0x12, 0x0C))
        return;
    channel_on();
    op_rep(0x20);
    a16_load(0x00FF);
    wr16(HDMA_TABLES + 0x90, 0x00FF);
    wr16(dba(WINDOW_03A6 + 2), 0x00FF);
    a16_load(0xFF00);
    wr16(HDMA_TABLES + 0x92, 0xFF00);
    flag_set(P_C, 1);
    d = op_sbc16(0x00F8, rd16(dba((uint16_t)SCROLL_Y)), &cpu.p);
    a16_load(d);
    op_sep(0x20);
    flag_set(P_C, 0);
    a8_load(op_adc8((uint8_t)d, 0x30, &cpu.p));
    wr8(HDMA_TABLES + 0x83, (uint8_t)cpu.a);
    wr8(HDMA_TABLES + 0x86, (uint8_t)cpu.a);
    do {
        a8_load(0x01);
        jsl_frame(0x82864Cu);
        abi_TaskWait();
        a8_load((uint8_t)(rd8(HDMA_TABLES + 0x80) + 1));
        wr8(HDMA_TABLES + 0x80, (uint8_t)cpu.a);
        a8_load((uint8_t)(rd8(HDMA_TABLES + 0x92) + 1));
        wr8(HDMA_TABLES + 0x92, (uint8_t)cpu.a);
        a8_load((uint8_t)(cpu.a ^ 0xFF));
        wr8(HDMA_TABLES + 0x93, (uint8_t)cpu.a);
        v = (uint8_t)(rd8(HDMA_TABLES + 0x83) - 1);
        a8_load(v);
        wr8(HDMA_TABLES + 0x83, v);
        wr8(HDMA_TABLES + 0x86, v);
    } while (v);
    abi_TaskExit();
}

/*
 * @routine 82:870D HdmaTableFill
 * WHAT IT DOES: Fills Y words of the HDMA tables at $7E:F400 with A, from
 * X on (16-bit).
 * HOW: PHP; 16-bit A, X and Y; A stored at band_hdma,X and X up two, Y
 * times; PLP. Only the Genie screen's task (80:A63B, its JSL at 80:A707)
 * calls it, and no script plays that screen's load: unit_hdma_table_fill.box
 * calls it at its own entry.
 * Entry: A = the word, X = the first byte's offset, Y = the count (16-bit,
 * as its caller leaves them); called by JSL. Exit: P as at entry; X past
 * the last word; Y 0.
 */
void abi_HdmaTableFill(void)
{
    php();
    op_rep(0x30);
    do {
        sta16(ea_longx(0x7EF400u));             /* band_hdma */
        inx16();
        inx16();
        dey16();
    } while (!IF_Z);
    plp();
}

/*
 * @routine 82:86CF HdmaEffect12
 * WHAT IT DOES: Window effect 12: HDMA table set $14 on register set 0
 * from the base, with the layers' window masks and the windows set, then
 * the task ends.
 * HOW: HdmaStartAtBase; the table's first values from $0399 (and its low
 * nibble); the channel on; TMW and TSW $10, WH0 $38, WH1 $C7, WH2 and
 * WH3's shadows 0, WOBJSEL 3; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect12(void)
{
    uint8_t v;

    if (start(abi_HdmaStartAtBase, 0x14, 0x00))
        return;
    v = rd8(dba(SUB_LAYERS_0399));
    a8_load(v);
    wr8(HDMA_TABLES + 0x80, v);
    a8_load((uint8_t)(v & 0x0F));
    wr8(HDMA_TABLES + 0x81, (uint8_t)cpu.a);
    channel_on();
    a8_load(0x10);
    wr8(dba(TMW_SHADOW), 0x10);
    wr8(dba(TSW_SHADOW), 0x10);
    a8_load(0x38);
    wr8(dba(WINDOW_03A6), 0x38);
    a8_load(0xC7);
    wr8(dba(WINDOW_03A6 + 1), 0xC7);
    wr8(dba(WINDOW_03A6 + 4), 0);
    wr8(dba(WINDOW_03A6 + 5), 0);
    a8_load(0x03);
    wr8(dba(WOBJSEL_SHADOW), 0x03);
    abi_TaskExit();
}

/*
 * @routine 82:8226 WindowColoursWait
 * WHAT IT DOES: Sets the window and colour math shadows the window effects
 * use (BG1 and BG2 masked on the main screen, the fixed colour added to
 * the backdrop), then waits a frame.
 * HOW: TMW 3, TSW 0, WOBJSEL $30, CGWSEL $20, CGADSUB $20 and the fixed
 * colour (red 6, green 3, blue 22) into their shadows; TaskWait 1.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR
 * from a task. Exit: as TaskWait, when the task resumes.
 */
void abi_WindowColoursWait(void)
{
    wr8(dba(TMW_SHADOW), 0x03);
    wr8(dba(TSW_SHADOW), 0x00);
    wr8(dba(WOBJSEL_SHADOW), 0x30);
    wr8(dba(CGWSEL_SHADOW), 0x20);
    wr8(dba(CGADSUB_SHADOW), 0x20);
    wr8(dba(COLDATA_RED), 0x06);
    wr8(dba(COLDATA_GREEN), 0x03);
    wr8(dba(COLDATA_BLUE), 0x16);
    a8_load(0x01);
    jsl_frame(0x82824Eu);
    abi_TaskWait();
}

/*
 * @routine 82:8253 WindowLinesFromShape
 * WHAT IT DOES: Writes the window's left and right edges for up to $0033
 * lines, from the bottom up: the shape in the table at $82:BBAE narrowed
 * to at most $0032 pixels either side of each line's middle, moved right
 * by $0030.
 * HOW: From the shape's last pair (the table's first word is its offset)
 * and the edges at X, back two bytes a line: the middle of the shape's
 * left and right; the left edge the larger of the shape's left and the
 * middle less the half width, the right edge the smaller of its right and
 * the middle plus it, each plus the shift, to $7E:F800 + X and + X + 1.
 * Ends when X goes negative, the shape's offset reaches 0 or the line
 * count runs out.
 * Entry: X 16-bit; X = the offset of the bottom line's edges; DB = $82;
 * called by JSR. Exit: M 8-bit; $002C, $002D = the last shape pair used,
 * $002E its middle; X, Y past it.
 */
void abi_WindowLinesFromShape(void)
{
    uint16_t y = rd16(dba(0xBBAE)), x = cpu.x;
    uint8_t left, right, mid, a;

    for (;;) {
        op_sep(P_M);
        /* the shape's edges and their middle; each edge pulled in toward
           the middle by $32, not past the shape, then moved by $30 */
        left = rd8(dba(0xBBAE) + y);
        wr8(dba(0x002C), left);
        right = rd8(dba(0xBBAF) + y);
        wr8(dba(0x002D), right);
        flag_set(P_C, 1);
        mid = op_sbc8(right, left, &cpu.p);
        flag_set(P_C, 0);
        mid = op_adc8((uint8_t)(mid >> 1), left, &cpu.p);
        wr8(dba(0x002E), mid);
        flag_set(P_C, 1);
        a = op_sbc8(mid, rd8(dba(0x0032)), &cpu.p);
        if (a < left)
            a = left;
        flag_set(P_C, 0);
        wr8(BAND_POS + x, op_adc8(a, rd8(dba(0x0030)), &cpu.p));
        flag_set(P_C, 0);
        a = op_adc8(mid, rd8(dba(0x0032)), &cpu.p);
        if (a >= right)
            a = right;
        flag_set(P_C, 0);
        a = op_adc8(a, rd8(dba(0x0030)), &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
        wr8(0x7EF801u + x, a);
        x -= 2;
        nz16_of(x);
        if (x & 0x8000)
            break;
        y -= 2;
        nz16_of(y);
        if (!y)
            break;
        if (!dec8_at(dba(0x0033)))
            break;
    }
    cpu.x = x;
    cpu.y = y;
    op_sep(P_M);
}

/*
 * @routine 82:82AB HalfSineBuild
 * WHAT IT DOES: Builds at $7F:5600 a half turn of sines as bytes (64
 * rising, then the same falling) and at $7F:5680 their complements.
 * HOW: With DB = $82, for each of 64 entries the high byte of the next
 * sine word at $82:87D3 goes to entry x and to entry 63 - x of the second
 * quarter ($7F:5640), and EORed with $FF to the same entries $80 on.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: DB = $82; X = $40; Y =
 * $80; C set, Z set.
 */
void abi_HalfSineBuild(void)
{
    uint8_t x, v;

    /* each quarter's 64 steps, forward and back, and their complements */
    cpu.db = 0x82;
    for (x = 0; x < 0x40; x++) {
        v = rd8(0x8287D4u + (uint8_t)(x << 1));
        wr8(0x7F5600u + x, v);
        wr8(0x7F5680u + x, (uint8_t)~v);
        wr8(0x7F5640u + (x ^ 0x3F), v);
        wr8(0x7F56C0u + (x ^ 0x3F), (uint8_t)~v);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)~v);
    }
    cpu.x = 0x40;
    cpu.y = 0x80;
    cmp8_of(0x40, 0x40);
}

/* The point less the camera (`at`, kept in `keep`) on screen: below
   `size`, or within `margin` before it. */
static int on_screen(uint16_t at, uint16_t keep, uint16_t size, uint16_t margin)
{
    uint16_t v;

    wr16(dba(keep), at);
    cmp16_of(at, size);
    if (at < size)
        return 1;
    flag_set(P_C, 0);
    v = op_adc16(at, margin, &cpu.p);
    cpu.a = v;
    cmp16_of(v, size);
    return v < size;
}

/*
 * @routine 82:8415 WindowBoxOff
 * WHAT IT DOES: WindowBoxAt's answer for a box off the screen: C set,
 * nothing added.
 * HOW: SEC, RTS. It lies just before WindowBoxAt, whose two off-screen
 * tests branch back to it (82:842D, 82:8445); no call reaches it, so the
 * script unit_window_box_off.box calls it at its own entry for A5.
 * Entry: reached by WindowBoxAt's branches (M 16-bit, X 8-bit). Exit: C
 * set; every other register and flag unchanged.
 */
void abi_WindowBoxOff(void)
{
    flag_set(P_C, 1);
}

/*
 * @routine 82:8417 WindowBoxAt
 * WHAT IT DOES: Adds to the HDMA table being built at $7F:5600 the lines
 * of a window 128 pixels wide and up to 96 lines tall whose top left is
 * the level point in $000C, $000E; C set, nothing added, when the box is
 * off the screen.
 * HOW: The point less the play field's scroll ($002C across, $002E down);
 * off when across is outside -128 to 255 or down outside -96 to 223.
 * Entries of 3 bytes from X, a line count then the window's left and
 * right: lines above the box (down above 0) as entries of no window (left
 * $FF, right 0), two when more than 127; then the box's lines (96, fewer
 * at the screen's bottom, or down + 97 when it starts above the screen)
 * with the window from across to across + 128, clipped to 0 and 255.
 * Entry: M 16-bit, X 8-bit; X = the next entry's offset; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: added: C clear, X past
 * the entries; off: C set (WindowBoxOff, 82:8415).
 */
void abi_WindowBoxAt(void)
{
    uint8_t x = (uint8_t)cpu.x;
    uint16_t at, v;

    flag_set(P_C, 1);
    at = rd16(dba(SCRATCH));
    at = op_sbc16(at, rd16(dba(0x1773)), &cpu.p);
    cpu.a = at;
    if (!on_screen(at, 0x002C, 0x0100, 0x0080))
        goto off;
    flag_set(P_C, 1);
    at = rd16(dba(0x000E));
    at = op_sbc16(at, rd16(dba(0x1777)), &cpu.p);
    cpu.a = at;
    if (!on_screen(at, 0x002E, 0x00E0, 0x0060))
        goto off;
    /* the lines above the box (up to 2 entries of at most $7F), then the
       box's height (to the screen's foot, at most $60) */
    at = rd16(dba(0x002E));
    if ((at & 0x8000) || !at) {
        flag_set(P_C, 0);
        v = op_adc16(at, 0x0061, &cpu.p);
    } else {
        if (at >= 0x0080) {
            flag_set(P_C, 1);
            wr16(0x7F5600u + x, op_sbc16(at, 0x007F, &cpu.p));
            wr16(0x7F5601u + x, 0x00FF);
            x = (uint8_t)(x + 3);
            at = 0x007F;
        }
        wr16(0x7F5600u + x, at);
        wr16(0x7F5601u + x, 0x00FF);
        x = (uint8_t)(x + 3);
        flag_set(P_C, 1);
        v = op_sbc16(0x00E0, rd16(dba(0x002E)), &cpu.p);
        cmp16_of(v, 0x0060);
        if (v >= 0x0060)
            v = 0x0060;
    }
    wr16(0x7F5600u + x, v);
    /* the window's left and right edges */
    at = rd16(dba(0x002C));
    if (at & 0x8000) {
        flag_set(P_C, 0);
        v = (uint16_t)((op_adc16(at, 0x0080, &cpu.p) & 0x00FF) << 8);
    } else {
        flag_set(P_C, 1);
        v = op_sbc16(0x0100, at, &cpu.p);
        cmp16_of(v, 0x0081);
        if (v >= 0x0081) {
            flag_set(P_C, 0);
            v = (uint16_t)(op_adc8((uint8_t)at, 0x80, &cpu.p) << 8 | (uint8_t)at);
        } else {
            v = (uint16_t)(at | 0xFF00);
        }
    }
    cpu.a = v;
    wr16(0x7F5601u + x, v);
    x = (uint8_t)(x + 3);
    cpu.x = x;
    nz8_of(x);
    flag_set(P_C, 0);
    return;
off:
    flag_set(P_C, 1);
}











#define CLAMP_BY 0x002Cu        /* AddClampFF's and SubClamp0's operand */

/*
 * @routine 82:87C1 AddClampFF
 * WHAT IT DOES: A plus the byte at $002C, held to $FF.
 * HOW: CLC, ADC $002C; a carry: A = $FF. No code calls it (no JSR, JSL, JMP or table word reaches it):
 * unit_dead_82.box calls it at its own entry.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: A the sum or $FF; C clear, or set and A $FF.
 */
void abi_AddClampFF(void)
{
    clc();
    adc8_v(rd8(ea_abs(CLAMP_BY)));
    if (!IF_C)
        return;
    lda8_v(0xFF);
}

/*
 * @routine 82:87CA SubClamp0
 * WHAT IT DOES: A less the byte at $002C, held to 0.
 * HOW: SEC, SBC $002C; a borrow: A = 0. No code calls it (no JSR, JSL, JMP or table word reaches it):
 * unit_dead_82.box calls it at its own entry.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: A the difference or 0; C set, or clear and A 0.
 */
void abi_SubClamp0(void)
{
    sec();
    sbc8_v(rd8(ea_abs(CLAMP_BY)));
    if (IF_C)
        return;
    lda8_v(0x00);
}
