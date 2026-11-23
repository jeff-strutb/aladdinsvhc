/*
 * level_picture.h: a level's picture composed from the cartridge's level data (the same composition the developer's map
 * dump writes as LL-start.ppm), pixel for pixel what the engine draws of the loaded level's two scroll layers.
 * LevelPicture is for a screen of the game's own that sets itself up again afterwards (docs/aladdin/native/LEVELSELECT.md).
 */
#ifndef ALADDIN_LEVEL_PICTURE_H
#define ALADDIN_LEVEL_PICTURE_H

#include <stdint.h>

#define LEVEL_PICTURE_W 96      /* the crop's size, the save screen's picture size: one game pixel to one menu pixel, never scaled (LEVELSELECT.md section 4) */
#define LEVEL_PICTURE_H 72
#define PIC_SCREEN 256
#define PIC_WINDOW_H 224
#define PIC_LINE_OFFSET 1       /* the picture's first row shows the layer's row scroll y + 1 (the SNES's one line offset, kept by the engine) */
#define PIC_VRAM_WORDS 0x8000
#define PIC_ZERO_MAGENTA 0      /* colour 0 of a layer: flat magenta */
#define PIC_ZERO_BACKDROP 2     /* colour 0 of a layer: the palette's colour 0 */

/* a picture: pixels as 0xRRGGBB, and for the layers which pixels are opaque (colour not 0; 2 in a tile of the high priority) */
typedef struct Pic {
    int w, h;
    uint32_t *px;
    uint8_t *opaque;
} Pic;

typedef struct PicLoaded {
    unsigned field_x, field_y;      /* the play field's scroll at the start, in pixels */
    unsigned back_x, back_y;        /* the backdrop's, after one ParallaxUpdate */
} PicLoaded;

Pic PicNew(int w, int h);
void PicFree(Pic *p);
uint32_t PicRgb(uint16_t colour15);                     /* a 15 bit colour as the engine shows it */
unsigned PicLayerPixels(uint32_t position);             /* a layer's scroll position in whole pixels */
void PicDrawTile(Pic *p, int x, int y, uint16_t word, int zero);
uint16_t PicPlaceWord(unsigned place);                  /* the screen map's word at a place */
/* the part of a layout (screen map place `first`, cols x rows screens) at left, top, w x h pixels */
Pic PicLayout(unsigned first, unsigned cols, unsigned rows, int zero, int left, int top, int w, int h);
Pic PicCompose(const Pic *field, const Pic *back, int ox, int oy);
const uint16_t *PicVram(void);                          /* the video memory as the loads wrote it while the shadow is on */
/* the level loaded as LevelLoad does, its video writes kept in PicVram until PicShadowOff */
void PicLevelLoad(unsigned level, PicLoaded *loaded);
void PicShadowOff(void);

/* The level's start picture's crop, LEVEL_PICTURE_W x LEVEL_PICTURE_H pixels as 0xRRGGBB, row by row, into rgb. It loads the
   level (as above), so the game's state, its video memory and the tasks the level's set-up starts are the level's after it:
   call it only from a screen that sets itself up again, and free the slots (TaskSlotsFree1To6) and actors. 0 when the level has
   no map. */
int LevelPicture(unsigned level, uint32_t *rgb);

#endif
