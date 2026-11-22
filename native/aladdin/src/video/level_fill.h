/*
 * level_fill.h: scenery for the space a view shows past a level's four edges
 * (video/level_fill.c, docs/aladdin/native/VIEW.md section 5).
 */
#ifndef ALADDIN_LEVEL_FILL_H
#define ALADDIN_LEVEL_FILL_H

#include <stdint.h>

#include "data/data.h"

#define FILL_BLANK 0xFFFFu          /* a cell with no tile: an empty place, or a band left to the backdrop */
#define FILL_MARGIN 64              /* metatiles (1,024 pixels) added past each of the level's edges */
#define FILL_CELLS_MAX 131072       /* the most cells (16 x 16 pixel metatiles) of a layer with its margins */
#define FILL_ROWS_MAX 256
#define FILL_COLS_MAX 1024

/* how a band (a row of metatiles past the right or left edge, a column past the bottom or top edge) was filled */
enum { FILL_NONE = 0, FILL_EXACT = 'B', FILL_EMPTY = 'E', FILL_CYCLE = 'R', FILL_MADE = 'C', FILL_BACKDROP = 'D' };

typedef struct FillBand {
    uint8_t method;
    uint16_t period;                /* exact: the metatiles it repeats after */
    uint16_t gap;                   /* exact: the metatiles between the end of the repeating stretch and the edge */
} FillBand;

typedef struct FillReport {
    unsigned lw, lh, fw, fh;        /* the level and the whole, in metatiles */
    unsigned ox, oy;                /* the level's top left cell in the whole: the left and top margins */
    FillBand right[FILL_ROWS_MAX];  /* by row of the level */
    FillBand left[FILL_ROWS_MAX];   /* by row of the level */
    FillBand down[FILL_COLS_MAX];   /* by column of the whole */
    FillBand up[FILL_COLS_MAX];     /* by column of the whole */
    unsigned ids, ids_over;         /* the level's different metatiles, and those past the table */
    unsigned seam_up, seam_down;    /* 1: every column of the level continues seamlessly past the top / bottom edge (see level_fill.c) */
} FillReport;

/* `cells` holds fw x fh metatiles row by row, the level's lw x lh with its top left cell at column ox, row oy (the margins
   left and above; those right and below are what remains); the rest is made. `field` is 1 for
   the play field (only metatiles with no collision are made new), 0 for the backdrop. `seed` fixes the arrangement. */
void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, unsigned ox, unsigned oy, int field, DataRef metatiles, uint32_t seed,
               FillReport *report);

#endif
