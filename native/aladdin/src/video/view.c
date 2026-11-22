/*
 * view.c: the view of the level (view.h).
 *
 * The rule (Jeff, 2026-11-22): the view is chosen from the window's proportion a = width / height, not from its
 * resolution, so a 1080p and a 4K window of one shape show the same game view; game pixels stay square.
 *   a >= 256 / 216 (landscape, or wider than the original's shown picture): 216 lines shown, round(216 a) across.
 *   narrower: 256 across, round(256 / a) lines shown.
 * Both are made even. A level's view adds the crop rows (LEVEL_CROP, 8) to the shown height, since its picture
 * starts that many rows down; level 2 crops 12, so its picture is 4 rows shorter inside the same view and the engine
 * fits it (video_draw's aspect) with 4 rows of letterbox, whatever the window. A still screen's view is the shown size,
 * at least 256 x 224, with the original's screen centred in it by video_draw.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "nshvc/nshvc.h"
#include "video/view.h"

View view = { VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0, 0, 0 };

#define SHOWN_H 216             /* the original's 224 rows less the 8 a level crops */
#define LEVEL_CROP 8
#define MOST 4096

static int forced;              /* ALADDIN_VIEW gave the view */
static int32_t forced_w, forced_h;

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
    long w, h;

    if (e && sscanf(e, "%ldx%ld", &w, &h) == 2) {
        forced = 1;
        forced_w = size_of(w, VIEW_ORIGINAL_W);
        forced_h = size_of(h, VIEW_ORIGINAL_H);
    }
    view_choose_still();
}

/* an even number nearest v */
static long even_near(double v)
{
    return 2 * (long)floor(v / 2.0 + 0.5);
}

/* the picture shown, in game pixels, from the window's proportion: both even, 0 with no window */
static void shown_size(int32_t *w, int32_t *h)
{
    nshvc_display d = nshvc_display_info();
    const char *fake = getenv("ALADDIN_VIEW_WINDOW");      /* a developer's setting: WxH, the window's pixels as the rule reads them */
    long fw, fh;
    double a;

    if (fake && sscanf(fake, "%ldx%ld", &fw, &fh) == 2) {
        d.width = (int32_t)fw;
        d.height = (int32_t)fh;
    }
    *w = *h = 0;
    if (d.width <= 0 || d.height <= 0)
        return;
    a = (double)d.width / (double)d.height;
    if (a >= (double)VIEW_ORIGINAL_W / SHOWN_H) {
        *h = SHOWN_H;
        *w = size_of(even_near(SHOWN_H * a), VIEW_ORIGINAL_W);
    } else {
        *w = VIEW_ORIGINAL_W;
        *h = size_of(even_near(VIEW_ORIGINAL_W / a), SHOWN_H);
    }
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

        fprintf(stderr, "view %d x %d (window %d x %d)\n", (int)w, (int)h, (int)d.width, (int)d.height);
    }
    return changed;
}

int view_choose(void)
{
    int32_t w, h;

    if (forced)
        return set(forced_w, forced_h, 1);
    shown_size(&w, &h);
    if (w == 0)
        return set(VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0);
    return set(w, h + LEVEL_CROP, 1);
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
