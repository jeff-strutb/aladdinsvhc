/*
 * level_picture.c: a level's picture composed from the cartridge's level data (level_picture.h). The level is loaded as
 * LevelLoad does it (the same calls), the video memory writes are kept in a copy (DevVramShadow, video.c), and the play
 * field and the backdrop are drawn from the screen map, the metatiles, the tiles and the palette as the load left them,
 * in Mode 1's order: field high priority, backdrop high priority, field, backdrop. Sprites, text and the status line are
 * not in it. Nothing of the original's. Moved here from the developer's map dump (system/dev_mapdump.c), which uses it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/data.h"
#include "data/level_tables.h"
#include "data/scroll_tables.h"
#include "game/game.h"
#include "level/level.h"
#include "map/map.h"
#include "system/state.h"
#include "system/system.h"
#include "video/level_maps.h"
#include "video/level_picture.h"
#include "video/video.h"
#include "video/video_internal.h"

#define TRANSPARENT_MAGENTA 0xFF00FFu
#define EMPTY_GREY 0x404040u

uint16_t *DevVramShadow;
static uint16_t vram[PIC_VRAM_WORDS];

Pic PicNew(int w, int h)
{
    Pic p;

    p.w = w;
    p.h = h;
    p.px = calloc((size_t)w * h, sizeof *p.px);
    p.opaque = calloc((size_t)w * h, 1);
    if (!p.px || !p.opaque) {
        fprintf(stderr, "level picture: no memory for a %d x %d picture\n", w, h);
        exit(1);
    }
    return p;
}

void PicFree(Pic *p)
{
    free(p->px);
    free(p->opaque);
}

uint32_t PicRgb(uint16_t c)
{
    unsigned r = c & 31, gr = c >> 5 & 31, b = c >> 10 & 31;

    /* a 5 bit channel to 8 bits as the engine does: its bits repeated */
    return (r << 3 | r >> 2) << 16 | (gr << 3 | gr >> 2) << 8 | (b << 3 | b >> 2);
}

unsigned PicLayerPixels(uint32_t position)
{
    return (position >> 8) & 0xFFFFu;
}

const uint16_t *PicVram(void)
{
    return vram;
}

/* the 4bpp pixel (0-15) at column x, row y of tile n of the low tile set (video address n * 16, a row a word for
   planes 0 and 1, then the same 8 words on for planes 2 and 3) */
static unsigned tile_pixel(unsigned n, unsigned x, unsigned y)
{
    unsigned base = (n * 16u) & (PIC_VRAM_WORDS - 1), bit = 7 - x;
    uint16_t lo = vram[base + y], hi = vram[(base + 8 + y) & (PIC_VRAM_WORDS - 1)];

    return (lo >> bit & 1u) | (lo >> (8 + bit) & 1u) << 1 | (hi >> bit & 1u) << 2 | (hi >> (8 + bit) & 1u) << 3;
}

/* a map word's 8 x 8 tile into the picture at x, y; colour 0 as `zero` says */
void PicDrawTile(Pic *p, int x, int y, uint16_t word, int zero)
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
                p->px[at] = PicRgb(g.palette[pal * 16 + c]);
                p->opaque[at] = (uint8_t)(1 + (word >> 13 & 1));   /* 1 opaque, 2 opaque in a tile of the high priority */
            } else if (zero == PIC_ZERO_MAGENTA) {
                p->px[at] = TRANSPARENT_MAGENTA;
            } else {
                p->px[at] = PicRgb(g.palette[0]);
            }
        }
}

/* the word of the screen map at place `place`, as the scroll layers read it (video/scroll_layer.c screen_at) */
uint16_t PicPlaceWord(unsigned place)
{
    return g.screen_map.places[place % SCREEN_MAP_PLACES];
}

Pic PicLayout(unsigned first, unsigned cols, unsigned rows, int zero, int left, int top, int w, int h)
{
    Pic p = PicNew(w, h);
    DataRef metatiles = level_metatiles(g.map_level);

    for (unsigned r = 0; r < rows; r++)
        for (unsigned c = 0; c < cols; c++) {
            uint16_t word = PicPlaceWord(first + r * 32 + c);
            int sx = (int)(c * PIC_SCREEN) - left, sy = (int)(r * PIC_SCREEN) - top;

            if (sx >= w || sy >= h || sx + PIC_SCREEN <= 0 || sy + PIC_SCREEN <= 0)
                continue;
            if (word & 0x8000) {
                for (int y = 0; y < PIC_SCREEN; y++)
                    for (int x = 0; x < PIC_SCREEN; x++)
                        if (sx + x >= 0 && sy + y >= 0 && sx + x < w && sy + y < h)
                            p.px[(size_t)(sy + y) * p.w + sx + x] = EMPTY_GREY;
                continue;
            }
            for (unsigned my = 0; my < 16; my++)
                for (unsigned mx = 0; mx < 16; mx++) {
                    unsigned at = (unsigned)(word + 2 * (my * 16 + mx)) / 2u;
                    uint16_t meta = (uint16_t)((at < LEVEL_TILES_WORDS ? g.level_tiles[at] : 0) | 0x8000);

                    for (unsigned half = 0; half < 4; half++)
                        PicDrawTile(&p, sx + (int)(mx * 16 + (half & 1) * 8), sy + (int)(my * 16 + (half >> 1) * 8),
                                    level_metatile_word(metatiles, meta, half * 2), zero);
                }
        }
    return p;
}

/* the field over the backdrop; the backdrop's pixel under the field's (x, y) is its own at (x + ox, y + oy). The
   order is Mode 1's: field high priority, backdrop high priority, field, backdrop. */
Pic PicCompose(const Pic *field, const Pic *back, int ox, int oy)
{
    Pic p = PicNew(field->w, field->h);

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
                p.px[at] = PicRgb(g.palette[0]);
        }
    return p;
}

void PicLevelLoad(unsigned level, PicLoaded *loaded)
{
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
    loaded->field_x = PicLayerPixels(g.layers[0].scroll_x);
    loaded->field_y = PicLayerPixels(g.layers[0].scroll_y);
    ParallaxUpdate();
    BgFollowLayers();
    loaded->back_x = PicLayerPixels(g.layers[1].scroll_x);
    loaded->back_y = PicLayerPixels(g.layers[1].scroll_y);
}

void PicShadowOff(void)
{
    DevVramShadow = NULL;
}

int LevelPicture(unsigned level, uint32_t *rgb)
{
    PicLoaded ld;
    unsigned cols, rows, bcols = 0, brows = 0;
    int ox, oy, px, py, cx, cy;
    Pic field, back, both;

    if (level >= LEVEL_MAPS || level_map_screens(level) == 0)
        return 0;
    PicLevelLoad(level, &ld);
    cols = level_map_layout_columns(level, 0);
    rows = level_map_layout_rows(level, 0);
    if (level_map_layout_count(level) > 1) {
        bcols = level_map_layout_columns(level, 1);
        brows = level_map_layout_rows(level, 1);
    }
    /* the window's rows are the layer's from scroll y + 1; the backdrop's pixel under the field's (x, y) is its own at
       (x + ox, y + oy) */
    ox = (int)ld.back_x - (int)ld.field_x;
    oy = (int)ld.back_y - (int)ld.field_y;
    field = PicLayout(0x21, cols, rows, PIC_ZERO_MAGENTA, (int)ld.field_x, (int)ld.field_y + PIC_LINE_OFFSET, PIC_SCREEN, PIC_WINDOW_H);
    if (bcols)
        back = PicLayout(0x221, bcols, brows, PIC_ZERO_BACKDROP, (int)ld.field_x + ox, (int)ld.field_y + PIC_LINE_OFFSET + oy,
                         PIC_SCREEN, PIC_WINDOW_H);
    else
        back = PicNew(PIC_SCREEN, PIC_WINDOW_H);
    both = PicCompose(&field, &back, 0, 0);
    /* centred on where the player stands, kept inside the window */
    px = (int)((g.actors[0].x >> 8) & 0xFFFF) - (int)ld.field_x;
    py = (int)((g.actors[0].y >> 8) & 0xFFFF) - (int)ld.field_y;
    cx = px - LEVEL_PICTURE_W / 2;
    cy = py - LEVEL_PICTURE_H / 2;
    cx = cx < 0 ? 0 : cx > PIC_SCREEN - LEVEL_PICTURE_W ? PIC_SCREEN - LEVEL_PICTURE_W : cx;
    cy = cy < 0 ? 0 : cy > PIC_WINDOW_H - LEVEL_PICTURE_H ? PIC_WINDOW_H - LEVEL_PICTURE_H : cy;
    for (int y = 0; y < LEVEL_PICTURE_H; y++)
        memcpy(rgb + (size_t)y * LEVEL_PICTURE_W, both.px + (size_t)(cy + y) * both.w + cx, LEVEL_PICTURE_W * sizeof *rgb);
    PicFree(&field);
    PicFree(&back);
    PicFree(&both);
    PicShadowOff();
    return 1;
}
