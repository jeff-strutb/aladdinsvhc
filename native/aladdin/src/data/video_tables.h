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

uint8_t palette_sequence_set(unsigned step);     /* PaletteSequenceTask: $80:BA6A + step, step < 6 */
uint8_t palette_sequence_frames(unsigned step);  /* $80:BA70 + step */
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

/* the text layouts (TextLayoutDraw 81:9734, TextLayoutDrawBig 81:979A):
   $82:B575 + layout (a byte offset, the layout's low 7 bits) gives its offset
   in bank $82; there, runs to a 0 byte: a count byte (low 6 bits; bit 6 one
   word repeated), the text map byte offset (word), then the words (one when
   repeated). A big layout: a count byte (low 6 bits), the offset (word), the
   high byte of every word, then a first tile (word) per character. */
DataRef text_layout(uint8_t layout);
uint8_t text_layout_byte(DataRef at, unsigned i);
uint16_t text_layout_word(DataRef at, unsigned i);  /* the word at byte i */

/* the palette cycles (video/palette_cycles.c): $84:E000 + cycle * 2 gives
   a script's offset from $84:E000; there its frames (byte), colours (byte)
   and the palette byte offset (word). Frames are at $84:E004 + an offset: a
   wait byte, then the colours (words). The original's offsets are kept. */
uint16_t palette_cycle_script(uint8_t cycle);
uint16_t palette_cycle_table16(uint16_t at);    /* the word at $84:E000 + at */
uint8_t palette_cycle_frame8(uint16_t at);      /* the byte at $84:E004 + at */
uint16_t palette_cycle_frame16(uint16_t at);    /* the word at $84:E004 + at */

/* Mode7Rotate's sines (RotateSine 81:B25B): the word at $82:87D3 + at, at
   0-$80 (even): 65 entries of a quarter turn, the value in its top 12 bits */
uint16_t rotate_sine_word(unsigned at);

/* palettes of 16 colours: the actors' at $84:B000 (ActorPaletteFromRom),
   the screens' at $87:FF40 (PaletteFromRom); colour i of palette n */
uint16_t actor_palette_colour(uint8_t n, unsigned i);
uint16_t rom_palette_colour(uint8_t n, unsigned i);

/* ShakeTask's shakes ($80:BAA3) and Bg1WobbleTask's wobbles ($80:BA76), 4
   bytes each, by the task's parameter (a byte offset) */
typedef struct ShakeEntry { uint8_t layer, sways, frames, step; } ShakeEntry;
typedef struct WobbleEntry { uint8_t step_x, step_y, swing, frames; } WobbleEntry;
ShakeEntry shake_entry(uint8_t at);
WobbleEntry wobble_entry(uint8_t at);

/* the status line's health marks: mark n's place, a byte offset in the
   status line ($81:F58D + n) */
uint8_t health_mark_place(uint8_t n);

/* the window effects (video/window_effects.c). Their tables are read
   through the data bank $80, as tasks run (the record's comments say $82):
   table set `set` (a byte offset, as the effect names it) of $80:BCBB:
   entry i's line count (0 ends) and its word; register set `regs` (a byte
   offset) of $80:BCA9: the transfer's mode and target register as bytes.
   The window's shape is read with the data bank $82 (the effects set it):
   $82:BBAE the offset of its last pair, then left, right pairs. */
typedef struct WindowTableEntry { uint8_t count; uint16_t word; } WindowTableEntry;
WindowTableEntry window_table_entry(uint8_t set, unsigned i);
uint8_t window_register_mode(uint8_t regs);
uint8_t window_register_target(uint8_t regs);
uint16_t window_shape_last(void);
uint8_t window_shape_byte(uint16_t at);         /* the byte at $82:BBAE + at */

/* ScrollUpdate (81:C5A4), through the data bank $80: the camera's windows
   at $80:C672 + w (w a byte offset): x bias, x width, y bias, y height, the
   camera point's y offset (words at +0, +2, +4, +6, +8); the set speeds at
   $80:C6EA + s (s 0 or 4): 24 bits from the fraction byte, as two words */
uint16_t camera_window_word(uint8_t w, unsigned field);
uint32_t scroll_speed(uint8_t s);

/* RandomFromList (81:AE8A), through the data bank $80: list `list` (the
   record's X / 2): $80:DCE7 + list * 2 gives its offset from $80:DCE7; there
   its mask, then its entries */
uint8_t random_list_byte(uint8_t list, unsigned i);    /* i 0: the mask; 1 on: the entries */

/* the colour bands (ColourBandsWrite 81:A08A), through the data bank $80:
   their count at $80:BA7E, then (lines, value) pairs; each buffer's offset
   in the table area ($80:BAA1 + buffer) */
uint8_t colour_band_count(void);
uint8_t colour_band_byte(unsigned i);       /* byte i of the pairs */
uint8_t colour_band_buffer(uint8_t buffer);

/* an actor's own picture list (ActorPictureQueueParts 81:94B3, through the
   data bank $80): Actor.own_pictures names the word that holds the list;
   the list's word for picture n names its parts: each a source, its bytes,
   and (for each after the first) its VRAM word address's step from the
   part before. Returns the parts, at most `max`, written to `parts`. */
typedef struct PicturePart {
    UploadSource source;
    uint16_t bytes;
    uint16_t step;              /* words after the part before; 0 for the first */
} PicturePart;
unsigned picture_parts(DataRef pictures, uint8_t picture, PicturePart *parts, unsigned max);

/* IrisCloseTask (80:B263): iris `iris`'s starting radius and its centre */
uint8_t iris_radius(uint8_t iris);
void iris_centre(uint8_t iris, uint16_t *x, uint16_t *y);

/* a per-line table the cartridge holds (LineChannel.cart): byte `at` of it */
DataRef line_table_at(uint8_t bank, uint16_t address);   /* the table the original's A1T and A1B name */
uint8_t line_table_byte(DataRef table, uint32_t at);

/* the text bands (video/text_bands.c, through the bank $80): channel 5's
   table by the buffer the commit uses (0 or 1: $80:CEEF, $80:CF3B), and the
   two window bytes a band pair sets, at $80:CF87 + step and $80:CF8B + step,
   as indexes into the 16 bands' bytes */
DataRef text_band_table(unsigned buffer);
uint8_t text_band_row_a(uint8_t step);
uint8_t text_band_row_b(uint8_t step);

/* a page of text (TextPageDraw 82:B190): the page's line count, each line's
   attribute and characters, and the tilemap offset (bytes) the list names for
   the line. `page` and `places` each step past what is read. */
typedef struct TextPageLine {
    uint16_t at;                /* the line's first word in the text map */
    uint8_t attribute;
    uint8_t length;
    const uint8_t *chars;
} TextPageLine;
unsigned text_page_begin(DataRef *page);                          /* the lines it holds */
TextPageLine text_page_line(DataRef *page, DataRef *places);     /* the next line */

#endif
