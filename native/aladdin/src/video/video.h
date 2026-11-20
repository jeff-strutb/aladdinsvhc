/*
 * video.h: the picture. The game's tile sets, maps, palette and sprites are
 * engine objects; the game writes them when its graphics change, and
 * video_draw sets the whole picture from the game's state once a picture.
 *
 * The loop calls three things:
 *   video_init     from init: the engine objects made.
 *   video_vblank   from update, last, on each frame whose NMI is on: what the
 *                  original's NMI uploaded at the start of the next frame (the
 *                  VRAM queue, the lists, tile blocks, words, the text buffer
 *                  and its parts, the upload by number, the palette, the
 *                  sprites), written to the objects. Nothing runs between the
 *                  end of one update and the next one's tasks, so this is the
 *                  original's order, and the draw after it shows what the
 *                  original showed in the frame that NMI began.
 *   video_draw     from draw: the whole picture, from the fields of g the
 *                  original's NMI and IRQ copied to the PPU.
 *
 * Writes by video address. The original names its destinations by VRAM word
 * address in its queue records and tables; vram_write is the one place that
 * turns an address into the objects that show it. Every write goes to every
 * object that shows that address on any screen, so a screen that changes
 * its layer bases after writing (the title, the Genie screen) shows what was
 * written before:
 *   $0000-$3FFF  BG tiles from base 0, 4bpp, 1,024 tiles; tiles 0-255 also
 *                kept for Mode7ScreenSetup; on a Mode 7 picture, also its
 *                map (low bytes) and pixels (high bytes)
 *   $0000-$0FFF  also BG tiles from base $5000, tiles 768-1,023 (the wrap)
 *   $4000-$47FF  BG2's map, 64 x 32; its first 32 x 32 also kept for
 *                Mode7ScreenSetup
 *   $4800-$4FFF  BG1's map, 64 x 32
 *   $5000-$7FFF  BG tiles from base $5000, 4bpp, tiles 0-767; the sprites'
 *                tiles are 256 on (the sprites' base $6000)
 *   $5000-$6FFF  BG3's tiles, 2bpp, 1,024
 *   $5C00-$5FFF  BG3's map, 32 x 32 (also BG1's on the title)
 * Writes of the low bytes alone are Mode 7's map, of the high bytes alone its
 * pixels.
 */
#ifndef ALADDIN_VIDEO_H
#define ALADDIN_VIDEO_H

#include <stdint.h>

void video_init(void);
void video_vblank(void);
void video_draw(void);

/* ---- writes by video address (the one numeric part) ---- */

typedef enum {
    VRAM_WORDS,         /* whole words */
    VRAM_LOW_BYTES,     /* the low bytes alone: Mode 7's map */
    VRAM_HIGH_BYTES     /* the high bytes alone: Mode 7's pixels */
} VramWrite;

/* count: words for VRAM_WORDS, bytes for the others */
void vram_write(uint16_t address, const void *data, uint32_t count, VramWrite how);
/* one word count times from address on (VramFillMap, the text layer) */
void vram_fill(uint16_t address, uint16_t word, uint32_t count);

/* ---- screen set-up (video/screen_setup.c, vram_copy.c, unpack.c, mode7_screen.c) ---- */

void ScrollStateClear(void);
void DisplayDefaults(void);
void VramCopy(int entry);           /* entry of the first copy table: the record's X / 7 */
void VramCopy2(int entry);          /* entry of the second table */
void UnpackSet(int set);            /* packed set to the unpack buffer (the original's bank $7F) */
void LevelGraphicsLoad(void);       /* the level's graphics (g's level) */
void Mode7ScreenSetup(uint8_t row_flag);

/* ---- palettes (video/palette.c) ---- */

void PaletteLoad(int set);

/* ---- sprites (video/oam.c) ---- */

void OamBufferClear(void);
void OamBuildBegin(void);
void OamDrawActors(void);

/* ---- the text layer (video/tilemap.c, screen_tasks.c) ---- */

void TextLayerClear(void);
void TextLayoutShow(int layout);    /* layout A; bit 7 blanks it */

/* The task entries of video/ (the colour fades, the palette sequence, the
   brightness tasks, the text blink, the Mode 7 zoom, the shakes, the colour
   bands, the window effects, the iris) are declared here when
   system/task.h gives the task's type. */

#endif
