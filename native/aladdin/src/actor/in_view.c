/*
 * in_view.c: which actors are near the view's middle, and the four lists
 * of them that the sprites are drawn from.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"

/*
 * @routine 81:9198 ActorsInView
 * WHAT IT DOES: Marks which actors can be hit this frame (bit 6 of their
 * flags): those in use whose view box, placed on them, holds the range
 * point (g.range_x, g.range_y); and lists those with bit 3 of anim_flags
 * set in g.draw_lists by anim_flags' bits 0-1, counted in g.draw_counts.
 * The lists hold 32 each: in the original a 33rd runs into the next
 * list's words; here it is left out and reported once.
 */
void ActorsInView(void)
{
    int i;

    for (i = 0; i < 4; i++)
        g.draw_count[i] = 0;
    for (i = 1; i < ACTOR_RECORDS; i++) {
        Actor *a = &g.actors[i];
        ViewBox b;

        if (a->state == ACTOR_STATE_FREE)
            continue;
        b = view_box(a->view_box);
        if ((uint16_t)(g.range_x - pixels(a->x) + b.x_off) < b.x_span
            && (uint16_t)(g.range_y - pixels(a->y) + b.y_off) < b.y_span) {
            a->flags |= 0x40;
            if (a->anim_flags & 0x08) {
                int list = a->anim_flags & 0x03;

                if (g.draw_count[list] < 32) {
                    g.draw_list[list][g.draw_count[list]++] = a;
                } else {
                    static int told;

                    if (!told++)
                        fprintf(stderr, "ActorsInView: a 33rd actor for draw list %d left out\n", list);
                }
            }
        } else {
            a->flags &= (uint8_t)~0x40;
        }
    }
}
