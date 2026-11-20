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
#include "input/input.h"
#include "map/map.h"
#include "system/task.h"
#include "video/video.h"
#include "video/scroll_layer.h"
#include "video/video_tasks.h"

/* ---- game/, map/, level/, input/: this session's, not yet converted ---- */

/* ---- past the title (stage 5) ---- */
TaskRun OptionsTask(Task *t);               /* 83:8979 */
TaskRun PasswordScreen(Task *t);            /* 84:AD50 */

/* A placeholder's report: `name` printed once to stderr. */
void unconverted(const char *name);

#endif
