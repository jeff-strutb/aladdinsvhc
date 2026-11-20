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

/* frame_step.c, play_time.c, level_load.c */
void FrameStep(void);                   /* 81:8311, 81:830D FrameStepLong */
void PlayTimeTick(void);                /* 81:85CE */
void LevelRamSetup(void);               /* 81:825F, 81:825B LevelRamSetupLong: for g.level */
TaskRun TextCardScreen(Task *t);        /* 81:ED33 */

/* logo_screen.c */
TaskRun LogoScreen(Task *t);            /* A5:E000 */

/* opening_story.c */
TaskRun OpeningStory(Task *t);          /* 80:AC87: never returns (TaskExec to the title) */
TaskRun OpeningSceneRun(Task *t);       /* 80:AD78 */
void ScreenFadeInStart(void);           /* 80:AEA5 */
void ScreenFadeOutStart(void);          /* 80:AEAF */
void OpeningActorPlace(unsigned entry); /* 80:AEC0: entry 0-21 of the story's placements */
TaskRun TitleAfterFadeOut(Task *t);     /* 80:9C79: task entry */

/* story_text.c */
TaskRun StoryTextTask(Task *t);         /* 81:F332: task entry */

/* title_task.c */
TaskRun TitleStartTask(Task *t);        /* 80:9C64 */
TaskRun TitleTask(Task *t);             /* 80:9C91: task entry */

/* menu_mark.c */
#define MENU_MARK_CELLS 7
extern const uint16_t menu_mark_cells[MENU_MARK_CELLS];  /* the mark's cells, words from its place */
void MenuMarkErase(uint16_t place);     /* 81:EF15: place = the text map's word index */
void TitleMenuBlink(void);              /* 81:EE55 */
void TitleMenuCursor(void);             /* 81:EDA6, 81:EDA2 TitleMenuCursorLong */

/* menu_mark_draw.c */
void MenuMarkDraw(uint16_t place);      /* 81:EF39 */
void MenuMarkMove(void);                /* 81:EE04 */

#endif
