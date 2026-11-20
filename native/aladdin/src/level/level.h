/*
 * level.h: the level's own running parts (the record's level/): where its
 * play starts, its screen and backdrop set-ups, its companion, its restart
 * point and exit, the view's edge rules and its value sequences.
 */
#ifndef ALADDIN_LEVEL_H
#define ALADDIN_LEVEL_H

#include "system/state.h"

/* level_tables.c */
void LevelStartPlace(void);             /* 81:84B2: the player and the camera at the level's start */
void Level035CSet(void);                /* 81:878B: g.progress_sequence and g.progress_stage from the level's entry */
void LevelCompanionStart(void);         /* 81:8623: the level's companion (actors[2]) at the player's place */
void LevelSongPlay(void);               /* 81:86DE */
void RestartPointCheck(void);           /* 81:86F0: once a frame of play */
void LevelSpotsSet(void);               /* 81:8716: the level's exit spots (g.level_spots) */
void LevelExitCheck(void);              /* 81:872E: once a frame of play */

/* the level's tasks (task entries; camera_trigger.c, level03_scene.c,
   level05_end_scene.c, level0d_scene.c, level10_scene.c, level_events.c) */
#include "system/task.h"
TaskRun CameraTriggerTask(Task *t);     /* 80:A22D */
TaskRun Level03Scene(Task *t);          /* 80:A12D */
TaskRun Level05EndScene(Task *t);       /* 80:A2A8 */
TaskRun Level0DScene(Task *t);          /* 80:A501 */
TaskRun Level10Scene(Task *t);          /* 80:A5AC */
TaskRun BandsSpeedUpTask(Task *t);      /* 81:A0C3 */
TaskRun GreyPastTask(Task *t);          /* 81:A128 */
TaskRun RedOutTask(Task *t);            /* 81:A15B */
TaskRun ClimbTopTask(Task *t);          /* 81:A30D */
TaskRun SoundWhileSlot1Task(Task *t);   /* 81:A1C0 */
TaskRun RiserCoursesTask(Task *t);      /* 81:A2D1 */
TaskRun SoundLaterTask(Task *t);        /* 81:A2FF */

/* level_flags.c, level_bits.c */
uint8_t LevelFlagFromMask(uint8_t flag);    /* 83:C2F0: g.level_switch_flags[flag] = 1 when its bit is in the mask for the
                                               stage, else 0; returns it */
int LevelFlagSpotFind(uint16_t x, uint16_t y);  /* 83:C2A9: the flag of the level's flagged spot at (x, y) (each rounded down
                                               to 8), -1 none (the original's C set); its one caller is kind $3B */
void Bits035DExpand(void);              /* 83:C2E7: all eight */

/* level_screen_setups.c */
void LevelScreenSetup(void);            /* 81:87EF: the level's own screen set-up */

/* level_backdrop.c */
void LevelBackdropStart(void);          /* 81:8E06 */

/* view_edges.c */
void ViewEdgeRuleRun(Actor *player);    /* 81:E97D: the rule g.view_edge_rule picks, from the player's frame work */

/* sequences.c */
void SequenceStart(Sequence *s);        /* 81:EC6F */
void SequencesStep(void);               /* 81:EC23 */


#endif
