/*
 * lines.c: the per-line effects: each channel that is on (g.line_effects_on)
 * has its table, and the line data it points into, turned into one value a
 * line, and those into the engine's per-line fields (nshvc_lines). This is
 * where the original's HDMA reached the PPU: here the game's own values go
 * to the engine's fields.
 */
#include <stdio.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/level_maps.h"
#include "video/video_internal.h"
#include "video/view.h"

#define LINES 224

/*
 * A window's edges in the picture's columns: the original's 256 columns sit at view.dx (the engine's origin moves
 * the layers and the lines, not the windows). A left edge at the screen's left edge (0) and a right edge at its
 * right edge (255) go on to the picture's, so the columns beside the original's screen behave as its edge columns
 * (a window that covers a line to its end covers the frame beside it; an empty one stays empty).
 */
int32_t video_window_left(int32_t left)
{
    return left == 0 ? 0 : left + view.dx;
}

int32_t video_window_right(int32_t right)
{
    return right == VIEW_ORIGINAL_W - 1 ? view.w - 1 : right + view.dx;
}

/* bytes a line each target takes from its data */
static const uint8_t line_bytes[LINE_TARGETS] = {
    [LINE_TM] = 1, [LINE_TS] = 1, [LINE_PALETTE] = 4, [LINE_BG1_Y] = 2, [LINE_BG2_X] = 2, [LINE_BG3_Y] = 2,
    [LINE_WINDOW1] = 2, [LINE_WINDOW2] = 2, [LINE_MODE] = 1, [LINE_M7_A] = 2, [LINE_FIXED] = 1,
    [LINE_WINDOW_USE_BG34] = 1,
};

static uint8_t data_at(uint32_t at)
{
    return at < LINE_DATA_SIZE ? g.line_data[at] : 0;
}

#define LINE_DATA_AT 0xF400u        /* where g.line_data starts in the original's bank $7E */
#define TEXT_BAND_VALUES_AT 0x1C9Eu /* where g.text_band_values starts in the original's work RAM */

/* a table's byte: the cartridge's (a level's own table) or the line data's */
static uint8_t table_byte(const LineChannel *c, uint32_t at)
{
    return c->cart.at ? line_table_byte(c->cart, at) : data_at(at);
}

/* a byte an indirect entry's pointer names (the original's address) */
static uint8_t pointed_byte(uint32_t address)
{
    if (address >= LINE_DATA_AT)
        return data_at(address - LINE_DATA_AT);
    if (address >= TEXT_BAND_VALUES_AT && address < TEXT_BAND_VALUES_AT + sizeof g.text_band_values)
        return g.text_band_values[address - TEXT_BAND_VALUES_AT];
    return 0;
}

/* One channel's value on each line from `top` (the bytes it took, low
   first, packed in 32 bits), read from its table as the original's HDMA
   reads it; `set[line]` marks the lines it wrote or held. */
static void channel_values(const LineChannel *c, uint32_t top, uint32_t value[LINES], uint8_t set[LINES])
{
    uint32_t line = top, k = line_bytes[c->target], at = c->cart.at ? 0 : c->table_at, held = 0;
    int any = 0;

    while (line < LINES) {
        uint8_t count = table_byte(c, at++);
        uint32_t lines, from;
        int repeat;

        if (!count)
            break;
        lines = (count & 0x7F) ? (count & 0x7Fu) : 0x80u;
        repeat = count >> 7;
        if (c->direct) {
            from = at;
            at += repeat ? k * lines : k;
        } else {
            from = (uint32_t)(table_byte(c, at) | table_byte(c, at + 1) << 8);
            at += 2;
        }
        for (uint32_t i = 0; i < lines && line < LINES; i++, line++) {
            if (i == 0 || repeat) {
                held = 0;
                for (uint32_t b = 0; b < k; b++)
                    held |= (uint32_t)(c->direct ? table_byte(c, from + b) : pointed_byte(from + b)) << (8 * b);
                if (repeat)
                    from += k;
                any = 1;
            }
            value[line] = held;
            set[line] = 1;
        }
    }
    /* past the table's end the register keeps its last value */
    for (; any && line < LINES; line++) {
        value[line] = held;
        set[line] = 1;
    }
}

/* the lines a channel set, from the first it set, to one engine field */
static void send(nshvc_line_field field, const int32_t *values, const uint8_t set[LINES])
{
    uint32_t first = 0;

    while (first < LINES && !set[first])
        first++;
    if (first < LINES)
        nshvc_lines(first, LINES - first, field, values + first);
}

static void send_channel(const LineChannel *c, uint32_t top)
{
    static uint32_t value[LINES];
    static int32_t field[LINES];
    uint8_t set[LINES];

    memset(set, 0, sizeof set);
    channel_values(c, top, value, set);
    switch (c->target) {
    case LINE_TM:
    case LINE_TS:
        for (int layer = 0; layer < 5; layer++) {
            for (int i = 0; i < LINES; i++)
                field[i] = (int32_t)(value[i] >> layer & 1);
            if (layer < 4)
                send((nshvc_line_field)((c->target == LINE_TM ? NSHVC_LINE_MAIN : NSHVC_LINE_SUB) + layer), field, set);
            else
                send(c->target == LINE_TM ? NSHVC_LINE_SPRITES_MAIN : NSHVC_LINE_SPRITES_SUB, field, set);
        }
        break;
    case LINE_MODE:
        for (int i = 0; i < LINES; i++)
            field[i] = (int32_t)(value[i] & 7);
        send(NSHVC_LINE_MODE, field, set);
        for (int i = 0; i < LINES; i++)
            field[i] = (int32_t)(value[i] >> 3 & 1);
        send(NSHVC_LINE_BG3_PRIORITY, field, set);
        for (int layer = 0; layer < 4; layer++) {
            for (int i = 0; i < LINES; i++)
                field[i] = value[i] >> (4 + layer) & 1 ? 16 : 8;
            send((nshvc_line_field)(NSHVC_LINE_TILE_SIZE + layer), field, set);
        }
        break;
    case LINE_WINDOW1:
    case LINE_WINDOW2:
        /* the original's columns, in the picture's: the engine's origin does not move windows (video_window_left) */
        for (int i = 0; i < LINES; i++)
            field[i] = video_window_left((int32_t)(value[i] & 0xFF));
        send(c->target == LINE_WINDOW1 ? NSHVC_LINE_WIN1_LEFT : NSHVC_LINE_WIN2_LEFT, field, set);
        for (int i = 0; i < LINES; i++)
            field[i] = video_window_right((int32_t)(value[i] >> 8 & 0xFF));
        send(c->target == LINE_WINDOW1 ? NSHVC_LINE_WIN1_RIGHT : NSHVC_LINE_WIN2_RIGHT, field, set);
        break;
    case LINE_BG1_Y:
    case LINE_BG2_X:
    case LINE_BG3_Y:
        /* 10 bits as the hardware's scroll; a layer shown from a level's whole-level map (video/level_maps.c) is
           longer than that, so its scroll is a signed 16-bit position */
        for (int i = 0; i < LINES; i++) {
            int layer = c->target == LINE_BG2_X ? 1 : 0;
            int32_t mx, my;

            level_maps_margin(layer, &mx, &my);     /* the level's (0, 0) is inside the map's margins */
            field[i] = level_maps_active(layer) && c->target != LINE_BG3_Y
                           ? (int32_t)(int16_t)value[i] + (c->target == LINE_BG2_X ? mx : my)
                           : (int32_t)(value[i] & 0x3FF);
        }
        if (c->target == LINE_BG2_X && video_level_view())
            for (int i = 0; i < LINES; i++)
                field[i] -= view.dx;    /* the original's screen is dx across in a level's view (video_draw) */
        send(c->target == LINE_BG1_Y ? NSHVC_LINE_SCROLL_Y + 0 : c->target == LINE_BG2_X ? NSHVC_LINE_SCROLL_X + 1
                                                                                         : NSHVC_LINE_SCROLL_Y + 2,
             field, set);
        break;
    case LINE_FIXED: {
        /* each write sets the channels its bits 5-7 name (red, green, blue)
           to its low 5 bits; the others keep theirs, from the frame's */
        uint8_t r = g.fixed_red, gr = g.fixed_green, b = g.fixed_blue;

        for (int i = 0; i < LINES; i++) {
            if (set[i]) {
                uint8_t v = (uint8_t)value[i];

                if (v & 0x20)
                    r = v & 0x1F;
                if (v & 0x40)
                    gr = v & 0x1F;
                if (v & 0x80)
                    b = v & 0x1F;
            }
            field[i] = (int32_t)(r | gr << 5 | b << 10);
        }
        send(NSHVC_LINE_FIXED, field, set);
        break;
    }
    case LINE_WINDOW_USE_BG34:
        /* a byte a line: BG3's window use in its low nibble, BG4's in its high */
        for (int layer = 2; layer < 4; layer++) {
            for (int i = 0; i < LINES; i++)
                field[i] = video_window_use((uint8_t)(value[i] >> (4 * (layer - 2)) & 0xF), g.window_logic[layer]);
            send((nshvc_line_field)(NSHVC_LINE_WIN_USE + layer), field, set);
        }
        break;
    case LINE_M7_A:
        for (int i = 0; i < LINES; i++)
            field[i] = (int16_t)value[i];
        send(NSHVC_LINE_M7_A, field, set);
        break;
    default: {
        static int told[LINE_TARGETS];

        if (!told[c->target]) {
            told[c->target] = 1;
            fprintf(stderr, "unconverted: per-line effect target %u\n", c->target);
        }
        break;
    }
    }
}

void video_lines_send(uint32_t top)
{
    for (int ch = 1; ch < 8; ch++)
        if (g.line_effects_on >> ch & 1)
            send_channel(&g.line_channel[ch], top);
}

int video_lines_tile_mode(void)
{
    for (int ch = 1; ch < 8; ch++) {
        const LineChannel *c = &g.line_channel[ch];
        uint32_t value[LINES];
        uint8_t set[LINES];

        if (!(g.line_effects_on >> ch & 1) || c->target != LINE_MODE)
            continue;
        memset(set, 0, sizeof set);
        channel_values(c, 0, value, set);
        for (int i = 0; i < LINES; i++)
            if (set[i] && (value[i] & 7) != 7)
                return (int)(value[i] & 7);
    }
    return 1;
}
