/*
 * snapshot.c: nothing yet (snapshot.h): the runahead mechanism's state list is to be used here, not a second one.
 */
#include "system/snapshot.h"

int snapshot_available(void) { return 0; }
size_t snapshot_max(void) { return 0; }
size_t snapshot_save(uint8_t *buf, size_t cap) { (void)buf; (void)cap; return 0; }
int snapshot_load(const uint8_t *buf, size_t len) { (void)buf; (void)len; return 0; }
uint32_t snapshot_layout_mark(void) { return 0; }
