/*
 * saves.h: the save slots' files (docs/aladdin/native/SAVES.md section 4): one file a slot in the engine's save folder,
 * holding a header (a format number, the size, the state layout's mark), the screenshot crop, the caption's values and the
 * snapshot (system/snapshot.h), then a hash of all of it. A file that does not match whole (size, mark, hash) is shown as
 * unreadable and never loaded.
 */
#ifndef SYSTEM_SAVES_H
#define SYSTEM_SAVES_H

#include <stddef.h>
#include <stdint.h>

#define SAVE_SLOTS 24                   /* 4 pages of 6 */
#define SAVE_PIC_W 96
#define SAVE_PIC_H 72
#define SAVE_CROP_LIFT 24               /* the crop's centre is this far above the player's feet (his place in the picture) */

enum { SAVE_EMPTY, SAVE_USED, SAVE_UNREADABLE };

typedef struct {
    int state;                          /* SAVE_* */
    uint32_t pic[SAVE_PIC_W * SAVE_PIC_H];
    char line1[24], line2[24];          /* the caption, made from the values the file keeps */
} SaveInfo;

/* the moment as the pause menu opened: the picture's crop and the caption's values (saves_capture, in update) */
typedef struct {
    int have;
    uint32_t pic[SAVE_PIC_W * SAVE_PIC_H];
    uint8_t sequence, stage, digits[6];
    uint64_t updates;
} SaveCapture;

/* the game as it stands (and its picture, from the engine) into c */
void saves_capture(SaveCapture *c);
/* all slots read and checked again */
void saves_scan(void);
const SaveInfo *saves_info(int slot);
/* the snapshot now with the capture c into the slot's file: 1 written (the engine writes it behind), 0 not (no snapshot) */
int saves_write(int slot, const SaveCapture *c);
/* the slot's snapshot put in: 1 on success; 0 nothing changed (empty, unreadable, another layout) */
int saves_load(int slot);

/* the engine's picture (w x h, 0xAARRGGBB) to the capture's crop around (cx, cy), the player's place in the picture:
   used by saves_capture, public for the checks */
void saves_crop(const uint32_t *picture, int w, int h, int cx, int cy, uint32_t *out);

#endif
