/*
 * window_effects.c: the windows some screens draw shapes with by HDMA:
 * a channel started with its table at the table area's base, the window
 * and colour math shadows set for a frame, a shape narrowed round its
 * middle, a half turn of sines, and a window box round a point in the
 * level.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_HdmaChannelFree(void);
void abi_HdmaEffect10(void);
void abi_HdmaEffect11(void);
void abi_HdmaEffect12(void);
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
 * the entries; off: C set (at $82:8415).
 */
void abi_WindowBoxAt(void)
{
    uint8_t x = (uint8_t)cpu.x;
    uint16_t at, v;

    flag_set(P_C, 1);
    at = op_sbc16(rd16(dba(SCRATCH)), rd16(dba(0x1773)), &cpu.p);
    cpu.a = at;
    if (!on_screen(at, 0x002C, 0x0100, 0x0080))
        goto off;
    flag_set(P_C, 1);
    at = op_sbc16(rd16(dba(0x000E)), rd16(dba(0x1777)), &cpu.p);
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










