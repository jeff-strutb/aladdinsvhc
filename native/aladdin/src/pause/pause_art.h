/*
 * pause_art.h: the controller picture the menu shows (pad_art.c, from Stunt Race FX's pause screen: the same drawing).
 */
#ifndef ALADDIN_PAUSE_ART_H
#define ALADDIN_PAUSE_ART_H

#include <stdint.h>

/* a 5 x 7 capital's rows, bit 4 leftmost; NULL: none (pad_glyphs.c) */
const uint8_t *pause_glyph(char c);

/* The controller (pad_art.c) blended over px (w x h, 0xAARRGGBB straight alpha): its drawing's pixels at k points each,
   its (500, 300) at the point (cx, cy), `density` pixels a point. */
void pad_draw(uint32_t *px, int w, int h, float density, float k, float cx, float cy);

#endif
