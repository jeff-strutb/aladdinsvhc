/*
 * snapshot.h: the whole game state at one moment, put into a buffer and taken back out of one (docs/aladdin/native/FRAMERATE.md
 * section 10). The save slots use it (SAVES.md).
 *
 * The state is the real world after a complete update: call these from an update's start (the pause menu's, the title's), not
 * inside a step. A load puts the world in as it was and the next update is the one after the save; the picture shows the loaded
 * world from its first picture. Sound: the song resumes at its place, a note that was sounding is silent until the next (10.2).
 * The buffer is not a stable format across builds with another layout: snapshot_layout_mark names the layout.
 */
#ifndef SYSTEM_SNAPSHOT_H
#define SYSTEM_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

int snapshot_available(void);
/* the most bytes a snapshot takes now (the view and the level change it): a buffer of this size always holds one */
size_t snapshot_max(void);
/* the state at this moment into buf (cap bytes): the bytes written, 0 on failure (a message on stderr says why) */
size_t snapshot_save(uint8_t *buf, size_t cap);
/* the state put back from buf (as snapshot_save wrote it, here or in another run of a build of this layout): 1 on success;
   on 0 nothing was changed: the layout, the lengths and every code in it are checked first */
int snapshot_load(const uint8_t *buf, size_t len);
/* a number that names the layout of the state: the sizes of g and its parts, the pointer fields, every registered video part and
   static by name and size, the shadowed engine objects. A snapshot of another layout is not loaded (snapshot_load returns 0) */
uint32_t snapshot_layout_mark(void);

#endif
