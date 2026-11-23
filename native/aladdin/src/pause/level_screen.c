/*
 * level_screen.c: the level select screen (level_screen.h). The cells are every level in the game's order, each with its
 * picture (video/level_picture.h, made when the game was built) and "STAGE n-m" (the game's own part and stage numbers).
 * A level not beaten is shown at UNBEATEN_PERCENT of its brightness and cannot be chosen. Choosing a beaten level shuts the screen and leaves
 * the level for the title to start.
 */
#include <stdio.h>
#include <string.h>

#include "game/beaten.h"
#include "pause/level_screen.h"
#include "pause/slot_grid.h"
#include "video/level_picture.h"

#define PER_PAGE 4
#define UNBEATEN_PERCENT 25     /* an unbeaten level's picture at this much of its brightness: shown, not chosen */
#define PIXELS (LEVEL_PICTURE_W * LEVEL_PICTURE_H)

static struct {
    int requested, active, taken;
    SlotCell cell[BEATEN_LEVELS];
    int beaten[BEATEN_LEVELS];
} s = { 0, 0, -1, { { 0 } }, { 0 } };

static uint32_t faded[BEATEN_LEVELS][PIXELS];
static SlotCell empty_cell;

static const SlotCell *cell_of(void *ctx, int index)
{
    (void)ctx;
    return index >= 0 && index < BEATEN_LEVELS ? &s.cell[index] : &empty_cell;
}

void level_screen_request_title(void) { s.requested = 1; }
int level_screen_requested(void) { return s.requested; }
int level_screen_active(void) { return s.active; }

/* the picture darkened to UNBEATEN_PERCENT of its brightness, a channel at a time */
static const uint32_t *fade_of(const uint32_t *pic, unsigned i)
{
    unsigned k;

    if (!pic)
        return NULL;
    for (k = 0; k < PIXELS; k++) {
        uint32_t c = pic[k];
        uint32_t r = (c >> 16 & 0xFF) * UNBEATEN_PERCENT / 100, g = (c >> 8 & 0xFF) * UNBEATEN_PERCENT / 100, b = (c & 0xFF) * UNBEATEN_PERCENT / 100;

        faded[i][k] = (c & 0xFF000000u) | r << 16 | g << 8 | b;
    }
    return faded[i];
}

void level_screen_open(void)
{
    SlotGridSpec spec;
    unsigned i;

    s.requested = 0;
    s.active = 1;
    s.taken = -1;
    for (i = 0; i < BEATEN_LEVELS; i++) {
        SlotCell *c = &s.cell[i];
        const uint32_t *pic = LevelPictureOf(BeatenListLevel(i));

        memset(c, 0, sizeof *c);
        s.beaten[i] = BeatenIs(BeatenListLevel(i));
        c->used = 1;
        c->picture = s.beaten[i] ? pic : fade_of(pic, i);
        snprintf(c->line1, sizeof c->line1, "STAGE %u-%u", BeatenListPart(i) + 1, BeatenListStage(i) + 1);
    }
    memset(&spec, 0, sizeof spec);
    spec.title = "LEVEL SELECT";
    spec.cols = 2;
    spec.rows = 2;
    spec.pages = (BEATEN_LEVELS + PER_PAGE - 1) / PER_PAGE;
    spec.pic_w = LEVEL_PICTURE_W;
    spec.pic_h = LEVEL_PICTURE_H;
    spec.cell = cell_of;
    slot_grid_open(&spec);
}

int level_screen_update(const nshvc_input *in)
{
    SlotResult r = slot_grid_update(in);

    if (r.what == SLOT_BACK) {
        slot_grid_close();
        s.active = 0;
        return LEVEL_SCREEN_BACK;
    }
    if (r.what == SLOT_CHOSEN && r.index >= 0 && r.index < BEATEN_LEVELS && s.beaten[r.index]) {  /* an unbeaten level is shown, not chosen */
        s.taken = r.index;
        slot_grid_close();
        s.active = 0;
        return LEVEL_SCREEN_CHOSEN;
    }
    return LEVEL_SCREEN_OPEN;
}

int level_screen_take(void)
{
    int e = s.taken;

    s.taken = -1;
    return e;
}
