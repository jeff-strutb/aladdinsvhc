/*
 * snapshot.h: the whole game state at one moment, put into a buffer and taken back out of one (docs/aladdin/native/SAVES.md).
 *
 * This is the interface the save slots use. It is to be implemented by the runahead mechanism (system/runahead.[ch], on the runahead session's branch,
 * which enumerates every piece of state a step touches: `g`, the files' statics, the engine's
 * tiles, maps and palettes, the sound). Until that is committed snapshot.c holds nothing and snapshot_available is 0: the
 * slots then cannot be written or loaded, and say so.
 */
#ifndef SYSTEM_SNAPSHOT_H
#define SYSTEM_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

int snapshot_available(void);
/* the most bytes a snapshot takes */
size_t snapshot_max(void);
/* the state at this moment into buf (cap bytes): the bytes written, 0 on failure */
size_t snapshot_save(uint8_t *buf, size_t cap);
/* the state put back from buf (as snapshot_save wrote it): 1 on success; on 0 nothing was changed */
int snapshot_load(const uint8_t *buf, size_t len);
/* a number that names the layout of the state (every piece, its size): a snapshot of another layout is not loaded */
uint32_t snapshot_layout_mark(void);

#endif
