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
 *   ALADDIN_DEV_BEATEN=1  print the levels the beaten levels' file gives (game/beaten.c) at the start
 *   ALADDIN_DEV_CHEAT=1   the options' cheat set from the start (the title's
 *       level select: R held with Up for the part and Right for the stage)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/data.h"
#include "game/beaten.h"
#include "data/level_tables.h"
#include "data/system_tables.h"
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
static unsigned chosen_items;

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

    if (env_number("ALADDIN_DEV_CHEAT", 0) || (getenv("ALADDIN_DEV_LEVEL") && *getenv("ALADDIN_DEV_LEVEL")))
        g.cheat_start = 1;      /* ALADDIN_DEV_LEVEL plays the level select, which waits for the cheat (dev_level.c) */
    if (env_number("ALADDIN_DEV_BEATEN", 0)) {      /* the beaten levels the file gave, to the error output */
        fprintf(stderr, "beaten:");
        for (unsigned i = 0; i < BEATEN_LEVELS; i++)
            if (BeatenIs(BeatenListLevel(i)))
                fprintf(stderr, " %02X", BeatenListLevel(i));
        fprintf(stderr, "\n");
    }
    if (!screen || !*screen)
        return;
    for (i = 0; i < sizeof screens / sizeof screens[0]; i++) {
        if (strcmp(screen, screens[i].name))
            continue;
        chosen = screens[i].entry;
        chosen_items = screens[i].items;
        return;
    }
    fprintf(stderr, "ALADDIN_DEV_SCREEN: no screen named %s\n", screen);
}

/*
 * DevStartEntry: the screen asked for, or TASK_ENTRY_NONE. Its state is set
 * here, as the boot task is about to start it: the boot's own screens set
 * the layers and the controls up again before this.
 */
TaskEntry DevStartEntry(void)
{
    if (chosen == TASK_ENTRY_NONE)
        return chosen;
    g.continues = CONTINUES;
    g.main_layers = LAYERS_PLAY;
    for (unsigned a = 0; a < 4; a++)
        g.control_masks[a] = control_option_mask(g.kept_options[0], a);
    g.gem_sets_total = (uint8_t)env_number("ALADDIN_DEV_ITEMS", chosen_items);
    g.progress_sequence = (uint8_t)env_number("ALADDIN_DEV_PART", g.progress_sequence);
    g.progress_stage = (uint8_t)env_number("ALADDIN_DEV_STAGE", g.progress_stage);
    return chosen;
}

/*
 * DevLevelPick: ALADDIN_DEV_START_LEVEL=N (a level number): the title's menu starts that level as the level select screen
 * does (game/beaten.c LevelSelectStart), at the menu's first update; -1 when not asked for.
 */
int DevLevelPick(void)
{
    static int known, level = -1;

    if (!known) {
        known = 1;
        if (getenv("ALADDIN_DEV_START_LEVEL") && *getenv("ALADDIN_DEV_START_LEVEL"))
            level = (int)env_number("ALADDIN_DEV_START_LEVEL", 0);
    }
    return level;
}

/*
 * DevStartAt: ALADDIN_DEV_AT=x,y[,camera_x,camera_y] (pixels): a level's play starts there (its first start and its
 * restarts), for reaching a level's end by a developer without playing all of it. The camera defaults to the place
 * less the view's half.
 */
void DevStartAt(LevelStart *s)
{
    static int known;
    static unsigned have, v[4];
    const char *e;

    if (!known) {
        known = 1;
        e = getenv("ALADDIN_DEV_AT");
        if (e && *e)
            have = (unsigned)sscanf(e, "%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3]);
    }
    if (have < 2 || g.level >= 0x14)     /* the screens between levels ($24 ...) start at their own place */
        return;
    s->x = (uint16_t)v[0];
    s->y = (uint16_t)v[1];
    s->camera_x = have >= 3 ? (uint16_t)v[2] : (uint16_t)(v[0] > 128 ? v[0] - 128 : 0);
    s->camera_y = have >= 4 ? (uint16_t)v[3] : (uint16_t)(v[1] > 112 ? v[1] - 112 : 0);
    {
        /* the player is kept inside the layout, and a camera that was not given is kept inside the scroll limits the game sets (the layout less
           a screen: video/scroll_layer.c); a place or camera the original cannot have is told, not used */
        unsigned width = level_map_layout_columns(g.level, 0) * 256u, height = level_map_layout_rows(g.level, 0) * 256u;

        if (s->x >= width || s->y >= height) {
            fprintf(stderr, "ALADDIN_DEV_AT: %u,%u is outside level %02X's layout (%u x %u)\n", (unsigned)s->x, (unsigned)s->y, (unsigned)g.level, width, height);
            s->x = (uint16_t)(s->x >= width ? width - 16 : s->x);
            s->y = (uint16_t)(s->y >= height ? height - 16 : s->y);
        }
        if (have < 3 && s->camera_x + 256u > width)
            s->camera_x = (uint16_t)(width - 256u);
        if (have < 4 && s->camera_y + 256u > height)
            s->camera_y = (uint16_t)(height > 256u ? height - 256u : 0);
    }
}
