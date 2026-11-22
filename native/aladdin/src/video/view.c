/*
 * view.c: the view of the level (view.h).
 *
 * The rule (Jeff, 2026-11-22): one game pixel is `scale` window pixels across and down, so the picture shown is the
 * window's pixels over the scale, floored and made even. The scale is 4, or ALADDIN_VIEW_SCALE (a float) for now.
 * It is lowered for a window that would show less than the original: the picture is never under 256 x 216 (a level
 * crops at least 8 dark rows from 224, so 216 shown rows are the original's whole level picture), and the scale then is
 * the largest that gives 256 x 216, which may be fractional (the engine draws at the window's pixels, so the picture
 * stays sharp, only larger).
 *
 * A level's view is that picture plus the rows it crops: w x (shown height + crop). A still screen's view is the picture
 * itself, at least 256 x 224 so the original's screen fits in it, centred by video_draw.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "nshvc/nshvc.h"
#include "video/view.h"

View view = { VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0, 0, 0 };

#define DEFAULT_SCALE 4.0
#define LEAST_SHOWN_H 216       /* the original's 224 rows less the 8 a level crops */
#define MOST 4096

static int forced;              /* ALADDIN_VIEW gave the view */
static int32_t forced_w, forced_h;
static double scale = DEFAULT_SCALE;

/* a size kept to the original's at least, and even, so that the original's view sits at whole pixels in it */
static int32_t size_of(long v, int32_t least)
{
    if (v < least)
        v = least;
    if (v > MOST)
        v = MOST;
    return (int32_t)(v & ~1L);
}

void view_init(void)
{
    const char *e = getenv("ALADDIN_VIEW");
    const char *s = getenv("ALADDIN_VIEW_SCALE");
    long w, h;

    if (e && sscanf(e, "%ldx%ld", &w, &h) == 2) {
        forced = 1;
        forced_w = size_of(w, VIEW_ORIGINAL_W);
        forced_h = size_of(h, VIEW_ORIGINAL_H);
    }
    if (s && atof(s) > 0.0)
        scale = atof(s);
    view_choose_still();
}

/* the picture shown at the window's pixels, in game pixels: both even, 0 with no window */
static void shown_size(int32_t *w, int32_t *h)
{
    nshvc_display d = nshvc_display_info();
    const char *fake = getenv("ALADDIN_VIEW_WINDOW");      /* a developer's setting: WxH, the window's pixels as the rule reads them */
    double s = scale;
    long fw, fh;

    if (fake && sscanf(fake, "%ldx%ld", &fw, &fh) == 2) {
        d.width = (int32_t)fw;
        d.height = (int32_t)fh;
    }
    *w = *h = 0;
    if (d.width <= 0 || d.height <= 0)
        return;
    if (d.width / s < VIEW_ORIGINAL_W || d.height / s < LEAST_SHOWN_H) {
        double sw = (double)d.width / VIEW_ORIGINAL_W, sh = (double)d.height / LEAST_SHOWN_H;

        s = sw < sh ? sw : sh;
    }
    *w = size_of((long)floor((double)d.width / s + 1e-6), 0);
    *h = size_of((long)floor((double)d.height / s + 1e-6), 0);
}

static int set(int32_t w, int32_t h, int square)
{
    int changed = w != view.w || h != view.h;

    view.w = w;
    view.h = h;
    view.dx = (w - VIEW_ORIGINAL_W) / 2;
    view.dy = (h - VIEW_ORIGINAL_H) / 2;
    view.square = square;
    if (changed && getenv("ALADDIN_DEV_VIEW")) {         /* a developer's line: the window and the view chosen from it */
        nshvc_display d = nshvc_display_info();

        fprintf(stderr, "view %d x %d (window %d x %d, scale %g)\n", (int)w, (int)h, (int)d.width, (int)d.height, scale);
    }
    return changed;
}

int view_choose(int crop)
{
    int32_t w, h;

    if (forced)
        return set(forced_w, forced_h, 1);
    shown_size(&w, &h);
    if (w == 0)
        return set(VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0);
    return set(w, size_of(h + crop, VIEW_ORIGINAL_H), 1);
}

int view_choose_still(void)
{
    int32_t w, h;

    if (forced)
        return set(forced_w, forced_h, 1);
    shown_size(&w, &h);
    if (w == 0)
        return set(VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0);
    return set(size_of(w, VIEW_ORIGINAL_W), size_of(h, VIEW_ORIGINAL_H), 1);
}
