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

/* the ending's text (video/text_bands.c): channel 5's bands and the page; the bands' state is the task's
   (Task.own.text_bands: its flip must start 0), channel 5's target and its turn on are the caller's */
void TextBandsCommit(Task *t);              /* 82:B270 */
void TextPageDraw(DataRef page, DataRef *places);  /* 82:B190: the page's lines at the places the list gives, in turn */
TaskRun TextBandsReveal(Task *t);           /* 82:B224: waits (TASK_GOES_ON until done) */
TaskRun TextBandsConceal(Task *t);          /* 82:B248: waits as TextBandsReveal */

/* routines a task's chain calls that wait (TASK_GOES_ON when done) */
TaskRun BrightnessUpAndWait(Task *t);       /* 81:F311 */
TaskRun BrightnessDownAndWait(Task *t);     /* 81:F31C */

#endif
