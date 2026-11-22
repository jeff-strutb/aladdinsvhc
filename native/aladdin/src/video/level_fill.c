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
 * SEAMLESS. A vertical margin (top, bottom) is seamless for a layer when it has no unseen pair; FillReport.seam_up and
 * seam_down say it, and the view (video/view.c) shows past the top and bottom only where both layers are.
 * The right margin is filled first, then the left, then the bottom over every column of the whole, then the top, the
 * corners included.
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
#define NO_ID 0xFFFFu
#define UNSET 0xFFFDu               /* a margin cell not yet made */

/* the level's different metatiles (ids), and which pairs sit side by side (`across`: b is right of a) or one above the other
   (`below`: b is under a) */
static struct {
    uint16_t id_of[65536];
    uint16_t cell_of[IDS_MAX];                      /* the metatile of each id */
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
    uint32_t reach[IDS_MAX][IDS_MAX / 32];          /* S: for u, the metatiles that begin a pair that goes on, outward of u: a pair (u, v) can be gone on from if v's outward set meets it */
    uint32_t has_above[IDS_MAX / 32];               /* the metatiles with a metatile of the level above them / below them somewhere: the corner bands' cells must have one, for the top and bottom margins to go on from them */
    uint32_t has_below[IDS_MAX / 32];
    uint32_t out_mask[IDS_MAX / 32];        /* the ids that have a position to go on outward from */
    uint8_t lat_ok[IDS_MAX];        /* ... and one beyond them along the band (they have a neighbour that way in the level) */
    uint32_t pos_store[FILL_CELLS_MAX];
} lv;

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
                lv.id_of[cell] = (uint16_t)lv.ids++;
            }
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
    for (int y = 0; y < lv.lh; y++)
        for (int x = 0; x < lv.lw; x++)
            if (in_level(x + s->ux, y + s->uy) && lv.id_of[level_cell(x + s->ux, y + s->uy)] != NO_ID)
                lv.bucket_pos[fill[lv.id_of[level_cell(x + s->ux, y + s->uy)]]++] = (uint32_t)(y * lv.lw + x);
    /* ext: the pairs (u, v) along the band that the level has and that can go on outward for ever: the metatiles that can
       go outward of u and of v again have such a pair, and so on (the largest such set of pairs, by repeated narrowing from
       the seen pairs). A depth's cells are chosen among these, so the next depths' cells can still be beside each other. */
    for (unsigned u = 0; u < IDS_MAX; u++)
        memcpy(lv.ext[u], s->lx ? lv.across[u] : lv.below[u], sizeof lv.ext[u]);
    for (int round = 0; round < 16; round++) {
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
#define KEEP 32                     /* the best-scoring candidates kept for a cell */
#define STATES_MAX 96               /* the most metatiles a cell's states hold of those the band allows */
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
    index_side(s);
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

/* the unseen pairs in each side's margin (the left, right, top, bottom in that order of claim for a pair that touches
   more than one), and the pairs there are */
static void count_unseen(FillReport *r)
{
    for (int y = 0; y < lv.fh; y++)
        for (int x = 0; x < lv.fw; x++)
            for (int dir = 0; dir < 2; dir++) {
                int x2 = x + (dir == 0), y2 = y + (dir == 1), side = -1;
                ptrdiff_t i = (ptrdiff_t)y * lv.fw + x;

                if (x2 >= lv.fw || y2 >= lv.fh)
                    continue;
                for (int pt = 0; pt < 2; pt++) {
                    int cx = pt ? x2 : x, cy = pt ? y2 : y, sd;

                    if (cx >= lv.ox && cx < lv.ox + lv.lw && cy >= lv.oy && cy < lv.oy + lv.lh)
                        sd = lv.tag[(size_t)(cy - lv.oy) * lv.lw + cx - lv.ox] - 1;       /* made inside the level: an empty place run to an edge */
                    else
                        sd = cx < lv.ox ? 0 : cx >= lv.ox + lv.lw ? 1 : cy < lv.oy ? 2 : 3;
                    if (sd >= 0 && (side < 0 || sd < side))
                        side = sd;
                }
                if (side < 0)
                    continue;
                r->pairs[side]++;
                if (!seen_at(i, dir == 0 ? 1 : lv.fw))
                    r->unseen[side]++;
            }
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
static void make_margin(const Side *s, ptrdiff_t base, ptrdiff_t sa, ptrdiff_t sb, unsigned bands, unsigned total, int rows,
                        DataRef metatiles, FillBand *report, unsigned *unfilled)
{
    uint8_t fixed[FILL_COLS_MAX], banned[FILL_COLS_MAX];

    memset(banned, 0, sizeof banned);
    for (int round = 0; round < ROUNDS; round++) {
        unsigned before = *unfilled;

        for (int d = s->dmin; d <= s->depth; d++)
            for (int t = 0; t < s->along; t++) {
                size_t ti = (size_t)(s->by + t * s->ly - d * s->uy) * lv.fw + s->bx + t * s->lx - d * s->ux;

                lv.cells[ti] = UNSET;
                source_of[ti] = -1;
            }
        memset(report, 0, bands * sizeof *report);
        fill_exact(base, sa, sb, bands, s->n_of, total, rows, metatiles, report, fixed, banned);
        synth_side(s, report, unfilled);
        if (round + 1 == ROUNDS || !conflicts(s, report, banned))
            break;
        *unfilled = before;
    }
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
                uint16_t pick = 0;

                if (x >= lv.ox && x < lv.ox + lv.lw && y >= lv.oy && y < lv.oy + lv.lh && !lv.tag[(size_t)(y - lv.oy) * lv.lw + x - lv.ox])
                    continue;                           /* the level's own cell, or an empty place inside it */
                cur = lv.cells[(size_t)y * lv.fw + x];
                have = cell_conflicts(x, y, cur);
                if (!have)
                    continue;
                bad++;
                best = have;
                for (unsigned id = 0; id < lv.ids; id++) {
                    unsigned n = cell_conflicts(x, y, lv.cell_of[id]);

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

void LevelFill(uint16_t *cells, unsigned lw, unsigned lh, unsigned fw, unsigned fh, unsigned ox, unsigned oy, int field, DataRef metatiles,
               uint32_t seed, FillReport *report)
{
    unsigned right = fw - lw - ox, below = fh - lh - oy;    /* the margins' sizes */
    ptrdiff_t w = fw, top = (ptrdiff_t)oy * w + ox;         /* the level's top left cell */
    unsigned unfilled = 0;
    Side sd;

    (void)field;
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
        if (right) {
            sd = (Side){ -1, 0, 0, 1, (int)(ox + lw - 1), (int)oy, (int)right, (int)lh, 1 - (int)max_right, n_right };
            make_margin(&sd, top, 1, w, lh, lw + right, 1, metatiles, report->right, &unfilled);
        }
        if (ox) {
            sd = (Side){ 1, 0, 0, 1, (int)ox, (int)oy, (int)ox, (int)lh, 1 - (int)max_left, n_left };
            make_margin(&sd, top + (ptrdiff_t)lw - 1, -1, w, lh, lw + ox, 1, metatiles, report->left, &unfilled);
        }
        if (below) {
            sd = (Side){ 0, -1, 1, 0, 0, (int)(oy + lh - 1), (int)below, (int)fw, 1 - (int)max_bottom, n_bottom };
            make_margin(&sd, (ptrdiff_t)oy * w, w, 1, fw, lh + below, 0, metatiles, report->down, &unfilled);
        }
        if (oy) {
            sd = (Side){ 0, 1, 1, 0, 0, (int)oy, (int)oy, (int)fw, 1 - (int)max_top, n_top };
            make_margin(&sd, (ptrdiff_t)(oy + lh - 1) * w, -w, 1, fw, lh + oy, 0, metatiles, report->up, &unfilled);
        }
    }
    for (unsigned y = 0; y < fh; y++)
        for (unsigned x = 0; x < fw; x++)
            if (cells[(size_t)y * fw + x] == UNSET)
                cells[(size_t)y * fw + x] = FILL_BLANK;
    repair();
    report->unfilled = unfilled;
    count_unseen(report);
    report->seam_down = below > 0 && report->unseen[3] == 0;
    report->seam_up = oy > 0 && report->unseen[2] == 0;
}
