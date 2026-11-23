/*
 * view.c: the view of the level (view.h).
 *
 * The rule (Jeff, 2026-11-22: "Just don't support portrait. Landscape down to 4:3 dimensions only."): the view is chosen
 * from the window's proportion a = width / height, not from its resolution, so a 1080p and a 4K window of one shape show the
 * same game view; game pixels stay square. The proportion is taken as at least 4:3. The picture shown is 216 lines (the
 * original's 224 less the 8 a level crops) and round(216 a) across, even, so at least 288: a window of 4:3 or wider shows
 * that, and a window narrower than 4:3 shows the 4:3 picture letterboxed (the engine fits it to the window, its shape
 * kept). Nothing is ever taller than the original's. A level's view adds the crop rows (LEVEL_CROP, 8) to the shown
 * height, since its picture starts that many rows down; level 2 crops 12, so its picture is 4 rows shorter inside the same
 * view and the engine fits it (video_draw's aspect) with 4 rows of letterbox, whatever the window. A still screen's view is
 * the shown size, at least 256 x 224, with the original's screen centred in it by video_draw.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "nshvc/nshvc.h"
#include "video/view.h"

View view = { VIEW_ORIGINAL_W, VIEW_ORIGINAL_H, 0, 0, 0 };

#define SHOWN_H 216             /* the original's 224 rows less the 8 a level crops */
#define NARROWEST_W 288         /* the shown picture at 4:3: 216 x 4 / 3 */
#define LEVEL_CROP 8
#define MOST VIEW_WIDEST_W

static long window_w, window_h; /* ALADDIN_DEV_WINDOW_AT: the window's pixels the rule reads, from that update on (0: none given) */
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
        /* the same limits as a window's: the original's height, at least 288 across, but for the original's own 256 x 224
           (the check that the game is the original's) */
        forced = 1;
        (void)h;
        forced_h = VIEW_ORIGINAL_H;
        forced_w = w == VIEW_ORIGINAL_W && h == VIEW_ORIGINAL_H ? VIEW_ORIGINAL_W : size_of(w, NARROWEST_W);
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
    if (window_w > 0 && window_h > 0) {
        d.width = (int32_t)window_w;
        d.height = (int32_t)window_h;
    }
    *w = *h = 0;
    if (d.width <= 0 || d.height <= 0)
        return;
    a = (double)d.width / (double)d.height;
    if (a < 4.0 / 3.0)
        a = 4.0 / 3.0;
    *h = SHOWN_H;
    *w = size_of(even_near(SHOWN_H * a), NARROWEST_W);
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

/* the view the window asks for now, for a level or for a still screen: ALADDIN_VIEW (or a developer's change at an update, view_dev_at) gives it and
   the window is not read; with no window (headless, or a window with no size) the view as it is, the original's 256 x 224 at the start */
void view_target(int level, int32_t *w, int32_t *h, int *square)
{
    int32_t sw, sh;

    if (forced) {
        *w = forced_w;
        *h = forced_h;
        *square = 1;
        return;
    }
    shown_size(&sw, &sh);
    *square = sw != 0;
    if (sw == 0) {
        /* no window (headless), or one with no size now (minimised): the view stays as it is, the original's 256 x 224 where none was chosen */
        *w = view.w;
        *h = view.h;
        *square = view.square;
    } else if (level) {
        *w = sw;
        *h = sh + LEVEL_CROP;
    } else {
        *w = size_of(sw, VIEW_ORIGINAL_W);
        *h = size_of(sh, VIEW_ORIGINAL_H);
    }
}

int view_apply(int32_t w, int32_t h, int square)
{
    return set(w, h, square);
}

int view_forced(void)
{
    return forced;
}

int view_choose(void)
{
    int32_t w, h;
    int square;

    view_target(1, &w, &h, &square);
    return set(w, h, square);
}

int view_choose_still(void)
{
    int32_t w, h;
    int square;

    view_target(0, &w, &h, &square);
    return set(w, h, square);
}

/* the developer's lists of changes at updates: ALADDIN_DEV_VIEW_AT=UPDATE:WxH[,UPDATE:WxH...] gives the view (the original's height, at least
   288 across, as ALADDIN_VIEW is clamped) and it is taken at once; ALADDIN_DEV_WINDOW_AT=UPDATE:WxH[,...] gives the window's pixels the
   rule reads (as a window resized would, 1920x1080 for 16:9), so the view follows by the rule and waits as a window's does (VIEW.md 4.8).
   Called at each update's start. */
#define AT_MAX 512

typedef struct AtList {
    int known, n, next;
    struct { uint64_t at; long w, h; } e[AT_MAX];
} AtList;

static void at_parse(AtList *l, const char *name)
{
    const char *e = getenv(name);

    l->known = 1;
    while (e && *e && l->n < AT_MAX) {
        unsigned long long u;
        long w, h;
        int used = 0;

        if (sscanf(e, "%llu:%ldx%ld%n", &u, &w, &h, &used) != 3 || !used) {
            fprintf(stderr, "%s: not UPDATE:WxH at \"%s\"\n", name, e);
            exit(2);
        }
        l->e[l->n].at = u;
        l->e[l->n].w = w;
        l->e[l->n++].h = h;
        e += used;
        while (*e == ',' || *e == ' ')
            e++;
    }
}

void view_dev_at(uint64_t update)
{
    static AtList views, windows;

    if (!views.known) {
        at_parse(&views, "ALADDIN_DEV_VIEW_AT");
        at_parse(&windows, "ALADDIN_DEV_WINDOW_AT");
    }
    while (views.next < views.n && views.e[views.next].at <= update) {
        forced = 1;
        forced_h = VIEW_ORIGINAL_H;
        forced_w = size_of(views.e[views.next++].w, NARROWEST_W);
    }
    while (windows.next < windows.n && windows.e[windows.next].at <= update) {
        window_w = windows.e[windows.next].w;
        window_h = windows.e[windows.next++].h;
    }
}
