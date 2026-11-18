/*
 * circle_wipe.c: the circle that closes in on a point (the wipe at a level's
 * end), drawn as the window's left and right edge on each scanline.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_CircleWipeStep(void);
void abi_CircleWipePlot(void);
void abi_LineRows(void);

/* The circle, drawn by the midpoint method an octant at a time. */
#define CIRCLE_X 0x00002Cu         /* the octant's x, from the radius down (word) */
#define CIRCLE_Y 0x00002Eu         /* the octant's y, from 0 up (word) */
#define CIRCLE_TERM 0x000030u      /* the midpoint method's term (word) */
#define CIRCLE_VIEW_X 0x001772u    /* subtracted from the point: words in layer 0's record */
#define CIRCLE_VIEW_Y 0x001776u
#define CIRCLE_CX 0x000032u        /* the centre's screen x (word) */
#define CIRCLE_CY 0x000034u        /* the centre's scanline (word) */
/* The plot's working bytes. */
#define SPAN_X 0x00000Cu           /* centre x - x, centre x + x (clamped to the screen) */
#define SPAN_Y 0x00000Eu           /* centre x - y, centre x + y */
#define ROW_UP_X 0x000014u         /* scanline centre - x, then its table offset (word) */
#define ROW_DOWN_X 0x000016u       /* scanline centre + x ($FF off screen), then its offset */
#define ROW_UP_Y 0x000018u         /* scanline centre - y, then its offset */
#define ROW_DOWN_Y 0x00001Au       /* scanline centre + y, then its offset */
/* The window's edges per scanline: left, right (a word each). */
#define WINDOW_EDGES 0x7F5000u

/* a - b, 0 when below; a + b, $FF when past; as bytes. */
static uint8_t less_or_0(uint8_t a, uint8_t b) { return a >= b ? (uint8_t)(a - b) : 0x00; }
static uint8_t less_or_ff(uint8_t a, uint8_t b) { return a >= b ? (uint8_t)(a - b) : 0xFF; }
static uint8_t more_or_ff(uint8_t a, uint8_t b) { return a + b > 0xFF ? 0xFF : (uint8_t)(a + b); }

/*
 * @routine 81:B2D9 CircleWipePlot
 * WHAT IT DOES: Puts one step of the circle into the window table: its
 * eight symmetric points as the left and right edges of four scanlines.
 * HOW: Clamps centre x -/+ x and -/+ y to 0..$FF, and the scanlines centre
 * -/+ x and -/+ y to $FF when past the screen (scanline $FF is outside the
 * picture); doubles each scanline into a word offset and stores the -/+ y
 * span at scanlines centre -/+ x, the -/+ x span at centre -/+ y.
 * Entry: DB a bank whose $0000-$1FFF is work RAM; called by JSR
 * (CircleWipeStep). Exit: M 16-bit, X 16-bit; A = the -/+ x span (word),
 * X = scanline centre + y's offset; N and Z from the last load; C as the
 * last clamp left it.
 */
void abi_CircleWipePlot(void)
{
    uint8_t cx, cy, x, y;
    uint16_t up_x, down_x, up_y, down_y;

    op_sep(0x30);
    cx = rd8(dba(CIRCLE_CX));
    cy = rd8(dba(CIRCLE_CY));
    x = rd8(dba(CIRCLE_X));
    y = rd8(dba(CIRCLE_Y));
    /* the spans, clamped to the screen; the scanlines, $FF off it */
    wr8(dba(SPAN_X), less_or_0(cx, x));
    wr8(dba(SPAN_X + 1), more_or_ff(cx, x));
    wr8(dba(SPAN_Y), less_or_0(cx, y));
    wr8(dba(SPAN_Y + 1), more_or_ff(cx, y));
    up_x = (uint16_t)(less_or_ff(cy, x) << 1);
    down_x = (uint16_t)(more_or_ff(cy, x) << 1);
    up_y = (uint16_t)(less_or_ff(cy, y) << 1);
    down_y = (uint16_t)(more_or_ff(cy, y) << 1);
    wr16(dba(ROW_UP_X), up_x);
    wr16(dba(ROW_DOWN_X), down_x);
    wr16(dba(ROW_UP_Y), up_y);
    wr16(dba(ROW_DOWN_Y), down_y);
    op_rep(0x30);
    wr16(WINDOW_EDGES + up_x, rd16(dba(SPAN_Y)));
    wr16(WINDOW_EDGES + down_x, rd16(dba(SPAN_Y)));
    wr16(WINDOW_EDGES + up_y, rd16(dba(SPAN_X)));
    wr16(WINDOW_EDGES + down_y, rd16(dba(SPAN_X)));
    cpu.x = down_y;
    a16_load(rd16(dba(SPAN_X)));
    flag_set(P_C, 0);
    /* V from the last clamp's add */
    flag_set(P_V, (cy ^ (uint8_t)(cy + y)) & (y ^ (uint8_t)(cy + y)) & 0x80);
}

/*
 * @routine 81:B264 CircleWipeStep
 * WHAT IT DOES: Draws the wipe's circle of radius A round the point X, Y
 * into the window table, when the point is on screen.
 * HOW: The octant's x = 0, y = A and the midpoint method's term ($30) =
 * 3 - 2A; the centre on screen = X and Y less the words at $1772 and $1776
 * (layer 0's record), and nothing is drawn when they are not below 256 and
 * 224; the 256 scanlines' edges are set to $00FF (no window); then
 * CircleWipePlot for each step while x is not past y, the term growing by
 * 4x + 6 (term negative) or by 4(x - y) + 10 with y down one.
 * Entry: M 16-bit, X 16-bit; A = the radius; X, Y = the point; DB a bank
 * whose $0000-$1FFF is work RAM; called by JSL. Exit: M 16-bit, X 16-bit;
 * drawn: A = the octant's y, C clear, X as CircleWipePlot left it; off
 * screen: A = the difference that failed, C set; $2C-$34 the circle's
 * working words.
 */
void abi_CircleWipeStep(void)
{
    uint16_t at, term, step, x, y;

    /* x = 0, y = the radius, the term 3 - 2r */
    wr16(dba(CIRCLE_X), 0);
    wr16(dba(CIRCLE_Y), cpu.a);
    flag_set(P_C, 0);
    wr16(dba(CIRCLE_TERM), (uint16_t)(op_adc16((uint16_t)(cpu.a << 1), 0xFFFD, &cpu.p) ^ 0xFFFF) + 1);
    /* the centre on screen, or nothing drawn */
    flag_set(P_C, 1);
    at = op_sbc16(cpu.x, rd16(dba(CIRCLE_VIEW_X)), &cpu.p);
    cpu.a = at;
    cmp16_of(at, 0x0100);
    if (at >= 0x0100)
        return;
    wr16(dba(CIRCLE_CX), at);
    flag_set(P_C, 1);
    at = op_sbc16(cpu.y, rd16(dba(CIRCLE_VIEW_Y)), &cpu.p);
    cpu.a = at;
    cmp16_of(at, 0x00E0);
    if (at >= 0x00E0)
        return;
    wr16(dba(CIRCLE_CY), at);
    for (at = 0; at < 0x0200; at += 2)
        wr16(WINDOW_EDGES + at, 0x00FF);
    cpu.x = 0x0200;
    /* each step plotted while x is not past y */
    for (;;) {
        y = rd16(dba(CIRCLE_Y));
        x = rd16(dba(CIRCLE_X));
        cpu.a = y;
        cmp16_of(y, x);
        if (y < x)
            return;
        abi_CircleWipePlot();
        x = rd16(dba(CIRCLE_X));
        term = rd16(dba(CIRCLE_TERM));
        /* the term's step (the original's ADCs leave V) */
        flag_set(P_C, 0);
        if (term & 0x8000) {
            step = op_adc16((uint16_t)(x << 2), 0x0006, &cpu.p);
        } else {
            step = op_adc16((uint16_t)((x - rd16(dba(CIRCLE_Y))) << 2), 0x000A, &cpu.p);
            wr16(dba(CIRCLE_Y), (uint16_t)(rd16(dba(CIRCLE_Y)) - 1));
        }
        flag_set(P_C, 0);
        term = op_adc16(step, term, &cpu.p);
        wr16(dba(CIRCLE_TERM), term);
        wr16(dba(CIRCLE_X), (uint16_t)(x + 1));
    }
}

/* LineRows: a line's ends and working words. */
#define LINE_X 0x002Cu             /* the x stepped along, from the first end (word) */
#define LINE_Y 0x002Eu             /* its row, from the first end's (word) */
#define LINE_END_X 0x0030u         /* the second end (word) */
#define LINE_END_Y 0x0032u
#define LINE_DX 0x000Cu            /* end x - x (word) */
#define LINE_DY 0x000Eu            /* end y - y (word) */
#define LINE_KEEP 0x0010u          /* 2 dy: the term's step when y stays (word) */
#define LINE_TERM 0x0012u          /* 2 dy - dx, then stepped (word) */
#define LINE_MOVE 0x0014u          /* 2 dy - 2 dx: the term's step when y moves (word) */
#define LINE_ROWS 0x7F5200u        /* per row (a byte offset), the x stored (word) */

/*
 * @routine 81:B38E LineRows
 * WHAT IT DOES: Steps a line from ($2C, $2E) to ($30, $32) by Bresenham's
 * method, x by one each step, storing each x as a word at $7F:5200 + its
 * y. No code refers to it.
 * HOW: dx and dy from the ends; the term 2 dy - dx; while x is not past
 * the end's x: the word at $7F:5200 + y = x; the term negative: it grows by
 * 2 dy; else y goes on by one and the term grows by 2 dy - 2 dx; x on by
 * one.
 * Entry: any widths; $2C, $2E the first end, $30, $32 the second (words,
 * x the lesser, the line no steeper than 1); DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: M 16-bit, X 16-bit; A = the end's x, C
 * clear; X = the last row stored; $2C = the end's x + 1, $2E the row
 * reached; $0C-$14 the working words.
 */
void abi_LineRows(void)
{
    op_rep(0x30);
    sec();
    lda16(ea_abs(LINE_END_X));
    sbc16_v(rd16(ea_abs(LINE_X)));
    sta16(ea_abs(LINE_DX));
    sec();
    lda16(ea_abs(LINE_END_Y));
    sbc16_v(rd16(ea_abs(LINE_Y)));
    sta16(ea_abs(LINE_DY));
    ACC16(asl);
    sta16(ea_abs(LINE_KEEP));
    sec();
    sbc16_v(rd16(ea_abs(LINE_DX)));
    sta16(ea_abs(LINE_TERM));
    sec();
    sbc16_v(rd16(ea_abs(LINE_DX)));
    sta16(ea_abs(LINE_MOVE));
    for (;;) {
        lda16(ea_abs(LINE_END_X));
        cmp16_v(rd16(ea_abs(LINE_X)));
        if (!IF_C)
            return;  /* rtl */
        ldx16(ea_abs(LINE_Y));
        lda16(ea_abs(LINE_X));
        sta16(ea_longx(LINE_ROWS));
        lda16(ea_abs(LINE_TERM));
        if (IF_N) {
            lda16(ea_abs(LINE_KEEP));
        } else {
            MEM16(inc, ea_abs(LINE_Y));
            lda16(ea_abs(LINE_MOVE));
        }
        clc();
        adc16_v(rd16(ea_abs(LINE_TERM)));
        sta16(ea_abs(LINE_TERM));
        MEM16(inc, ea_abs(LINE_X));
    }
}
