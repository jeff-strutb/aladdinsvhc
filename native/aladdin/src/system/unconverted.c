/*
 * unconverted.c: placeholders for the routines not converted yet that
 * system/ and the boot screens call (unconverted.h, and the owners' headers
 * that declare them: video/video.h, actor/actor.h). Each is a weak
 * definition: the owner's own definition replaces it at link time, with no
 * edit here. Each prints its name once to stderr (the report counts these
 * lines) and does nothing; one that answers gives "none" (no actor, no
 * button); a task entry ends its task at once, so a screen that waits for it
 * goes on; a routine on a task's chain goes on.
 */
#include <stdio.h>
#include <string.h>

#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
#include "system/runahead.h"
#include "system/unconverted.h"

#define PLACEHOLDER __attribute__((weak))

void unconverted(const char *name)
{
    enum { MAX_NAMES = 128 };
    static const char *said[MAX_NAMES];
    static unsigned count;
    unsigned i;

    if (runahead_ahead())       /* told once, by the real step */
        return;
    for (i = 0; i < count; i++)
        if (!strcmp(said[i], name))
            return;
    if (count < MAX_NAMES)
        said[count++] = name;
    fprintf(stderr, "unconverted: %s\n", name);
}
