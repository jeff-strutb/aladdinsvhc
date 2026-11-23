/*
 * level_screen.c: the level select screen (level_screen.h). The cells are the beaten levels in the game's order, each with its
 * picture (video/level_picture.h, made when the game was built) and "STAGE n-m" (the game's own part and stage numbers).
 * Choosing a level shuts the screen and leaves the level for the title to start. With nothing beaten the screen says so.
 */
#include <stdio.h>
#include <string.h>

#include "game/beaten.h"
#include "pause/level_screen.h"
#include "pause/slot_grid.h"
#include "video/level_picture.h"

#define PER_PAGE 4

static struct {
    int requested, active, taken;
    unsigned count;
    SlotCell cell[BEATEN_LEVELS];
    unsigned entry[BEATEN_LEVELS];
} s = { 0, 0, -1, 0, { { 0 } }, { 0 } };

static SlotCell empty_cell;

static const SlotCell *cell_of(void *ctx, int index)
{
    (void)ctx;
    return index >= 0 && (unsigned)index < s.count ? &s.cell[index] : &empty_cell;
}

void level_screen_request_title(void) { s.requested = 1; }
int level_screen_requested(void) { return s.requested; }
int level_screen_active(void) { return s.active; }

void level_screen_open(void)
{
    SlotGridSpec spec;
    unsigned i;

    s.requested = 0;
    s.active = 1;
    s.taken = -1;
    s.count = BeatenCount();
    for (i = 0; i < s.count; i++) {
        unsigned e = BeatenFind(i);
        SlotCell *c = &s.cell[i];

        memset(c, 0, sizeof *c);
        s.entry[i] = e;
        c->used = 1;
        c->picture = LevelPictureOf(BeatenListLevel(e));
        snprintf(c->line1, sizeof c->line1, "STAGE %u-%u", BeatenListPart(e) + 1, BeatenListStage(e) + 1);
    }
    memset(&spec, 0, sizeof spec);
    spec.title = "LEVEL SELECT";
    spec.cols = 2;
    spec.rows = 2;
    spec.pages = s.count ? (int)((s.count + PER_PAGE - 1) / PER_PAGE) : 1;
    spec.pic_w = LEVEL_PICTURE_W;
    spec.pic_h = LEVEL_PICTURE_H;
    spec.cell = cell_of;
    slot_grid_open(&spec);
    if (!s.count) {
        static const char *const ok[1] = { "OK" };

        slot_grid_ask("NO STAGES CLEARED YET", ok, 1);
    }
}

int level_screen_update(const nshvc_input *in)
{
    SlotResult r = slot_grid_update(in);

    if (r.what == SLOT_BACK || (r.what == SLOT_ANSWER && !s.count)) {
        slot_grid_close();
        s.active = 0;
        return LEVEL_SCREEN_BACK;
    }
    if (r.what == SLOT_CHOSEN && r.index >= 0 && (unsigned)r.index < s.count) {
        s.taken = (int)s.entry[r.index];
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
