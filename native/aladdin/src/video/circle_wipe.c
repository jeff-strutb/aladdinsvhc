/*
 * circle_wipe.c: the circle that closes in on a point (the wipe at a level's
 * end), drawn as the window's left and right edge on each scanline; and the
 * same circle growing round an actor.
 *
 * In a level (video_level_view) the circle is in the view's terms (VIEW.md
 * 6.3): its centre is the level point less the view's corner, its edges are
 * columns of the view (16 bits, held to 0 and the view's last column) and its
 * lines the view's, in the table the LINE_WINDOW1_WIDE channel reads
 * (g.wide_data from WIDE_CIRCLE: runs of up to 127 lines, each after its
 * count). On the still screens (the Genie's, the story's iris) it is the
 * original's, in the table at $F500 of 256 columns and 224 lines.
 */
#include <stdint.h>

#include "actor/actor.h"
#include "actor/actor_own.h"
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

#define WIPE_TABLE 0x0100u          /* the window table the wipe writes (the original's $7E:F500) in g.line_data: a word a scanline, left then right */
#define WIPE_ROWS 256u

/* a - b, 0 when below; a - b, $FF when below; a + b, $FF when past */
static uint8_t less_or_0(uint8_t a, uint8_t b) { return a >= b ? (uint8_t)(a - b) : 0x00; }
static uint8_t less_or_ff(uint8_t a, uint8_t b) { return a >= b ? (uint8_t)(a - b) : 0xFF; }
static uint8_t more_or_ff(uint8_t a, uint8_t b) { return a + b > 0xFF ? 0xFF : (uint8_t)(a + b); }

static void edges_at(uint8_t row, uint8_t left, uint8_t right)
{
    g.line_data[WIPE_TABLE + 2u * row] = left;
    g.line_data[WIPE_TABLE + 2u * row + 1] = right;
}

/*
 * @routine 81:B2D9 CircleWipePlot
 * WHAT IT DOES: Puts one step of the circle into the window table: its
 * eight symmetric points as the left and right edges of four scanlines.
 * HOW: The spans centre x -/+ x and -/+ y, held to 0..$FF; the scanlines
 * centre -/+ x and -/+ y, $FF (outside the picture) when past it; the -/+ y
 * span goes to the scanlines centre -/+ x, the -/+ x span to centre -/+ y.
 */
static void plot(uint8_t cx, uint8_t cy, uint8_t x, uint8_t y)
{
    uint8_t span_x_left = less_or_0(cx, x), span_x_right = more_or_ff(cx, x);
    uint8_t span_y_left = less_or_0(cx, y), span_y_right = more_or_ff(cx, y);

    edges_at(less_or_ff(cy, x), span_y_left, span_y_right);
    edges_at(more_or_ff(cy, x), span_y_left, span_y_right);
    edges_at(less_or_ff(cy, y), span_x_left, span_x_right);
    edges_at(more_or_ff(cy, y), span_x_left, span_x_right);
}

/* ---- in a level: the view's columns and lines ---- */

#define WIDE_RUN 127u               /* lines in a run of the wide table */

/* where line `row`'s 4 bytes are in g.wide_data: after the counts of the runs before it and its own run's */
static unsigned wide_row_at(unsigned row)
{
    return WIDE_CIRCLE + (row / WIDE_RUN) * (1 + 4 * WIDE_RUN) + 1 + 4 * (row % WIDE_RUN);
}

static void wide_edges_at(int32_t row, int32_t left, int32_t right)
{
    unsigned at;

    if (row < 0 || row >= view.h)
        return;
    at = wide_row_at((unsigned)row);
    g.wide_data[at] = (uint8_t)left;
    g.wide_data[at + 1] = (uint8_t)(left >> 8);
    g.wide_data[at + 2] = (uint8_t)right;
    g.wide_data[at + 3] = (uint8_t)(right >> 8);
}

static int32_t span_left(int32_t a, int32_t b) { return a >= b ? a - b : 0; }
static int32_t span_right(int32_t a, int32_t b) { return a + b > view.w - 1 ? view.w - 1 : a + b; }

/* the eight symmetric points of one step as the left and right edges of four lines (plot above; rows outside the view are none) */
static void wide_plot(int32_t cx, int32_t cy, int32_t x, int32_t y)
{
    int32_t span_x_left = span_left(cx, x), span_x_right = span_right(cx, x);
    int32_t span_y_left = span_left(cx, y), span_y_right = span_right(cx, y);

    wide_edges_at(cy - x, span_y_left, span_y_right);
    wide_edges_at(cy + x, span_y_left, span_y_right);
    wide_edges_at(cy - y, span_x_left, span_x_right);
    wide_edges_at(cy + y, span_x_left, span_x_right);
}

/* CircleWipeStep in a level: the point (the level's, less the view's corner) must be inside the view */
static int wide_wipe(uint16_t radius, uint16_t x, uint16_t y)
{
    int32_t cx = (int16_t)(uint16_t)(x - (uint16_t)g.layers[0].scroll_x);
    int32_t cy = (int16_t)(uint16_t)(y - (uint16_t)g.layers[0].scroll_y);
    int32_t px = 0, py = radius, term = 3 - 2 * (int32_t)radius;

    if (cx < 0 || cx >= view.w || cy < 0 || cy >= view.h)
        return 0;
    for (unsigned row = 0; row < (unsigned)view.h; row++)
        wide_edges_at((int32_t)row, WIDE_NONE, 0);
    for (unsigned run = 0; run * WIDE_RUN < (unsigned)view.h; run++) {
        unsigned lines = (unsigned)view.h - run * WIDE_RUN;

        g.wide_data[WIDE_CIRCLE + run * (1 + 4 * WIDE_RUN)] = (uint8_t)(0x80 | (lines < WIDE_RUN ? lines : WIDE_RUN));
    }
    g.wide_data[wide_row_at((unsigned)view.h - 1) + 4] = 0;       /* the table's end */
    while (py >= px) {
        wide_plot(cx, cy, px, py);
        if (term < 0) {
            term += 4 * px + 6;
        } else {
            term += 4 * (px - py) + 10;
            py--;
        }
        px++;
    }
    return 1;
}

/*
 * @routine 81:B264 CircleWipeStep
 * WHAT IT DOES: Draws the wipe's circle of radius `radius` round the point
 * x, y into the window table, when the point is on the screen; returns 1
 * when it drew.
 * HOW: The point less the play field's scroll (layer 0's x and y words at
 * its record's +$3A and +$3E, as the original reads them: the fraction byte
 * and the low byte of the whole part) must be below 256 and 224 (the view's
 * width and height in a level). The 256 scanlines' edges (the view's lines in
 * a level) are set to no window ($FF, 0); then the midpoint method
 * an octant at a time: x from 0, y from the radius, the term 3 - 2 radius,
 * each step plotted while x is not past y, the term growing by 4x + 6
 * (negative) or by 4(x - y) + 10 with y down one. In a level the edges are 16
 * bits and held to the view's last column.
 */
int CircleWipeStep(uint16_t radius, uint16_t x, uint16_t y)
{
    uint16_t cx, cy;
    uint16_t px = 0, py = radius, term = (uint16_t)(3 - 2 * radius);

    if (video_level_view())
        return wide_wipe(radius, x, y);
    cx = (uint16_t)(x - (uint16_t)g.layers[0].scroll_x);
    cy = (uint16_t)(y - (uint16_t)g.layers[0].scroll_y);
    if (cx >= 0x0100 || cy >= 0x00E0)
        return 0;
    for (unsigned row = 0; row < WIPE_ROWS; row++)
        edges_at((uint8_t)row, 0xFF, 0x00);
    while (py >= px) {
        plot((uint8_t)cx, (uint8_t)cy, (uint8_t)px, (uint8_t)py);
        if (term & 0x8000) {
            term = (uint16_t)(term + 4 * px + 6);
        } else {
            term = (uint16_t)(term + 4 * (uint16_t)(px - py) + 10);
            py--;
        }
        px++;
    }
    return 1;
}

/*
 * @routine 83:EAAF ActorCircleWipeStep
 * WHAT IT DOES: One step of a circle wipe round the actor: its radius (+$45)
 * grows by one and the circle is drawn round its place on the screen (8
 * pixels above its y), into the window table.
 */
void ActorCircleWipeStep(Actor *a)
{
    uint16_t x = (uint16_t)(a->x >> 8), y = (uint16_t)(a->y >> 8);
    uint16_t view_x = (uint16_t)(g.layers[0].scroll_x >> 8), view_y = (uint16_t)(g.layers[0].scroll_y >> 8);

    a->own.bytes[OWN_45]++;
    CircleWipeStep(a->own.bytes[OWN_45], (uint16_t)(x - view_x), (uint16_t)(y - view_y - 8));
}
