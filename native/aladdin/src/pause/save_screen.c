/*
 * save_screen.c: the save slots' screen (save_screen.h).
 *
 * In game: an empty slot saves at once; a used one asks OVERWRITE, LOAD or CANCEL, then (as QUIT asks REALLY QUIT) once more
 * before it does it. From the title: a used slot asks once, then loads; an empty one does nothing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pause/save_screen.h"
#include "pause/slot_grid.h"
#include "system/saves.h"
#include "system/snapshot.h"

enum { ASK_NONE, ASK_WHICH, ASK_REALLY_OVERWRITE, ASK_REALLY_LOAD, ASK_NOTE };

static struct {
    SaveFrom from;
    int ask, requested;
    SaveCapture cap;
    SlotCell cell[SAVE_SLOTS];
} s;

static const SlotCell *cell_of(void *ctx, int index)
{
    (void)ctx;
    return &s.cell[index < 0 || index >= SAVE_SLOTS ? 0 : index];
}

static void fill_cells(void)
{
    int i;

    for (i = 0; i < SAVE_SLOTS; i++) {
        const SaveInfo *si = saves_info(i);
        SlotCell *c = &s.cell[i];

        memset(c, 0, sizeof *c);
        c->used = si->state != SAVE_EMPTY;
        c->picture = si->state == SAVE_USED ? si->pic : NULL;
        if (si->state == SAVE_EMPTY)
            snprintf(c->line1, sizeof c->line1, "SLOT %d", i + 1);
        else
            snprintf(c->line1, sizeof c->line1, "%s", si->line1);
        snprintf(c->line2, sizeof c->line2, "%s", si->state == SAVE_USED ? si->line2 : si->state == SAVE_EMPTY ? "" : "SLOT");
        if (si->state == SAVE_UNREADABLE)
            snprintf(c->line2, sizeof c->line2, "SLOT %d", i + 1);
    }
}

void save_screen_capture(void)
{
    if (getenv("ALADDIN_DEV_TRACE"))
        fprintf(stderr, "save screen: picture taken at update %llu\n", (unsigned long long)nshvc_updates());
    saves_capture(&s.cap);
}
void save_screen_request_title(void) { s.requested = 1; }
int save_screen_requested(void) { return s.requested; }

void save_screen_open(SaveFrom from)
{
    SlotGridSpec spec;

    s.requested = 0;
    s.from = from;
    if (getenv("ALADDIN_DEV_TRACE"))
        fprintf(stderr, "save screen opened from the %s\n", from == SAVE_FROM_GAME ? "game" : "title");
    s.ask = ASK_NONE;
    saves_scan();
    fill_cells();
    memset(&spec, 0, sizeof spec);
    spec.title = from == SAVE_FROM_GAME ? "SAVE GAME" : "SAVED GAMES";
    spec.cols = 3;
    spec.rows = 2;
    spec.pages = SAVE_SLOTS / 6;
    spec.pic_w = SAVE_PIC_W;
    spec.pic_h = SAVE_PIC_H;
    spec.cell = cell_of;
    slot_grid_open(&spec);
}

static void ask_note(const char *line)
{
    static const char *const ok[1] = { "OK" };

    s.ask = ASK_NOTE;
    slot_grid_ask(line, ok, 1);
}

static void ask_yes(int kind, int slot)
{
    static const char *const yn[2] = { "YES", "NO" };
    char line[40];

    snprintf(line, sizeof line, "%s SLOT %d?", kind == ASK_REALLY_OVERWRITE ? "REALLY OVERWRITE" : "REALLY LOAD", slot + 1);
    s.ask = kind;
    slot_grid_ask(line, yn, 2);
}

static void saved(int slot)
{
    if (!saves_write(slot, &s.cap))
        ask_note("CANNOT SAVE HERE");
    fill_cells();
    slot_grid_changed();
}

int save_screen_update(const nshvc_input *in)
{
    SlotResult r = slot_grid_update(in);
    int slot = r.index;

    if (r.what == SLOT_BACK) {
        slot_grid_close();
        return SAVE_SCREEN_BACK;
    }
    if (r.what == SLOT_CHOSEN) {
        const SaveInfo *si = saves_info(slot);

        if (s.from == SAVE_FROM_GAME) {
            if (si->state == SAVE_EMPTY)
                saved(slot);
            else if (si->state == SAVE_UNREADABLE)
                ask_yes(ASK_REALLY_OVERWRITE, slot);
            else {
                static const char *const which[3] = { "OVERWRITE", "LOAD", "CANCEL" };
                char line[24];

                snprintf(line, sizeof line, "SLOT %d", slot + 1);
                s.ask = ASK_WHICH;
                slot_grid_ask(line, which, 3);
            }
        } else if (si->state == SAVE_USED)
            ask_yes(ASK_REALLY_LOAD, slot);
    } else if (r.what == SLOT_ANSWER) {
        int ask = s.ask;

        s.ask = ASK_NONE;
        if (ask == ASK_WHICH) {
            if (r.answer == 0)
                ask_yes(ASK_REALLY_OVERWRITE, slot);
            else if (r.answer == 1)
                ask_yes(ASK_REALLY_LOAD, slot);
        } else if (ask == ASK_REALLY_OVERWRITE && r.answer == 0) {
            saved(slot);
        } else if (ask == ASK_REALLY_LOAD && r.answer == 0) {
            if (getenv("ALADDIN_DEV_TRACE"))
                fprintf(stderr, "save screen: load slot %d\n", slot);
            if (saves_load(slot)) {
                slot_grid_close();
                return SAVE_SCREEN_LOADED;
            }
            ask_note("CANNOT LOAD THIS");
        }
    }
    return SAVE_SCREEN_OPEN;
}
