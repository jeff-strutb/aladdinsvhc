/*
 * dev_saves.c: a developer's tool, off unless asked for by the environment: save and load a slot at a given update, to check
 * that a load puts the game back exactly (docs/aladdin/native/SAVES.md; native/checks/saves/exact.sh).
 *
 *   ALADDIN_DEV_SAVE_AT=U[,SLOT]   at the start of update U (before its step) the game is saved to SLOT (default 0)
 *   ALADDIN_DEV_LOAD_AT=V[,SLOT]   at the start of update V the slot is loaded and the update ends there (no step: the next
 *                                  update is the one after the save)
 *   ALADDIN_DEV_FAULT=1            planted fault for the check: the player's x is left as it was before the load (a field
 *                                  the snapshot "left out"), so the traces must then differ
 */
#include <stdio.h>
#include <stdlib.h>

#include "nshvc/nshvc.h"
#include "system/saves.h"
#include "system/state.h"
#include "system/system.h"

static int parse(const char *name, uint64_t *at, int *slot)
{
    const char *v = getenv(name);
    char *end;

    if (!v || !*v)
        return 0;
    *at = strtoull(v, &end, 0);
    *slot = *end == ',' ? atoi(end + 1) : 0;
    return 1;
}

/* in update, where the pause menu's update stands: 1 when a load ended the update */
int DevSaves(uint64_t u)
{
    uint64_t at;
    int slot;

    if (parse("ALADDIN_DEV_SAVE_AT", &at, &slot) && u == at) {
        static SaveCapture cap;

        saves_capture(&cap);
        fprintf(stderr, "dev_saves: update %llu saved to slot %d: %s\n", (unsigned long long)u, slot, saves_write(slot, &cap) ? "written" : "FAILED");
    }
    if (parse("ALADDIN_DEV_LOAD_AT", &at, &slot) && u == at) {
        uint32_t keep = g.actors[0].x;
        int ok = saves_load(slot);

        fprintf(stderr, "dev_saves: update %llu loaded slot %d: %s\n", (unsigned long long)u, slot, ok ? "loaded" : "FAILED");
        if (ok && getenv("ALADDIN_DEV_FAULT"))
            g.actors[0].x = keep;
        return ok;
    }
    return 0;
}
