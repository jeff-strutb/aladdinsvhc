/*
 * game.h: the screens from power-on to the title: the boot task, the logo,
 * the opening story and the title's task (the record's game/progress.c's
 * BootTask, logo_screen.c, opening_story.c, title_task.c). Stage 5's
 * screens add theirs here.
 */
#ifndef ALADDIN_GAME_H
#define ALADDIN_GAME_H

#include "system/task.h"

/* progress.c */
TaskRun BootTask(Task *t);              /* 81:8000: task entry */

/* logo_screen.c */
TaskRun LogoScreen(Task *t);            /* A5:E000 */

/* opening_story.c */
TaskRun OpeningStory(Task *t);          /* 80:AC87: never returns (TaskExec to the title) */
TaskRun OpeningSceneRun(Task *t);       /* 80:AD78 */
void ScreenFadeInStart(void);           /* 80:AEA5 */
void ScreenFadeOutStart(void);          /* 80:AEAF */
void OpeningActorPlace(unsigned entry); /* 80:AEC0: entry 0-21 of the story's placements */
TaskRun TitleAfterFadeOut(Task *t);     /* 80:9C79: task entry */

/* title_task.c */
TaskRun TitleStartTask(Task *t);        /* 80:9C64 */
TaskRun TitleTask(Task *t);             /* 80:9C91: task entry */

#endif
