/*
 * view.h: the view of the level, in game pixels (docs/aladdin/native/VIEW.md).
 *
 * The original's view is 256 x 224. The native game's is w x h, never
 * smaller than that on either axis, and dx = (w - 256) / 2, dy = (h - 224) / 2
 * are how far each side of the original's view it reaches. Every rule that
 * was a 256 x 224 screen size in the original is written from these, so
 * that at 256 x 224 (dx = dy = 0) it is the original's value.
 *
 * The view is not game state: it is not in g, and no play records it. It
 * is chosen from the window: one game pixel is `scale` window pixels across
 * and down (Jeff, 2026-11-22: a fixed pixel scale, so a bigger window shows
 * more of the level, never stretched). It changes only where a level loads
 * and where a still screen starts (video_view_level and video_view_still,
 * video.h), never during a level: a window resized in a level keeps the
 * level's view and the engine fits the picture to the window, its shape
 * kept (`square`, nshvc_screen.aspect).
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

/* At init: reads ALADDIN_VIEW and ALADDIN_VIEW_SCALE, and sets the view for a still screen. */
void view_init(void);

/* The view for a level whose picture crops its top `crop` rows (video_cropped_rows), or for a still screen, from the
   window as it is now. Returns 1 if the view's size changed. ALADDIN_VIEW=WxH, when set, gives the view and the
   window is not read; with no window (headless) the view is 256 x 224. */
int view_choose(int crop);
int view_choose_still(void);

#endif
