/*
 * level_fill.c: scenery for the space a view shows past a level's four edges, made from the level's own metatiles so that
 * what the level cuts off is completed (docs/aladdin/native/VIEW.md section 5; Jeff 2026-11-22: no black, props and such
 * half cut off must be completed).
 *
 * DESIGN NOTE. The game runs as the original at 256 x 224 and a wider picture is a frame round that screen, so the view
 * shows past a level's four edges. The whole-level maps (video/level_maps.c) are made larger by FILL_MARGIN metatiles past
 * each edge and this fills them. Each layer is filled on its own, a metatile (16 x 16 pixels) a cell. A "band" is a row of
 * cells for the right and left margins and a column of cells for the bottom and top ones; "inward" is toward the level.
 *
 * THE BAR. Every pair of metatiles that touch, across or down, in a margin or across its seam with the level, should
 * occur touching the same way somewhere in the level's own layout (FillReport.unseen counts those that do not, by side).
 * A prop is then complete: no seam pair is unseen, as a half window's right-half tile beside a backdrop tile is.
 *
 * THE METHODS, in order:
 *   B, exact: the band's cells in the level repeat with a period p (cell i equals cell i + p) over a stretch of at
 *     least 3 periods and 4 cells, p up to 64, and ending within GAP_ACROSS (one screen) of the left or right edge or
 *     GAP_DOWN (32 pixels) of the top or bottom one; the one ending nearest the edge is used (the smaller period on a tie),
 *     and the band goes on beyond the edge as that stretch's pattern. A whole period is the context.
 *   E, empty (top and bottom): a band whose edge cell draws nothing goes on with that cell.
 *   R, cycle (top and bottom): the band's last 1 to 4 cells go round and round.
 *   B, E and R bands are kept only where every pair they make (along the band, with the level, and with the band before
 *   it) is seen in the level; the others are left to S.
 *   S, synthesis: the rest, a cell at a time, outward from the edge (depth by depth, along the edge). Each cell is the
 *     cell of the level whose context matches best: candidates are the level positions whose inward neighbour is the
 *     cell's inward neighbour and whose neighbour along the band is the one already set; each is scored by how many
 *     cells of the window of CTX_IN cells inward and CTX_SIDE cells each way along the band (the margin's cells made so
 *     far included) it matches exactly, and the best score wins (a tie by a fixed pseudo-random pick, from a seed of the
 *     level and layer, so every play is the same). A cell grown this way keeps whole what the level has whole: where the
 *     edge cuts a prop, the cells beyond come from a whole copy of it elsewhere in the level. Where nothing has both
 *     neighbours the cell is taken with the inward one alone, and failing that is blank (D, counted unfilled).
 * No collision kind is looked at: the margins are pictures and nothing reads their kind.
 * WHAT IS SEEN. The view is never taller than the original's and at least 288 across (video/view.c), so only the left and right
 * margins can be seen, and the top and bottom ones only where the original itself scrolls past the map's edge (the backdrop's
 * scroll is negative in level $10: the original's own picture, which the exact bands of the top margin continue as the ring
 * wrapped). The exact search is therefore made for the left and right margins; the top and bottom ones are made by the exact
 * bands and the depth at a time path, and are not worked further.
 * The right margin is filled first, then the left, then the bottom over every column of the whole, then the top, the
 * corners included.
 *
 * The fill is in the layers' engine maps only. The level's own map (g.level_tiles, g.screen_map) is not touched and
 * map/collision.c and map/tile_changes.c read nothing else, so nothing can stand on, hit or change a fill tile.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
#define CYCLE_MAX 4                 /* the longest cycle (cells) the R method repeats: a floor of 32 pixels is 2 */
#define CTX_IN 8                    /* S: the context's cells inward, and along the band each way */
#define CTX_SIDE 3
#define CAND_MAX 256                /* S: the most candidates scored for a cell; more are sampled evenly */
#define TIES_MAX 64
/* Empty places (FILL_BLANK) that run in from a level's edge could be made like the margin beyond them (trimmed bands, the band's own
   cells ending where they begin; the code is here). They are not: the original draws them blank, and the 256 x 224 pictures must be the
   original's (with them made, 9 to 132 pictures of 7 of the 19 plays differed). So they are the level's cells, as a blank cell, and a view
   that shows them (level $0A's shaft beside its empty screens) shows them blank. */
#define FILL_TRIM_VOIDS 0
#define EXT_ROUNDS 2                /* the rounds of the pair relation the depth at a time path reads: it is the fall-back of the sides that cannot be made, its costs need not be exact */
#define NO_ID 0xFFFFu
#define UNSET 0xFFFDu               /* a margin cell not yet made */

/* the level's different metatiles (ids), and which pairs sit side by side (`across`: b is right of a) or one above the other
   (`below`: b is under a) */
static struct {
    uint16_t id_of[65536];
    uint16_t run_h[IDS_MAX], run_v[IDS_MAX];        /* the longest run of each metatile in the level, across and down */
    uint16_t cell_of[IDS_MAX];                      /* the metatile of each id */
    uint32_t count[IDS_MAX];                        /* how many cells of the level hold each metatile: one is a one-off (VIEW.md 5.w) */
    uint32_t across[IDS_MAX][IDS_MAX / 32];
    uint32_t below[IDS_MAX][IDS_MAX / 32];
    uint32_t across_t[IDS_MAX][IDS_MAX / 32];       /* the same pairs by their second metatile: the ones that go before it */
    uint32_t below_t[IDS_MAX][IDS_MAX / 32];
    uint32_t vset[IDS_MAX][IDS_MAX / 32];           /* S: for a side, by the id of an inward neighbour, the ids of the cells that can go outward of it */
    unsigned ids, over;
    uint32_t rng;
    uint8_t real[FILL_CELLS_MAX];                   /* the level's cells (y * lw + x) that are the level's own: not an empty place (FILL_BLANK) */
    uint8_t tag[FILL_CELLS_MAX];                    /* the level's cells made by the fill (an empty place that runs to an edge): 1 left, 2 right, 3 top, 4 bottom; else 0 */
    unsigned trim_left[FILL_ROWS_MAX], trim_right[FILL_ROWS_MAX];       /* by row: the empty places from each edge in */
    unsigned trim_top[FILL_COLS_MAX], trim_bottom[FILL_COLS_MAX];       /* by column of the whole */
    /* the cells being filled: fw x fh, the level lw x lh with its top left at (ox, oy) */
    uint16_t *cells;
    int fw, fh, lw, lh, ox, oy;
    /* S's candidates, for the side being made, by the id of the cell inward of them */
    uint32_t bucket_start[IDS_MAX + 2];
    uint32_t *bucket_pos;           /* level positions y * lw + x */
    uint8_t out_ok[IDS_MAX];        /* the side's metatiles that have a candidate to go on outward from (they are the inward neighbour of some position) */
    uint32_t ext[IDS_MAX][IDS_MAX / 32];            /* S: pairs (u, v) that go on: seen, and the metatiles outward of u and of v again have a seen pair */
    uint32_t ext_t[IDS_MAX][IDS_MAX / 32];
    uint32_t vset_t[IDS_MAX][IDS_MAX / 32];         /* the same by the outer metatile: the ones that can sit inward of it */
    uint32_t reach[IDS_MAX][IDS_MAX / 32];          /* S: for u, the metatiles that begin a pair that goes on, outward of u: a pair (u, v) can be gone on from if v's outward set meets it */
    uint32_t has_above[IDS_MAX / 32];               /* the metatiles with a metatile of the level above them / below them somewhere: the corner bands' cells must have one, for the top and bottom margins to go on from them */
    uint32_t has_below[IDS_MAX / 32];
    uint32_t out_mask[IDS_MAX / 32];        /* the ids that have a position to go on outward from */
    uint8_t lat_ok[IDS_MAX];        /* ... and one beyond them along the band (they have a neighbour that way in the level) */
    uint32_t pos_store[FILL_CELLS_MAX];
} lv;

/* a side the play field's level ends in a wall (every row of its edge column solid): the playable area ends there and the margin beyond is not made, so
   the backdrop alone shows (VIEW.md 5.w). 0 left, 1 right. */
static int fill_wall[2];

static uint64_t quad_key(uint16_t a, uint16_t b, uint16_t c, uint16_t d);
static int quad_has(uint64_t k);
static void quads_learn(void);

/* where each margin cell of S came from: the level's cell index (y * lw + x), or -1 (the level's own, or made by another method) */
static int32_t source_of[FILL_CELLS_MAX];
static uint8_t kind_of[FILL_CELLS_MAX];        /* how each cell was made: FILL_KIND_* (level_fill.h) */

const int32_t *level_fill_source(void)
{
    return source_of;
}

const uint8_t *level_fill_kind(void)
{
    return kind_of;
}


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


/* whether the pair of metatiles a, b with b right of a (across) or under it (down) is one the level has */
static int seen_across(uint16_t a, uint16_t b)
{
    return lv.id_of[a] != NO_ID && lv.id_of[b] != NO_ID && pair_in(lv.across, lv.id_of[a], lv.id_of[b]);
}

static int seen_down(uint16_t a, uint16_t b)
{
    return lv.id_of[a] != NO_ID && lv.id_of[b] != NO_ID && pair_in(lv.below, lv.id_of[a], lv.id_of[b]);
}

/* the cells at indices i (first) and i + delta, delta one of +-1 (across) or +-fw (down): whether the pair is the level's */
static int seen_at(ptrdiff_t i, ptrdiff_t delta)
{
    uint16_t a = lv.cells[i], b = lv.cells[i + delta];

    if (delta == 1)
        return seen_across(a, b);
    if (delta == -1)
        return seen_across(b, a);
    if (delta == lv.fw)
        return seen_down(a, b);
    return seen_down(b, a);
}

/* a one-off: a metatile the level holds in one cell only (a sign, a door, the one piece of a scene). A margin never holds a copy of one (VIEW.md 5.w) */
static int fill_base(void)             /* ALADDIN_DEV_FILL_BASE=1: the fill as it was before VIEW.md 5.w (the check's "before") */
{
    static int base = -1;

    if (base < 0)
        base = getenv("ALADDIN_DEV_FILL_BASE") != NULL;
    return base;
}

static int oneoff_cell(uint16_t c)
{
    return !fill_base() && c != FILL_BLANK && c != UNSET && lv.id_of[c] != NO_ID && lv.count[lv.id_of[c]] == 1;
}

/* the level's metatiles into the table: ids and the pairs side by side */
static void learn(void)
{
    const uint16_t *level = lv.cells + (size_t)lv.oy * lv.fw + lv.ox;

    memset(lv.id_of, 0xFF, sizeof lv.id_of);
    memset(lv.across, 0, sizeof lv.across);
    memset(lv.below, 0, sizeof lv.below);
    memset(lv.across_t, 0, sizeof lv.across_t);
    memset(lv.below_t, 0, sizeof lv.below_t);
    lv.ids = lv.over = 0;
    for (int r = 0; r < lv.lh; r++)
        for (int c = 0; c < lv.lw; c++) {
            uint16_t cell = level[(size_t)r * lv.fw + c];

            if (!lv.real[(size_t)r * lv.lw + c])
                continue;
            if (lv.id_of[cell] == NO_ID) {
                if (lv.ids == IDS_MAX) {
                    lv.over++;
                    continue;
                }
                lv.cell_of[lv.ids] = cell;
                lv.count[lv.ids] = 0;
                lv.id_of[cell] = (uint16_t)lv.ids++;
            }
            if (lv.id_of[cell] != NO_ID)
                lv.count[lv.id_of[cell]]++;
        }
    for (int r = 0; r < lv.lh; r++)
        for (int c = 0; c < lv.lw; c++) {
            unsigned a = lv.id_of[level[(size_t)r * lv.fw + c]];

            if (a == NO_ID || !lv.real[(size_t)r * lv.lw + c])
                continue;
            if (c + 1 < lv.lw && lv.real[(size_t)r * lv.lw + c + 1] && lv.id_of[level[(size_t)r * lv.fw + c + 1]] != NO_ID) {
                pair_set(lv.across, a, lv.id_of[level[(size_t)r * lv.fw + c + 1]]);
                pair_set(lv.across_t, lv.id_of[level[(size_t)r * lv.fw + c + 1]], a);
            }
            if (r + 1 < lv.lh && lv.real[(size_t)(r + 1) * lv.lw + c] && lv.id_of[level[(size_t)(r + 1) * lv.fw + c]] != NO_ID) {
                pair_set(lv.below, a, lv.id_of[level[(size_t)(r + 1) * lv.fw + c]]);
                pair_set(lv.below_t, lv.id_of[level[(size_t)(r + 1) * lv.fw + c]], a);
            }
        }
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
   cell to the cycle's first) is a pair the level has somewhere (the band's own steps are the level's). 0 if none */
static int cycle_extend(uint16_t *v, ptrdiff_t sa, unsigned n, unsigned total)
{
    for (unsigned p = 1; p <= CYCLE_MAX && p <= n; p++) {
        uint16_t edge = v[(ptrdiff_t)(n - 1) * sa], first = v[(ptrdiff_t)(n - p) * sa];

        if (sa > 0 ? !seen_down(edge, first) : !seen_down(first, edge))
            continue;
        for (unsigned i = n; i < total; i++)
            v[(ptrdiff_t)i * sa] = v[(ptrdiff_t)(n - p + (i - n) % p) * sa];
        return 1;
    }
    return 0;
}

/* whether band k, laid from cells index `o` by steps `sa`, `n` cells of it the level's and `total` in all, makes only pairs
   the level has: along the band from the level's last cell, and with band k - 1 (`sb` before it) where that is `fixed` */
static int band_seen(ptrdiff_t o, ptrdiff_t sa, ptrdiff_t sb, unsigned k, unsigned n, unsigned total, const uint8_t *fixed)
{
    for (unsigned i = n - 1; i + 1 < total; i++)
        if (!seen_at(o + (ptrdiff_t)i * sa, sa))
            return 0;
    if (k > 0 && fixed[k - 1])
        for (unsigned i = n; i < total; i++)
            if (!seen_at(o + (ptrdiff_t)i * sa - sb, sb))
                return 0;
    return 1;
}

/* the exact methods on one margin: `bands` bands, band k's cell i at index base + k * sb + i * sa, the level's n of them
   (i < n), `total` in all. `rows` says which gap the exact method allows (1: the left and right margins). A band made
   only where its pairs are the level's (band_seen); the others stay UNSET for S. */
static void fill_exact(ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, const unsigned *n_of, unsigned total, int rows,
                       DataRef metatiles, FillBand *report, uint8_t *fixed, const uint8_t *banned)
{
    Stretch s;

    for (unsigned k = 0; k < bands; k++) {
        uint16_t *v = lv.cells + base + (ptrdiff_t)k * sb;
        ptrdiff_t o = base + (ptrdiff_t)k * sb;
        unsigned n = n_of[k];
        int method = 0;

        fixed[k] = 0;
        memset(&report[k], 0, sizeof report[k]);
        if (find_stretch(v, sa, n, rows ? GAP_ACROSS : GAP_DOWN, &s)) {
            extend(v, sa, n, total, &s);
            method = FILL_EXACT;
            report[k].period = (uint16_t)s.p;
            report[k].gap = (uint16_t)(n - s.b);
        } else if (!rows && edge_empty(v[(ptrdiff_t)(n - 1) * sa], metatiles)) {
            for (unsigned i = n; i < total; i++)
                v[(ptrdiff_t)i * sa] = v[(ptrdiff_t)(n - 1) * sa];
            method = FILL_EMPTY;
        } else if (!rows && cycle_extend(v, sa, n, total)) {
            method = FILL_CYCLE;
        }
        if (method && rows && (k == 0 || k + 1 == bands)) {
            /* a corner band: its cells must have a metatile above (the first) or below (the last) in the level, for the top and bottom margins */
            const uint32_t *mask = k == 0 ? lv.has_above : lv.has_below;

            for (unsigned i = n; i < total; i++) {
                unsigned id = lv.id_of[v[(ptrdiff_t)i * sa]];

                if (id == NO_ID || !(mask[id >> 5] >> (id & 31) & 1u))
                    method = 0;
            }
        }
        if (method && !banned[k] && band_seen(o, sa, sb, k, n, total, fixed)) {
            fixed[k] = 1;
            report[k].method = (uint8_t)method;
            for (unsigned i = n; i < total; i++)
                kind_of[o + (ptrdiff_t)i * sa] = method == FILL_EXACT ? FILL_KIND_EXACT : method == FILL_EMPTY ? FILL_KIND_EMPTY : FILL_KIND_CYCLE;
        } else {
            for (unsigned i = n; i < total; i++)
                v[(ptrdiff_t)i * sa] = UNSET;
        }
    }
}

/* ---- S, synthesis ---- */

typedef struct Side {
    int ux, uy;                     /* the step inward, toward the level */
    int lx, ly;                     /* the step along the band */
    int bx, by;                     /* the level's edge cell of band 0 */
    int depth, along;               /* the cells outward, the bands */
    int dmin;                       /* the first depth made: 1, or less where empty places run in from the edge (depth 0 is the edge cell, negative inside) */
    const unsigned *n_of;           /* by band: the level's cells of it that are its own (the rest, empty places to the edge, are made) */
} Side;

static uint16_t level_cell(int x, int y)
{
    return lv.cells[(size_t)(lv.oy + y) * lv.fw + lv.ox + x];
}

static int in_level(int x, int y)
{
    return x >= 0 && y >= 0 && x < lv.lw && y < lv.lh && lv.real[(size_t)y * lv.lw + x];
}

/* the level positions whose inward neighbour (by `side`) is a metatile with an id, kept by that id */
static int ext_ready;
static void build_ext(const Side *s);

static void index_side(const Side *s)
{
    unsigned count[IDS_MAX + 1];
    uint32_t fill[IDS_MAX + 1];

    memset(count, 0, sizeof count);
    for (int y = 0; y < lv.lh; y++)
        for (int x = 0; x < lv.lw; x++)
            if (in_level(x + s->ux, y + s->uy) && lv.id_of[level_cell(x + s->ux, y + s->uy)] != NO_ID)
                count[lv.id_of[level_cell(x + s->ux, y + s->uy)]]++;
    lv.bucket_start[0] = 0;
    for (unsigned i = 0; i < IDS_MAX; i++)
        lv.bucket_start[i + 1] = lv.bucket_start[i] + count[i];
    memcpy(fill, lv.bucket_start, sizeof fill);
    for (unsigned i = 0; i < IDS_MAX; i++) {
        const uint32_t *row = s->lx ? lv.across[i] : lv.below[i];
        unsigned any = 0;

        for (unsigned w = 0; w < IDS_MAX / 32; w++)
            any |= row[w] != 0;
        lv.out_ok[i] = lv.bucket_start[i + 1] > lv.bucket_start[i];
        lv.lat_ok[i] = (uint8_t)(lv.out_ok[i] && any);
    }
    /* out_mask: the metatiles a chain of copies can go on from for ever: the largest set in which each has an outward
       neighbour (a metatile of the level that sits outward of it) that is in the set too. A cell made of any other
       would be a dead end a few cells further out (a metatile only the level's edge has). */
    memset(lv.out_mask, 0, sizeof lv.out_mask);
    for (unsigned i = 0; i < IDS_MAX; i++)
        if (lv.out_ok[i])
            lv.out_mask[i >> 5] |= 1u << (i & 31);
    for (int changed = 1; changed;) {
        changed = 0;
        for (unsigned i = 0; i < lv.ids; i++) {
            unsigned hit = 0;

            if (!(lv.out_mask[i >> 5] >> (i & 31) & 1u))
                continue;
            for (unsigned w = 0; w < IDS_MAX / 32 && !hit; w++)
                hit = (lv.vset[i][w] & lv.out_mask[w]) != 0;
            if (!hit) {
                lv.out_mask[i >> 5] &= ~(1u << (i & 31));
                changed = 1;
            }
        }
    }
    lv.bucket_pos = lv.pos_store;
    memset(lv.vset, 0, sizeof lv.vset);
    for (int y = 0; y < lv.lh; y++)
        for (int x = 0; x < lv.lw; x++)
            if (in_level(x + s->ux, y + s->uy) && lv.id_of[level_cell(x + s->ux, y + s->uy)] != NO_ID && lv.id_of[level_cell(x, y)] != NO_ID)
                pair_set(lv.vset, lv.id_of[level_cell(x + s->ux, y + s->uy)], lv.id_of[level_cell(x, y)]);
    memset(lv.vset_t, 0, sizeof lv.vset_t);
    for (unsigned i = 0; i < lv.ids; i++)
        for (unsigned w = 0; w < IDS_MAX / 32; w++)
            for (uint32_t bits = lv.vset[i][w]; bits; bits &= bits - 1)
                pair_set(lv.vset_t, w * 32 + (unsigned)__builtin_ctz(bits), i);
    for (int y = 0; y < lv.lh; y++)
        for (int x = 0; x < lv.lw; x++)
            if (in_level(x + s->ux, y + s->uy) && lv.id_of[level_cell(x + s->ux, y + s->uy)] != NO_ID)
                lv.bucket_pos[fill[lv.id_of[level_cell(x + s->ux, y + s->uy)]]++] = (uint32_t)(y * lv.lw + x);
    ext_ready = 0;
}

/* the pair relation the depth at a time path reads (made only where it runs: the exact search reads the pairs themselves) */
static void build_ext(const Side *s)
{
    /* ext: the pairs (u, v) along the band that the level has and that can go on outward for ever: the metatiles that can
       go outward of u and of v again have such a pair, and so on (the largest such set of pairs, by repeated narrowing from
       the seen pairs). A depth's cells are chosen among these, so the next depths' cells can still be beside each other. */
    for (unsigned u = 0; u < IDS_MAX; u++)
        memcpy(lv.ext[u], s->lx ? lv.across[u] : lv.below[u], sizeof lv.ext[u]);
    for (int round = 0; round < EXT_ROUNDS; round++) {
        int changed = 0;

        for (unsigned u = 0; u < lv.ids; u++) {
            uint32_t *reach = lv.reach[u];

            /* reach: the metatiles that sit along the band, in a pair that goes on, from some metatile outward of u */
            memset(reach, 0, sizeof lv.reach[u]);
            for (unsigned w = 0; w < IDS_MAX / 32; w++)
                for (uint32_t bits = lv.vset[u][w]; bits; bits &= bits - 1) {
                    const uint32_t *row = lv.ext[w * 32 + (unsigned)__builtin_ctz(bits)];

                    for (unsigned i = 0; i < IDS_MAX / 32; i++)
                        reach[i] |= row[i];
                }
            for (unsigned w = 0; w < IDS_MAX / 32; w++)
                for (uint32_t bits = lv.ext[u][w]; bits; bits &= bits - 1) {
                    unsigned v = w * 32 + (unsigned)__builtin_ctz(bits), hit = 0;

                    for (unsigned i = 0; i < IDS_MAX / 32 && !hit; i++)
                        hit = (lv.vset[v][i] & reach[i]) != 0;
                    if (!hit) {
                        lv.ext[u][w] &= ~(1u << (v & 31));
                        changed = 1;
                    }
                }
        }
        if (!changed)
            break;
    }
    memset(lv.ext_t, 0, sizeof lv.ext_t);
    for (unsigned u = 0; u < lv.ids; u++)
        for (unsigned w = 0; w < IDS_MAX / 32; w++)
            for (uint32_t bits = lv.ext[u][w]; bits; bits &= bits - 1)
                pair_set(lv.ext_t, w * 32 + (unsigned)__builtin_ctz(bits), u);
    ext_ready = 1;
}

static int known_at(int x, int y)
{
    return x >= 0 && y >= 0 && x < lv.fw && y < lv.fh && lv.cells[(size_t)y * lv.fw + x] != UNSET;
}

#define CTX_MAX ((CTX_IN) * (2 * (CTX_SIDE) + 1))

/* how many cells of the context window of the target (tx, ty) (CTX_IN cells inward, CTX_SIDE each way along the band) the
   level position (px, py) matches */
static unsigned context_score(const Side *s, int tx, int ty, int px, int py)
{
    unsigned score = 0;

    for (int i = 1; i <= CTX_IN; i++)
        for (int j = -CTX_SIDE; j <= CTX_SIDE; j++) {
            int cx = tx + i * s->ux + j * s->lx, cy = ty + i * s->uy + j * s->ly;
            int sx = px + i * s->ux + j * s->lx, sy = py + i * s->uy + j * s->ly;

            if (!known_at(cx, cy) || !in_level(sx, sy))
                continue;
            if (lv.cells[(size_t)cy * lv.fw + cx] == level_cell(sx, sy))
                score++;
        }
    return score;
}

/* S works a depth at a time: for every cell along the band the candidates (a level position and its cell), then the path
   through them, one candidate per cell, of least cost: a candidate costs the context it misses, a step from one to the next
   a large cost if the pair is one the level never has, and a little if it is not the next cell of the same copy. The
   candidates of a cell are the best scorers among the level positions whose inward neighbour is the cell's, the
   continuation of the copy its inward neighbour is from (the cell beyond that neighbour's source), and the cell beyond the
   previous cell's source along the band. A cell already set (by an exact band) has that cell alone. */
#define KEEP 8                      /* the best-scoring candidates kept for a cell */
#define STATES_MAX 24               /* the most metatiles a cell's states hold of those the band allows */
#define CAND_MAX_PER (STATES_MAX + 2 * KEEP + 8)
#define COST_UNSEEN 10000
#define COST_BREAK 3                /* a candidate that does not continue its inward or its lateral neighbour's copy */

typedef struct Cand {
    uint16_t value;
    int32_t pos;                    /* the level position (y * lw + x), -1 if the cell was set already */
    int32_t cost;                   /* the least cost of a path to here */
    int16_t back;                   /* the previous cell's candidate on that path */
    int16_t node;                   /* this candidate's own cost */
} Cand;

static Cand cand[FILL_COLS_MAX][CAND_MAX_PER];
static unsigned cand_n[FILL_COLS_MAX];

static int add_candidate(int t, int32_t pos, unsigned score, int depth_break)
{
    Cand *c;

    for (unsigned i = 0; i < cand_n[t]; i++)
        if (cand[t][i].pos == pos)
            return 0;
    if (cand_n[t] >= CAND_MAX_PER)
        return 0;
    c = &cand[t][cand_n[t]++];
    c->value = level_cell((int)(pos % lv.lw), (int)(pos / lv.lw));
    c->pos = pos;
    c->node = (int16_t)((int)CTX_MAX - (int)score + depth_break * COST_BREAK);
    c->cost = 0;
    c->back = -1;
    return 1;
}

static int pair_along(const Side *s, uint16_t a, uint16_t b)
{
    return s->lx ? seen_across(a, b) : seen_down(a, b);
}

/* the cells that can go along the band after / before a cell (by its id) */
static const uint32_t *after_of(const Side *s, unsigned id)
{
    (void)s;
    return lv.ext[id];
}

static const uint32_t *before_of(const Side *s, unsigned id)
{
    (void)s;
    return lv.ext_t[id];
}

#define SET_WORDS (IDS_MAX / 32)

static uint32_t set_v[FILL_COLS_MAX][SET_WORDS];    /* the ids a cell may be, from its inward neighbour (or the one it is) */
static uint32_t set_f[FILL_COLS_MAX][SET_WORDS];    /* ... that the cells before it along the band can lead to */
static uint32_t set_z[FILL_COLS_MAX][SET_WORDS];    /* ... and that lead on to the cells after it too */

static int set_empty(const uint32_t *a)
{
    for (unsigned i = 0; i < SET_WORDS; i++)
        if (a[i])
            return 0;
    return 1;
}

/* out = (the union of the rows `row(id)` for ids in `from`) & mask, empty if from is */
static void set_step(uint32_t *out, const uint32_t *from, const uint32_t *mask, const uint32_t *(*row)(const Side *, unsigned), const Side *s)
{
    uint32_t acc[SET_WORDS];

    memset(acc, 0, sizeof acc);
    for (unsigned w = 0; w < SET_WORDS; w++)
        for (uint32_t bits = from[w]; bits; bits &= bits - 1) {
            const uint32_t *r = row(s, w * 32 + (unsigned)__builtin_ctz(bits));

            for (unsigned i = 0; i < SET_WORDS; i++)
                acc[i] |= r[i];
        }
    for (unsigned i = 0; i < SET_WORDS; i++)
        out[i] = acc[i] & mask[i];
}

/* S on one margin: every UNSET cell, depth by depth outward. Marks the bands.
   A depth is settled along the whole band at once. First, for each cell, which metatiles it may be: those the level has
   outward of its inward neighbour, narrowed forward and backward along the band to the ones whose neighbours can be pairs the
   level has (so that a path of seen pairs is found whenever there is one). Then each such metatile gets its best-matching
   position in the level, the path of least cost is taken (above), and the cells are set. */
static void synth_side(const Side *s, FillBand *report, unsigned *unfilled)
{
    uint32_t fallback_stride = (uint32_t)(lv.lw * lv.lh) / KEEP + 1;
    int32_t first_pos[IDS_MAX];

    if (s->depth <= 0 || lv.lw * lv.lh == 0)
        return;
    if (!ext_ready)
        build_ext(s);
    for (int d = s->dmin; d <= s->depth; d++) {
        int any = 0;

        /* which metatiles each cell may be */
        for (int t = 0; t < s->along; t++) {
            size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

            if (lv.cells[ti] != UNSET) {
                unsigned kid = lv.id_of[lv.cells[ti]];

                memset(set_v[t], 0, sizeof set_v[t]);
                if (kid != NO_ID)
                    set_v[t][kid >> 5] |= 1u << (kid & 31);
            } else {
                unsigned id = lv.id_of[lv.cells[ti + (ptrdiff_t)s->uy * lv.fw + s->ux]];

                any = 1;
                if (id != NO_ID) {
                    uint32_t both[SET_WORDS], corner[SET_WORDS];

                    memcpy(corner, lv.vset[id], sizeof corner);
                    if (s->lx == 0 && (t == 0 || t == s->along - 1)) {      /* a side margin's corner band: a cell the top or bottom margin can go on from */
                        const uint32_t *mask = t == 0 ? lv.has_above : lv.has_below;
                        uint32_t narrowed[SET_WORDS];

                        for (unsigned w = 0; w < SET_WORDS; w++)
                            narrowed[w] = corner[w] & mask[w];
                        if (!set_empty(narrowed))
                            memcpy(corner, narrowed, sizeof corner);
                    }

                    for (unsigned w = 0; w < SET_WORDS; w++)
                        both[w] = corner[w] & lv.out_mask[w];
                    memcpy(set_v[t], set_empty(both) ? corner : both, sizeof set_v[t]);    /* the ones that can be gone on from */
                } else
                    memset(set_v[t], 0, sizeof set_v[t]);
            }
        }
        if (!any)
            continue;
        for (int t = 0; t < s->along; t++) {
            if (t == 0)
                memcpy(set_f[t], set_v[t], sizeof set_f[t]);
            else {
                set_step(set_f[t], set_f[t - 1], set_v[t], after_of, s);
                if (set_empty(set_f[t]))
                    memcpy(set_f[t], set_v[t], sizeof set_f[t]);        /* no way on: a break, started again */
            }
        }
        for (int t = s->along - 1; t >= 0; t--) {
            if (t == s->along - 1)
                memcpy(set_z[t], set_f[t], sizeof set_z[t]);
            else {
                uint32_t back[SET_WORDS];

                set_step(back, set_z[t + 1], set_f[t], before_of, s);
                if (set_empty(back))
                    memcpy(back, set_f[t], sizeof back);
                memcpy(set_z[t], back, sizeof set_z[t]);
            }
        }
        /* the candidates: for each allowed metatile its best position, and the continuations of the copies */
        for (int t = 0; t < s->along; t++) {
            int tx = s->bx + t * s->lx - d * s->ux, ty = s->by + t * s->ly - d * s->uy;
            size_t ti = (size_t)ty * lv.fw + tx;
            uint16_t inward;
            unsigned id;
            int32_t psrc;

            cand_n[t] = 0;
            if (lv.cells[ti] != UNSET) {
                cand[t][0] = (Cand){ lv.cells[ti], -1, 0, -1, 0 };
                cand_n[t] = 1;
                continue;
            }
            inward = lv.cells[ti + (ptrdiff_t)s->uy * lv.fw + s->ux];
            id = lv.id_of[inward];
            psrc = source_of[ti + (ptrdiff_t)s->uy * lv.fw + s->ux];
            if (id != NO_ID) {
                uint32_t from = lv.bucket_start[id], to = lv.bucket_start[id + 1];
                unsigned top_score[KEEP], top_n = 0;
                int32_t top_pos[KEEP];
                uint32_t n = to - from, stride = n / CAND_MAX + 1, start = n ? next_random() % stride : 0;

                /* the first position of each allowed metatile, scored; the best STATES_MAX of them (by a count of scores) */
                unsigned tn = 0, bin[CTX_MAX + 2];
                static int32_t tpos[IDS_MAX];
                static uint8_t tscore[IDS_MAX];

                memset(first_pos, 0xFF, sizeof first_pos);
                for (uint32_t b = from; b < to; b++) {
                    uint32_t at = lv.bucket_pos[b];
                    unsigned vid = lv.id_of[level_cell((int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw))];

                    if (vid == NO_ID || first_pos[vid] >= 0 || !(set_z[t][vid >> 5] >> (vid & 31) & 1u))
                        continue;
                    first_pos[vid] = (int32_t)at;
                    tpos[tn] = (int32_t)at;
                    tscore[tn++] = (uint8_t)context_score(s, tx, ty, (int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw));
                }
                memset(bin, 0, sizeof bin);
                for (unsigned i = 0; i < tn; i++)
                    bin[tscore[i]]++;
                {
                    unsigned least = 0, room = STATES_MAX;

                    for (int sc = CTX_MAX; sc >= 0; sc--) {
                        if (bin[sc] >= room) {
                            least = (unsigned)sc;
                            break;
                        }
                        room -= bin[sc];
                        least = (unsigned)sc;
                    }
                    for (unsigned i = 0; i < tn; i++)
                        if (tscore[i] > least || (tscore[i] == least && room-- > 0))
                            add_candidate(t, tpos[i], tscore[i], 1);
                }
                /* better positions of the metatiles already allowed: the best scorers of a sample */
                for (uint32_t b = from + start; b < to; b += stride) {
                    uint32_t at = lv.bucket_pos[b];
                    unsigned vid = lv.id_of[level_cell((int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw))], score, k;

                    if (vid == NO_ID || !(set_z[t][vid >> 5] >> (vid & 31) & 1u))
                        continue;
                    score = context_score(s, tx, ty, (int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw));
                    if (top_n == KEEP && score <= top_score[KEEP - 1])
                        continue;
                    k = top_n < KEEP ? top_n++ : KEEP - 1;
                    while (k > 0 && top_score[k - 1] < score) {
                        top_score[k] = top_score[k - 1];
                        top_pos[k] = top_pos[k - 1];
                        k--;
                    }
                    top_score[k] = score;
                    top_pos[k] = (int32_t)at;
                }
                for (unsigned k = 0; k < top_n; k++)
                    add_candidate(t, top_pos[k], top_score[k], 1);
                /* the continuation of the inward neighbour's copy */
                if (psrc >= 0) {
                    int px = (int)(psrc % lv.lw) - s->ux, py = (int)(psrc / lv.lw) - s->uy;

                    if (in_level(px, py) && in_level(px + s->ux, py + s->uy) && level_cell(px + s->ux, py + s->uy) == inward) {
                        int32_t at = py * lv.lw + px;
                        unsigned vid = lv.id_of[level_cell(px, py)];

                        if (vid != NO_ID && (set_z[t][vid >> 5] >> (vid & 31) & 1u)) {
                            for (unsigned i = 0; i < cand_n[t]; i++)
                                if (cand[t][i].pos == at)
                                    cand[t][i].node = (int16_t)(cand[t][i].node - COST_BREAK);
                            add_candidate(t, at, context_score(s, tx, ty, px, py), 0);
                        }
                    }
                }
                /* the cells beyond the previous cell's candidates' sources along the band */
                if (t > 0)
                    for (unsigned i = 0; i < cand_n[t - 1]; i++) {
                        int32_t pp = cand[t - 1][i].pos;
                        int px, py;
                        unsigned vid;

                        if (pp < 0)
                            continue;
                        px = (int)(pp % lv.lw) + s->lx;
                        py = (int)(pp / lv.lw) + s->ly;
                        if (!in_level(px, py) || !in_level(px + s->ux, py + s->uy) || level_cell(px + s->ux, py + s->uy) != inward)
                            continue;
                        vid = lv.id_of[level_cell(px, py)];
                        if (vid != NO_ID && (set_z[t][vid >> 5] >> (vid & 31) & 1u))
                            add_candidate(t, py * lv.lw + px, context_score(s, tx, ty, px, py), 1);
                    }
            }
            if (cand_n[t] == 0) {
                /* nothing goes beside this inward neighbour in the level (it is a metatile only the level's edge has): the one
                   pair no one has is made, with a metatile that can be gone on from, and the cells outward of it are right again */
                for (uint32_t at = 0; at < (uint32_t)(lv.lw * lv.lh) && cand_n[t] < KEEP; at += fallback_stride) {
                    unsigned vid = lv.id_of[level_cell((int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw))];

                    if (vid != NO_ID && (lv.out_mask[vid >> 5] >> (vid & 31) & 1u))
                        add_candidate(t, (int32_t)at, context_score(s, tx, ty, (int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw)), 1);
                }
                if (cand_n[t] == 0)
                    for (uint32_t at = 0; at < (uint32_t)(lv.lw * lv.lh) && cand_n[t] < KEEP; at += fallback_stride)
                        add_candidate(t, (int32_t)at, 0, 1);
                for (unsigned i = 0; i < cand_n[t]; i++)
                    cand[t][i].node = (int16_t)(cand[t][i].node + COST_UNSEEN / 4);
            }
        }
        /* the path: least cost, a step costing COST_UNSEEN when the pair is not the level's, COST_BREAK when not the same copy's next cell */
        for (int t = 0; t < s->along; t++)
            for (unsigned j = 0; j < cand_n[t]; j++) {
                Cand *c = &cand[t][j];
                int32_t best = 0x7FFFFFFF;

                c->back = -1;
                if (t == 0) {
                    c->cost = c->node;
                    continue;
                }
                for (unsigned i = 0; i < cand_n[t - 1]; i++) {
                    const Cand *p = &cand[t - 1][i];
                    int32_t step = p->cost + c->node;

                    if (!pair_along(s, p->value, c->value)) {
                        /* a pair the level never has: the least of them is one the next depth can put right */
                        unsigned pi = lv.id_of[p->value], ci = lv.id_of[c->value], healable = 0;

                        step += COST_UNSEEN;
                        if (pi != NO_ID && ci != NO_ID)
                            for (unsigned i2 = 0; i2 < IDS_MAX / 32 && !healable; i2++)
                                healable = (lv.vset[ci][i2] & lv.reach[pi][i2]) != 0;
                        if (!healable)
                            step += COST_UNSEEN;
                    } else if (lv.id_of[p->value] != NO_ID && lv.id_of[c->value] != NO_ID &&
                             !pair_in(lv.ext, lv.id_of[p->value], lv.id_of[c->value]))
                        step += COST_UNSEEN / 2;
                    else if (!(p->pos >= 0 && c->pos == p->pos + s->lx + s->ly * lv.lw))
                        step += COST_BREAK;
                    if (step < best) {
                        best = step;
                        c->back = (int16_t)i;
                    }
                }
                c->cost = best;
            }
        {
            int16_t at = 0;
            int32_t best = 0x7FFFFFFF;

            for (unsigned j = 0; j < cand_n[s->along - 1]; j++)
                if (cand[s->along - 1][j].cost < best) {
                    best = cand[s->along - 1][j].cost;
                    at = (int16_t)j;
                }
            for (int t = s->along - 1; t >= 0; t--) {
                int tx = s->bx + t * s->lx - d * s->ux, ty = s->by + t * s->ly - d * s->uy;
                size_t ti = (size_t)ty * lv.fw + tx;
                const Cand *c = &cand[t][at];

                if (lv.cells[ti] == UNSET) {
                    lv.cells[ti] = c->value;
                    source_of[ti] = c->pos;
                    kind_of[ti] = c->node >= COST_UNSEEN / 4 ? FILL_KIND_FALLBACK : FILL_KIND_SYNTH;
                    if (!report[t].method)
                        report[t].method = FILL_SYNTH;
                    if (c->node >= COST_UNSEEN / 4)
                        (*unfilled)++;
                }
                at = c->back;
                if (at < 0)
                    at = 0;
            }
        }
    }
}

/* the unseen pairs in each side's margin (a pair that touches more than one is the top's or the bottom's if it touches a row above or
   below the level, as the top and bottom margins make the corners; else the left's or the right's), and the pairs there are */
/* which side's margin a cell is in (0 left, 1 right, 2 top, 3 bottom: the corners are the top's and bottom's, which make them), -1 for a cell of the level */
static int cell_side(int cx, int cy)
{
    if (cx >= lv.ox && cx < lv.ox + lv.lw && cy >= lv.oy && cy < lv.oy + lv.lh)
        return lv.tag[(size_t)(cy - lv.oy) * lv.lw + cx - lv.ox] - 1;       /* made inside the level: an empty place run to an edge */
    return cy < lv.oy ? 2 : cy >= lv.oy + lv.lh ? 3 : cx < lv.ox ? 0 : 1;
}

static int walled(int cx, int cy)       /* a cell of a margin that is not made: past a wall */
{
    return cy >= lv.oy && cy < lv.oy + lv.lh && ((cx < lv.ox && fill_wall[0]) || (cx >= lv.ox + lv.lw && fill_wall[1]));
}

/* the 2 x 2 blocks in each side's margin and across its seam (a block that touches two sides is the latter's), and those the level never has */
static void count_quads(FillReport *r)
{
    for (int by = 0; by + 1 < lv.fh; by++)
        for (int bx = 0; bx + 1 < lv.fw; bx++) {
            uint16_t c[4];
            int ok = 1, side = -1;

            for (int k = 0; k < 4 && ok; k++) {
                int cx = bx + (k & 1), cy = by + (k >> 1), sd;

                c[k] = lv.cells[(size_t)cy * lv.fw + cx];
                ok = c[k] != UNSET && c[k] != FILL_BLANK && !walled(cx, cy);
                sd = cell_side(cx, cy);
                if (sd > side)
                    side = sd;
            }
            if (!ok || side < 0)
                continue;
            r->quads[side]++;
            if (!quad_has(quad_key(c[0], c[1], c[2], c[3])))
                r->unseen_quads[side]++;
        }
}

static void count_unseen(FillReport *r)
{
    for (int y = 0; y < lv.fh; y++)
        for (int x = 0; x < lv.fw; x++)
            for (int dir = 0; dir < 2; dir++) {
                int x2 = x + (dir == 0), y2 = y + (dir == 1), side = -1;
                ptrdiff_t i = (ptrdiff_t)y * lv.fw + x;

                if (x2 >= lv.fw || y2 >= lv.fh || walled(x, y) || walled(x2, y2))
                    continue;
                for (int pt = 0; pt < 2; pt++) {
                    int cx = pt ? x2 : x, cy = pt ? y2 : y, sd;

                    if (cx >= lv.ox && cx < lv.ox + lv.lw && cy >= lv.oy && cy < lv.oy + lv.lh)
                        sd = lv.tag[(size_t)(cy - lv.oy) * lv.lw + cx - lv.ox] - 1;       /* made inside the level: an empty place run to an edge */
                    else
                        sd = cy < lv.oy ? 2 : cy >= lv.oy + lv.lh ? 3 : cx < lv.ox ? 0 : 1;      /* the corners belong to the top and bottom margins, which make them */
                    if (sd >= 0 && (side < 0 || sd > side))
                        side = sd;         /* a pair between a side margin's cell and a top or bottom margin's is the latter's: it is made after */
                }
                if (side < 0)
                    continue;
                r->pairs[side]++;
                if (!seen_at(i, dir == 0 ? 1 : lv.fw))
                    r->unseen[side]++;
            }
}


/* ---- 2 x 2 blocks ----
   A pair bar lets through an edge made of two tiles that each border the other somewhere. A block bar does not: every 2 x 2 block
   of touching cells in a margin, or across its seam, should be one the level's own layout has (by cell values; a block with a cell
   not yet made, or an empty place, is not judged). The table holds the level's blocks. */
#define QUAD_BITS 18
#define QUAD_EMPTY (~0ull)
static uint64_t quad_tab[1u << QUAD_BITS];

static uint64_t quad_key(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    return (uint64_t)a | (uint64_t)b << 16 | (uint64_t)c << 32 | (uint64_t)d << 48;
}

static uint32_t quad_slot(uint64_t k)
{
    return (uint32_t)((k * 0x9E3779B97F4A7C15ull) >> (64 - QUAD_BITS));
}

static void quad_add(uint64_t k)
{
    uint32_t h = quad_slot(k);

    while (quad_tab[h] != QUAD_EMPTY) {
        if (quad_tab[h] == k)
            return;
        h = (h + 1) & ((1u << QUAD_BITS) - 1);
    }
    quad_tab[h] = k;
}

static int quad_has(uint64_t k)
{
    uint32_t h = quad_slot(k);

    while (quad_tab[h] != QUAD_EMPTY) {
        if (quad_tab[h] == k)
            return 1;
        h = (h + 1) & ((1u << QUAD_BITS) - 1);
    }
    return 0;
}

static void quads_learn(void)
{
    memset(quad_tab, 0xFF, sizeof quad_tab);
    memset(lv.run_h, 0, sizeof lv.run_h);
    memset(lv.run_v, 0, sizeof lv.run_v);
    for (int r = 0; r < lv.lh; r++)
        for (int c = 0; c < lv.lw; c++) {
            const uint16_t *p = lv.cells + (size_t)(lv.oy + r) * lv.fw + lv.ox + c;
            unsigned id = lv.id_of[*p], n = 1;

            if (id == NO_ID || !lv.real[(size_t)r * lv.lw + c])
                continue;
            if (c > 0 && p[-1] == *p)
                continue;                               /* not the first of its run across */
            while (c + (int)n < lv.lw && p[n] == *p)
                n++;
            if (n > lv.run_h[id])
                lv.run_h[id] = (uint16_t)n;
        }
    for (int c = 0; c < lv.lw; c++)
        for (int r = 0; r < lv.lh; r++) {
            const uint16_t *p = lv.cells + (size_t)(lv.oy + r) * lv.fw + lv.ox + c;
            unsigned id = lv.id_of[*p], n = 1;

            if (id == NO_ID || !lv.real[(size_t)r * lv.lw + c])
                continue;
            if (r > 0 && p[-lv.fw] == *p)
                continue;
            while (r + (int)n < lv.lh && p[(size_t)n * lv.fw] == *p)
                n++;
            if (n > lv.run_v[id])
                lv.run_v[id] = (uint16_t)n;
        }
    for (int r = 0; r + 1 < lv.lh; r++)
        for (int c = 0; c + 1 < lv.lw; c++)
            if (lv.real[(size_t)r * lv.lw + c] && lv.real[(size_t)r * lv.lw + c + 1] && lv.real[(size_t)(r + 1) * lv.lw + c] &&
                lv.real[(size_t)(r + 1) * lv.lw + c + 1]) {
                const uint16_t *p = lv.cells + (size_t)(lv.oy + r) * lv.fw + lv.ox + c;

                quad_add(quad_key(p[0], p[1], p[lv.fw], p[lv.fw + 1]));
            }
}

/* the blocks of the cell at (x, y) with value v that are judged (all four cells made, none an empty place) and are not the level's */
static unsigned quads_broken(int x, int y, uint16_t v)
{
    unsigned n = 0;

    for (int by = y - 1; by <= y; by++)
        for (int bx = x - 1; bx <= x; bx++) {
            uint16_t c[4];
            int ok = bx >= 0 && by >= 0 && bx + 1 < lv.fw && by + 1 < lv.fh;

            for (int k = 0; k < 4 && ok; k++) {
                int cx = bx + (k & 1), cy = by + (k >> 1);

                c[k] = cx == x && cy == y ? v : lv.cells[(size_t)cy * lv.fw + cx];
                ok = c[k] != UNSET && c[k] != FILL_BLANK && !walled(cx, cy);
            }
            if (ok && !quad_has(quad_key(c[0], c[1], c[2], c[3])))
                n++;
        }
    return n;
}

/* 1 for each direction in which a run of the metatile v through (x, y), the cells made or the level's, is longer than any run of it the level has:
   a margin that goes on with one tile for longer than the level ever does is a flat strip, not the level's art */
static unsigned runs_broken(int x, int y, uint16_t v)
{
    unsigned id = lv.id_of[v], n = 0, a;

    if (id == NO_ID)
        return 0;
    for (int dir = 0; dir < 2; dir++) {
        int dx = dir == 0, dy = dir == 1;

        a = 1;
        for (int k = 1; x - k * dx >= 0 && y - k * dy >= 0 && lv.cells[(size_t)(y - k * dy) * lv.fw + x - k * dx] == v; k++)
            a++;
        for (int k = 1; x + k * dx < lv.fw && y + k * dy < lv.fh && lv.cells[(size_t)(y + k * dy) * lv.fw + x + k * dx] == v; k++)
            a++;
        if (a > (dir == 0 ? lv.run_h[id] : lv.run_v[id]))
            n++;
    }
    return n;
}

static int csp_quad_mode;           /* 0: blocks not judged; 1: a cell that breaks a block is refused; 2: those are tried last */

/* ---- the exact search: constraint satisfaction over a margin's cells ----
   Variables: every cell of the side (depth 1 to s->depth, band 0 to s->along - 1), its domain a set of metatile ids. Constraints:
   the pair with the cell inward (a metatile the level has outward of it: vset; at depth 1 the level's edge cell), and the
   pair with the next cell along the band (the level's across or down pairs). A cell already set (an exact band) is a single
   value. Domains start as the metatiles a chain of copies can go on from for ever (out_mask), the corner bands' cells
   narrowed to ones a top or bottom margin can go on from, and are made arc consistent (a value stays only where each
   neighbour has a value that goes with it). The cells are then given values in depth-major order, the value order by
   context score (ties by a seeded hash), each choice propagated, and a dead end backs up to the choice before it, across
   depths. What is made has no pair the level never has. A budget of nodes ends the search (1 returned for a fill, 0 for none). */
#define CSP_VARS_MAX (FILL_MARGIN * FILL_COLS_MAX)
/* the deepest side searched: a side's depth is its margin, which is not held to FILL_MARGIN (the level select's pictures
   have a right margin of 97 on one level) but is within the whole's width or height; LevelFill refuses a whole wider than
   FILL_COLS_MAX. The tables by depth (csp_reach, csp_acc_cache, csp_acc_known) are sized by it. */
#define CSP_DEPTH_MAX FILL_COLS_MAX
#define CSP_TRAIL_MAX 40000
#define CSP_CANDS 24                /* the values tried for a cell, the best by context score */
#define CSP_NODES(vars) (8u * (vars) + 20000u)

static uint32_t csp_dom[CSP_VARS_MAX][SET_WORDS];
static struct { uint32_t var; uint32_t old[SET_WORDS]; } csp_trail[CSP_TRAIL_MAX];
static unsigned csp_trail_n;
static uint32_t csp_queue[CSP_VARS_MAX];
static unsigned csp_qhead, csp_qlen;
static uint8_t csp_queued[CSP_VARS_MAX];
static int csp_depth, csp_along;
static const Side *csp_side_cur;
static int csp_relax;               /* 0: every pair must be the level's; 1: the seam pair of the bands that have no chain to the end of the margin is let go; 2: every seam pair; 3: and the pairs along the band at depth 1 */

static int csp_tail_forever;        /* 1: the last depth's cells must be ones a chain can go on from for ever (the margin goes on past it) */
static uint8_t csp_virgin[CSP_VARS_MAX];               /* the cell's domain is still the one it began with (the metatiles a chain of the rest can follow) */
static uint32_t csp_acc_cache[4][CSP_DEPTH_MAX + 2][SET_WORDS];   /* what the revision of a neighbour by a virgin cell gives, by relation and depth */
static uint8_t csp_acc_known[4][CSP_DEPTH_MAX + 2];
static int csp_gave_up;             /* the last search ended on its work or trail limit, not on a proof */
static uint64_t csp_ops, csp_ops_limit;      /* the work done (revisions' word operations) and the most a search may do: past it the search is given up */

static int csp_enqueue(uint32_t v)
{
    if (csp_queued[v])
        return 1;
    csp_queued[v] = 1;
    csp_queue[(csp_qhead + csp_qlen++) % CSP_VARS_MAX] = v;
    return 1;
}

/* n's domain narrowed to what goes with some value of c's: rel(v) for each value v of c. 0 if n's domain is empty or the trail is full. */
static int csp_revise(uint32_t n, uint32_t c, const uint32_t (*rel)[SET_WORDS], int relid)
{
    uint32_t acc[SET_WORDS], now[SET_WORDS];
    int changed = 0, depth_of_c = (int)(c / (unsigned)csp_along) + 1;

    if (csp_virgin[c] && csp_acc_known[relid][depth_of_c]) {
        memcpy(acc, csp_acc_cache[relid][depth_of_c], sizeof acc);
    } else {
        memset(acc, 0, sizeof acc);
        for (unsigned w = 0; w < SET_WORDS; w++)
            for (uint32_t bits = csp_dom[c][w]; bits; bits &= bits - 1) {
                const uint32_t *r = rel[w * 32 + (unsigned)__builtin_ctz(bits)];

                csp_ops += SET_WORDS;
                for (unsigned i = 0; i < SET_WORDS; i++)
                    acc[i] |= r[i];
            }
        if (csp_virgin[c]) {
            memcpy(csp_acc_cache[relid][depth_of_c], acc, sizeof acc);
            csp_acc_known[relid][depth_of_c] = 1;
        }
    }
    for (unsigned i = 0; i < SET_WORDS; i++) {
        now[i] = csp_dom[n][i] & acc[i];
        changed |= now[i] != csp_dom[n][i];
    }
    if (!changed)
        return 1;
    if (csp_trail_n == CSP_TRAIL_MAX || csp_ops > csp_ops_limit) {
        csp_gave_up = 1;
        return 0;
    }
    csp_trail[csp_trail_n].var = n;
    memcpy(csp_trail[csp_trail_n].old, csp_dom[n], sizeof csp_dom[n]);
    csp_trail_n++;
    memcpy(csp_dom[n], now, sizeof now);
    csp_virgin[n] = 0;
    if (set_empty(now))
        return 0;
    csp_enqueue(n);
    return 1;
}

static void csp_undo(unsigned mark)
{
    while (csp_trail_n > mark) {
        csp_trail_n--;
        memcpy(csp_dom[csp_trail[csp_trail_n].var], csp_trail[csp_trail_n].old, sizeof csp_dom[0]);
    }
    for (; csp_qlen; csp_qlen--)
        csp_queued[csp_queue[csp_qhead++ % CSP_VARS_MAX]] = 0;
    csp_qhead = 0;
}

/* arc consistency from the queued cells; 0 on a domain with no value (the queue is then cleared) */
static int csp_propagate(void)
{
    const Side *s = csp_side_cur;
    const uint32_t (*after)[SET_WORDS] = s->lx ? lv.across : lv.below;
    const uint32_t (*before)[SET_WORDS] = s->lx ? lv.across_t : lv.below_t;

    while (csp_qlen) {
        uint32_t c = csp_queue[csp_qhead++ % CSP_VARS_MAX];
        int d = (int)(c / (unsigned)csp_along) + 1, t = (int)(c % (unsigned)csp_along);

        csp_qlen--;
        csp_queued[c] = 0;
        if ((d < csp_depth && !csp_revise(c + (uint32_t)csp_along, c, lv.vset, 0)) ||
            (d > 1 && !csp_revise(c - (uint32_t)csp_along, c, lv.vset_t, 1)) ||
            (!(csp_relax >= 3 && d == 1) && t + 1 < csp_along && !csp_revise(c + 1, c, after, 2)) ||
            (!(csp_relax >= 3 && d == 1) && t > 0 && !csp_revise(c - 1, c, before, 3))) {
            for (; csp_qlen; csp_qlen--)
                csp_queued[csp_queue[csp_qhead++ % CSP_VARS_MAX]] = 0;
            csp_qhead = 0;
            return 0;
        }
    }
    csp_qhead = 0;
    return 1;
}

static int csp_single(uint32_t v)
{
    int found = -1;

    for (unsigned w = 0; w < SET_WORDS; w++)
        if (csp_dom[v][w]) {
            if (found >= 0 || (csp_dom[v][w] & (csp_dom[v][w] - 1)))
                return -1;
            found = (int)(w * 32 + (unsigned)__builtin_ctz(csp_dom[v][w]));
        }
    return found;
}

#define CSP_POOL (1u << 20)
static uint16_t csp_pool_val[CSP_POOL];     /* the candidate values of the choices on the stack, each in the order to try */
static int32_t csp_pool_pos[CSP_POOL];      /* the level position each is taken from, -1 if none */
static unsigned csp_pool_top;

typedef struct CspFrame {
    uint32_t var;
    unsigned mark;                  /* the trail before this choice */
    unsigned off, n, next;          /* its candidates in the pool, how many, the next to try */
} CspFrame;

static CspFrame csp_frames[CSP_VARS_MAX];

/* the values of a cell's domain in the order to try them: best context first (and the copy of its neighbours' sources first of all) */
static int csp_order_cmp(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;

    return x < y ? -1 : x > y;
}

static void csp_candidates(const Side *s, int d, int t, CspFrame *f)
{
    int tx = s->bx + t * s->lx - d * s->ux, ty = s->by + t * s->ly - d * s->uy;
    size_t ti = (size_t)ty * lv.fw + tx;
    unsigned id = lv.id_of[lv.cells[ti + (ptrdiff_t)s->uy * lv.fw + s->ux]];
    static int16_t best[IDS_MAX];
    static int32_t best_pos[IDS_MAX];
    static uint64_t order[IDS_MAX];
    int32_t psrc = source_of[ti + (ptrdiff_t)s->uy * lv.fw + s->ux];
    int32_t lsrc = t > 0 ? source_of[ti - (ptrdiff_t)s->ly * lv.fw - s->lx] : -1;
    uint32_t v = (uint32_t)(d - 1) * (uint32_t)csp_along + (uint32_t)t;
    unsigned count = 0;

    memset(best, 0xFF, sizeof best);
    if (id != NO_ID) {
        uint32_t from = lv.bucket_start[id], to = lv.bucket_start[id + 1], n = to - from, stride = n / (2 * CAND_MAX) + 1;

        for (uint32_t b = from + (n ? next_random() % stride : 0); b < to; b += stride) {
            uint32_t at = lv.bucket_pos[b];
            unsigned vid = lv.id_of[level_cell((int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw))];
            int sc;

            if (vid == NO_ID || !(csp_dom[v][vid >> 5] >> (vid & 31) & 1u))
                continue;
            sc = (int)context_score(s, tx, ty, (int)(at % (unsigned)lv.lw), (int)(at / (unsigned)lv.lw));
            if (sc > best[vid]) {
                best[vid] = (int16_t)sc;
                best_pos[vid] = (int32_t)at;
            }
        }
        if (csp_relax && d == 1) {
            /* the seam pair is let go here, so no bucket holds what goes beside this edge cell: the cells of the level that best
               match the edge's surroundings are found over the whole level (a value chosen with no context at all is a flat strip) */
            uint32_t total = (uint32_t)(lv.lw * lv.lh), step = total / (4 * CAND_MAX) + 1;

            for (uint32_t at = next_random() % step; at < total; at += step) {
                int px = (int)(at % (unsigned)lv.lw), py = (int)(at / (unsigned)lv.lw);
                unsigned vid = lv.id_of[level_cell(px, py)];
                int sc;

                if (vid == NO_ID || !in_level(px, py) || !(csp_dom[v][vid >> 5] >> (vid & 31) & 1u))
                    continue;
                sc = (int)context_score(s, tx, ty, px, py);
                if (sc > best[vid]) {
                    best[vid] = (int16_t)sc;
                    best_pos[vid] = (int32_t)at;
                }
            }
        }
        /* the continuations of the copies the neighbours are part of: first */
        for (int p = 0; p < 2; p++) {
            int32_t src = p ? lsrc : psrc;
            int px, py;

            if (src < 0)
                continue;
            px = (int)(src % lv.lw) + (p ? s->lx : -s->ux);
            py = (int)(src / lv.lw) + (p ? s->ly : -s->uy);
            if (in_level(px, py)) {
                unsigned vid = lv.id_of[level_cell(px, py)];

                if (vid != NO_ID && (csp_dom[v][vid >> 5] >> (vid & 31) & 1u)) {
                    best[vid] = (int16_t)(1000 - p);
                    best_pos[vid] = py * lv.lw + px;
                }
            }
        }
    }
    for (unsigned w = 0; w < SET_WORDS; w++)
        for (uint32_t bits = csp_dom[v][w]; bits; bits &= bits - 1) {
            unsigned vid = w * 32 + (unsigned)__builtin_ctz(bits), broken = 0;
            int sc = best[vid] < 0 ? 0 : best[vid];

            if (csp_quad_mode) {
                broken = quads_broken(tx, ty, lv.cell_of[vid]) + runs_broken(tx, ty, lv.cell_of[vid]);
                if (broken && csp_quad_mode == 1)
                    continue;
            }
            /* the best match to the level's art first; of equals, the fewest blocks and runs the level never has (a flat strip is no art) */
            order[count++] = ((uint64_t)(2000 - sc) << 40) | ((uint64_t)(broken > 7 ? 7 : broken) << 32) |
                             ((uint64_t)((vid * 2654435761u ^ (uint32_t)lv.rng) & 0xFFFFu) << 16) | vid;
        }
    qsort(order, count, sizeof order[0], csp_order_cmp);
    f->off = csp_pool_top;
    f->n = 0;
    for (unsigned k = 0; k < count && csp_pool_top < CSP_POOL; k++) {
        unsigned vid = (unsigned)(order[k] & 0xFFFFu);

        csp_pool_val[csp_pool_top] = (uint16_t)vid;
        csp_pool_pos[csp_pool_top] = best[vid] < 0 ? -1 : best_pos[vid];
        csp_pool_top++;
        f->n++;
    }
    f->next = 0;
}

static uint8_t csp_assigned[CSP_VARS_MAX];
static unsigned csp_last_nodes, csp_last_budget, csp_last_v;

static size_t csp_cell(const Side *s, uint32_t v)
{
    int d = (int)(v / (unsigned)s->along) + 1, t = (int)(v % (unsigned)s->along);

    return (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;
}

static void csp_clear_from(const Side *s, uint32_t from, uint32_t to)
{
    for (uint32_t j = from; j < to; j++)
        if (csp_assigned[j]) {
            lv.cells[csp_cell(s, j)] = UNSET;
            source_of[csp_cell(s, j)] = -1;
            csp_assigned[j] = 0;
        }
}

/* 1 if the side is made whole (its cells set), else 0 with its cells as they were */
static uint32_t csp_reach[CSP_DEPTH_MAX + 2][SET_WORDS];     /* [j]: the metatiles a chain of j more cells can go outward from */


static int csp_side(const Side *s, FillBand *report, int corner, int ac_only, int relax)
{
    unsigned vars = (unsigned)(s->depth * s->along), sp = 0, nodes = 0, budget = csp_quad_mode == 1 ? 2u * vars + 4000u : CSP_NODES(vars);
    uint32_t v, hw = 0;

    if (s->depth <= 0 || s->depth > CSP_DEPTH_MAX || s->dmin != 1 || vars > CSP_VARS_MAX || lv.lw * lv.lh == 0)
        return 0;
    /* a cell at depth d must be a metatile that j = depth - d more can follow: reach[0] is all, reach[j + 1] those with an outward
       neighbour in reach[j] */
    for (unsigned w = 0; w < SET_WORDS; w++)
        csp_reach[0][w] = 0;
    for (unsigned i = 0; i < lv.ids; i++)
        csp_reach[0][i >> 5] |= 1u << (i & 31);
    for (int j = 0; j < s->depth; j++) {
        memset(csp_reach[j + 1], 0, sizeof csp_reach[0]);
        for (unsigned i = 0; i < lv.ids; i++) {
            unsigned hit = 0;

            for (unsigned w = 0; w < SET_WORDS && !hit; w++)
                hit = (lv.vset[i][w] & csp_reach[j][w]) != 0;
            if (hit)
                csp_reach[j + 1][i >> 5] |= 1u << (i & 31);
        }
    }
    csp_relax = relax;
    csp_ops = 0;
    csp_gave_up = 0;
    csp_ops_limit = 60000000ull;
    csp_side_cur = s;
    csp_depth = s->depth;
    csp_along = s->along;
    csp_trail_n = 0;
    csp_qhead = csp_qlen = 0;
    memset(csp_queued, 0, vars);
    memset(csp_assigned, 0, vars);
    for (int d = 1; d <= s->depth; d++)
        for (int t = 0; t < s->along; t++) {
            uint32_t var = (uint32_t)(d - 1) * (uint32_t)s->along + (uint32_t)t;
            size_t ti = csp_cell(s, var);
            uint32_t *dom = csp_dom[var];

            if (lv.cells[ti] != UNSET) {
                unsigned kid = lv.id_of[lv.cells[ti]];

                memset(dom, 0, sizeof csp_dom[0]);
                if (kid == NO_ID)
                    return 0;
                dom[kid >> 5] |= 1u << (kid & 31);
            } else {
                memcpy(dom, csp_tail_forever && d == s->depth ? lv.out_mask : csp_reach[s->depth - d], sizeof csp_dom[0]);
                if (d == 1) {
                    unsigned eid = lv.id_of[lv.cells[ti + (ptrdiff_t)s->uy * lv.fw + s->ux]];

                    if (eid == NO_ID)
                        return 0;
                    uint32_t seam[SET_WORDS];
                    unsigned forced = 1;

                    for (unsigned w = 0; w < SET_WORDS; w++) {
                        seam[w] = dom[w] & lv.vset[eid][w];
                        forced &= seam[w] == 0;
                    }
                    if (relax == 0 || (relax == 1 && !forced))
                        memcpy(dom, seam, sizeof seam);     /* the seam pair must be the level's (else it is let go, and counts) */
                }
                if (corner && s->lx == 0 && (t == 0 || t == s->along - 1)) {
                    const uint32_t *mask = t == 0 ? lv.has_above : lv.has_below;

                    for (unsigned w = 0; w < SET_WORDS; w++)
                        dom[w] &= mask[w];
                }
            }
            if (set_empty(dom))
                return 0;
            csp_virgin[var] = lv.cells[ti] == UNSET && memcmp(dom, csp_reach[s->depth - d], sizeof csp_dom[0]) == 0;
        }
    memset(csp_acc_known, 0, sizeof csp_acc_known);
    for (v = 0; v < vars; v++)
        csp_enqueue(v);
    if (!csp_propagate()) {
        csp_undo(0);
        return 0;
    }
    csp_trail_n = 0;                /* the first narrowing stays */
    csp_pool_top = 0;
    if (ac_only)
        return 1;
    for (v = 0; v < vars;) {
        CspFrame *f;
        int placed = 0, resumed = 0;

        if (sp && csp_frames[sp - 1].var == v) {
            f = &csp_frames[sp - 1];
            csp_pool_top = f->off + f->n;
            csp_undo(f->mark);
            resumed = 1;
        } else {
            int single = csp_single(v);

            if (single >= 0) {              /* decided by what came before: no choice here */
                if (lv.cells[csp_cell(s, v)] == UNSET) {
                    size_t at = csp_cell(s, v);

                    if (csp_quad_mode == 1 && (quads_broken((int)(at % (size_t)lv.fw), (int)(at / (size_t)lv.fw), lv.cell_of[single]) ||
                                               runs_broken((int)(at % (size_t)lv.fw), (int)(at / (size_t)lv.fw), lv.cell_of[single]))) {
                        /* a block the level never has, and no other choice: back up to the choice before */
                        if (!sp)
                            break;
                        csp_clear_from(s, csp_frames[sp - 1].var, hw);
                        hw = csp_frames[sp - 1].var;
                        v = csp_frames[sp - 1].var;
                        if (nodes++ >= budget)
                            break;
                        continue;
                    }
                    lv.cells[at] = lv.cell_of[single];
                    source_of[at] = -1;
                    csp_assigned[v] = 1;
                }
                if (v + 1 > hw)
                    hw = v + 1;
                v++;
                continue;
            }
            f = &csp_frames[sp++];
            f->var = v;
            f->mark = csp_trail_n;
            csp_candidates(s, (int)(v / (unsigned)s->along) + 1, (int)(v % (unsigned)s->along), f);
        }
        (void)resumed;
        while (f->next < f->n && nodes < budget) {
            unsigned vid = csp_pool_val[f->off + f->next];
            int32_t pos = csp_pool_pos[f->off + f->next];

            f->next++;
            nodes++;
            if (csp_trail_n + 1 >= CSP_TRAIL_MAX) {
                nodes = budget;
                break;
            }
            csp_trail[csp_trail_n].var = v;
            memcpy(csp_trail[csp_trail_n].old, csp_dom[v], sizeof csp_dom[0]);
            csp_trail_n++;
            memset(csp_dom[v], 0, sizeof csp_dom[0]);
            csp_dom[v][vid >> 5] |= 1u << (vid & 31);
            csp_virgin[v] = 0;
            csp_enqueue(v);
            if (csp_propagate()) {
                lv.cells[csp_cell(s, v)] = lv.cell_of[vid];
                source_of[csp_cell(s, v)] = pos;
                csp_assigned[v] = 1;
                placed = 1;
                break;
            }
            csp_undo(f->mark);
        }
        if (placed) {
            if (v + 1 > hw)
                hw = v + 1;
            v++;
            continue;
        }
        if (nodes >= budget)
            break;
        /* every candidate failed: back up to the choice before, across depths */
        csp_pool_top = csp_frames[sp - 1].off;
        sp--;
        if (!sp)
            break;
        csp_clear_from(s, csp_frames[sp - 1].var, hw);
        hw = csp_frames[sp - 1].var;
        v = csp_frames[sp - 1].var;
    }
    csp_last_nodes = nodes;
    csp_last_budget = budget;
    csp_last_v = v;
    if (v < vars) {
        csp_clear_from(s, 0, vars);
        csp_undo(0);
        return 0;
    }
    for (int t = 0; t < s->along; t++)
        if (!report[t].method)
            report[t].method = FILL_SYNTH;
    for (uint32_t j = 0; j < vars; j++)
        if (csp_assigned[j])
            kind_of[csp_cell(s, j)] = FILL_KIND_SYNTH;
    csp_undo(0);
    return 1;
}

/* the exact bands of a margin that a synthesised neighbour could not be made to match (an unseen pair between the two), for
   the next round to leave to S; returns how many are new */
static unsigned conflicts(const Side *s, const FillBand *report, uint8_t *banned)
{
    unsigned found = 0;

    for (int d = s->dmin; d <= s->depth; d++)
        for (int t = 0; t + 1 < s->along; t++) {
            int tx = s->bx + t * s->lx - d * s->ux, ty = s->by + t * s->ly - d * s->uy;
            size_t ti = (size_t)ty * lv.fw + tx;

            if (pair_along(s, lv.cells[ti], lv.cells[ti + (ptrdiff_t)s->ly * lv.fw + s->lx]))
                continue;
            for (int k = t; k <= t + 1; k++) {
                int lo = k, hi = k;

                if (!(report[k].method == FILL_EXACT || report[k].method == FILL_EMPTY || report[k].method == FILL_CYCLE))
                    continue;
                /* the whole run of exact bands it is in: a neighbour made to fit it would only move the trouble to the next */
                while (lo > 0 && (report[lo - 1].method == FILL_EXACT || report[lo - 1].method == FILL_EMPTY || report[lo - 1].method == FILL_CYCLE))
                    lo--;
                while (hi + 1 < s->along && (report[hi + 1].method == FILL_EXACT || report[hi + 1].method == FILL_EMPTY || report[hi + 1].method == FILL_CYCLE))
                    hi++;
                for (int j = lo; j <= hi; j++)
                    if (!banned[j]) {
                        banned[j] = 1;
                        found++;
                    }
            }
        }
    return found;
}

/* a margin made whole: the exact bands, then S; where an exact band and its neighbour do not fit, up to ROUNDS times
   again with those bands left to S */
#define ROUNDS 3
/* the side's cells unset and the exact bands laid (those not banned) */
static void prepare_margin(const Side *s, ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, unsigned total, int rows,
                           DataRef metatiles, FillBand *report, uint8_t *fixed, const uint8_t *banned)
{
    for (int d = s->dmin; d <= s->depth; d++)
        for (int t = 0; t < s->along; t++) {
            size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

            lv.cells[ti] = UNSET;
            source_of[ti] = -1;
            kind_of[ti] = 0;
        }
    memset(report, 0, bands * sizeof *report);
    fill_exact(base, sa, sb, bands, s->n_of, total, rows, metatiles, report, fixed, banned);
}

static void make_margin_body(const Side *s, ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, unsigned total, int rows,
                             DataRef metatiles, FillBand *report, unsigned *unfilled, uint8_t *outcome, uint8_t *blocks)
{
    uint8_t fixed[FILL_COLS_MAX], banned[FILL_COLS_MAX];

    /* The exact search. Three set-ups: with the exact bands as they are and the corner bands' cells ones the top and bottom margins
       can go on from; without the exact bands; and without the corner condition. For each, arc consistency says how many depths
       can be made with no pair the level never has (the most, by halving: fewer depths is never harder). A side that can be made all
       the way is made. One that cannot is made as far as it can, and the depths beyond are made by the depth at a time path, so
       that the pairs the level never has are as far from the seam as can be. */
    int best_attempt = -1, best_depth = 0;
    unsigned exact_kept = 0;

    csp_quad_mode = 0;
    if (!rows) {
        /* the top and bottom margins (the backdrop's, and only where the original's own window goes past the map): the exact bands; what they
           do not make stays empty */
        memset(banned, 0, sizeof banned);
        prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
        return;
    }
    index_side(s);
    {
        /* the weakest set-up (no exact bands, no corner condition) at depth 1: if arc consistency ends it, nothing can be made, and no
           set-up is worth trying */
        Side first = *s;

        memset(banned, 1, sizeof banned);
        prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
        first.depth = 1;
        csp_tail_forever = 0;
        csp_quad_mode = 0;
        if (!csp_side(&first, report, 0, 1, 0) && !csp_gave_up)
            *outcome = FILL_OUTCOME_PROVED;
    }
    /* whether the whole depth can be made: first with the exact bands as they are, then without them, then without the corner condition
       (the exact bands and the corner condition only add constraints, so the last is the weakest and the deepest) */
    for (int pass = 0; pass < 2 && *outcome != FILL_OUTCOME_PROVED; pass++)
    for (int attempt = 0; attempt < 3; attempt++) {
        csp_quad_mode = 1 + pass;                       /* first with no block the level never has, then with the pairs alone (blocks last) */
        Side probe = *s;

        if (attempt == 1 && !exact_kept)
            continue;                                   /* no exact band was laid: the same set-up as the first */
        memset(banned, attempt >= 1, sizeof banned);
        prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
        if (attempt == 0) {
            exact_kept = 0;
            for (unsigned k = 0; k < bands; k++)
                exact_kept += report[k].method != 0;
        }
        probe.depth = s->depth;
        csp_tail_forever = 0;
        csp_last_nodes = csp_last_v = 0;
        if (csp_side(&probe, report, attempt < 2, 0, 0)) {
            *outcome = FILL_OUTCOME_MADE;
            *blocks = pass == 0 ? 'Q' : 'q';
            if (getenv("ALADDIN_DEV_FILL"))
                fprintf(stderr, "fill: side u=(%d,%d) csp made %d of %d depths (attempt %d), nodes %u of %u\n", s->ux, s->uy, s->depth, s->depth, attempt, csp_last_nodes, csp_last_budget);
            return;
        }
    }
    if (*outcome != FILL_OUTCOME_PROVED) {
        /* not all of it: the most depths that can be made, by halving, in the weakest set-up (blocks last) */
        int lo = 1, hi = s->depth - 1;

        csp_quad_mode = 2;
        Side probe = *s;

        memset(banned, 1, sizeof banned);
        while (lo <= hi) {
            int mid = (lo + hi + 1) / 2;

            prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
            probe.depth = mid;
            csp_tail_forever = 0;
            csp_last_nodes = csp_last_v = 0;
            if (csp_side(&probe, report, 0, 0, 0)) {
                best_depth = mid;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
        best_attempt = 2;
    }
    if (best_depth > 0 && *outcome != FILL_OUTCOME_PROVED) {
        Side part = *s;

        memset(banned, best_attempt >= 1, sizeof banned);
        prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
        part.depth = best_depth;
        csp_tail_forever = 0;
        csp_quad_mode = 2;
        csp_last_nodes = csp_last_v = 0;
        if (csp_side(&part, report, best_attempt < 2, 0, 0)) {
            *outcome = FILL_OUTCOME_PARTIAL;
            if (getenv("ALADDIN_DEV_FILL"))
                fprintf(stderr, "fill: side u=(%d,%d) csp made %d of %d depths (attempt %d), nodes %u of %u\n", s->ux, s->uy, best_depth, s->depth, best_attempt, csp_last_nodes, csp_last_budget);
            /* the margin beyond: the one place the level's pairs run out, the cells there made with the fewest pairs let go (first those of the
               bands with no way on, then every seam pair, then those along the band), and every pair beyond as the level has it */
            part = *s;
            part.bx = s->bx - best_depth * s->ux;
            part.by = s->by - best_depth * s->uy;
            part.depth = s->depth - best_depth;
            csp_tail_forever = 0;
            for (int relax = 1; relax <= 3; relax++) {
                csp_last_nodes = csp_last_v = 0;
                if (csp_side(&part, report, 0, 0, relax)) {
                    if (getenv("ALADDIN_DEV_FILL"))
                        fprintf(stderr, "fill: side u=(%d,%d) the rest made, the break at depth %d, relaxed %d\n", s->ux, s->uy, best_depth + 1, relax);
                    return;
                }
            }
            part = *s;
            part.dmin = best_depth + 1;
            synth_side(&part, report, unfilled);
            return;
        }
        *outcome = FILL_OUTCOME_GAVE_UP;
    } else {
        /* not even the first cells can be made without a pair the level never has (the side is one of those the level's own cells cannot
           fill): the fewest such pairs, all at the seam. The seam pair of the bands that have no way on is let go first, then every
           seam pair, then the pairs along the band at depth 1. The cells beyond are made with none. */
        csp_quad_mode = 2;
        for (int relax = 1; relax <= 3; relax++)
            for (int attempt = 2; attempt <= 2; attempt++) {
                memset(banned, 1, sizeof banned);
                prepare_margin(s, base, sa, sb, bands, total, rows, metatiles, report, fixed, banned);
                csp_tail_forever = 0;
                if (!csp_side(s, report, attempt < 2, 1, relax))
                    continue;
                csp_last_nodes = csp_last_v = 0;
                if (csp_side(s, report, attempt < 2, 0, relax)) {
                    *outcome = FILL_OUTCOME_RELAXED;
                    if (getenv("ALADDIN_DEV_FILL"))
                        fprintf(stderr, "fill: side u=(%d,%d) csp made all depths with the seam relaxed (level %d), nodes %u of %u\n", s->ux, s->uy, relax, csp_last_nodes, csp_last_budget);
                    return;
                }
            }
        if (!*outcome)
            *outcome = FILL_OUTCOME_GAVE_UP;
        if (getenv("ALADDIN_DEV_FILL"))
            fprintf(stderr, "fill: side u=(%d,%d) no depth can be made without a pair the level never has\n", s->ux, s->uy);
    }
    /* no consistent fill found (none, or the budget ran out): the depth at a time path, which keeps the fewest broken pairs */
    memset(banned, 0, sizeof banned);
    for (int round = 0; round < ROUNDS; round++) {
        unsigned before = *unfilled;

        for (int d = s->dmin; d <= s->depth; d++)
            for (int t = 0; t < s->along; t++) {
                size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

                lv.cells[ti] = UNSET;
                source_of[ti] = -1;
                kind_of[ti] = 0;
            }
        memset(report, 0, bands * sizeof *report);
        fill_exact(base, sa, sb, bands, s->n_of, total, rows, metatiles, report, fixed, banned);
        synth_side(s, report, unfilled);
        if (round + 1 == ROUNDS || !conflicts(s, report, banned))
            break;
        *unfilled = before;
    }
}

/* ---- the copy: what a side that cannot be made from the level's pairs falls back to ----
   A side the exact search could not make (its cells beyond the seam made with pairs the level never has) can come out as flat strips:
   a margin that goes on with one tile for longer than the level ever does, which a few unseen pairs of real art are better than.
   The copy lays the level's own columns beyond the edge: the margin's depth d is the level's column cs + d (right) or cs - d
   (left), the same rows, cs chosen so that the one seam (the edge column and the first copied column) has the fewest unseen pairs.
   Inside the copy every pair and block is the level's. It is used where the exact search's result has a run longer than the level's or
   more unseen pairs than the copy's seam. */
static unsigned side_unseen_and_runs(const Side *s, unsigned *runs)
{
    unsigned unseen = 0;

    *runs = 0;
    for (int d = 1; d <= s->depth; d++)
        for (int t = 0; t < s->along; t++) {
            int tx = s->bx + t * s->lx - d * s->ux, ty = s->by + t * s->ly - d * s->uy;
            uint16_t v = lv.cells[(size_t)ty * lv.fw + tx], inward = lv.cells[(size_t)(ty + s->uy) * lv.fw + tx + s->ux];

            unseen += !(s->ux == 1 ? seen_across(v, inward) : seen_across(inward, v));
            if (t > 0)
                unseen += !pair_along(s, lv.cells[(size_t)(ty - s->ly) * lv.fw + tx - s->lx], v);
            *runs += runs_broken(tx, ty, v) != 0;
        }
    return unseen;
}

static int copy_side(const Side *s, FillBand *report)
{
    int dir = s->ux == 1 ? -1 : 1;              /* the copy's columns go outward of the level this way: left: toward x = 0 from cs */
    int best_cs = -1;
    unsigned best_unseen = ~0u, runs_now, unseen_now;

    if (s->lx != 0 || s->depth + 2 > lv.lw)
        return 0;
    unseen_now = side_unseen_and_runs(s, &runs_now);
    for (int cs = (dir == 1 ? 0 : s->depth); dir == 1 ? cs + s->depth < lv.lw - 0 && cs + s->depth <= lv.lw - 1 : cs < lv.lw; cs++) {
        unsigned bad = 0;
        int ok = 1;

        for (int t = 0; t < s->along && ok; t++) {
            uint16_t edge = level_cell(s->ux == 1 ? 0 : lv.lw - 1, t), first = level_cell(cs + dir * 1, t);

            for (int d = 1; d <= s->depth && ok; d++)
                ok = in_level(cs + dir * d, t) && level_cell(cs + dir * d, t) != FILL_BLANK;     /* not an empty place */
            bad += !(s->ux == 1 ? seen_across(first, edge) : seen_across(edge, first));
        }
        if (ok && bad < best_unseen) {
            best_unseen = bad;
            best_cs = cs;
        }
    }
    if (best_cs < 0 || (runs_now == 0 && best_unseen >= unseen_now))
        return 0;
    for (int d = 1; d <= s->depth; d++)
        for (int t = 0; t < s->along; t++) {
            size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

            lv.cells[ti] = level_cell(best_cs + dir * d, t);
            source_of[ti] = (best_cs + dir * d) + t * lv.lw;
            kind_of[ti] = FILL_KIND_SYNTH;
        }
    for (int t = 0; t < s->along; t++)
        report[t].method = FILL_SYNTH;
    return 1;
}


/* ---- Q, strip quilting of a left or right margin (docs VIEW.md 5.z) ----
   The margin is built outward from the level's edge by whole full-height strips of the level's own columns, so a prop is whole down the strip.
   Each strip is chosen by how well its first Q_O columns match the Q_O columns already placed (the level's own edge, or the strip before),
   and is joined to them along the cheapest seam through those columns, row by row (a dynamic programme over the row's switch point, its cost the
   level's unseen pairs and 2 x 2 blocks the cut would make), so the cut goes round props rather than through them. No strip's source overlaps
   another's, none is the same distance from its copy as another (no fixed period), none is next to its own copy. */
static int fill_field;              /* the layer being filled is the play field (its metatiles have collision) */
static uint8_t fill_behaviour[4096];            /* its metatiles' behaviour bytes (the game loads g.tile_behaviour after the maps are built) */

#define Q_W 12                      /* a strip's columns, the most: one near the level's end is shorter (Q_O + 2 at least), its new columns the fewer */
#define Q_SHORT 60                  /* the cost of each column a strip is short by */
#define Q_O 6                       /* the columns it overlaps the placed ones */
#define Q_PAIR 100                  /* the cost of a pair the level never has */
#define Q_QUAD 30                   /* ... of a 2 x 2 block the level never has */
#define Q_MISMATCH 3                /* ... of a cell of the overlap that differs from the placed one (the context) */
#define Q_PICK 5                    /* the lowest-cost strips one is picked from, by chance, among those within Q_SLACK of the best */
#define Q_SLACK 6
#define Q_SURF 300                  /* ... of a column pair whose walkable surfaces (the rows where solid ground begins under empty) the level never has side by side */
#define Q_CUT 300                   /* ... of a seam that cuts a prop: the cell before it goes on into the next one of a prop of the level, or the one after it comes from the one before (VIEW.md 5.w) */
#define Q_GIVE_UP 20000             /* a strip costing more than this is not used: the margin is left as it was */

static uint16_t q_col[FILL_COLS_MAX][FILL_ROWS_MAX];       /* [u][t]: the level's columns, then the margin's, in the margin's outward order */
static int32_t q_src[FILL_COLS_MAX][FILL_ROWS_MAX];        /* where each came from: the level cell index, or -1 */


/* A column's surfaces: a hash of the rows where a tile with collision (a solid tile, or a surface) has an empty tile above it (the top row counts as
   above-empty): the heights a player walks at. Two columns side by side in the margin must have surfaces the level has side by side (equal, or a step
   as the level's own art steps), or a ledge would meet a stair face-on. */
static int q_solid(uint16_t cell)
{
    unsigned idx;

    if (cell == FILL_BLANK || cell == UNSET)
        return 0;
    idx = (unsigned)cell >> 3;
    if (idx >= sizeof fill_behaviour)
        return 0;
    return tile_kind(fill_behaviour[idx]) != TILE_KIND_EMPTY && !(tile_kind(fill_behaviour[idx]) & TILE_KIND_OUT);
}

static uint64_t q_surface_sig(const uint16_t *col, int rows)
{
    uint64_t h = 1469598103934665603ull;
    int above = 0;

    for (int t = 0; t < rows; t++) {
        int now = q_solid(col[t]);

        if (now && !above) {
            h ^= (uint64_t)(t + 1);
            h *= 1099511628211ull;
        }
        above = now;
    }
    return h;
}

#define Q_PAIRS_SIZE 8192
static uint64_t q_pairs[Q_PAIRS_SIZE];      /* the level's column pairs' keys (open addressing; 0 empty) */

static uint64_t q_pair_key(uint64_t a, uint64_t b)
{
    uint64_t k = (a * 0x9E3779B97F4A7C15ull) ^ (b + 0x7F4A7C15ull + (a << 6) + (a >> 2));

    return k ? k : 1;
}

static void q_pair_add(uint64_t k)
{
    unsigned i = (unsigned)(k >> 20) & (Q_PAIRS_SIZE - 1);

    while (q_pairs[i] && q_pairs[i] != k)
        i = (i + 1) & (Q_PAIRS_SIZE - 1);
    q_pairs[i] = k;
}

static int q_pair_has(uint64_t k)
{
    unsigned i = (unsigned)(k >> 20) & (Q_PAIRS_SIZE - 1);

    while (q_pairs[i]) {
        if (q_pairs[i] == k)
            return 1;
        i = (i + 1) & (Q_PAIRS_SIZE - 1);
    }
    return 0;
}

static DataRef fill_metatiles;                  /* the level's metatile table (for what a metatile draws) */
static uint16_t prop_of[FILL_CELLS_MAX];       /* by level cell: the prop's number (from 1), 0 for none */

static int scenery_cell(int x, int y)
{
    uint16_t c;

    if (!in_level(x, y))
        return 0;
    c = level_cell(x, y);
    return c != FILL_BLANK && c != UNSET && !edge_empty(c, fill_metatiles) && !(fill_field && q_solid(c));
}

static void find_props(FillReport *r)
{
    static int stack[FILL_CELLS_MAX];
    unsigned n = 0;

    memset(prop_of, 0, sizeof prop_of);
    for (int y0 = 0; y0 < lv.lh; y0++)
        for (int x0 = 0; x0 < lv.lw; x0++) {
            int sp = 0, minx = x0, maxx = x0;

            if (prop_of[(size_t)y0 * lv.lw + x0] || !scenery_cell(x0, y0))
                continue;
            n++;
            stack[sp++] = y0 * lv.lw + x0;
            prop_of[(size_t)y0 * lv.lw + x0] = (uint16_t)(n > 65535 ? 65535 : n);
            while (sp) {
                int i = stack[--sp], x = i % lv.lw, y = i / lv.lw;
                static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };

                minx = x < minx ? x : minx;
                maxx = x > maxx ? x : maxx;
                for (int k = 0; k < 4; k++) {
                    int nx = x + dx[k], ny = y + dy[k];

                    if (nx >= 0 && ny >= 0 && nx < lv.lw && ny < lv.lh && !prop_of[(size_t)ny * lv.lw + nx] && scenery_cell(nx, ny)) {
                        prop_of[(size_t)ny * lv.lw + nx] = prop_of[(size_t)y0 * lv.lw + x0];
                        stack[sp++] = ny * lv.lw + nx;
                    }
                }
            }
            if (maxx - minx + 1 > Q_W - Q_O) {
                /* too wide to be a prop: take its cells back */
                for (int y = 0; y < lv.lh; y++)
                    for (int x = 0; x < lv.lw; x++)
                        if (prop_of[(size_t)y * lv.lw + x] == (uint16_t)(n > 65535 ? 65535 : n))
                            prop_of[(size_t)y * lv.lw + x] = 0;
            } else {
                if (r)
                    r->props++;
            }
        }
}


static int q_across_ok(const Side *s, uint16_t a, uint16_t b)       /* a, b the cells at u, u + 1 */
{
    return s->ux == -1 ? seen_across(a, b) : seen_across(b, a);
}

/* whether joining the margin's column `prev` to the strip's column `cur` at row t cuts a prop of the level (prop_of, by level cell) */
static int q_cut(const Side *s, int32_t sprev, int32_t scur)
{
    int32_t l = s->ux == -1 ? sprev : scur, r = s->ux == -1 ? scur : sprev;

    if (l < 0 || r < 0 || r == l + 1)
        return 0;
    if (l % lv.lw + 1 < lv.lw && prop_of[l] && prop_of[l + 1] == prop_of[l])
        return 1;
    return r % lv.lw > 0 && prop_of[r] && prop_of[r - 1] == prop_of[r];
}

/* the cost and seam of strip `s0` placed after column `e`: `seam[t]` the row's switch point (columns before it of the overlap kept), `*cost` the sum; -1 if it cannot be placed */
static int quilt_try(const Side *s, int e, int s0, int kmin, unsigned limit, unsigned *cost, uint8_t *seam)
{
    int rows = s->along;
    static int dp[FILL_ROWS_MAX][Q_O + 1];
    static uint8_t back[FILL_ROWS_MAX][Q_O + 1];
    unsigned mism = 0;

    for (int t = 0; t < rows; t++)
        for (int j = 0; j < Q_O; j++)
            mism += q_col[e - Q_O + 1 + j][t] != q_col[s0 + j][t];
    if (Q_MISMATCH * mism > limit) {                /* the context alone costs more than the strips kept: not worth the seam */
        *cost = ~0u;
        return -1;
    }
    for (int t = 0; t < rows; t++) {
        for (int k = kmin; k <= Q_O; k++) {
            int c = 0;
            uint16_t prev = q_col[e - Q_O][t];

            for (int j = 0; j <= Q_O; j++) {
                uint16_t cur = j == Q_O ? q_col[s0 + Q_O][t] : (j < k ? q_col[e - Q_O + 1 + j][t] : q_col[s0 + j][t]);

                c += !q_across_ok(s, prev, cur) * Q_PAIR;
                if (j == k && j < Q_O + 1 && !fill_base())
                    c += q_cut(s, q_src[j == 0 ? e - Q_O : e - Q_O + j][t], q_src[s0 + j][t]) * Q_CUT;
                prev = cur;
            }
            if (t == 0) {
                dp[0][k] = c;
                continue;
            }
            dp[t][k] = 1 << 30;
            for (int k0 = k - 1 > kmin ? k - 1 : kmin; k0 <= (k + 1 < Q_O ? k + 1 : Q_O); k0++) {     /* the cut moves at most a column a row */
                int tc = dp[t - 1][k0] + c + (k0 > k ? k0 - k : k - k0);

                for (int j = 0; j <= Q_O; j++) {
                    uint16_t up = j == Q_O ? q_col[s0 + Q_O][t - 1] : (j < k0 ? q_col[e - Q_O + 1 + j][t - 1] : q_col[s0 + j][t - 1]);
                    uint16_t dn = j == Q_O ? q_col[s0 + Q_O][t] : (j < k ? q_col[e - Q_O + 1 + j][t] : q_col[s0 + j][t]);
                    uint16_t pu = j == 0 ? q_col[e - Q_O][t - 1] : (j - 1 < k0 ? q_col[e - Q_O + j][t - 1] : q_col[s0 + j - 1][t - 1]);
                    uint16_t pd = j == 0 ? q_col[e - Q_O][t] : (j - 1 < k ? q_col[e - Q_O + j][t] : q_col[s0 + j - 1][t]);

                    if (j < Q_O)
                        tc += !seen_down(up, dn) * Q_PAIR;
                    if (s->ux == -1)
                        tc += !quad_has(quad_key(pu, up, pd, dn)) * Q_QUAD;
                    else
                        tc += !quad_has(quad_key(up, pu, dn, pd)) * Q_QUAD;
                }
                if (tc < dp[t][k]) {
                    dp[t][k] = tc;
                    back[t][k] = (uint8_t)k0;
                }
            }
        }
    }
    {
        int best = 1 << 30, bk = kmin;

        for (int k = kmin; k <= Q_O; k++)
            if (dp[rows - 1][k] < best) {
                best = dp[rows - 1][k];
                bk = k;
            }
        for (int t = rows - 1; t >= 0; t--) {
            seam[t] = (uint8_t)bk;
            if (t > 0)
                bk = back[t][bk];
        }
        *cost = (unsigned)best + Q_MISMATCH * mism;
        if (fill_field) {
            /* the columns round the join as the seam leaves them: the surfaces of each pair side by side */
            uint16_t colv[Q_O + 2][FILL_ROWS_MAX];
            uint64_t prev = 0;

            for (int t = 0; t < rows; t++) {
                colv[0][t] = q_col[e - Q_O][t];
                for (int j = 0; j < Q_O; j++)
                    colv[1 + j][t] = j < seam[t] ? q_col[e - Q_O + 1 + j][t] : q_col[s0 + j][t];
                colv[Q_O + 1][t] = q_col[s0 + Q_O][t];
            }
            for (int j = 0; j < Q_O + 2; j++) {
                uint64_t sg = q_surface_sig(colv[j], rows);

                if (j > 0 && sg != prev && !q_pair_has(q_pair_key(prev, sg)))
                    *cost += Q_SURF;
                prev = sg;
            }
        }
    }
    return 0;
}


/* the surface breaks of a margin as it stands: adjacent columns, from the level's edge outward, whose surfaces the level never has side by side (the
   level's own columns give the pairs it has) */
static int side_surface_breaks(const Side *s)
{
    uint16_t col[FILL_ROWS_MAX];
    uint64_t prev = 0;
    int breaks = 0;

    if (!fill_field || s->lx != 0 || s->along > FILL_ROWS_MAX)
        return 0;
    memset(q_pairs, 0, sizeof q_pairs);
    for (int d = -(lv.lw - 1); d <= s->depth; d++) {
        uint64_t sg;

        for (int t = 0; t < s->along; t++)
            col[t] = lv.cells[(size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux];
        sg = q_surface_sig(col, s->along);
        if (d > -(lv.lw - 1)) {
            if (d <= 0)
                q_pair_add(q_pair_key(prev, sg));
            else
                breaks += sg != prev && !q_pair_has(q_pair_key(prev, sg));
        }
        prev = sg;
    }
    return breaks;
}

static int quilt_side(const Side *s, FillBand *report)
{
    int lw = lv.lw, rows = s->along, need = lw - 1 + s->depth, e = lw - 1;
    static int32_t used_s0[64], used_shift[64];
    int used = 0;
    unsigned old_runs, old_unseen, new_runs, new_unseen;
    int old_breaks;
    static uint8_t col_ok[FILL_COLS_MAX];

    if (s->lx != 0 || lw < Q_W + Q_O + 2 || need >= FILL_COLS_MAX || rows > FILL_ROWS_MAX)
        return 0;
    for (int t = 0; t < rows; t++)
        if (s->n_of[t] != (unsigned)lw)
            return 0;                               /* an empty place runs in from the edge: left to the exact search */
    old_unseen = side_unseen_and_runs(s, &old_runs);
    old_breaks = side_surface_breaks(s);
    for (int u = 0; u < lw; u++) {
        int x = s->ux == -1 ? u : lw - 1 - u, ok = 1;

        for (int t = 0; t < rows; t++) {
            q_col[u][t] = level_cell(x, t);
            q_src[u][t] = x + t * lw;
            ok &= q_col[u][t] != FILL_BLANK && in_level(x, t);
        }
        col_ok[u] = (uint8_t)ok;
    }
    if (fill_field) {
        uint64_t prev = 0;

        memset(q_pairs, 0, sizeof q_pairs);
        for (int u = 0; u < lw; u++) {
            uint64_t sg = q_surface_sig(q_col[u], rows);

            if (u > 0)
                q_pair_add(q_pair_key(prev, sg));
            prev = sg;
        }
    }
    if (getenv("ALADDIN_DEV_QUILT") && fill_field) {
        fprintf(stderr, "quilt ux=%d surface hashes of columns 230-255:", s->ux);
        for (int u = lw - 26; u < lw; u++)
            fprintf(stderr, " %04x", (unsigned)(q_surface_sig(q_col[u], rows) & 0xFFFF));
        fprintf(stderr, "\n");
    }
    while (e < need) {
        int kmin = e - Q_O + 1 >= lw ? 0 : Q_O;     /* overlap columns that are the level's own cannot be replaced */
        int cand_s0[Q_PICK], cand_new[Q_PICK];
        unsigned cand_cost[Q_PICK], cand_n = 0, best = ~0u;
        static uint8_t seam_best[Q_PICK][FILL_ROWS_MAX];
        static uint8_t seam[FILL_ROWS_MAX];

        for (int s0 = 0; s0 + Q_O + 2 <= lw; s0++) {
            int fresh = lw - (s0 + Q_O) < Q_W - Q_O ? lw - (s0 + Q_O) : Q_W - Q_O;       /* the new columns this strip gives */
            int shift = e + 1 - (s0 + Q_O), ok = shift >= 2;
            unsigned cost;

            for (int j = 0; j < Q_O + fresh && ok; j++)
                ok = col_ok[s0 + j];
            for (int i = 0; i < used && ok; i++)
                ok = abs(s0 - used_s0[i]) >= Q_O && abs(shift - used_shift[i]) >= Q_O;
            if (!ok)
                continue;
            if (quilt_try(s, e, s0, kmin, cand_n == Q_PICK ? cand_cost[Q_PICK - 1] : ~0u, &cost, seam) < 0)
                continue;
            cost += (unsigned)(Q_W - Q_O - fresh) * Q_SHORT;
            if (cost < best)
                best = cost;
            /* keep the Q_PICK lowest */
            if (cand_n < Q_PICK || cost < cand_cost[cand_n - 1]) {
                int at = (int)(cand_n < Q_PICK ? cand_n++ : Q_PICK - 1);

                while (at > 0 && cand_cost[at - 1] > cost) {
                    cand_cost[at] = cand_cost[at - 1];
                    cand_s0[at] = cand_s0[at - 1];
                    cand_new[at] = cand_new[at - 1];
                    memcpy(seam_best[at], seam_best[at - 1], (size_t)rows);
                    at--;
                }
                cand_cost[at] = cost;
                cand_s0[at] = s0;
                cand_new[at] = fresh;
                memcpy(seam_best[at], seam, (size_t)rows);
            }
        }
        if (getenv("ALADDIN_DEV_QUILT") && fill_field && cand_n)
            fprintf(stderr, "  strip after column %d: best cost %u (s0 %d), %u candidates kept\n", e, best, cand_s0[0], cand_n);
        if (!cand_n || best > Q_GIVE_UP)
            return 0;
        {
            unsigned within = 0, pick;

            while (within < cand_n && cand_cost[within] <= best + Q_SLACK)
                within++;
            pick = next_random() % within;
            {
                int s0 = cand_s0[pick], shift = e + 1 - (s0 + Q_O);
                const uint8_t *sm = seam_best[pick];

                used_s0[used] = s0;
                used_shift[used++] = shift;
                for (int t = 0; t < rows; t++) {
                    for (int j = 0; j < Q_O; j++)
                        if (j >= sm[t]) {
                            q_col[e - Q_O + 1 + j][t] = q_col[s0 + j][t];
                            q_src[e - Q_O + 1 + j][t] = q_src[s0 + j][t];
                        }
                    for (int j = Q_O; j < Q_O + cand_new[pick] && e + 1 + j - Q_O < FILL_COLS_MAX; j++) {
                        q_col[e + 1 + j - Q_O][t] = q_col[s0 + j][t];
                        q_src[e + 1 + j - Q_O][t] = q_src[s0 + j][t];
                    }
                }
                e += cand_new[pick];
            }
        }
    }
    /* put the margin in, keeping the one it replaces if this one has more unseen pairs than that and a few */
    {
        static uint16_t keep[FILL_COLS_MAX * FILL_ROWS_MAX];
        static int32_t keep_src[FILL_COLS_MAX * FILL_ROWS_MAX];
        static uint8_t keep_kind[FILL_COLS_MAX * FILL_ROWS_MAX];
        size_t n = 0;

        for (int d = 1; d <= s->depth; d++)
            for (int t = 0; t < rows; t++, n++) {
                size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

                keep[n] = lv.cells[ti];
                keep_src[n] = source_of[ti];
                keep_kind[n] = kind_of[ti];
                lv.cells[ti] = q_col[lw - 1 + d][t];
                source_of[ti] = q_src[lw - 1 + d][t];
                kind_of[ti] = FILL_KIND_QUILT;
            }
        /* a one-off in the strip (a sign on a ledge: the strip took the ledge) is left out: its cell is read as the nearest cell of the same row
           that is not a one-off, in the margin or the level's edge cell, so the ledge goes on and the sign is not there twice (VIEW.md 5.w) */
        if (!fill_base())
            for (int t = 0; t < rows; t++) {
                uint16_t rowv[FILL_COLS_MAX];

                for (int d = 0; d <= s->depth; d++)
                    rowv[d] = lv.cells[(size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux];
                for (int d = 1; d <= s->depth; d++) {
                    int pick = -1;

                    if (!oneoff_cell(rowv[d]))
                        continue;
                    for (int k = 1; k <= s->depth && pick < 0; k++) {
                        if (d - k >= 0 && !oneoff_cell(rowv[d - k]))
                            pick = d - k;
                        else if (d + k <= s->depth && !oneoff_cell(rowv[d + k]))
                            pick = d + k;
                    }
                    if (pick >= 0) {
                        size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

                        lv.cells[ti] = rowv[pick];
                        source_of[ti] = -1;
                    }
                }
            }
        new_unseen = side_unseen_and_runs(s, &new_runs);
        if (new_unseen > old_unseen + 8 || new_runs > old_runs + 8 || side_surface_breaks(s) > old_breaks) {
            n = 0;
            for (int d = 1; d <= s->depth; d++)
                for (int t = 0; t < rows; t++, n++) {
                    size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

                    lv.cells[ti] = keep[n];
                    source_of[ti] = keep_src[n];
                    kind_of[ti] = keep_kind[n];
                }
            return 0;
        }
    }
    if (getenv("ALADDIN_DEV_QUILT") && fill_field) {
        /* the surface breaks in the margin and at its seam: adjacent columns whose surfaces the level never has side by side */
        int breaks = 0;
        uint64_t prev = q_surface_sig(q_col[lw - 1], rows);

        for (int u = lw; u <= need; u++) {
            uint64_t sg = q_surface_sig(q_col[u], rows);

            breaks += sg != prev && !q_pair_has(q_pair_key(prev, sg));
            prev = sg;
        }
        fprintf(stderr, "quilt ux=%d: surface breaks %d in %d margin columns\n", s->ux, breaks, need - lw + 1);
    }
    for (int t = 0; t < rows; t++)
        report[t].method = FILL_QUILT;
    return 1;
}

static void make_margin(const Side *s, ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, unsigned total, int rows,
                        DataRef metatiles, FillBand *report, unsigned *unfilled, uint8_t *outcome, uint8_t *blocks)
{
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    make_margin_body(s, base, sa, sb, bands, total, rows, metatiles, report, unfilled, outcome, blocks);
    if (rows && *outcome != FILL_OUTCOME_MADE && copy_side(s, report))
        *blocks = 'c';
    if (rows) {
        int synth = 0;

        for (int t = 0; t < s->along; t++)
            synth |= report[t].method != FILL_EXACT && report[t].method != FILL_EMPTY && report[t].method != 0;
        if (synth && quilt_side(s, report))
            *blocks = 'Z';
        if (getenv("ALADDIN_DEV_QUILT") && fill_field && s->lx == 0)
            fprintf(stderr, "quilt final ux=%d method %c: surface breaks %d in %d margin columns\n", s->ux, *blocks ? *blocks : '-', side_surface_breaks(s), s->depth);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (getenv("ALADDIN_DEV_FILL"))
        fprintf(stderr, "fillside u=(%d,%d) %d x %d cells: %.1f ms, outcome %c\n", s->ux, s->uy, s->depth, s->along,
                (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) * 1e-6, *outcome ? *outcome : '-');
}

/* the unseen pairs a margin cell is in (with its four neighbours, the level's included) */
static unsigned cell_conflicts(int x, int y, uint16_t v)
{
    unsigned n = 0;
    const uint16_t *c = lv.cells;
    size_t i = (size_t)y * lv.fw + x;

    if (x > 0 && !seen_across(c[i - 1], v))
        n++;
    if (x + 1 < lv.fw && !seen_across(v, c[i + 1]))
        n++;
    if (y > 0 && !seen_down(c[i - lv.fw], v))
        n++;
    if (y + 1 < lv.fh && !seen_down(v, c[i + lv.fw]))
        n++;
    return n;
}

/* Local search over the margins (min-conflicts): each margin cell in an unseen pair takes the metatile of the level that
   leaves it in the fewest, among those the rounds before it left it in, a tie going to the cell's own, else (sideways, to leave
   a plateau) to a pseudo-random one. Run until no unseen pair is left or REPAIR_SWEEPS sweeps. The synthesis makes the
   pairs along and between its bands right wherever a path exists; what its depth-by-depth order and the corners leave is
   mended here. */
#define REPAIR_SWEEPS 24
static void repair(void)
{
    uint16_t order_unused = 0;

    (void)order_unused;
    for (int sweep = 0; sweep < REPAIR_SWEEPS; sweep++) {
        unsigned bad = 0, moved = 0;

        for (int y = 0; y < lv.fh; y++)
            for (int x = 0; x < lv.fw; x++) {
                uint16_t cur;
                unsigned have, best, ties = 0;
                int dup;
                uint16_t pick = 0;

                if (walled(x, y))
                    continue;
                if (y < lv.oy || y >= lv.oy + lv.lh)
                    continue;                           /* the top and bottom margins are not mended: the exact bands made them */
                if (x >= lv.ox && x < lv.ox + lv.lw && !lv.tag[(size_t)(y - lv.oy) * lv.lw + x - lv.ox])
                    continue;                           /* the level's own cell, or an empty place inside it */
                cur = lv.cells[(size_t)y * lv.fw + x];
                dup = !(x >= lv.ox && x < lv.ox + lv.lw) && oneoff_cell(cur);   /* a margin cell holding a one-off: always replaced */
                if (kind_of[(size_t)y * lv.fw + x] == FILL_KIND_QUILT && !dup)
                    continue;                           /* a quilted margin's cells are the level's strips, whole: not mended cell by cell */
                have = cell_conflicts(x, y, cur) + (dup ? 1000u : 0u);
                if (!have)
                    continue;
                bad++;
                best = have;
                for (unsigned id = 0; id < lv.ids; id++) {
                    unsigned n;

                    if (lv.count[id] == 1 && !fill_base())
                        continue;                       /* a margin never takes a one-off */
                    n = cell_conflicts(x, y, lv.cell_of[id]);

                    if (n < best) {
                        best = n;
                        ties = 1;
                        pick = lv.cell_of[id];
                    } else if (n == best && n < have + 1 && lv.cell_of[id] != cur) {
                        ties++;
                        if (next_random() % ties == 0)
                            pick = lv.cell_of[id];
                    }
                }
                if (ties && (best < have || (best == have && (next_random() & 3u) == 0))) {
                    lv.cells[(size_t)y * lv.fw + x] = pick;
                    kind_of[(size_t)y * lv.fw + x] = FILL_KIND_REPAIR;
                    source_of[(size_t)y * lv.fw + x] = -1;
                    moved++;
                }
            }
        if (!bad)
            break;
        (void)moved;
    }
}

/* ---- the audit (VIEW.md 5.w): what the margins do to the level's one-off cells and props ----
   A one-off is a metatile the level holds in one cell only. A prop is a connected run (4 neighbours) of the level's scenery cells (not blank, and for the
   play field not solid) at most Q_W - Q_O columns wide. A margin's strip boundary is a pair of side by side margin or edge cells whose sources are not
   side by side in the level; it is inside a prop where the cell before it in the level continues into the next one of the same prop, or the cell after
   it continues from the one before. An edge cut is a prop at the level's edge column whose neighbour in the margin is not its own continuation. */
/* the level cell index a cell of the whole came from: its own, its source, or -1 */
static int32_t audit_source(int x, int y)
{
    if (x >= lv.ox && x < lv.ox + lv.lw && y >= lv.oy && y < lv.oy + lv.lh)
        return (y - lv.oy) * lv.lw + x - lv.ox;
    return source_of[(size_t)y * lv.fw + x];
}

static void fill_audit(FillReport *r)
{
    r->oneoff_cells = r->oneoff_copies = r->props = 0;
    memset(r->cuts, 0, sizeof r->cuts);
    memset(r->edge_cuts, 0, sizeof r->edge_cuts);
    for (unsigned i = 0; i < lv.ids; i++)
        r->oneoff_cells += lv.count[i] == 1;
    for (int y = 0; y < lv.fh; y++)
        for (int x = 0; x < lv.fw; x++) {
            uint16_t c = lv.cells[(size_t)y * lv.fw + x];

            if (x >= lv.ox && x < lv.ox + lv.lw && y >= lv.oy && y < lv.oy + lv.lh)
                continue;
            if (c != FILL_BLANK && lv.id_of[c] != NO_ID && lv.count[lv.id_of[c]] == 1)
                r->oneoff_copies++;
        }
    find_props(r);
    for (int y = lv.oy; y < lv.oy + lv.lh; y++)
        for (int x = 0; x + 1 < lv.fw; x++) {
            int32_t a = audit_source(x, y), b = audit_source(x + 1, y);
            int side = x + 1 <= lv.ox ? 0 : x >= lv.ox + lv.lw - 1 ? 1 : -1;
            uint16_t pa, pb;

            if (side < 0 || a < 0 || b < 0)
                continue;
            /* a left margin pair has its second cell inward; the pair is contiguous when b is a's next cell */
            if (b == a + 1 && a / lv.lw == b / lv.lw)
                continue;
            pa = a % lv.lw + 1 < lv.lw ? prop_of[a] : 0;
            pb = b % lv.lw > 0 ? prop_of[b] : 0;
            if ((pa && prop_of[a + 1] == pa) || (pb && prop_of[b - 1] == pb))
                r->cuts[side]++;
            if (getenv("ALADDIN_DEV_AUDIT") && ((pa && prop_of[a + 1] == pa) || (pb && prop_of[b - 1] == pb)))
                fprintf(stderr, "audit cut: margin row %d between whole columns %d and %d: sources (%d,%d) and (%d,%d)\n", y - lv.oy, x, x + 1, a % lv.lw, a / lv.lw, b % lv.lw, b / lv.lw);
        }
    memset(r->edge_solid, 0, sizeof r->edge_solid);
    for (int y = 0; y < lv.lh; y++) {
        r->edge_solid[0] += fill_field && in_level(0, y) && q_solid(level_cell(0, y));
        r->edge_solid[1] += fill_field && in_level(lv.lw - 1, y) && q_solid(level_cell(lv.lw - 1, y));
        if (scenery_cell(0, y) && lv.ox && !fill_wall[0] && !seen_across(lv.cells[(size_t)(lv.oy + y) * lv.fw + lv.ox - 1], level_cell(0, y)))
            r->edge_cuts[0]++;
        if (scenery_cell(lv.lw - 1, y) && lv.fw > lv.ox + lv.lw && !fill_wall[1] && !seen_across(level_cell(lv.lw - 1, y), lv.cells[(size_t)(lv.oy + y) * lv.fw + lv.ox + lv.lw]))
            r->edge_cuts[1]++;
    }
}

/* ---- the layout's empty places, tiled (VIEW.md 5.w) ----
   A backdrop's empty places (places of the layout the original's camera does not show, or shows as the plain colour) are, in a wide view, filled by repeating
   the row's nearest non-empty place across them: each cell is the cell one place (PLACE_CELLS) before it, or after it where the run begins at the left
   edge. A row is tiled only where the cell pair the repeat makes at its seam is one the level has side by side (seen_across), so the art meets itself where
   it already does; the other rows stay empty. */
#define PLACE_CELLS 16

static void tile_voids(void)
{
    for (int y = 0; y < lv.lh; y++) {
        uint16_t *row = lv.cells + (size_t)(lv.oy + y) * lv.fw + lv.ox;

        for (int a = 0; a < lv.lw;) {
            int b;

            if (row[a] != FILL_BLANK) {
                a++;
                continue;
            }
            for (b = a; b + 1 < lv.lw && row[b + 1] == FILL_BLANK; b++)
                ;
            if (a > 0 && a - PLACE_CELLS >= 0 && row[a - PLACE_CELLS] != FILL_BLANK && seen_across(row[a - 1], row[a - PLACE_CELLS])) {
                for (int x = a; x <= b; x++)
                    row[x] = row[x - PLACE_CELLS];
            } else if (a == 0 && b + 1 < lv.lw && b + PLACE_CELLS < lv.lw && row[b + PLACE_CELLS] != FILL_BLANK && seen_across(row[b + PLACE_CELLS], row[b + 1])) {
                for (int x = b; x >= a; x--)
                    row[x] = row[x + PLACE_CELLS];
            }
            a = b + 1;
        }
    }
}

/* ---- the level's edge columns, with the props the edge cuts taken out (VIEW.md 5.w; Jeff 2026-11-23: "just get rid of half barrels anywhere") ----
   In a wide view a prop the map cuts at its own edge (the map begins or ends in the middle of a barrel) is shown half: its other half is not in the level.
   A cell of the left edge column is cut when it is scenery and no metatile of the level sits left of it anywhere (nothing precedes it in the level: its
   only neighbour is the one to its right); of the right edge column, when nothing follows it anywhere. Each such cell is replaced by the level's commonest
   metatile that draws nothing. The play field's scenery only (no solid cell is touched). */
static int has_pairs(const uint32_t *set)
{
    for (unsigned w = 0; w < IDS_MAX / 32; w++)
        if (set[w])
            return 1;
    return 0;
}

static void trim_edges(void)
{
    unsigned count = 0;
    uint16_t empty = FILL_BLANK;

    for (unsigned i = 0; i < lv.ids; i++)
        if (lv.cell_of[i] != FILL_BLANK && edge_empty(lv.cell_of[i], fill_metatiles) && lv.count[i] > count) {
            count = lv.count[i];
            empty = lv.cell_of[i];
        }
    if (empty == FILL_BLANK)
        return;
    for (int y = 0; y < lv.lh; y++) {
        uint16_t l = level_cell(0, y), r = level_cell(lv.lw - 1, y);

        if (scenery_cell(0, y) && !has_pairs(lv.across_t[lv.id_of[l]]))
            lv.cells[(size_t)(lv.oy + y) * lv.fw + lv.ox] = empty;
        if (scenery_cell(lv.lw - 1, y) && !has_pairs(lv.across[lv.id_of[r]]))
            lv.cells[(size_t)(lv.oy + y) * lv.fw + lv.ox + lv.lw - 1] = empty;
    }
}

void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, unsigned ox, unsigned oy, int field, int wide, DataRef metatiles,
               uint32_t seed, FillReport *report)
{
    unsigned right = fw - lw - ox, below = fh - lh - oy;    /* the margins' sizes */
    ptrdiff_t w = fw, top = (ptrdiff_t)oy * w + ox;         /* the level's top left cell */
    unsigned unfilled = 0;
    Side sd;

    fill_field = field;
    fill_metatiles = metatiles;
    if (field)
        for (unsigned i = 0; i < sizeof fill_behaviour; i++)
            fill_behaviour[i] = tile_behaviour_byte(metatiles, i);
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
    lv.cells = cells;
    lv.fw = (int)fw;
    lv.fh = (int)fh;
    lv.lw = (int)lw;
    lv.lh = (int)lh;
    lv.ox = (int)ox;
    lv.oy = (int)oy;
    /* the level's own cells: an empty place (FILL_BLANK) is not one. Empty places that run in from an edge are made like the
       margin beyond them, the band's own cells ending where they begin; those inside, with a cell of the level's each side, stay empty. */
    for (unsigned y = 0; y < lh; y++)
        for (unsigned x = 0; x < lw; x++)
            lv.real[(size_t)y * lw + x] = !FILL_TRIM_VOIDS || cells[(size_t)(oy + y) * fw + ox + x] != FILL_BLANK;
    learn();
    if (wide && !fill_base()) {
        if (!field)
            tile_voids();
        trim_edges();
        for (unsigned y = 0; y < lh; y++)
            for (unsigned x = 0; x < lw; x++)
                lv.real[(size_t)y * lw + x] = !FILL_TRIM_VOIDS || cells[(size_t)(oy + y) * fw + ox + x] != FILL_BLANK;
        learn();
    }
    quads_learn();
    find_props(NULL);
    fill_wall[0] = fill_wall[1] = 0;
    if (field && !fill_base()) {
        int l = lh > 0, r = lh > 0;

        for (unsigned y = 0; y < lh; y++) {
            l &= in_level(0, (int)y) && q_solid(level_cell(0, (int)y));
            r &= in_level((int)lw - 1, (int)y) && q_solid(level_cell((int)lw - 1, (int)y));
        }
        fill_wall[0] = l;
        fill_wall[1] = r;
    }
    memset(lv.has_above, 0, sizeof lv.has_above);
    memset(lv.has_below, 0, sizeof lv.has_below);
    for (unsigned i = 0; i < lv.ids; i++)
        for (unsigned w = 0; w < IDS_MAX / 32; w++) {
            if (lv.below_t[i][w])
                lv.has_above[i >> 5] |= 1u << (i & 31);
            if (lv.below[i][w])
                lv.has_below[i >> 5] |= 1u << (i & 31);
        }
    memset(source_of, 0xFF, sizeof source_of);
    memset(kind_of, 0, sizeof kind_of);
    memset(lv.tag, 0, sizeof lv.tag);
    memset(lv.trim_left, 0, sizeof lv.trim_left);
    memset(lv.trim_right, 0, sizeof lv.trim_right);
    memset(lv.trim_top, 0, sizeof lv.trim_top);
    memset(lv.trim_bottom, 0, sizeof lv.trim_bottom);
    report->ids = lv.ids;
    report->ids_over = lv.over;
    for (unsigned y = 0; y < fh; y++)
        for (unsigned x = 0; x < fw; x++)
            if (x < ox || x >= ox + lw || y < oy || y >= oy + lh)
                cells[(size_t)y * fw + x] = UNSET;
    for (unsigned y = 0; y < lh; y++) {
        unsigned a = 0, b = 0;

        while (a < lw && !lv.real[(size_t)y * lw + a])
            a++;
        if (a == lw)
            continue;                                   /* no cell of the level's in the row: it is no band's */
        while (!lv.real[(size_t)y * lw + lw - 1 - b])
            b++;
        lv.trim_left[y] = a;
        lv.trim_right[y] = b;
        for (unsigned x = 0; x < a; x++) {
            cells[(size_t)(oy + y) * fw + ox + x] = UNSET;
            lv.tag[(size_t)y * lw + x] = 1;
        }
        for (unsigned x = lw - b; x < lw; x++) {
            cells[(size_t)(oy + y) * fw + ox + x] = UNSET;
            lv.tag[(size_t)y * lw + x] = 2;
        }
    }
    for (unsigned x = 0; x < lw; x++) {
        unsigned a = 0, b = 0;

        while (a < lh && !lv.real[(size_t)a * lw + x])
            a++;
        if (a == lh)
            continue;
        while (!lv.real[(size_t)(lh - 1 - b) * lw + x])
            b++;
        lv.trim_top[ox + x] = a;
        lv.trim_bottom[ox + x] = b;
        for (unsigned y = 0; y < a; y++) {
            if (cells[(size_t)(oy + y) * fw + ox + x] != UNSET)
                lv.tag[(size_t)y * lw + x] = 3;
            cells[(size_t)(oy + y) * fw + ox + x] = UNSET;
        }
        for (unsigned y = lh - b; y < lh; y++) {
            if (cells[(size_t)(oy + y) * fw + ox + x] != UNSET)
                lv.tag[(size_t)y * lw + x] = 4;
            cells[(size_t)(oy + y) * fw + ox + x] = UNSET;
        }
    }
    {
        /* by band, the level's cells that are its own */
        static unsigned n_right[FILL_ROWS_MAX], n_left[FILL_ROWS_MAX], n_top[FILL_COLS_MAX], n_bottom[FILL_COLS_MAX];
        unsigned max_right = 0, max_left = 0, max_top = 0, max_bottom = 0;

        for (unsigned y = 0; y < lh; y++) {
            n_right[y] = lw - lv.trim_right[y];
            n_left[y] = lw - lv.trim_left[y];
            max_right = lv.trim_right[y] > max_right ? lv.trim_right[y] : max_right;
            max_left = lv.trim_left[y] > max_left ? lv.trim_left[y] : max_left;
        }
        for (unsigned x = 0; x < fw; x++) {
            n_bottom[x] = lh - lv.trim_bottom[x];
            n_top[x] = lh - lv.trim_top[x];
            max_bottom = lv.trim_bottom[x] > max_bottom ? lv.trim_bottom[x] : max_bottom;
            max_top = lv.trim_top[x] > max_top ? lv.trim_top[x] : max_top;
        }
        /* the sides first (each row of the level), then the bottom and top over every column of the whole, the corners included */
        if (right && !fill_wall[1]) {
            sd = (Side){ -1, 0, 0, 1, (int)(ox + lw - 1), (int)oy, (int)right, (int)lh, 1 - (int)max_right, n_right };
            make_margin(&sd, top, 1, w, lh, lw + right, 1, metatiles, report->right, &unfilled, &report->outcome[1], &report->blocks[1]);
        }
        if (ox && !fill_wall[0]) {
            sd = (Side){ 1, 0, 0, 1, (int)ox, (int)oy, (int)ox, (int)lh, 1 - (int)max_left, n_left };
            make_margin(&sd, top + (ptrdiff_t)lw - 1, -1, w, lh, lw + ox, 1, metatiles, report->left, &unfilled, &report->outcome[0], &report->blocks[0]);
        }
        if (below) {
            sd = (Side){ 0, -1, 1, 0, 0, (int)(oy + lh - 1), (int)below, (int)fw, 1 - (int)max_bottom, n_bottom };
            make_margin(&sd, (ptrdiff_t)oy * w, w, 1, fw, lh + below, 0, metatiles, report->down, &unfilled, &report->outcome[3], &report->blocks[3]);
        }
        if (oy) {
            sd = (Side){ 0, 1, 1, 0, 0, (int)oy, (int)oy, (int)fw, 1 - (int)max_top, n_top };
            make_margin(&sd, (ptrdiff_t)(oy + lh - 1) * w, -w, 1, fw, lh + oy, 0, metatiles, report->up, &unfilled, &report->outcome[2], &report->blocks[2]);
        }
    }
    for (unsigned y = 0; y < fh; y++)
        for (unsigned x = 0; x < fw; x++)
            if (cells[(size_t)y * fw + x] == UNSET)
                cells[(size_t)y * fw + x] = FILL_BLANK;
    repair();
    report->unfilled = unfilled;
    count_unseen(report);
    count_quads(report);
    fill_audit(report);
}
