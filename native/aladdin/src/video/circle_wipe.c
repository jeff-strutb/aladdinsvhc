/*
 * circle_wipe.c: the circle that closes in on a point (the wipe at a level's
 * end), drawn as the window's left and right edge on each scanline; and the
 * same circle growing round an actor.
 */
#include <stdint.h>

#include "actor/actor.h"
#include "actor/actor_own.h"
#include "system/state.h"
#include "video/video.h"

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

/*
 * @routine 81:B264 CircleWipeStep
 * WHAT IT DOES: Draws the wipe's circle of radius `radius` round the point
 * x, y into the window table, when the point is on the screen; returns 1
 * when it drew.
 * HOW: The point less the play field's scroll (layer 0's x and y words at
 * its record's +$3A and +$3E, as the original reads them: the fraction byte
 * and the low byte of the whole part) must be below 256 and 224. The 256
 * scanlines' edges are set to no window ($FF, 0); then the midpoint method
 * an octant at a time: x from 0, y from the radius, the term 3 - 2 radius,
 * each step plotted while x is not past y, the term growing by 4x + 6
 * (negative) or by 4(x - y) + 10 with y down one.
 */
int CircleWipeStep(uint16_t radius, uint16_t x, uint16_t y)
{
    uint16_t cx = (uint16_t)(x - (uint16_t)g.layers[0].scroll_x);
    uint16_t cy = (uint16_t)(y - (uint16_t)g.layers[0].scroll_y);
    uint16_t px = 0, py = radius, term = (uint16_t)(3 - 2 * radius);

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
