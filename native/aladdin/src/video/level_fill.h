/*
 * level_fill.h: scenery for the space a view shows past a level's right and bottom edges
 * (video/level_fill.c, docs/aladdin/native/VIEW.md section 5).
 */
#ifndef ALADDIN_LEVEL_FILL_H
#define ALADDIN_LEVEL_FILL_H

#include <stdint.h>

#include "data/data.h"

#define FILL_BLANK 0xFFFFu          /* a cell with no tile: an empty place, or a band left to the backdrop */
#define FILL_MARGIN 64              /* metatiles (1,024 pixels) added past the level's right and bottom edges */
#define FILL_CELLS_MAX 131072       /* the most cells (16 x 16 pixel metatiles) of a layer with its margins */
#define FILL_ROWS_MAX 256
#define FILL_COLS_MAX 1024

/* how a band (a row of metatiles past the right edge, a column past the bottom edge) was filled */
enum { FILL_NONE = 0, FILL_EXACT = 'B', FILL_MADE = 'C', FILL_BACKDROP = 'D' };

typedef struct FillBand {
    uint8_t method;
    uint16_t period;                /* exact: the metatiles it repeats after */
    uint16_t gap;                   /* exact: the metatiles between the end of the repeating stretch and the edge */
} FillBand;

typedef struct FillReport {
    unsigned lw, lh, fw, fh;        /* the level and the whole, in metatiles */
    FillBand right[FILL_ROWS_MAX];  /* by row of the level */
    FillBand down[FILL_COLS_MAX];   /* by column of the whole */
    unsigned ids, ids_over;         /* the level's different metatiles, and those past the table */
} FillReport;

/* `cells` holds fw x fh metatiles row by row, the level's lw x lh at the top left; the rest is made. `field` is 1 for
   the play field (only metatiles with no collision are made new), 0 for the backdrop. `seed` fixes the arrangement. */
void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, int field, DataRef metatiles, uint32_t seed,
               FillReport *report);

#endif
