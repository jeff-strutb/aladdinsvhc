/*
 * video_tasks.h: video/'s task entries (system/task.h's contract).
 */
#ifndef ALADDIN_VIDEO_TASKS_H
#define ALADDIN_VIDEO_TASKS_H

#include "system/task.h"

TaskRun ColourFadeInTask(Task *t);          /* 81:9F0E */
TaskRun ColourFadeOutTask(Task *t);         /* 81:9F52 */
TaskRun PaletteSequenceTask(Task *t);       /* 81:9E75 */
TaskRun BrightnessUpTask(Task *t);          /* 81:9E8E */
TaskRun BrightnessDownTask(Task *t);        /* 81:9EB9 */
TaskRun Mode7ZoomInTask(Task *t);           /* 81:B157 */
TaskRun ShakeTask(Task *t);                 /* 81:A1D5 */
TaskRun Bg1WobbleTask(Task *t);             /* 81:9F98 */
TaskRun WindowEffectTask(Task *t);          /* 82:8000 */
TaskRun ColourBandTask(Task *t);            /* 81:A00C */
TaskRun IrisCloseTask(Task *t);             /* 80:B263: the iris scenes make it (g.scene_cue_first the iris, slot 3) */
TaskRun TextBlinkTask(Task *t);             /* 81:9E3B: the player's start makes it (slot 2) */

/* routines a task's chain calls that wait (TASK_GOES_ON when done) */
TaskRun BrightnessUpAndWait(Task *t);       /* 81:F311 */
TaskRun BrightnessDownAndWait(Task *t);     /* 81:F31C */

#endif
