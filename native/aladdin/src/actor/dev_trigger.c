/*
 * dev_trigger.c: the on-screen test of the original's view, and the developer's log of the triggers
 * (docs/aladdin/native/VIEW.md 4.4): ALADDIN_DEV_TRIGGERS=1 prints each trigger test that turns true, once while it stays true.
 */
#include <stdio.h>
#include <stdlib.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"

/* the in-view flag (the view's box) and the original's box both hold the range point */
bool ActorOnScreen(const Actor *a)
{
    bool on = (a->flags & 0x40) && a->on_screen;

    if (getenv("ALADDIN_DEV_TRIGGERS")) {
        ViewBox o = view_box_screen(a->view_box);

        DevTrigger(4, "screen", a, on, (int32_t)(int16_t)(pixels(a->x) - g.range_x), (int32_t)(int16_t)(pixels(a->y) - g.range_y),
                   (int32_t)o.x_off - (int32_t)o.x_span + 1, (int32_t)o.x_off, (int32_t)o.y_off - (int32_t)o.y_span + 1, (int32_t)o.y_off);
    }
    return on;
}

void DevTrigger(int id, const char *what, const Actor *a, bool result, int32_t dx, int32_t dy, int32_t lo_x, int32_t hi_x, int32_t lo_y, int32_t hi_y)
{
    static uint8_t was[ACTOR_RECORDS][8];
    static int on = -1;
    int n = (int)(a - g.actors);

    if (on < 0) {
        const char *e = getenv("ALADDIN_DEV_TRIGGERS");

        on = e && *e && *e != '0';
    }
    if (!on || n < 0 || n >= ACTOR_RECORDS)
        return;
    if (!result) {
        was[n][id] = 0;
        return;
    }
    if (was[n][id])
        return;
    was[n][id] = 1;
    fprintf(stderr, "TRIGGER %s kind=%02X actor=%d dx=%d dy=%d x=[%d,%d] y=[%d,%d]\n", what, a->kind, n, (int)dx, (int)dy,
            (int)lo_x, (int)hi_x, (int)lo_y, (int)hi_y);
}
