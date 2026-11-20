/*
 * unconverted.h: the routines system/ and the boot screens call that are
 * not converted yet. Those an owner's header already declares (video.h,
 * actor.h) are declared there; the rest are declared here with the
 * parameters the calls give them, as the interface asked of each owner
 * (plan, "Cross-area calls"), until the owner's header has them.
 * unconverted.c holds a placeholder for each that prints its name once to
 * stderr; the owner's definition replaces it at link time. When nothing is
 * left here or there, both files go.
 *
 * Numbers are the record's own: a set, a song, a layout, a table's entry.
 */
#ifndef ALADDIN_UNCONVERTED_H
#define ALADDIN_UNCONVERTED_H

#include <stdint.h>

#include "actor/actor.h"
#include "audio/sound.h"
#include "map/map.h"
#include "system/task.h"
#include "video/video.h"
#include "video/video_tasks.h"

/* ---- video/ (native-video): not in video.h yet ---- */
void ScrollUpdate(void);                    /* 81:C5A4 */
void LayersScroll(void);                    /* 81:C1FA */
void BgFollowLayers(void);                  /* 81:8568 */
void PaletteCyclesStep(void);               /* 81:EC98 */
void LevelScrollLimits(void);               /* 81:C032 */
void LevelActorGraphicsLoad(void);          /* 81:B9F4 */
TaskRun WindowEffectTask(Task *t);          /* 82:8000 */

/* ---- game/, map/, level/, input/: this session's, not yet converted ---- */
int PadConfirmPressed(void);                /* 80:A04F: A, X, B, Y or START newly pressed (input/pad_buttons.c) */

/* ---- past the title (stage 5) ---- */
void LevelNumberText(void);                 /* 80:A0B1 */
int LevelSelect(void);                      /* 80:A05C: 1 when it changed the level (the record's C clear) */
TaskRun OptionsTask(Task *t);               /* 83:8979 */
TaskRun PasswordScreen(Task *t);            /* 84:AD50 */
TaskRun GameStartTask(Task *t);             /* 81:8095 */
TaskRun DemoTask(Task *t);                  /* 81:8031 */

/* A placeholder's report: `name` printed once to stderr. */
void unconverted(const char *name);

#endif
