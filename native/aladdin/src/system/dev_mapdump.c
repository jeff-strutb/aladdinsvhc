/*
 * dev_mapdump.c: a developer's tool, off unless asked for by the environment: a level's whole map as pictures.
 * Nothing of the original's, and the game plays the same with it off (the one hook, video.c's write_words, does
 * nothing while DevVramShadow is null).
 *
 *   ALADDIN_DEV_MAPDUMP=DIR          at update 1, load the level ALADDIN_DEV_MAPDUMP_LEVEL (hex or decimal, default 0)
 *                                    as LevelLoad does, write the files below into DIR, and end the run
 *   ALADDIN_DEV_MAPDUMP_LEVEL=N
 *   ALADDIN_DEV_MAPDUMP_HOLD=1       do not end the run: keep the loaded level on the screen (no tasks run, nothing
 *                                    moves) so the engine's own picture of it can be written (--shots) and compared
 *
 * Files, named by the level in two hex digits (LL):
 *   LL-field.ppm       the play field's whole map (layout 0: every screen, its metatiles' 8 x 8 tiles, the tile
 *                      set and palette as the load leaves them) at a pixel a pixel; colour 0 flat magenta; a place
 *                      the screen map marks empty dark grey
 *   LL-field-grid.ppm  the same with a white line every 256 pixels
 *   LL-back.ppm        the backdrop layout (layout 1, the screen map from place $221) the same way
 *   LL-both.ppm        the field over the backdrop, the backdrop at its start offset from the field (the start
 *                      scroll of each layer, after one parallax update); field colour 0 shows the backdrop
 *   LL-start.ppm       the 256 x 224 window of LL-both at the start camera, from row camera y + 1 (the picture's own
 *                      offset of one line), as the game draws it at that moment, but for sprites, text and the status line
 *   LL-backmap.ppm     BG2's video map as the load left it (64 x 32 tiles, or level 7's 64 x 64), through the
 *                      same tile set and palette: what the backdrop layer shows where it is not streamed
 *   LL-info.txt        the size, empty places, limits, start, locks, exit spots and placements of the level
 * The pictures are PPM; sips converts them to PNG.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/actor_tables.h"
#include "data/data.h"
#include "data/level_tables.h"
#include "data/scroll_tables.h"
#include "data/system_tables.h"
#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
#include "system/state.h"
#include "system/system.h"
#include "video/level_maps.h"
#include "video/video.h"
#include "video/video_internal.h"

#define VRAM_WORDS_ALL 0x8000
#define SCREEN 256
#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define TRANSPARENT_MAGENTA RGB(255, 0, 255)
#define EMPTY_GREY RGB(64, 64, 64)
#define GRID_WHITE RGB(255, 255, 255)
#define LINE_OFFSET 1       /* the picture's first row shows the layer's row scroll y + 1 (the SNES's one line offset, kept by the engine) */
#define ZERO_MAGENTA 0
#define ZERO_BACKDROP 2

uint16_t *DevVramShadow;
static int held;
static uint16_t vram[VRAM_WORDS_ALL];

/* a picture: pixels as 0xRRGGBB, and for the field and backdrop which are opaque (colour not 0) */
typedef struct Pic {
    int w, h;
    uint32_t *px;
    uint8_t *opaque;
} Pic;

static Pic pic_new(int w, int h)
{
    Pic p;

    p.w = w;
    p.h = h;
    p.px = calloc((size_t)w * h, sizeof *p.px);
    p.opaque = calloc((size_t)w * h, 1);
    if (!p.px || !p.opaque) {
        fprintf(stderr, "mapdump: no memory for a %d x %d picture\n", w, h);
        exit(1);
    }
    return p;
}

static void pic_free(Pic *p)
{
    free(p->px);
    free(p->opaque);
}

static uint32_t rgb_of(uint16_t c)
{
    unsigned r = c & 31, gr = c >> 5 & 31, b = c >> 10 & 31;

    /* a 5 bit channel to 8 bits as the engine does: its bits repeated */
    return (r << 3 | r >> 2) << 16 | (gr << 3 | gr >> 2) << 8 | (b << 3 | b >> 2);
}

static void pic_write(const char *dir, const char *name, const Pic *p, int grid)
{
    char path[1024];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "mapdump: cannot write %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", p->w, p->h);
    for (int y = 0; y < p->h; y++)
        for (int x = 0; x < p->w; x++) {
            uint32_t c = p->px[(size_t)y * p->w + x];

            if (grid && (x % SCREEN == 0 || y % SCREEN == 0))
                c = GRID_WHITE;
            putc(c >> 16 & 255, f);
            putc(c >> 8 & 255, f);
            putc(c & 255, f);
        }
    fclose(f);
}

/* the 4bpp pixel (0-15) at column x, row y of tile n of the low tile set (video address n * 16, a row a word for
   planes 0 and 1, then the same 8 words on for planes 2 and 3) */
static unsigned tile_pixel(unsigned n, unsigned x, unsigned y)
{
    unsigned base = (n * 16u) & (VRAM_WORDS_ALL - 1), bit = 7 - x;
    uint16_t lo = vram[base + y], hi = vram[(base + 8 + y) & (VRAM_WORDS_ALL - 1)];

    return (lo >> bit & 1u) | (lo >> (8 + bit) & 1u) << 1 | (hi >> bit & 1u) << 2 | (hi >> (8 + bit) & 1u) << 3;
}

/* a map word's 8 x 8 tile into the picture at x, y; colour 0 as `zero` says */
static void draw_tile(Pic *p, int x, int y, uint16_t word, int zero)
{
    unsigned n = word & 0x3FF, pal = word >> 10 & 7;
    int hflip = word >> 14 & 1, vflip = word >> 15 & 1;

    for (int ty = 0; ty < 8; ty++)
        for (int tx = 0; tx < 8; tx++) {
            unsigned c = tile_pixel(n, (unsigned)(hflip ? 7 - tx : tx), (unsigned)(vflip ? 7 - ty : ty));
            int px = x + tx, py = y + ty;
            size_t at;

            if (px < 0 || py < 0 || px >= p->w || py >= p->h)
                continue;
            at = (size_t)py * p->w + px;
            if (c) {
                p->px[at] = rgb_of(g.palette[pal * 16 + c]);
                p->opaque[at] = (uint8_t)(1 + (word >> 13 & 1));   /* 1 opaque, 2 opaque in a tile of the high priority */
            } else if (zero == ZERO_MAGENTA) {
                p->px[at] = TRANSPARENT_MAGENTA;
            } else {
                p->px[at] = rgb_of(g.palette[0]);
            }
        }
}

/* the word of the screen map at place `place`, as the scroll layers read it (video/scroll_layer.c screen_at) */
static uint16_t place_word(unsigned place)
{
    return g.screen_map.places[place % SCREEN_MAP_PLACES];
}

/* a layout's whole map from screen map place `first`, `cols` x `rows` screens */
static Pic render_layout(unsigned first, unsigned cols, unsigned rows, int zero)
{
    Pic p = pic_new((int)(cols * SCREEN), (int)(rows * SCREEN));
    DataRef metatiles = level_metatiles(g.map_level);

    for (unsigned r = 0; r < rows; r++)
        for (unsigned c = 0; c < cols; c++) {
            uint16_t w = place_word(first + r * 32 + c);

            if (w & 0x8000) {
                for (int y = 0; y < SCREEN; y++)
                    for (int x = 0; x < SCREEN; x++)
                        p.px[(size_t)(r * SCREEN + (unsigned)y) * p.w + c * SCREEN + (unsigned)x] = EMPTY_GREY;
                continue;
            }
            for (unsigned my = 0; my < 16; my++)
                for (unsigned mx = 0; mx < 16; mx++) {
                    unsigned at = (unsigned)(w + 2 * (my * 16 + mx)) / 2u;
                    uint16_t meta = (uint16_t)((at < LEVEL_TILES_WORDS ? g.level_tiles[at] : 0) | 0x8000);

                    for (unsigned half = 0; half < 4; half++)
                        draw_tile(&p, (int)(c * SCREEN + mx * 16 + (half & 1) * 8), (int)(r * SCREEN + my * 16 + (half >> 1) * 8),
                                  level_metatile_word(metatiles, meta, half * 2), zero);
                }
        }
    return p;
}


/* a layer's map as the game shows it (video/level_maps.c): the level and the fill past its right and bottom edges, a
   red line along the level's own edges */
static Pic render_words(const uint16_t *words, uint32_t w, uint32_t h, uint32_t level_w, uint32_t level_h, int zero)
{
    Pic p = pic_new((int)(w * 8), (int)(h * 8));

    for (uint32_t r = 0; r < h; r++)
        for (uint32_t c = 0; c < w; c++)
            draw_tile(&p, (int)(c * 8), (int)(r * 8), words[(size_t)r * w + c], zero);
    for (int y = 0; y < p.h; y++)
        if (level_w * 8 < w * 8u)
            p.px[(size_t)y * p.w + level_w * 8] = RGB(255, 0, 0);
    for (int x = 0; x < p.w; x++)
        if (level_h * 8 < h * 8u)
            p.px[(size_t)level_h * 8 * p.w + x] = RGB(255, 0, 0);
    return p;
}

/* the fill's bands, runs of the same method and period together, in pixels of the level */
static void fill_runs(FILE *f, const FillBand *b, unsigned from, unsigned to, const char *what)
{
    for (unsigned i = from; i < to;) {
        unsigned j = i, gap_least = b[i].gap, gap_most = b[i].gap;

        while (j + 1 < to && b[j + 1].method == b[i].method && b[j + 1].period == b[i].period) {
            j++;
            gap_least = b[j].gap < gap_least ? b[j].gap : gap_least;
            gap_most = b[j].gap > gap_most ? b[j].gap : gap_most;
        }
        if (b[i].method == FILL_EXACT)
            fprintf(f, "  %s %u-%u (px %u-%u): B exact, period %u px, repeating stretch ends %u-%u px inside the edge\n", what, i, j,
                    i * 16, j * 16 + 15, b[i].period * 16, gap_least * 16, gap_most * 16);
        else
            fprintf(f, "  %s %u-%u (px %u-%u): %c %s\n", what, i, j, i * 16, j * 16 + 15, b[i].method ? b[i].method : '-',
                    b[i].method == FILL_MADE ? "made from the level's own metatiles, by adjacency" :
                    b[i].method == FILL_BACKDROP ? "backdrop alone (blank)" : "none");
        i = j + 1;
    }
}

static void fill_write(const char *dir, unsigned level)
{
    char path[512];
    FILE *f;

    snprintf(path, sizeof path, "%s/%02X-fill.txt", dir, level);
    f = fopen(path, "w");
    if (!f)
        return;
    for (int layer = 0; layer < 2; layer++) {
        const FillReport *r = level_maps_fill(layer);
        unsigned count[3] = { 0, 0, 0 };

        fprintf(f, "level $%02X, %s: level %u x %u px, with the fill %u x %u px; %u different metatiles (%u past the table)\n", level,
                layer ? "backdrop" : "play field", r->lw * 16, r->lh * 16, r->fw * 16, r->fh * 16, r->ids, r->ids_over);
        if (r->fw > r->lw) {
            fprintf(f, " past the right edge, by 16 px row of the level:\n");
            fill_runs(f, r->right, 0, r->lh, "row");
        } else {
            fprintf(f, " past the right edge: none (the layer repeats by wrapping at its own width)\n");
        }
        if (r->fh > r->lh) {
            fprintf(f, " past the bottom edge, by 16 px column of the whole:\n");
            fill_runs(f, r->down, 0, r->fw, "column");
            for (unsigned i = 0; i < r->fw; i++)
                count[r->down[i].method == FILL_EXACT ? 0 : r->down[i].method == FILL_MADE ? 1 : 2]++;
            fprintf(f, " bottom bands: %u exact, %u made, %u backdrop\n", count[0], count[1], count[2]);
        }
    }
    fclose(f);
}

/* BG2's video map as the load left it: base and the pages' layout as video.c's map_part */
static Pic render_backmap(void)
{
    int tall = g.bg_map[1] == VIDEO_MAP_BACK_TALL;
    unsigned base = tall ? 0x3800u : 0x4000u, rows = tall ? 64 : 32;
    Pic p = pic_new(64 * 8, (int)rows * 8);

    for (unsigned y = 0; y < rows; y++)
        for (unsigned x = 0; x < 64; x++) {
            unsigned page = (x >> 5 & 1) | (tall ? (y >> 5) << 1 : 0u);
            uint16_t w = vram[(base + page * 0x400u + (y & 31) * 32 + (x & 31)) & (VRAM_WORDS_ALL - 1)];

            draw_tile(&p, (int)x * 8, (int)y * 8, w, ZERO_MAGENTA);
        }
    return p;
}

static unsigned layer_pixels(uint32_t position)
{
    return (position >> 8) & 0xFFFFu;
}

/* the field over the backdrop; the backdrop's pixel under the field's (x, y) is its own at (x + ox, y + oy). The
   order is Mode 1's: field high priority, backdrop high priority, field, backdrop. */
static Pic compose(const Pic *field, const Pic *back, int ox, int oy)
{
    Pic p = pic_new(field->w, field->h);

    for (int y = 0; y < field->h; y++)
        for (int x = 0; x < field->w; x++) {
            size_t at = (size_t)y * field->w + x;
            int bx = x + ox, by = y + oy;

            int inside = bx >= 0 && by >= 0 && bx < back->w && by < back->h;
            size_t bat = inside ? (size_t)by * back->w + bx : 0;
            unsigned fo = field->opaque[at], bo = inside ? back->opaque[bat] : 0;

            if (fo == 2 || (fo && bo != 2))
                p.px[at] = field->px[at];
            else if (inside && bo)
                p.px[at] = back->px[bat];
            else
                p.px[at] = rgb_of(g.palette[0]);
        }
    return p;
}

/* a level's camera locks, as the code applies them (read from level/, actor/ and player/, by their file) */
static void locks_text(FILE *f, unsigned level)
{
    fprintf(f, "\ncamera locks (x, y are world pixels; a camera x is the view's left edge, the view is 256 wide)\n");
    fprintf(f, "  every level: LevelStartPlace sets scroll_x_min 0 and scroll_x_max $7F00 (no limit); limits come from the layout\n");
    switch (level) {
    case 0x03:
        fprintf(f, "  level03_scene.c: camera x at most $0400 from the start (view $0000-$04FF at its right end); when the player is\n");
        fprintf(f, "    at y low byte $B0, x low byte $F0 or more with the camera at screen 4 or past: max $0500, scroll kind 4;\n");
        fprintf(f, "    camera at screen 5: min $0500, so the camera is fixed at $0500: ROOM x $0500-$05FF (full height)\n");
        fprintf(f, "    later the max goes to $7F00 (open)\n");
        break;
    case 0x0D:
        fprintf(f, "  level0d_scene.c: camera x at most $0900 (view to $09FF); player kept at y $18 or more; at x $09F0, y $78 or\n");
        fprintf(f, "    more: max $0A00 and scroll kind 4; camera at screen $0A: min $0A00 so the camera is fixed: ROOM x $0A00-$0AFF\n");
        fprintf(f, "    after the scene's cue the max opens ($7FFF)\n");
        break;
    case 0x10:
        fprintf(f, "  level10_scene.c: camera x at most $10A0 (view to $119F); at player x $1190 or more with y low byte $A8 or more:\n");
        fprintf(f, "    max $11A0, scroll_x_held set, scroll kind 4; the camera stops at $11A0: ROOM x $11A0-$129F (the view's\n");
        fprintf(f, "    right edge is $12A0 exclusive); at the scene's end: min $11A0, max $7FA0, hold cleared, scroll kind 0\n");
        break;
    case 0x04:
    case 0x16:
        fprintf(f, "  camera_trigger.c: when the camera is at screen $0B across and 2 down all actors are freed; when the camera is at\n");
        fprintf(f, "    screen $0F across (camera x $0F00-$0FFF) with the player alive, scroll_x_held is raised: the camera stays\n");
        fprintf(f, "    where it is then (x in $0F00-$0FFF, y free), and a kind $6B is placed at (3896, 944)\n");
        break;
    case 0x05:
        fprintf(f, "  level05_end_scene.c: when the player is at x $0E20 or more with the camera's y under 256, the camera moves to y $B8\n");
        fprintf(f, "    then on to x $0E00 and is held both ways (scroll kind 4, both holds): end scene ROOM x $0E00-$0EFF, y $B8-$197\n");
        fprintf(f, "    (camera window step $28; window effect $14)\n");
        break;
    case 0x06:
        fprintf(f, "  level 6: scroll kind 2 (the view runs by itself) and view edge rule 3 holds the player at most +$F0 from the\n");
        fprintf(f, "    view's left edge (video/scroll_update.c); the backdrop drifts in bands (layer setup mode $0C), not streamed\n");
        break;
    case 0x0A:
    case 0x19:
        fprintf(f, "  level_events.c ClimbTopTask: once the player stands at the top (camera y 0, x low byte $80 or more, y low byte\n");
        fprintf(f, "    under $78), or at a restart, scroll_limit_y high byte is cleared: the camera's y cannot go below 0; then when\n");
        fprintf(f, "    the camera passes x $100 the screen map place $21 (layout column 0, row 0) is made empty ($FE)\n");
        fprintf(f, "    camera window step $50\n");
        break;
    case 0x11:
        fprintf(f, "  level $11 set-up: scroll_limit_x high byte 1: camera x at most $0100 (view $0000-$01FF); kind $81 raises it by 1 screen\n");
        break;
    case 0x12:
    case 0x1C:
        fprintf(f, "  levels $12 and $1C: the view scrolls by itself (scroll kind $0A or $0E, view edge rule 1) with scroll_y_held 1:\n");
        fprintf(f, "    the camera's y stays where it starts\n");
        break;
    default:
        break;
    }
    fprintf(f, "  kind $81 (boss arena, actor/kind81.c): when the player is on firm ground at x $01E0 or more: scroll_x_held up, the\n");
    fprintf(f, "    limit across raised by one screen, the view scrolls right by 3 a frame to x $0200 and stops there with both\n");
    fprintf(f, "    layers' edges off: ARENA x $0200-$02FF at the camera's y of that moment (placements below say which level has it)\n");
    fprintf(f, "  kind $98 (boss, actor/kind98.c): at its start scroll_x_held and scroll_y_held are set to 1: the camera stays\n");
    fprintf(f, "    wherever it is then; the boss is at screen x $80, y $38 (placements below say where it is placed)\n");
    fprintf(f, "  player holds (player/held.c, scene.c, carpet.c, player_states.c): scenes set scroll_x_held and scroll_y_held at the\n");
    fprintf(f, "    camera's place of that moment, none a fixed rectangle\n");
}

static void info_write(const char *dir, unsigned level, unsigned cols, unsigned rows, int has_back, unsigned bcols, unsigned brows,
                       int back_ox, int back_oy, unsigned back_x0, unsigned back_y0)
{
    char path[1024];
    FILE *f;
    Actor *pl = &g.actors[0];

    snprintf(path, sizeof path, "%s/%02X-info.txt", dir, level);
    f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "level $%02X\n", level);
    fprintf(f, "layout 0 (the play field): %u x %u screens = %u x %u pixels; screens built %u; layouts %u\n", cols, rows, cols * 256,
            rows * 256, level_map_screens(level), level_map_layout_count(level));
    if (has_back)
        fprintf(f, "layout 1 (the backdrop, from place $221): %u x %u screens = %u x %u pixels\n", bcols, brows, bcols * 256, brows * 256);
    else
        fprintf(f, "layout 1: none (the backdrop layer reads place $221 on, which is not laid out)\n");
    fprintf(f, "empty places in layout 0 (screen map entry negative; the scroll layers read them as list 0, the camera stops stop at them):");
    {
        int n = 0;

        for (unsigned r = 0; r < rows; r++)
            for (unsigned c = 0; c < cols; c++)
                if (place_word(0x21 + r * 32 + c) & 0x8000) {
                    fprintf(f, " (col %u,row %u)", c, r);
                    n++;
                }
        if (!n)
            fprintf(f, " none");
    }
    if (has_back) {
        int n = 0;

        fprintf(f, "\nempty places in layout 1:");
        for (unsigned r = 0; r < brows; r++)
            for (unsigned c = 0; c < bcols; c++)
                if (place_word(0x221 + r * 32 + c) & 0x8000) {
                    fprintf(f, " (col %u,row %u)", c, r);
                    n++;
                }
        if (!n)
            fprintf(f, " none");
    }
    fprintf(f, "\nstart: player x %u y %u facing %s; camera x %u y %u (view x %u-%u, y %u-%u)\n", (unsigned)(pl->x >> 8) & 0xFFFF,
            (unsigned)(pl->y >> 8) & 0xFFFF, pl->facing ? "left" : "right", layer_pixels(g.layers[0].scroll_x),
            layer_pixels(g.layers[0].scroll_y), layer_pixels(g.layers[0].scroll_x), layer_pixels(g.layers[0].scroll_x) + 255,
            layer_pixels(g.layers[0].scroll_y), layer_pixels(g.layers[0].scroll_y) + 223);
    fprintf(f, "camera limits as the load leaves them: camera x 0..%u (scroll_limit_x), camera y 0..%u (scroll_limit_y); view reaches x %u, y %u\n",
            g.scroll_limit_x, g.scroll_limit_y, g.scroll_limit_x + 255u, g.scroll_limit_y + 223u);
    fprintf(f, "scroll_x_min %u, scroll_x_max %u, scroll_x_held %u, scroll_y_held %u, scroll kind %u, view edge rule %u\n", g.scroll_x_min,
            g.scroll_x_max, g.scroll_x_held, g.scroll_y_held, g.scroll_kind, g.view_edge_rule);
    fprintf(f, "the engine's whole map is %u x %u; fall death is at camera limit y + $E6 = %u\n", cols * 256, rows * 256,
            g.scroll_limit_y + 0xE6u);
    fprintf(f, "backdrop: layer_setup_mode $%02X, bg2_follows $%02X, layer 1 edges $%02X (0: not streamed), BG2 map %s,\n",
            g.layer_setup_mode, g.bg2_follows, g.layers[1].edges, g.bg_map[1] == VIDEO_MAP_BACK_TALL ? "$3800 64 x 64 (level 7's)" : "$4000 64 x 32");
    fprintf(f, "  backdrop scroll at start (after one ParallaxUpdate): x %u y %u; BG2 position x %u y %u; backdrop pixel under field pixel (x, y)\n",
            back_x0, back_y0, layer_pixels(g.bg_x[1]), layer_pixels(g.bg_y[1]));
    fprintf(f, "  is the backdrop's at (x %+d, y %+d); BG2 scroll lines (per-band x scrolls) %s; %s\n", back_ox, back_oy,
            (g.line_effects_on & 0x10) || (g.line_effects_on & 0x04) ? "ON: the picture of the backdrop is banded, LL-both shows its start scroll only" : "off",
            g.layers[1].edges ? "backdrop streamed from layout 1" : "backdrop NOT streamed (see LL-backmap)");
    fprintf(f, "exit spots (%u):", g.level_spot_count);
    for (unsigned i = 0; i < g.level_spot_count; i++)
        fprintf(f, " (x %u y %u leads to %02X)", spot_x(g.level_spots, i), spot_y(g.level_spots, i), spot_leads_to(g.level_spots, i));
    fprintf(f, "\nplacements: %u; of kind $81 or $98:", placement_count(level));
    for (unsigned i = 0; i < placement_count(level); i++) {
        Placement pl2 = placement_at(level, i);

        if (pl2.kind == 0x81 || pl2.kind == 0x98)
            fprintf(f, " kind $%02X at x %u y %u byte %02X;", pl2.kind, pl2.x_word & 0x3FFF, pl2.y_word & 0x3FFF, pl2.byte);
    }
    fprintf(f, "\nnot in the pictures: tiles the game rewrites in play (map/tile_changes.c, animated tiles, queued tile writes),\n");
    fprintf(f, "  palette cycles and fades, per-band and per-line scrolls, sprites, the status line, and screen map pokes by tasks\n");
    locks_text(f, level);
    fclose(f);
}

static void dump_level(const char *dir, unsigned level)
{
    char name[64];
    unsigned cols, rows, bcols = 0, brows = 0;
    int has_back;
    unsigned fx, fy, bx0, by0;
    int ox, oy;
    Pic field, back, backc, both, start, backmap;

    memset(vram, 0, sizeof vram);
    DevVramShadow = vram;
    g.level = (uint16_t)level;
    g.level_restart = 0;
    LevelRamSetup();
    DisplayDefaults();
    LevelMapBuild();
    LevelScreenSetup();
    LayersFill();
    LevelSpotsSet();
    TileBehaviourLoad();
    LevelBackdropStart();
    fx = layer_pixels(g.layers[0].scroll_x);
    fy = layer_pixels(g.layers[0].scroll_y);
    ParallaxUpdate();
    BgFollowLayers();
    bx0 = layer_pixels(g.layers[1].scroll_x);
    by0 = layer_pixels(g.layers[1].scroll_y);

    cols = level_map_layout_columns(level, 0);
    rows = level_map_layout_rows(level, 0);
    has_back = level_map_layout_count(level) > 1;
    if (has_back) {
        bcols = level_map_layout_columns(level, 1);
        brows = level_map_layout_rows(level, 1);
    }
    ox = (int)bx0 - (int)fx;
    oy = (int)by0 - (int)fy;

    field = render_layout(0x21, cols, rows, ZERO_MAGENTA);
    snprintf(name, sizeof name, "%02X-field.ppm", level);
    pic_write(dir, name, &field, 0);
    snprintf(name, sizeof name, "%02X-field-grid.ppm", level);
    pic_write(dir, name, &field, 1);
    if (has_back) {
        back = render_layout(0x221, bcols, brows, ZERO_MAGENTA);
        backc = render_layout(0x221, bcols, brows, ZERO_BACKDROP);
    } else {
        back = pic_new(SCREEN, SCREEN);
        backc = pic_new(SCREEN, SCREEN);
    }
    snprintf(name, sizeof name, "%02X-back.ppm", level);
    pic_write(dir, name, &back, 0);
    both = compose(&field, &backc, ox, oy);
    snprintf(name, sizeof name, "%02X-both.ppm", level);
    pic_write(dir, name, &both, 0);
    start = pic_new(SCREEN, 224);
    for (int y = 0; y < 224; y++)
        for (int x = 0; x < SCREEN; x++)
            if ((unsigned)x + fx < (unsigned)both.w && (unsigned)y + fy + LINE_OFFSET < (unsigned)both.h)
                start.px[(size_t)y * start.w + x] = both.px[(size_t)((unsigned)y + fy + LINE_OFFSET) * both.w + (unsigned)x + fx];
    snprintf(name, sizeof name, "%02X-start.ppm", level);
    pic_write(dir, name, &start, 0);
    backmap = render_backmap();
    snprintf(name, sizeof name, "%02X-backmap.ppm", level);
    pic_write(dir, name, &backmap, 0);
    for (int layer = 0; layer < 2; layer++) {
        uint32_t mw, mh, lw, lh;
        const uint16_t *words = level_maps_words(layer, &mw, &mh, &lw, &lh);

        if (words) {
            Pic fillp = render_words(words, mw, mh, lw, lh, ZERO_MAGENTA);

            snprintf(name, sizeof name, "%02X-fill%s.ppm", level, layer ? "back" : "field");
            pic_write(dir, name, &fillp, 0);
            pic_free(&fillp);
        }
    }
    fill_write(dir, level);
    info_write(dir, level, cols, rows, has_back, bcols, brows, ox, oy, bx0, by0);
    pic_free(&field);
    pic_free(&back);
    pic_free(&backc);
    pic_free(&both);
    pic_free(&start);
    pic_free(&backmap);
    DevVramShadow = NULL;
}

/*
 * DevMapDumpLevel: called each update; at update 1, with ALADDIN_DEV_MAPDUMP set, writes the level's files and ends
 * the run (the game is no longer in a state to go on).
 */
void DevMapDumpLevel(uint64_t u)
{
    static const char *dir;
    static unsigned level;
    static int known;

    if (!known) {
        const char *v = getenv("ALADDIN_DEV_MAPDUMP");

        known = 1;
        dir = v && *v ? v : NULL;
        v = getenv("ALADDIN_DEV_MAPDUMP_LEVEL");
        level = v && *v ? (unsigned)strtoul(v, NULL, 0) : 0;
    }
    if (!dir || u != 1)
        return;
    if (level >= LEVEL_MAPS || level_map_screens(level) == 0) {
        fprintf(stderr, "mapdump: level $%02X has no map\n", level);
        exit(2);
    }
    dump_level(dir, level);
    fflush(NULL);
    if (!getenv("ALADDIN_DEV_MAPDUMP_HOLD"))
        exit(0);
    held = 1;
    g.blank = 0;
    g.brightness = 15;
    g.main_layers &= 0x03;              /* the two scroll layers only: no text layer, no sprites */
    g.palette_upload = 1;
    NmiEnable();
}

/* 1 while the loaded level is held on the screen: the update then does only the picture's work */
int DevMapDumpHeld(void)
{
    return held;
}
