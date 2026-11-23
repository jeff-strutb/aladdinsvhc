/*
 * save_crop.c: the screenshot crop of a save (saves.h), apart so the checks can use it alone.
 */
#include "system/saves.h"

void saves_crop(const uint32_t *picture, int w, int h, int cx, int cy, uint32_t *out)
{
    int x0 = cx - SAVE_PIC_W / 2, y0 = cy - SAVE_PIC_H / 2, i, j;

    if (x0 > w - SAVE_PIC_W) x0 = w - SAVE_PIC_W;
    if (y0 > h - SAVE_PIC_H) y0 = h - SAVE_PIC_H;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    for (j = 0; j < SAVE_PIC_H; j++)
        for (i = 0; i < SAVE_PIC_W; i++) {
            int x = x0 + i, y = y0 + j;

            out[j * SAVE_PIC_W + i] = x < w && y < h ? picture[y * w + x] : 0xFF000000u;
        }
}
