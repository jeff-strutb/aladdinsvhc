/*
 * level_fill.c: scenery for the space a view shows past a level's four edges, from the level's own metatiles
 * (docs/aladdin/native/VIEW.md section 5; Jeff 2026-11-22: no black).
 *
 * DESIGN NOTE. The game runs as the original at 256 x 224 and a wider picture is a frame round that screen, so the view
 * shows past a level's left and top edges as well as its right and bottom ones (and a held view of the rooms and arenas,
 * scroll_x_min/max, kind $81, shows past a side). So the whole-level maps (video/level_maps.c) are made larger by
 * FILL_MARGIN metatiles past each edge and this fills them. Each layer is filled on its own, a metatile (16 x 16 pixels)
 * a cell, from the level's cells, in the order VIEW.md gives. A "band" is a row of cells for the right and left margins
 * and a column of cells for the bottom and top ones; each margin is the same method seen from its own edge (the left one
 * runs a row leftward from the level's first cell, the top one a column upward), "the edge" below being that margin's.
 *
 *   B, exact: the band's cells in the level repeat with a period p (cell i equals cell i + p) over a stretch of at
 *     least 3 periods and 4 cells, p up to 64, and ending within GAP_ACROSS (one screen) of the left or right edge or
 *     GAP_DOWN (32 pixels) of the top or bottom one: further in, a stretch is part of the level's own picture (a building's
 *     wall) and continuing it would be wrong. Of such stretches the one ending nearest the edge is used (the
 *     smaller period on a tie), so a unique feature at the edge (an exit, an end wall) is skipped, and the band goes
 *     on beyond the edge as that stretch's pattern, at the position's own phase (cell i is the stretch's cell
 *     i - k p, k least with it inside the stretch's last period). The cells are the game's own and the continuation
 *     is exact. Measured on the metatile words of the level's map: equal words are equal pixels.
 *   E, empty (the top and bottom margins only): a band whose level cell at the edge draws nothing goes on drawing
 *     nothing, so the other layer or the colour behind shows. Tried after B, since a stretch whose pattern ends in
 *     empty cells goes on as the pattern.
 *   R, cycle (the top and bottom margins only): the band's last 1 to 4 cells go on round and round, where the one step
 *     they add, the edge cell to the cycle's first, is a pair the level has somewhere: a floor or a wall of earth goes
 *     on down as the level's own earth does where it is deeper.
 *   C, made: a band with no such stretch is laid cell by cell, from the edge outward, from the level's own metatiles:
 *     a cell may be a metatile only where each side already decided is a pair that occurs somewhere in the level (the
 *     cell inward, the band before it; the band after it where that band is of kind B). The play field's left and
 *     right margins use only metatiles with no collision (kind empty), everything else any: the margins are
 *     pictures and nothing reads their kind, and a floor goes on down as earth only if the earth may be used. The choice is weighted by how often the level
 *     has each metatile, from a fixed seed (xorshift32 of the level and layer), so every play is the same. A band
 *     that finds no metatile is tried again ATTEMPTS times in all, then is D.
 *   D, backdrop: blank cells, so the other layer or the colour behind shows, as at an empty place.
 * SEAMLESS. A vertical margin (top, bottom) is seamless for a layer when each of the level's columns is R, E or B (the bottom's
 * stretch may end two cells before the edge, the top's must reach it): the art goes on exactly, or on by steps the level
 * itself has, or nothing is drawn. FillReport.seam_up and seam_down say it, and the view (video/view.c) shows past the top and bottom only where
 * both layers are. C and D are made up, not the level's own continuation, so they are not seamless.
 * The right margin is filled first (each row of the level), then the left, then the bottom margin over every column of
 * the whole, then the top, the corners included (columns that already have the side margins' cells).
 *
 * The fill is in the layers' engine maps only. The level's own map (g.level_tiles, g.screen_map) is not touched and
 * map/collision.c and map/tile_changes.c read nothing else, so nothing can stand on, hit or change a fill tile.
 */
#include <stddef.h>
#include <string.h>

#include "data/game_tables.h"
#include "data/scroll_tables.h"
#include "map/map.h"
#include "video/level_fill.h"

#define PERIOD_MAX 64
#define COPIES_MIN 3
#define CELLS_MIN 4
#define GAP_ACROSS 16               /* a stretch must end within one screen of the right edge: past the edge the unique features are the last screen's */
#define GAP_DOWN 2                  /* and within 32 pixels of the bottom edge: the original's camera never shows the last 32 */
#define IDS_MAX 1024
#define ATTEMPTS 8
#define CYCLE_MAX 4                 /* the longest cycle (cells) the R method repeats: a floor of 32 pixels is 2 */
#define NO_ID 0xFFFFu
#define NONE 0xFFFEu
#define IMPOSSIBLE (-2)
#define ABSENT (-1)

/* the level's different metatiles (ids), how often each is, whether it has no collision, and which pairs sit side by
   side (`across`: b is right of a) or one above the other (`below`: b is under a) */
static struct {
    uint16_t id_of[65536];
    uint16_t cell_of[IDS_MAX];
    uint32_t count[IDS_MAX];
    uint8_t decorative[IDS_MAX];
    uint32_t across[IDS_MAX][IDS_MAX / 32];
    uint32_t below[IDS_MAX][IDS_MAX / 32];
    unsigned ids, over;
    uint32_t rng;
    int any_kind;                   /* 1: a made cell may be a metatile of any kind (the top and bottom margins), 0: one with no collision only (the play field's sides) */
} lv;

typedef struct Stretch {
    unsigned p, b;                  /* the period; the stretch ends before cell b */
} Stretch;

static uint32_t next_random(void)
{
    uint32_t x = lv.rng;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    lv.rng = x;
    return x;
}

static int pair_in(const uint32_t (*m)[IDS_MAX / 32], unsigned a, unsigned b)
{
    return (int)(m[a][b >> 5] >> (b & 31) & 1u);
}

static void pair_set(uint32_t (*m)[IDS_MAX / 32], unsigned a, unsigned b)
{
    m[a][b >> 5] |= 1u << (b & 31);
}

/* the stretch of the n cells v[0], v[stride] ... that ends nearest the end and repeats; 0 when there is none */
static int find_stretch(const uint16_t *v, ptrdiff_t stride, unsigned n, unsigned gap_most, Stretch *best)
{
    int found = 0;

    for (unsigned p = 1; p <= PERIOD_MAX && p < n; p++) {
        unsigned run = 0;
        /* the matching positions a stretch of COPIES_MIN periods and CELLS_MIN cells has: its length less p */
        unsigned least = (COPIES_MIN - 1) * p;

        if (p < CELLS_MIN && CELLS_MIN - p > least)
            least = CELLS_MIN - p;

        for (unsigned i = 0; i + p <= n; i++) {
            int match = i + p < n && v[(ptrdiff_t)i * stride] == v[(ptrdiff_t)(i + p) * stride];

            if (match) {
                run++;
                continue;
            }
            /* the run of matching positions ends at i: the stretch is run + p cells ending at i + p */
            if (run >= least && n - (i + p) <= gap_most && (!found || i + p > best->b)) {
                best->p = p;
                best->b = i + p;
                found = 1;
            }
            run = 0;
        }
    }
    return found;
}

/* cells n.. of the band as the stretch's pattern goes on */
static void extend(uint16_t *v, ptrdiff_t stride, unsigned n, unsigned total, const Stretch *s)
{
    unsigned from = s->b - s->p;

    for (unsigned i = n; i < total; i++)
        v[(ptrdiff_t)i * stride] = v[(ptrdiff_t)(from + (i - from) % s->p) * stride];
}

/* the id of a cell as a neighbour: ABSENT for no neighbour is the caller's; a metatile the table does not hold is
   IMPOSSIBLE (nothing is known to sit beside it) */
static int neighbour(uint16_t cell)
{
    return lv.id_of[cell] == NO_ID ? IMPOSSIBLE : (int)lv.id_of[cell];
}

static int allowed(unsigned id, int left, int up, int right, int down)
{
    return (lv.decorative[id] || lv.any_kind) && (left == ABSENT || pair_in(lv.across, (unsigned)left, id)) &&
           (up == ABSENT || pair_in(lv.below, (unsigned)up, id)) && (right == ABSENT || pair_in(lv.across, id, (unsigned)right)) &&
           (down == ABSENT || pair_in(lv.below, id, (unsigned)down));
}

/* a cell of the level's metatiles beside the neighbours given (ABSENT: none; IMPOSSIBLE: the cell cannot be made),
   weighted by how often each metatile is in the level; NONE where none may be */
static uint16_t make_cell(int left, int up, int right, int down)
{
    uint64_t total = 0, pick;

    if (left == IMPOSSIBLE || up == IMPOSSIBLE || right == IMPOSSIBLE || down == IMPOSSIBLE)
        return NONE;
    for (unsigned id = 0; id < lv.ids; id++)
        if (allowed(id, left, up, right, down))
            total += lv.count[id];
    if (total == 0)
        return NONE;
    pick = next_random() % total;
    for (unsigned id = 0; id < lv.ids; id++)
        if (allowed(id, left, up, right, down)) {
            if (pick < lv.count[id])
                return lv.cell_of[id];
            pick -= lv.count[id];
        }
    return NONE;
}

/* the level's metatiles into the table: ids, counts, which have no collision, and the pairs side by side */
static void learn(const uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned ox, unsigned oy, int field, DataRef metatiles)
{
    const uint16_t *level = cells + (size_t)oy * fw + ox;

    memset(lv.id_of, 0xFF, sizeof lv.id_of);
    memset(lv.count, 0, sizeof lv.count);
    memset(lv.across, 0, sizeof lv.across);
    memset(lv.below, 0, sizeof lv.below);
    lv.ids = lv.over = 0;
    for (unsigned r = 0; r < lh; r++)
        for (unsigned c = 0; c < lw; c++) {
            uint16_t cell = level[(size_t)r * fw + c];

            if (lv.id_of[cell] == NO_ID) {
                if (lv.ids == IDS_MAX) {
                    lv.over++;
                    continue;
                }
                lv.id_of[cell] = (uint16_t)lv.ids;
                lv.cell_of[lv.ids] = cell;
                lv.decorative[lv.ids] =
                    (uint8_t)(cell == FILL_BLANK || !field ||
                              tile_kind(tile_behaviour_byte(metatiles, (uint16_t)(cell >> 3))) == TILE_KIND_EMPTY);
                lv.ids++;
            }
            lv.count[lv.id_of[cell]]++;
        }
    for (unsigned r = 0; r < lh; r++)
        for (unsigned c = 0; c < lw; c++) {
            unsigned a = lv.id_of[level[(size_t)r * fw + c]];

            if (a == NO_ID)
                continue;
            if (c + 1 < lw && lv.id_of[level[(size_t)r * fw + c + 1]] != NO_ID)
                pair_set(lv.across, a, lv.id_of[level[(size_t)r * fw + c + 1]]);
            if (r + 1 < lh && lv.id_of[level[(size_t)(r + 1) * fw + c]] != NO_ID)
                pair_set(lv.below, a, lv.id_of[level[(size_t)(r + 1) * fw + c]]);
        }
}

/* a neighbour at cell offset `delta` from a cell (-1 left, -fw above, +1 right, +fw below) into the slots make_cell takes */
static void place_neighbour(int nb[4], ptrdiff_t delta, ptrdiff_t fw, int id)
{
    nb[delta == -1 ? 0 : delta == -fw ? 1 : delta == 1 ? 2 : 3] = id;
}

/* band k of a margin laid from the level's edge outward. The band's cell i is at index `o` + i * sa of the cells (sa: +1
   right, -1 left, +fw down, -fw up); band k + 1 is sb on from band k. The cell inward of each is decided; so is band
   k - 1's; band k + 1's is where it is of kind B (`fixed`). */
static int make_band(uint16_t *cells, ptrdiff_t fw, ptrdiff_t o, ptrdiff_t sa, ptrdiff_t sb, unsigned k, unsigned bands, unsigned n,
                     unsigned total, const uint8_t *fixed)
{
    for (unsigned attempt = 0; attempt < ATTEMPTS; attempt++) {
        unsigned i = n;

        for (; i < total; i++) {
            ptrdiff_t a = o + (ptrdiff_t)i * sa;
            int nb[4] = { ABSENT, ABSENT, ABSENT, ABSENT };
            uint16_t v;

            place_neighbour(nb, -sa, fw, neighbour(cells[a - sa]));
            if (k > 0)
                place_neighbour(nb, -sb, fw, neighbour(cells[a - sb]));
            if (k + 1 < bands && fixed[k + 1])
                place_neighbour(nb, sb, fw, neighbour(cells[a + sb]));
            v = make_cell(nb[0], nb[1], nb[2], nb[3]);
            if (v == NONE)
                break;
            cells[a] = v;
        }
        if (i == total)
            return 1;
    }
    return 0;
}

/* whether a metatile draws nothing: every one of its four tiles is tile 0 (a blank place is no cell at all) */
static int edge_empty(uint16_t cell, DataRef metatiles)
{
    if (cell == FILL_BLANK)
        return 1;
    for (unsigned half = 0; half < 8; half += 2)
        if (level_metatile_word(metatiles, (uint16_t)(cell | 0x8000), half) & 0x03FFu)
            return 0;
    return 1;
}

/* the R method: the band's last p cells (p up to CYCLE_MAX) go on round and round, where the one step they add (the edge
   cell to the cycle's first) is a pair the level has somewhere (the other steps are the band's own). The cells' order
   along the band is `sa` on: for the bottom margin each cell is below the one before, for the top margin above it. */
static int cycle_extend(uint16_t *v, ptrdiff_t sa, unsigned n, unsigned total)
{
    for (unsigned p = 1; p <= CYCLE_MAX && p <= n; p++) {
        uint16_t edge = v[(ptrdiff_t)(n - 1) * sa], first = v[(ptrdiff_t)(n - p) * sa];
        unsigned a = lv.id_of[edge], b = lv.id_of[first];

        if (a == NO_ID || b == NO_ID)
            continue;
        if (sa > 0 ? !pair_in(lv.below, a, b) : !pair_in(lv.below, b, a))
            continue;
        for (unsigned i = n; i < total; i++)
            v[(ptrdiff_t)i * sa] = v[(ptrdiff_t)(n - p + (i - n) % p) * sa];
        return 1;
    }
    return 0;
}

/* one margin: `bands` bands, band k's cell i at index base + k * sb + i * sa of the cells, the level's n of them (i < n),
   `total` in all. `rows` says which gap the exact method allows (rows of the level for the left and right margins, columns
   for the top and bottom). */
static void fill_margin(uint16_t *cells, unsigned fw, ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, unsigned n,
                        unsigned total, int rows, DataRef metatiles, FillBand *report)
{
    uint8_t fixed[FILL_COLS_MAX];
    Stretch s;

    lv.any_kind = !rows;

    memset(fixed, 0, sizeof fixed);
    for (unsigned k = 0; k < bands; k++) {
        uint16_t *v = cells + base + (ptrdiff_t)k * sb;

        if (find_stretch(v, sa, n, rows ? GAP_ACROSS : GAP_DOWN, &s)) {
            extend(v, sa, n, total, &s);
            fixed[k] = 1;
            report[k].method = FILL_EXACT;
            report[k].period = (uint16_t)s.p;
            report[k].gap = (uint16_t)(n - s.b);
        } else if (!rows && edge_empty(v[(ptrdiff_t)(n - 1) * sa], metatiles)) {
            /* the level's last cell is empty (nothing drawn): the margin goes on empty */
            for (unsigned i = n; i < total; i++)
                v[(ptrdiff_t)i * sa] = FILL_BLANK;
            fixed[k] = 1;
            report[k].method = FILL_EMPTY;
        } else if (!rows && cycle_extend(v, sa, n, total)) {
            fixed[k] = 1;
            report[k].method = FILL_CYCLE;
        }
    }
    for (unsigned k = 0; k < bands; k++) {
        if (fixed[k])
            continue;
        if (make_band(cells, fw, base + (ptrdiff_t)k * sb, sa, sb, k, bands, n, total, fixed)) {
            report[k].method = FILL_MADE;
        } else {
            for (unsigned i = n; i < total; i++)
                cells[base + (ptrdiff_t)k * sb + (ptrdiff_t)i * sa] = FILL_BLANK;
            report[k].method = FILL_BACKDROP;
        }
        fixed[k] = 1;
    }
}

void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, unsigned ox, unsigned oy, int field, DataRef metatiles,
               uint32_t seed, FillReport *report)
{
    unsigned right = fw - lw - ox, below = fh - lh - oy;    /* the margins' sizes */
    ptrdiff_t w = fw, top = (ptrdiff_t)oy * w + ox;         /* the level's top left cell */

    memset(report, 0, sizeof *report);
    report->lw = lw;
    report->lh = lh;
    report->fw = fw;
    report->fh = fh;
    report->ox = ox;
    report->oy = oy;
    if (lh > FILL_ROWS_MAX || fw > FILL_COLS_MAX || (fw == lw && fh == lh))
        return;
    lv.rng = seed ? seed : 1;
    learn(cells, lw, lh, fw, ox, oy, field, metatiles);
    report->ids = lv.ids;
    report->ids_over = lv.over;
    /* the sides first (each row of the level), then the bottom and top over every column of the whole, the corners included */
    if (right)
        fill_margin(cells, fw, top, 1, w, lh, lw, lw + right, 1, metatiles, report->right);
    if (ox)
        fill_margin(cells, fw, top + (ptrdiff_t)lw - 1, -1, w, lh, lw, lw + ox, 1, metatiles, report->left);
    if (below)
        fill_margin(cells, fw, (ptrdiff_t)oy * w, w, 1, fw, lh, lh + below, 0, metatiles, report->down);
    if (oy)
        fill_margin(cells, fw, (ptrdiff_t)(oy + lh - 1) * w, -w, 1, fw, lh, lh + oy, 0, metatiles, report->up);
    /* a vertical margin is seamless when each column of the level goes on from its edge exactly: an empty edge, or a
       stretch that repeats up to the edge (the bottom's last two cells are the ones the original's camera never showed,
       so a stretch may end before them; the top's first cell is shown, so there it may not) */
    report->seam_down = below > 0;
    report->seam_up = oy > 0;
    for (unsigned k = ox; k < ox + lw; k++) {
        if (below && report->down[k].method != FILL_EMPTY && report->down[k].method != FILL_CYCLE && report->down[k].method != FILL_EXACT)
            report->seam_down = 0;
        if (oy && report->up[k].method != FILL_EMPTY && report->up[k].method != FILL_CYCLE && !(report->up[k].method == FILL_EXACT && report->up[k].gap == 0))
            report->seam_up = 0;
    }
}
