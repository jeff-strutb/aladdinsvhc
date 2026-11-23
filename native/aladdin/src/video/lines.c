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
#include "video/between.h"
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

/* the state the tables are read from: g, or the newest update's while a picture eases towards it (between.h) */
static const GameState *src = &g;

static uint8_t data_at(uint32_t at)
{
    return at < LINE_DATA_SIZE ? src->line_data[at] : 0;
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
        return src->text_band_values[address - TEXT_BAND_VALUES_AT];
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

/* A line's value eased towards the newest update's (between.h): the lanes of a target that are places (a layer's
   scroll, 16 bits; a window's edges, a byte each; Mode 7's A, 16 bits), each the shorter way round and only within
   `far` (a jump is shown where it was). Other targets are not eased: they are switches, modes and colours. */
static uint32_t line_ease(uint8_t target, uint32_t was, uint32_t now)
{
    switch (target) {
    case LINE_BG1_Y:
    case LINE_BG2_X:
    case LINE_BG3_Y:
        return (uint16_t)(was + (uint32_t)between_ease(was & 0xFFFF, now & 0xFFFF, 0, 16, 64));
    case LINE_M7_A:
        return (uint16_t)(was + (uint32_t)between_ease(was & 0xFFFF, now & 0xFFFF, 0, 16, 0x2000));
    case LINE_WINDOW1:
    case LINE_WINDOW2: {
        uint32_t l = (uint8_t)(was + (uint32_t)between_ease(was & 0xFF, now & 0xFF, 0, 8, 64));
        uint32_t r = (uint8_t)((was >> 8) + (uint32_t)between_ease(was >> 8 & 0xFF, now >> 8 & 0xFF, 0, 8, 64));

        return l | r << 8;
    }
    default:
        return was;
    }
}

/* the fraction of a layer's per-line scroll eased between updates (engine 6.10): 256ths; 0 where not eased */
static int32_t line_sub[LINES];

static uint8_t *window_far;         /* the developer's (video_lines_windows_sample): the lines whose step is too far to ease */

/* A window channel's edges eased with the windows' own steps (between.h): from their step before the newest to the
   newest, on the lines the channel sets in the picture's world and in both, when it is the same effect in both. 0
   (nothing changed) when the windows' states do not both have it. Where the newest step came before the easing to
   the one before had ended, a line eases from where the pictures had got to */
static int window_track_values(const LineChannel *c, uint32_t top, uint32_t value[LINES], const uint8_t set[LINES])
{
    static uint32_t from[LINES], to[LINES], before[LINES];
    uint8_t set_from[LINES], set_to[LINES], set_before[LINES];
    const GameState *p, *w, *n;
    float pf, at, keep;
    int ch = (int)(c - g.line_channel);
    const LineChannel *cp, *cw, *cn;

    if (!between_windows(&p, &pf, &w, &n, &at))
        return 0;
    cw = &w->line_channel[ch];
    cn = &n->line_channel[ch];
    if (!(w->line_effects_on >> ch & 1) || !(n->line_effects_on >> ch & 1) || cw->target != c->target ||
        cn->target != c->target || cw->direct != cn->direct || !cw->cart.at != !cn->cart.at)
        return 0;
    memset(set_from, 0, sizeof set_from);
    memset(set_to, 0, sizeof set_to);
    src = w;
    channel_values(cw, top, from, set_from);
    src = n;
    channel_values(cn, top, to, set_to);
    cp = &p->line_channel[ch];
    if (pf < 1.0f && (p->line_effects_on >> ch & 1) && cp->target == c->target && cp->direct == cw->direct &&
        !cp->cart.at == !cw->cart.at) {
        memset(set_before, 0, sizeof set_before);
        src = p;
        channel_values(cp, top, before, set_before);
        keep = between_amount_set(pf);
        for (int i = 0; i < LINES; i++)
            if (set_before[i] && set_from[i])
                from[i] = line_ease(c->target, before[i], from[i]);
        between_amount_set(keep);
    }
    src = &g;
    keep = between_amount_set(at);
    for (int i = 0; i < LINES; i++)
        if (set[i] && set_from[i] && set_to[i]) {
            /* at its step's end the newest exactly, a step too far to ease too */
            value[i] = at >= 1.0f ? to[i] : line_ease(c->target, from[i], to[i]);
            if (window_far && at < 1.0f) {
                int dl = (int)(to[i] & 0xFF) - (int)(from[i] & 0xFF), dr = (int)(to[i] >> 8 & 0xFF) - (int)(from[i] >> 8 & 0xFF);

                window_far[i] = (uint8_t)(dl > 64 || dl < -64 || dr > 64 || dr < -64);
            }
        }
    between_amount_set(keep);
    return 1;
}

/* the frame's own window edges (g.window_edges) as a picture shows them: eased with the windows' steps */
void video_window_edges(uint8_t e[4])
{
    const GameState *p, *w, *n;
    float pf, at, keep;
    uint8_t from[4];

    if (!between_windows(&p, &pf, &w, &n, &at)) {
        memcpy(e, g.window_edges, 4);
        return;
    }
    keep = between_amount_set(pf);  /* where the pictures had got to when the newest step came */
    for (int k = 0; k < 4; k++)
        from[k] = pf >= 1.0f ? w->window_edges[k]
                             : (uint8_t)(p->window_edges[k] + between_ease(p->window_edges[k], w->window_edges[k], 0, 8, 64));
    between_amount_set(at);
    for (int k = 0; k < 4; k++)     /* at the step's end, the newest exactly */
        e[k] = at >= 1.0f ? n->window_edges[k] : (uint8_t)(from[k] + between_ease(from[k], n->window_edges[k], 0, 8, 64));
    between_amount_set(keep);
}

static void send_channel(const LineChannel *c, uint32_t top)
{
    static uint32_t value[LINES], value_now[LINES];
    static int32_t field[LINES];
    uint8_t set[LINES], set_now[LINES];
    int scroll = c->target == LINE_BG1_Y || c->target == LINE_BG2_X || c->target == LINE_BG3_Y;

    memset(set, 0, sizeof set);
    memset(line_sub, 0, sizeof line_sub);
    channel_values(c, top, value, set);
    if (between_easing() && (c->target == LINE_WINDOW1 || c->target == LINE_WINDOW2) && window_track_values(c, top, value, set)) {
        /* the windows: eased with their own steps */
    } else if (between_easing()) {
        /* the same channel at the newest update, read from its state, when it is the same effect there */
        const GameState *n = between_now();
        const LineChannel *cn = &n->line_channel[c - g.line_channel];

        if ((n->line_effects_on >> (c - g.line_channel) & 1) && cn->target == c->target && cn->direct == c->direct &&
            !cn->cart.at == !c->cart.at) {
            memset(set_now, 0, sizeof set_now);
            src = n;
            channel_values(cn, top, value_now, set_now);
            src = &g;
            for (int i = 0; i < LINES; i++)
                if (set[i] && set_now[i]) {
                    if (scroll && between_fractions()) {
                        /* a whole scroll at each update, eased in 256ths: its whole pixels and its fraction */
                        int64_t fx = between_ease_fx((int64_t)(value[i] & 0xFFFF) << 8, (int64_t)(value_now[i] & 0xFFFF) << 8, 24, 64 << 8);

                        line_sub[i] = (int32_t)(fx & 0xFF);
                        value[i] = (uint32_t)(fx >> 8) & 0xFFFF;
                    } else {
                        value[i] = line_ease(c->target, value[i], value_now[i]);
                    }
                }
        }
    }
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
        if (between_fractions())        /* the eased lines' fractions, and 0 on the lines not eased (engine 6.10) */
            send(c->target == LINE_BG1_Y ? NSHVC_LINE_SCROLL_Y_SUB + 0 : c->target == LINE_BG2_X ? NSHVC_LINE_SCROLL_X_SUB + 1
                                                                                             : NSHVC_LINE_SCROLL_Y_SUB + 2,
                 line_sub, set);
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

/* the developer's (between.c, ALADDIN_DEV_STEPS): a hash of the window edges each line shows, from the channels that
   set them, and of the frame's own window edges */
uint32_t video_lines_windows_hash(void)
{
    static uint32_t value[LINES];
    uint8_t set[LINES];
    uint32_t h = 2166136261u;

    for (int k = 0; k < 4; k++)
        h = (h ^ g.window_edges[k]) * 16777619u;
    for (int ch = 1; ch < 8; ch++) {
        const LineChannel *c = &g.line_channel[ch];

        if (!(g.line_effects_on >> ch & 1) || (c->target != LINE_WINDOW1 && c->target != LINE_WINDOW2))
            continue;
        memset(set, 0, sizeof set);
        channel_values(c, 0, value, set);
        for (int i = 0; i < LINES; i++)
            h = (h ^ (set[i] ? value[i] : 0xFFFFFFFFu)) * 16777619u;
    }
    return h;
}

/* the developer's (between.c, ALADDIN_DEV_STEP_CHECK): the window edges a picture shows, eased: the frame's four in
   value[0..3], then each window channel's lines (up to two channels, LINES each; 0xFFFFFFFF on a line it does not
   set), and in far[] the lines whose step was too far to ease (a line of window appearing or going) */
void video_lines_windows_sample(uint32_t value[4 + 2 * LINES], uint8_t far[4 + 2 * LINES])
{
    static uint32_t v[LINES];
    uint8_t set[LINES], e[4];
    int k = 0;
    const GameState *p, *w, *n;
    float pf, at;

    memset(far, 0, 4 + 2 * LINES);
    video_window_edges(e);
    for (int j = 0; j < 4; j++)
        value[j] = e[j];
    if (between_windows(&p, &pf, &w, &n, &at) && at < 1.0f)
        for (int j = 0; j < 4; j++) {
            int d = (uint8_t)(n->window_edges[j] - w->window_edges[j]);  /* the shorter way round, as between_ease */

            d = d >= 128 ? d - 256 : d;
            far[j] = (uint8_t)(d > 64 || d < -64);
        }
    for (int j = 4; j < 4 + 2 * LINES; j++)
        value[j] = 0xFFFFFFFFu;
    for (int ch = 1; ch < 8 && k < 2; ch++) {
        const LineChannel *c = &g.line_channel[ch];

        if (!(g.line_effects_on >> ch & 1) || (c->target != LINE_WINDOW1 && c->target != LINE_WINDOW2))
            continue;
        memset(set, 0, sizeof set);
        channel_values(c, 0, v, set);
        window_far = far + 4 + k * LINES;
        window_track_values(c, 0, v, set);
        window_far = NULL;
        for (int i = 0; i < LINES; i++)
            value[4 + k * LINES + i] = set[i] ? v[i] : 0xFFFFFFFFu;
        k++;
    }
}

/* the developer's (between.c, ALADDIN_DEV_STEPS): a hash of the values each line takes from the channels of target
   `target` (LINE_FIXED: the colour bands) */
uint32_t video_lines_target_hash(uint8_t target)
{
    static uint32_t value[LINES];
    uint8_t set[LINES];
    uint32_t h = 2166136261u;

    for (int ch = 1; ch < 8; ch++) {
        const LineChannel *c = &g.line_channel[ch];

        if (!(g.line_effects_on >> ch & 1) || c->target != target)
            continue;
        memset(set, 0, sizeof set);
        channel_values(c, 0, value, set);
        for (int i = 0; i < LINES; i++)
            h = (h ^ (set[i] ? value[i] : 0xFFFFFFFFu)) * 16777619u;
    }
    return h;
}
