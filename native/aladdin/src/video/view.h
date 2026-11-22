/*
 * view.h: the view of the level, in game pixels (docs/aladdin/native/VIEW.md).
 *
 * The original's view is 256 x 224. The native game's is w x h, never
 * smaller than that on either axis, and dx = (w - 256) / 2, dy = (h - 224) / 2
 * are how far each side of the original's view it reaches. Every rule that
 * was a 256 x 224 screen size in the original is written from these, so
 * that at 256 x 224 (dx = dy = 0) it is the original's value.
 *
 * The view is not game state: it is not in g, and no play records it. It is
 * set at init and changes only between updates.
 */
#ifndef ALADDIN_VIDEO_VIEW_H
#define ALADDIN_VIDEO_VIEW_H

#include <stdint.h>

#define VIEW_ORIGINAL_W 256
#define VIEW_ORIGINAL_H 224

typedef struct View {
    int32_t w, h;       /* the view, in game pixels; both even */
    int32_t dx, dy;     /* (w - 256) / 2, (h - 224) / 2 */
} View;

extern View view;

/* At init. ALADDIN_VIEW=WxH (a developer's setting) gives the view; without it, 256 x 224. */
void view_init(void);

#endif
