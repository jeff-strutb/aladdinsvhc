/*
 * scroll_tables.h: the cartridge's tables the scroll layers read
 * (video/scroll_layer.c). Places are written once, in scroll_tables.c.
 */
#ifndef ALADDIN_SCROLL_TABLES_H
#define ALADDIN_SCROLL_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* LayerSetup (81:C0F7): $80:C648 + layer * 7 */
typedef struct LayerSetupEntry {
    uint8_t map_base_high;      /* its screen map base's high byte */
    uint8_t tilemap_base_high;  /* its video map's high byte */
    uint16_t tilemap_mask;
    uint8_t screen_flip_high;
    uint8_t column_length;      /* tiles */
    uint8_t row_length;
} LayerSetupEntry;
LayerSetupEntry layer_setup_entry(unsigned layer);     /* layer 0 the play field, 1 the backdrop */

/* EdgePlace (81:C146): $80:C656: where each edge starts from the scroll,
   x then y, left, right, top, bottom */
int16_t edge_start_x(unsigned edge);
int16_t edge_start_y(unsigned edge);

/* LayerQueueColumn, LayerQueueRow: $80:C666: the drawn position's step by side (0 right or down, 1 left or up) */
int16_t layer_draw_step(unsigned side);
/* LayerDrawColumn, LayerDrawRow: $80:C66A: the edge a side draws at (an edge's record offset / 10); 0-1 columns, 2-3 rows */
uint8_t side_edge(unsigned side);
/* LayersScroll: $80:C66E: that edge's bit in a layer's edge mask */
uint8_t side_edge_bit(unsigned side);

/* tile `half` (a byte offset: 0, 2, 4, 6) of the metatile at `metatile`
   (the original's address in the metatiles' bank, $8000 up) */
uint16_t level_metatile_word(DataRef metatiles, uint16_t metatile, unsigned half);

#endif
