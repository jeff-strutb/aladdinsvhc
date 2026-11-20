/*
 * scroll_layer.h: the scroll layers (video/scroll_layer.c), as other areas
 * call them. LayersFill is in video.h, LayersCopyToVram in video_internal.h.
 */
#ifndef ALADDIN_SCROLL_LAYER_H
#define ALADDIN_SCROLL_LAYER_H

#include <stdint.h>

void LevelScrollLimits(void);               /* 81:C032 */
void LayersSetup(void);                     /* 81:C0B6 */
void LayersScroll(void);                    /* 81:C1FA (and 81:C1F6 LayersScrollLong): once a frame */
/* 81:C177 (and 81:C173 TilemapWordLong): the play field's video map word
   address of the tile at pixel (x, y) */
uint16_t TilemapWord(uint16_t x, uint16_t y);

#endif
