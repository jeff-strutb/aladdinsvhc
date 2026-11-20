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

#include "system/unconverted.h"

#define PLACEHOLDER __attribute__((weak))

void unconverted(const char *name)
{
    enum { MAX_NAMES = 128 };
    static const char *said[MAX_NAMES];
    static unsigned count;
    unsigned i;

    for (i = 0; i < count; i++)
        if (!strcmp(said[i], name))
            return;
    if (count < MAX_NAMES)
        said[count++] = name;
    fprintf(stderr, "unconverted: %s\n", name);
}

PLACEHOLDER TaskRun WindowEffectTask(Task *t)
{
    unconverted("WindowEffectTask");
    return task_exit(t);
}

PLACEHOLDER uint16_t TilemapWord(uint16_t x, uint16_t y)
{
    unconverted("TilemapWord");
    (void)x;
    (void)y;
    return 0;
}

PLACEHOLDER void BlockSourceAdd(uint16_t source)
{
    unconverted("BlockSourceAdd");
    (void)source;
}

PLACEHOLDER int PadConfirmPressed(void)
{
    unconverted("PadConfirmPressed");
    return 0;
}

PLACEHOLDER void LevelNumberText(void)
{
    unconverted("LevelNumberText");
}

PLACEHOLDER int LevelSelect(void)
{
    unconverted("LevelSelect");
    return 0;
}

PLACEHOLDER TaskRun OptionsTask(Task *t)
{
    unconverted("OptionsTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun PasswordScreen(Task *t)
{
    unconverted("PasswordScreen");
    return task_exit(t);
}

PLACEHOLDER TaskRun GameStartTask(Task *t)
{
    unconverted("GameStartTask");
    return task_exit(t);
}

PLACEHOLDER TaskRun DemoTask(Task *t)
{
    unconverted("DemoTask");
    return task_exit(t);
}

PLACEHOLDER void ScrollUpdate(void)
{
    unconverted("ScrollUpdate");
}

PLACEHOLDER void LayersScroll(void)
{
    unconverted("LayersScroll");
}

PLACEHOLDER void BgFollowLayers(void)
{
    unconverted("BgFollowLayers");
}

PLACEHOLDER void PaletteCyclesStep(void)
{
    unconverted("PaletteCyclesStep");
}

PLACEHOLDER void LevelScrollLimits(void)
{
    unconverted("LevelScrollLimits");
}

PLACEHOLDER void LevelActorGraphicsLoad(void)
{
    unconverted("LevelActorGraphicsLoad");
}
