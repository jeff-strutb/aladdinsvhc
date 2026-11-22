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
enum { FILL_NONE = 0, FILL_EXACT = 'B', FILL_EMPTY = 'E', FILL_CYCLE = 'R', FILL_SYNTH = 'S', FILL_BACKDROP = 'D' };

/* how the exact search of a side ended: 0 not run (no margin), made whole, made with seam pairs let go, made to some depth (the rest by the depth at a time path), proved
   (arc consistency leaves no cell that has only the level's own pairs: the side cannot be 0), given up (its work limit) */
enum { FILL_OUTCOME_NONE = 0, FILL_OUTCOME_MADE = 'M', FILL_OUTCOME_RELAXED = 'R', FILL_OUTCOME_PARTIAL = 'P', FILL_OUTCOME_PROVED = 'X', FILL_OUTCOME_GAVE_UP = 'G' };

/* how a cell of the whole was made (level_fill_kind) */
enum { FILL_KIND_LEVEL = 0, FILL_KIND_EXACT, FILL_KIND_EMPTY, FILL_KIND_CYCLE, FILL_KIND_SYNTH, FILL_KIND_FALLBACK, FILL_KIND_REPAIR };

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
    unsigned pairs[4], unseen[4];   /* by side (left, right, top, bottom): the pairs of touching metatiles in the margin and across its seam, and those the level's own layout never has */
    unsigned unfilled;              /* cells no candidate could give (left blank) */
    uint8_t outcome[4];             /* by side (left, right, top, bottom): how the exact search ended, FILL_OUTCOME_* */
} FillReport;

/* `cells` holds fw x fh metatiles row by row, the level's lw x lh with its top left cell at column ox, row oy (the margins
   left and above; those right and below are what remains); the rest is made. `field` is 1 for
   the play field (only metatiles with no collision are made new), 0 for the backdrop. `seed` fixes the arrangement. */
void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, unsigned ox, unsigned oy, int field, DataRef metatiles, uint32_t seed,
               FillReport *report);

/* the last LevelFill's provenance by cell of the whole (fw x fh): the level cell index (y * lw + x) a synthesised cell was copied from, else -1;
   and the kind. For the developer's map dump. */
const int32_t *level_fill_source(void);
const uint8_t *level_fill_kind(void);

#endif
