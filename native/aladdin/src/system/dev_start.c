/*
 * dev_start.c: a developer's tool, off unless asked for by the environment:
 * start the game at a screen that is otherwise far into a play, with the
 * state that screen reads set, to look at it or run it. Nothing of the
 * original's; read once, at the boot task's first step (BootTask).
 *
 *   ALADDIN_DEV_SCREEN=NAME    start NAME instead of the logo and the title:
 *       genie, continue, level-end, story-card, ending-story, ending (the
 *       stages 4-$0B, 50 items), credits (the single stage with the credits'
 *       text, fewer than 50 items)
 *   ALADDIN_DEV_PART=N, ALADDIN_DEV_STAGE=N   the progress (level-end and
 *       story-card read the part; 6 is the last)
 *   ALADDIN_DEV_ITEMS=N   the items counted (the ending's stages need 50 or
 *       more); default 60, 10 for credits
 *   ALADDIN_DEV_CHEAT=1   the options' cheat set from the start (the title's
 *       level select: R held with Up for the part and Right for the stage)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/level_tables.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"

#define ITEMS_ALL 60
#define ITEMS_FEW 10
#define LAYERS_PLAY 0x17            /* BG1-BG3 and sprites on the main screen, as a level leaves them */
#define CONTINUES 2

static const struct {
    const char *name;
    TaskEntry entry;
    unsigned items;                 /* the default items counted, 0: not set */
} screens[] = {
    { "genie", TASK_ENTRY_GENIE_SCREEN, 0 },
    { "continue", TASK_ENTRY_CONTINUE_SCREEN, 0 },
    { "level-end", TASK_ENTRY_LEVEL_END, 0 },
    { "story-card", TASK_ENTRY_STORY_CARD, 0 },
    { "ending-story", TASK_ENTRY_ENDING_STORY, 0 },
    { "ending", TASK_ENTRY_ENDING_STAGES, ITEMS_ALL },
    { "credits", TASK_ENTRY_ENDING_STAGES, ITEMS_FEW },
};

static TaskEntry chosen = TASK_ENTRY_NONE;

/* a number in the environment, or `otherwise` */
static unsigned env_number(const char *name, unsigned otherwise)
{
    const char *v = getenv(name);

    return v && *v ? (unsigned)strtoul(v, NULL, 0) : otherwise;
}

/*
 * DevStartApply: reads the environment once: the cheat, and the screen's
 * state when a screen is asked for.
 */
void DevStartApply(void)
{
    const char *screen = getenv("ALADDIN_DEV_SCREEN");
    unsigned i;

    if (env_number("ALADDIN_DEV_CHEAT", 0))
        g.cheat_start = 1;
    if (!screen || !*screen)
        return;
    for (i = 0; i < sizeof screens / sizeof screens[0]; i++) {
        if (strcmp(screen, screens[i].name))
            continue;
        chosen = screens[i].entry;
        g.continues = CONTINUES;
        g.main_layers = LAYERS_PLAY;
        for (unsigned a = 0; a < 4; a++)
            g.control_masks[a] = control_option_mask(g.kept_options[0], a);
        g.gem_sets_total = (uint8_t)env_number("ALADDIN_DEV_ITEMS", screens[i].items);
        g.progress_sequence = (uint8_t)env_number("ALADDIN_DEV_PART", g.progress_sequence);
        g.progress_stage = (uint8_t)env_number("ALADDIN_DEV_STAGE", g.progress_stage);
        return;
    }
    fprintf(stderr, "ALADDIN_DEV_SCREEN: no screen named %s\n", screen);
}

/* the screen asked for, or TASK_ENTRY_NONE */
TaskEntry DevStartEntry(void)
{
    return chosen;
}
