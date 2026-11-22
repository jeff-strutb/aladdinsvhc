/*
 * video_internal.h: what video/'s files share, not for the rest of the game.
 */
#ifndef ALADDIN_VIDEO_INTERNAL_H
#define ALADDIN_VIDEO_INTERNAL_H

#include <stdint.h>

#include "nshvc/nshvc.h"
#include "system/state.h"

/* video.c */
nshvc_tiles video_sprite_tiles(void);       /* the sprites' tile set (their names are its tiles 256 on) */
uint16_t vram_step(uint8_t step_byte);      /* the address step a queue record's step byte gives */
void video_mode7_off(void);                 /* DisplayDefaults: Mode 7's outside back to repeat, writes no longer reach it */
void video_mode7_build(uint8_t row_flag, const uint8_t *planar, const uint8_t *map_words);   /* Mode7ScreenSetup's conversion */

/* video.c: BG1's or BG2's video map (VIDEO_MAP_FRONT, VIDEO_MAP_BACK; 64 x 32) written whole from its words, row by row */
void video_map_write(VideoMap map, const uint16_t *words);

/* video.c: a window use nibble of the original's and its logic, as the engine's bits */
uint8_t video_window_use(uint8_t nibble, uint8_t logic);

/* hud_layer.c: the text layer in a level (VIEW.md 6.2) */
void hud_layer_init(void);                  /* from video_init */
void hud_layer_write(unsigned x, unsigned y, unsigned n, const uint16_t *words);   /* n cells of the text map's row y from column x on */
void hud_layer_set(nshvc_layer *l, int32_t scroll_x);   /* BG3 `l` drawn in a level's view, its corners' lines cut */
int video_level_view(void);                 /* 1 while a level is the screen: its view is the whole view, not the original's 256 x 224 */

/* where each level window effect's table is in g.wide_data (the LINE_WINDOW*_WIDE channels' tables) */
#define WIDE_BOXES 0x000            /* effect 8: two halves of 0x200 bytes, built in turn */
#define WIDE_SPOT 0x400             /* effect 10: 0x200 bytes, the last two the spot's line count */
#define WIDE_NARROW 0x600           /* effect 11: 0x200 bytes */
#define WIDE_CIRCLE 0x800           /* the circle wipe: view.h lines, in runs of 127 each after a count byte */
#define WIDE_NONE 0xFFFFu           /* a window's left edge when there is no window: past every right edge */

/* scroll.c: the layers' positions as the NMI sent them */
void video_scroll_latch(void);              /* ScrollRegistersWrite (80:8793), from video_vblank */
int32_t video_layer_x(int layer);
int32_t video_layer_y(int layer);

/* uploads.c: the NMI's other uploads */
void VramListsUpload(void);                 /* 80:8883 */
void VramWordsUpload(void);                 /* 80:884C */
void TextBufferPartsUpload(void);           /* 80:849E */
void VramUploadByNumber(void);              /* 80:84E2 */

/* parallax.c */
void ParallaxUpdate(void);                  /* 81:C772 (ParallaxUpdateLong 81:C77B): the backdrop's scroll from the play field's */

/* scroll_layer.c */
void LayersCopyToVram(void);                /* 80:8366 */

/* lines.c: the per-line effects */
void video_lines_send(uint32_t top);        /* from video_draw: each effect that is on, from line `top` */
int video_lines_tile_mode(void);            /* the first mode other than 7 a per-line mode change gives (1 if none) */

#endif
