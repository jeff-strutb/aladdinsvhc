/*
 * video_tables.h: video's own tables (the video session's file).
 */
#ifndef ALADDIN_VIDEO_TABLES_H
#define ALADDIN_VIDEO_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* the window effects' quarter sine: 64 steps rising 0 to 15
   (video/window_effects.c's HalfSineBuild) */
uint8_t quarter_sine_step(unsigned step);   /* step < 64 */

/* where an upload's bytes are, as the original names them by bank */
typedef enum {
    FROM_CART,                  /* the cartridge: `cart` */
    FROM_UNPACKED,              /* g.unpacked at `at` (the original's bank $7F) */
    FROM_TEXT_MAP               /* g.text_map at byte `at` (the original's $7E:D800 on) */
} UploadFrom;

typedef struct UploadSource {
    uint8_t from;               /* UploadFrom */
    uint16_t at;
    const uint8_t *cart;        /* FROM_CART */
} UploadSource;

/* VramCopy, VramCopy2 (81:F5DC, 81:F60D): entry `entry` of copy table 0
   ($80:F197, 7 bytes an entry: source long, VRAM address, words; 55 entries)
   or of table 1 (the same table 37 entries on). Every source is in bank $7F. */
typedef struct VramCopyEntry {
    uint16_t from;              /* byte offset in g.unpacked */
    uint16_t to;                /* VRAM word address */
    uint16_t words;
} VramCopyEntry;
#define VRAM_COPIES 55
VramCopyEntry vram_copy_entry(unsigned table, unsigned entry);

uint8_t palette_sequence_set(unsigned step);     /* PaletteSequenceTask: $81:BA6A + step, step < 6 */
uint8_t palette_sequence_frames(unsigned step);  /* $81:BA70 + step */
uint8_t mode7_palette_row(uint8_t at);           /* Mode7TileRowsConvert: $81:B407 + at, at < $30 */
int16_t mode7_quarter_sine(unsigned step);       /* SineTableBuild: $81:B3E5, 17 words from 0 to $100 */

/* LevelPalettesLoad (81:86BF): $80:B812 + level gives its list's offset from
   $80:B812: a count, then the palette sets */
unsigned level_palette_count(uint16_t level);
uint8_t level_palette_set(uint16_t level, unsigned i);

/* TextBufferPartUpload (80:84AA): part 0-2 of $80:B61B */
typedef struct TextPart {
    uint16_t to;                /* VRAM word address */
    uint16_t from;              /* byte offset in g.text_map */
    uint16_t bytes;
} TextPart;
TextPart text_part(unsigned part);

/* VramUploadByNumber (80:84E2): number 1-11 of $80:B62F/$80:B630 */
typedef enum { UPLOAD_WORDS, UPLOAD_LOW_BYTES, UPLOAD_HIGH_BYTES } UploadHow;
typedef struct NumberedUpload {
    uint8_t how;                /* UploadHow */
    uint16_t to;                /* VRAM word address */
    UploadSource source;
    uint16_t bytes;
} NumberedUpload;
#define NUMBERED_UPLOADS 11
NumberedUpload numbered_upload(unsigned number);

/* VramListUpload (80:8899): record `record` of the list a value sequence's
   word names (bank $80): 8 bytes (VRAM address, source long, bytes, nonzero
   when another follows) */
typedef struct VramListRecord {
    uint16_t to;
    UploadSource source;
    uint16_t bytes;
    uint8_t more;
} VramListRecord;
DataRef vram_list_at(uint16_t word);            /* the list a sequence's word names */
VramListRecord vram_list_record(DataRef list, unsigned record);

#endif
