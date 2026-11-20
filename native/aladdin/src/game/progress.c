/*
 * progress.c: the first task, from power-on to the title. (The record's
 * progress.c also holds the game's start and progress routines: stage 5's.)
 */
#include "game/game.h"
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

enum { BOOT_START, BOOT_WAITED, BOOT_TEXT_CARD, BOOT_LOGO, BOOT_TITLE };

/*
 * @routine 81:8000 BootTask
 * WHAT IT DOES: The first task (slot 0, made by PowerOn): the text upload's
 * VRAM address and size set, a frame's wait, then the text card, the
 * sound sets, the logo screen, the NMI on, and on to the title
 * (TitleStartTask, the same task going on).
 * HOW: The region check (84:AFAE RegionCheck) is dead here: it clears $005D
 * and sets it on a PAL console only (the board is NTSC, the branch never
 * runs), and nothing in the game reads $005D; so it is not converted.
 */
TaskRun BootTask(Task *t)
{
    uint16_t *at = task_enter(t);
    TaskRun r;

    switch (*at) {
    case BOOT_START:
        g.text_upload_to = 0x5C00;
        g.text_upload_bytes = 0x0800;
        *at = BOOT_WAITED;
        return task_wait(t, 1);
    case BOOT_WAITED:
        *at = BOOT_TEXT_CARD;
        /* fall through */
    case BOOT_TEXT_CARD:
        r = TextCardScreen(t);
        if (r != TASK_GOES_ON)
            return r;
        SoundSetsLoad();
        *at = BOOT_LOGO;
        /* fall through */
    case BOOT_LOGO:
        r = LogoScreen(t);
        if (r != TASK_GOES_ON)
            return r;
        NmiEnable();
        *at = BOOT_TITLE;
        /* fall through */
    case BOOT_TITLE:
        break;
    }
    return TitleStartTask(t);
}
