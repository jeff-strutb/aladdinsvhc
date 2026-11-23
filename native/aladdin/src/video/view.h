/*
 * view.h: the view of the level, in game pixels (docs/aladdin/native/VIEW.md).
 *
 * The original's view is 256 x 224. The native game's is w x h, 224 tall and
 * at least 288 across (landscape down to 4:3: Jeff, 2026-11-22), and dx = (w - 256) / 2, dy = (h - 224) / 2 = 0
 * are how far each side of the original's view it reaches. Every rule that
 * was a 256 x 224 screen size in the original is written from these, so
 * that at 256 x 224 (dx = dy = 0) it is the original's value.
 *
 * The view is not game state: it is not in g, and no play records it. It
 * is chosen from the window: from the window's proportion (Jeff, 2026-11-22: not its resolution; square game pixels, never stretched). It is chosen where a level loads and
 * where a still screen starts (video_view_level and video_view_still,
 * video.h), and follows the window at the start of an update (video_view_follow,
 * VIEW.md 4.8), so it is the same in the update before's world and the newest.
 * Between a change and a settled window the engine fits the picture to the
 * window, its shape kept (`square`, nshvc_screen.aspect).
 */
#ifndef ALADDIN_VIDEO_VIEW_H
#define ALADDIN_VIDEO_VIEW_H

#include <stdint.h>

#define VIEW_ORIGINAL_W 256
#define VIEW_ORIGINAL_H 224

typedef struct View {
    int32_t w, h;       /* the view, in game pixels; both even */
    int32_t dx, dy;     /* (w - 256) / 2, (h - 224) / 2 */
    int32_t square;     /* 1: the picture is shown at its own shape, square game pixels (a window or ALADDIN_VIEW chose the view) */
} View;

extern View view;

/* At init: reads ALADDIN_VIEW, and sets the view for a still screen. */
void view_init(void);

/* The view for a level (its picture 8 rows shorter than the view: the crop; level 2 crops 12), or for a still screen,
   from the window's proportion as it is now. Returns 1 if the view's size changed. ALADDIN_VIEW=WxH, when set, gives the view and the
   window is not read; with no window (headless) the view is 256 x 224. */
int view_choose(void);
int view_choose_still(void);
/* the view the window asks for now (view_choose's rule) without setting it, for a level or a still screen; view_apply sets a view (1 if its size
   changed); view_forced: ALADDIN_VIEW or ALADDIN_DEV_VIEW_AT gives the view and no window is read; view_dev_at(update): the developer's changes
   due by that update are taken (called at each update's start). VIEW.md 4.8 */
void view_target(int level, int32_t *w, int32_t *h, int *square);
int view_apply(int32_t w, int32_t h, int square);
int view_forced(void);
void view_dev_at(uint64_t update);

#endif
