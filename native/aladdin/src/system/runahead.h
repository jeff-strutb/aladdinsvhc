/*
 * runahead.h: the picture answers the controls with no added delay (docs/aladdin/native/FRAMERATE.md, section 9).
 *
 * Each tick the real world steps once with the pad as read; then the world is saved, stepped on `runahead_n()` more
 * updates with the same pad held (the ahead steps), and left as the picture's newest world. The picture's "was" and
 * "now" (video/between.h) are then the last two ahead worlds. The next tick puts the saved real world back before it
 * steps. The game's logic is not changed: an ahead step is the same update the real one runs.
 */
#ifndef SYSTEM_RUNAHEAD_H
#define SYSTEM_RUNAHEAD_H

#include <stddef.h>

#include "nshvc/nshvc.h"

/* main: once, from the program's arguments (--headless turns it off unless ALADDIN_RUNAHEAD is set) */
void runahead_init(int argc, char **argv);
/* update: first thing; puts the real world back if the last tick ran ahead */
void runahead_begin(void);
/* update: after the real step and its checks; runs the ahead steps with `step` and the same input */
void runahead_after(const nshvc_input *input, void (*step)(const nshvc_input *));
/* how many ahead steps a tick takes (0: off) */
unsigned runahead_n(void);

/* true while an ahead step runs: what must not leave the game's state (sound and music sent, files written) checks it */
int runahead_ahead(void);
/* for a routine that must not run in an ahead step (a level's load, the view's change, the sound's load): returns true
   inside one, and the ahead then ends with the step before; the caller returns at once */
int runahead_guard(void);
/* a static the steps change (registered once, before the first update; use RUNAHEAD_KEEP) */
void runahead_keep(void *p, size_t bytes);
/* before a write of `bytes` at `p` that an ahead step may make: the bytes put back when the real world is */
void runahead_log(void *p, size_t bytes);

#define RUNAHEAD_JOIN2(a, b) a##b
#define RUNAHEAD_JOIN(a, b) RUNAHEAD_JOIN2(a, b)
/* at file scope after the declaration of a static (or global) the steps change: saved and put back with the world */
#define RUNAHEAD_KEEP(var) \
    static void __attribute__((constructor)) RUNAHEAD_JOIN(runahead_keep_, __LINE__)(void) { runahead_keep(&(var), sizeof(var)); }

#endif
