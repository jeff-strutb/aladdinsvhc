/*
 * wanted_levels.c: weak stand-ins for routines of other areas that the
 * levels-and-screens code calls before their owners land them (the stage 5
 * rules: "STAND-INS AND KIND ROWS"). Each prints its name once and does
 * nothing; the owner's definition replaces it at link time. Delete each
 * when its owner lands it.
 */
#include "game/game.h"
#include "player/player.h"
#include "system/task.h"
#include "system/unconverted.h"
#include "video/video_tasks.h"

#define STAND_IN __attribute__((weak))

/* video/'s */
STAND_IN TaskRun TextBlinkTask(Task *t)
{
    unconverted("TextBlinkTask");
    return task_exit(t);
}

/* player/'s (n5-player) */
STAND_IN void PlayerKeepOnScreenX(Actor *a)
{
    unconverted("PlayerKeepOnScreenX");
    (void)a;
}

STAND_IN void PlayerKeepOnScreen(Actor *a)
{
    unconverted("PlayerKeepOnScreen");
    (void)a;
}

STAND_IN void PlayerEnter8CFrom1(void)
{
    unconverted("PlayerEnter8CFrom1");
}
