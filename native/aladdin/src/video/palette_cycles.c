/*
 * palette_cycles.c: the palette cycles: six records (g.palette_cycle and
 * g.palette_cycle_running), each playing a script of colour frames into the
 * palette, round and round.
 */
#include <stdio.h>

#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:EC8E PaletteCycleStart
 * WHAT IT DOES: Starts palette cycle `cycle` in record `slot` (0-5; the
 * original's X / 14): its first step finds its script.
 * HOW: PaletteCycleStartLong (81:EC8A), its call for other banks, is this.
 */
void PaletteCycleStart(uint8_t slot, uint8_t cycle)
{
    if (slot >= 6) {
        fprintf(stderr, "video: palette cycle record %u is not one of the six\n", slot);
        return;
    }
    g.palette_cycle[slot].step = 0;
    g.palette_cycle[slot].cycle = cycle;
    g.palette_cycle_running[slot]++;
}

/*
 * @routine 81:ECEF PaletteCycleFrame
 * WHAT IT DOES: Puts a cycle's next frame in the palette: its wait, then its
 * colours from the cycle's first colour on; the palette sent.
 */
static void cycle_frame(PaletteCycle *c)
{
    uint16_t from = c->next;
    unsigned to = c->at / 2u;

    c->wait = palette_cycle_frame8(from++);
    /* a frame of 0 colours is the original's 65,536 (reported, left out) */
    if (c->colours == 0)
        fprintf(stderr, "video: palette cycle %u has frames of 0 colours\n", c->cycle);
    for (unsigned i = 0; i < c->colours; i++, from = (uint16_t)(from + 2))
        if (to + i < 256)
            g.palette[to + i] = palette_cycle_frame16(from);
    c->next = from;
    c->step++;
    g.palette_upload++;
}

/*
 * @routine 81:EC98 PaletteCyclesStep
 * WHAT IT DOES: Steps every running palette cycle once, in record order.
 * HOW: The step each record names:
 *   0 PaletteCycleLoad (81:ECC3): its script found, its first frame kept,
 *     its frames, colours and first colour read;
 *   1 PaletteCycleRestart (81:ECDF): the frames left set, the next frame
 *     its first, and on into PaletteCycleFrame;
 *   2 PaletteCycleFrame;
 *   3 PaletteCycleWait (81:ED24): the wait counted down; at 0 the frames
 *     left counted down, step 2 while some are left, else 1.
 * PaletteCycleStartLong (81:EC8A) is PaletteCycleStart.
 */
void PaletteCyclesStep(void)
{
    for (int n = 0; n < 6; n++) {
        PaletteCycle *c = &g.palette_cycle[n];
        uint16_t script;

        if (!g.palette_cycle_running[n])
            continue;
        switch (c->step) {
        case 0:
            script = palette_cycle_script(c->cycle);
            c->first = script;
            c->frames = (uint8_t)palette_cycle_table16(script);
            c->colours = (uint8_t)(palette_cycle_table16(script) >> 8);
            c->at = palette_cycle_table16((uint16_t)(script + 2));
            c->step++;
            break;
        case 1:
            c->left = c->frames;
            c->next = c->first;
            c->step++;
            cycle_frame(c);
            break;
        case 2:
            cycle_frame(c);
            break;
        case 3:
            if (--c->wait)
                break;
            c->step = --c->left ? 2 : 1;
            break;
        default: {
            static int told;

            if (!told) {
                told = 1;
                fprintf(stderr, "video: palette cycle step %u is not one of the four\n", c->step);
            }
            break;
        }
        }
    }
}
