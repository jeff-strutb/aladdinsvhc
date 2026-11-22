/*
 * view.c: the view of the level (view.h).
 */
#include <stdio.h>
#include <stdlib.h>

#include "video/view.h"

View view = { VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0, 0 };

/* a size kept to the original's at least, and even, so that the original's view sits at whole pixels in it */
static int32_t size_of(long v, int32_t least)
{
    if (v < least)
        v = least;
    if (v > 4096)
        v = 4096;
    return (int32_t)(v & ~1L);
}

void view_init(void)
{
    const char *e = getenv("ALADDIN_VIEW");
    long w, h;

    if (e && sscanf(e, "%ldx%ld", &w, &h) == 2) {
        view.w = size_of(w, VIEW_ORIGINAL_W);
        view.h = size_of(h, VIEW_ORIGINAL_H);
    }
    view.dx = (view.w - VIEW_ORIGINAL_W) / 2;
    view.dy = (view.h - VIEW_ORIGINAL_H) / 2;
}
