/*
 * game.h: the screens from power-on to the title: the boot task, the logo,
 * the opening story and the title's task (the record's game/progress.c's
 * BootTask, logo_screen.c, opening_story.c, title_task.c). Stage 5's
 * screens add theirs here.
 */
#ifndef ALADDIN_GAME_H
#define ALADDIN_GAME_H

#include "actor/actor.h"
#include "system/task.h"

/* progress.c */
TaskRun BootTask(Task *t);              /* 81:8000: task entry */
TaskRun DemoTask(Task *t);              /* 81:8031: the title's task goes on as the demo's play */
void DemoStart(void);                   /* 81:8066 (81:8062 Long): demo g.demo_number's level and input */
TaskRun GameStartTask(Task *t);         /* 81:8095: task entry (the title's GAME START) */
TaskRun LevelPlayStartTask(Task *t);    /* 81:80A3: play at the level the progress gives */
TaskRun DemoPlayStart(Task *t);         /* 81:80D3 */
void PlayStartCounts(void);             /* 81:80E0 (81:80DC Long): the random seed, full health */
void NewGameState(void);                /* 81:8118 (81:8114 Long): a new game's counts and first level */
void LevelFromProgress(void);           /* 81:8157: g.level from the progress sequence and stage */
TaskRun NextPartTask(Task *t);          /* 81:8181: task entry (the story card's TaskExec) */

/* continue_screen.c, genie_screen.c: task entries */
TaskRun ContinueScreenTask(Task *t);    /* 80:B2E3 */
TaskRun GenieScreen(Task *t);           /* 80:A63B */

/* level_end.c, story_card.c: task entries */
TaskRun LevelEndTask(Task *t);          /* 81:F61D */
TaskRun StoryCardTask(Task *t);         /* 81:F10D: the story card between acts */
TaskRun EndingStory(Task *t);           /* 80:AF42: the final act's scene (story_scenes.c) */

/* play_loop.c: the game's task in play; none returns */
TaskRun PlayLoopStart(Task *t);         /* 81:8191: the level loaded, then the loop */
TaskRun PlayLoop(Task *t);              /* 81:82CF */

/* pause.c */
TaskRun PauseCheck(Task *t);            /* 81:8398: once a frame of play; waits while paused */

/* frame_step.c, play_time.c, level_load.c */
bool FrameStep(void);                   /* 81:8311, 81:830D FrameStepLong: true when an actor's TaskExec
                                           ended the frame (system/task.h: the chain returns task_exec_take) */
void PlayTimeTick(void);                /* 81:85CE */
void LevelRamSetup(void);               /* 81:825F, 81:825B LevelRamSetupLong: for g.level */
TaskRun LevelLoad(Task *t);             /* 81:81A7 (81:81A3 Long): g.level loaded and started; it waits */
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

/* the player's code filed in game/ (player/player.h: his resume points from
   PLAYER_RESUME_GAME up; the scenes' PlayerSceneLand, PlayerSceneStand,
   PlayerEndingRide); returns as PlayerCode */
int PlayerSceneCode(Actor *a);

#endif
