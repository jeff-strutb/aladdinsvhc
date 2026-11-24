/*
 * oam.c: the frame's sprites. The game builds g.sprites each frame as the
 * engine's sprites, in the original's order (the first in front); the
 * frame's end sends them when g.frame_ready says the frame is built.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor/actor.h"
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "video/view.h"

#define SPRITES 128             /* the original's sprites, at its view */
#define FLASH_PALETTE 0x0A00    /* palette 5, as the tile word's bits */

/* the sprites not yet used this frame (the original's $084D) */
static uint32_t sprites_free;

/* the frame's sprites, as the engine's, sized at init for the view */
static nshvc_sprite *list;
static uint8_t *owner;                  /* each sprite's actor record (its place in g.actors), for the picture's easing */
static int16_t (*anchor)[2];            /* each sprite's actor's place on the screen as it was drawn (draw_actor's at_x, at_y) */
static int32_t (*anchor_fx)[2];         /* the same in 256ths: the actor's 16.8 place less the origin's (the camera's 24.8 and the shift) */
static uint32_t budget;

/*
 * The sprites a view allows: the original's 128 at 256 x 224, and for a
 * larger view 128 scaled by its area, rounded up.
 */
static uint32_t sprite_budget(void)
{
    uint64_t area = (uint64_t)view.w * (uint64_t)view.h;
    uint64_t whole = (uint64_t)VIEW_ORIGINAL_W * VIEW_ORIGINAL_H;

    return (uint32_t)(((uint64_t)SPRITES * area + whole - 1) / whole);
}

void OamInit(void)
{
    budget = sprite_budget();
    free(list);                         /* the view changed at a level's load or a still screen: the list is sized again */
    free(owner);
    free(anchor);
    free(anchor_fx);
    list = calloc(budget, sizeof *list);
    owner = calloc(budget, sizeof *owner);
    anchor = calloc(budget, sizeof *anchor);
    anchor_fx = calloc(budget, sizeof *anchor_fx);
    if (!list || !owner || !anchor || !anchor_fx) {
        fprintf(stderr, "oam: no memory for %u sprites\n", (unsigned)budget);
        exit(1);
    }
}

/* the view changed in play (VIEW.md 4.8): the lists made to the new budget, what they hold kept (the frame's sprites are rebuilt by the next step) */
void OamResize(void)
{
    uint32_t was = budget;

    budget = sprite_budget();
    list = realloc(list, budget * sizeof *list);
    owner = realloc(owner, budget * sizeof *owner);
    anchor = realloc(anchor, budget * sizeof *anchor);
    anchor_fx = realloc(anchor_fx, budget * sizeof *anchor_fx);
    if (!list || !owner || !anchor || !anchor_fx) {
        fprintf(stderr, "oam: no memory for %u sprites\n", (unsigned)budget);
        exit(1);
    }
    if (budget > was) {
        memset(list + was, 0, (budget - was) * sizeof *list);
        memset(owner + was, 0, (budget - was) * sizeof *owner);
        memset(anchor + was, 0, (budget - was) * sizeof *anchor);
        memset(anchor_fx + was, 0, (budget - was) * sizeof *anchor_fx);
    }
}

const nshvc_sprite *OamSprites(void)
{
    return list;
}

const uint8_t *OamSpriteOwners(void)
{
    return owner;
}

const int16_t (*OamSpriteAnchors(void))[2]
{
    return (const int16_t (*)[2])anchor;
}

const int32_t (*OamSpriteAnchorsFx(void))[2]
{
    return (const int32_t (*)[2])anchor_fx;
}

uint32_t OamSpriteCapacity(void)
{
    return budget;
}

/* The frame's sprites, as far as g.sprite_count, are state: video_vblank copies them to the shown lists while g.frame_ready stays
   set, as in a scene whose update builds none, so they outlive the update that built them (the snapshot found this). */
size_t oam_snapshot_size(void)
{
    return 4 + (size_t)g.sprite_count * (sizeof *list + sizeof *owner + sizeof *anchor + sizeof *anchor_fx);
}

size_t oam_snapshot_save(uint8_t *out)
{
    uint32_t n = g.sprite_count < budget ? g.sprite_count : budget;
    uint8_t *o = out;

    o[0] = (uint8_t)n; o[1] = (uint8_t)(n >> 8); o[2] = (uint8_t)(n >> 16); o[3] = (uint8_t)(n >> 24);
    o += 4;
    memcpy(o, list, n * sizeof *list);
    o += n * sizeof *list;
    memcpy(o, owner, n * sizeof *owner);
    o += n * sizeof *owner;
    memcpy(o, anchor, n * sizeof *anchor);
    o += n * sizeof *anchor;
    memcpy(o, anchor_fx, n * sizeof *anchor_fx);
    o += n * sizeof *anchor_fx;
    return (size_t)(o - out);
}

/* the section as oam_snapshot_save wrote it fits `len` */
int oam_snapshot_check(const uint8_t *in, size_t len)
{
    uint32_t n;

    if (len < 4)
        return 0;
    n = (uint32_t)in[0] | (uint32_t)in[1] << 8 | (uint32_t)in[2] << 16 | (uint32_t)in[3] << 24;
    return len == 4 + (size_t)n * (sizeof *list + sizeof *owner + sizeof *anchor + sizeof *anchor_fx);
}

/* put in (the view's lists are sized): as many as the lists hold */
void oam_snapshot_load(const uint8_t *in)
{
    uint32_t n = (uint32_t)in[0] | (uint32_t)in[1] << 8 | (uint32_t)in[2] << 16 | (uint32_t)in[3] << 24, all = n;
    const uint8_t *p = in + 4;

    if (n > budget)
        n = budget;
    memcpy(list, p, n * sizeof *list);
    p += all * sizeof *list;
    memcpy(owner, p, n * sizeof *owner);
    p += all * sizeof *owner;
    memcpy(anchor, p, n * sizeof *anchor);
    p += all * sizeof *anchor;
    memcpy(anchor_fx, p, n * sizeof *anchor_fx);
}

/*
 * @routine 81:95D5 OamBufferClear
 * WHAT IT DOES: Takes every sprite off the screen.
 * HOW: The original puts all 128 below the screen (y $E0); none drawn is
 * the same picture.
 */
void OamBufferClear(void)
{
    g.sprite_count = 0;
}

/*
 * @routine 81:9230 OamBuildBegin
 * WHAT IT DOES: Starts the frame's sprites: none used, all free (128 at the
 * original's view, more in a larger one: see sprite_budget), the frame not
 * ready.
 */
void OamBuildBegin(void)
{
    g.frame_ready = 0;
    g.sprite_count = 0;
    sprites_free = budget;
}

/* the point of the level at the screen's top left, for sprites */
static int16_t origin_x, origin_y;
static uint32_t origin_fx, origin_fy;   /* the same in 256ths (24 bits), for the picture's places between pixels */

/* a 24-bit difference of two 16.8 places, signed */
static int32_t fx_diff(uint32_t a, uint32_t b)
{
    uint32_t d = (a - b) & 0xFFFFFFu;

    return d & 0x800000u ? (int32_t)d - 0x1000000 : (int32_t)d;
}

/*
 * The play field's x in 256ths, as the sprites' places between pixels take it. The scroll kinds that run the scroll on
 * by 256ths (2, 6, 10 and 14, all to the right) add to its fraction every frame, and then the tail holds its pixels at
 * the level's right bound (scroll_x_max, or scroll_limit_x), the fraction counting on underneath (scroll_update.c, as
 * the original). The original drew sprites at whole pixels, so the fraction never showed; between pixels it moved a
 * sprite that stood still by up to a pixel every frame while the layers stood still (the credits' "The End", Jeff
 * 2026-11-24). A camera held at that bound stands at the bound's pixel: its fraction is taken as 0 there.
 */
static uint32_t camera_fx(void)
{
    uint32_t x = g.layers[0].scroll_x;
    uint16_t px = (uint16_t)(x >> 8);
    int runs_on = g.scroll_kind == 2 || g.scroll_kind == 6 || g.scroll_kind == 10 || g.scroll_kind == 14;

    if (runs_on && (px == g.scroll_limit_x || px == g.scroll_x_max))
        return x & 0xFFFF00u;
    return x;
}

/*
 * @routine 81:9309 OamDrawActor
 * WHAT IT DOES: Puts one actor's frame into this frame's sprites at its
 * place on the screen: each piece within 32 lines above to the view's height
 * down gets a sprite; a frame with as many pieces as are left, or more, is not drawn.
 * HOW: Flickering actors (+$08 bit 4) skipped on frames whose
 * g.sprite_flicker has bit 0; the palette from the actor's look, or the
 * flash palette (+$08 bit 5 and g.sprite_flicker bit 1, which no script
 * sets), or an actor's slot past the original's four: its tiles from its
 * own tile set, its palette 3 + the slot's number while its look's are 7;
 * each piece's x (its mirrored x when the actor faces right, the
 * flip across turned over), y and tile word plus the actor's tile base,
 * its priority from +$09 bits 4-5 turned over. Every piece counts against
 * the sprites left, drawn or not. The piece's tile word bit 12 makes it
 * 16 x 16 (else 8 x 8). x is the piece's whole place in the view (the
 * original's 9 bits, which showed a piece 256 or more across at the left, are
 * not kept).
 */
static void draw_actor(const Actor *a)
{
    const SpritePiece *p;
    unsigned n;
    uint16_t palette, base, priority;
    unsigned slot = look_slot(a->look);       /* 5 and up: tiles and palette of its own */
    int16_t at_x, at_y;
    int mirrored = a->flip & 1;

    if ((a->anim_flags & 0x10) && (g.sprite_flicker & 0x01))
        return;
    palette = (uint16_t)(a->look & 0x0E00);
    if ((a->anim_flags & 0x20) && (g.sprite_flicker & 0x02))
        palette = FLASH_PALETTE;
    n = actor_pieces(a, &p);
    if (n >= sprites_free)
        return;
    sprites_free -= n;
    at_x = (int16_t)((uint16_t)(a->x >> 8) - (uint16_t)origin_x);
    at_y = (int16_t)((uint16_t)(a->y >> 8) - (uint16_t)origin_y);
    base = (uint16_t)(a->look & 0x01FF);
    priority = (uint16_t)(((a->flags << 8) ^ 0x3000) & 0x3000);
    for (unsigned i = 0; i < n; i++, p++) {
        int16_t x = (int16_t)(at_x + (mirrored ? p->x_mirrored : p->x));
        int16_t y = (int16_t)(at_y + p->y);
        uint16_t word;
        nshvc_sprite *s;

        if (y < -32 - view.dy || y >= VIEW_ORIGINAL_H + view.dy)
            continue;                   /* off the view's height (the original's screen is dy down in it) */
        word = (uint16_t)(((p->tile + base) & 0xCFFF) | priority);
        if (mirrored)
            word ^= 0x4000;
        word |= palette;
        owner[g.sprite_count] = (uint8_t)(a - g.actors);
        anchor[g.sprite_count][0] = at_x;
        anchor[g.sprite_count][1] = at_y;
        anchor_fx[g.sprite_count][0] = fx_diff(a->x, origin_fx);
        anchor_fx[g.sprite_count][1] = fx_diff(a->y, origin_fy);
        s = &list[g.sprite_count++];
        memset(s, 0, sizeof *s);
        s->x = x;
        s->y = y;
        s->width = s->height = (uint16_t)(p->tile & 0x1000 ? 2 : 1);
        if (slot) {
            s->tiles = video_slot_tiles(slot);
            s->tile = (uint16_t)(SLOT_TILES * ((slot - 5) % SLOT_SET_SLOTS) + (p->tile & 0x1FF));
            s->palette = (word & SLOT_PALETTE_BITS) == SLOT_PALETTE_BITS ? slot + 3 : (uint32_t)(word >> 9 & 7);
        } else {
            s->tiles = video_sprite_tiles();
            s->tile = (uint16_t)(256 + (word & 0x1FF));
            s->palette = (uint8_t)(word >> 9 & 7);
        }
        s->priority = (uint8_t)(word >> 12 & 3);
        s->hflip = (uint8_t)(word >> 14 & 1);
        s->vflip = (uint8_t)(word >> 15 & 1);
    }
}

/*
 * @routine 81:92EE OamDrawList
 * WHAT IT DOES: Draws the actors of one of the draw lists, in order.
 */
static void draw_list(int list)
{
    for (unsigned i = 0; i < g.draw_count[list]; i++)
        draw_actor(g.draw_list[list][i]);
}

/*
 * @routine 81:9258 OamDrawActors
 * WHAT IT DOES: Puts this frame's actors into the sprites, list by list,
 * the player between the second list and the third; the rest stay off.
 * HOW: The sprites' origin is the play field's scroll plus BG1's shifts,
 * or BG2's when g.sprites_take_bg2_shift. The player is drawn when its
 * record is live and animated (+$08 bit 3).
 */
void OamDrawActors(void)
{
    const Actor *player = &g.actors[0];

    origin_x = (int16_t)((uint16_t)(g.layers[0].scroll_x >> 8) +
                         (g.sprites_take_bg2_shift ? g.bg2_x_shift : g.bg1_x_shift));
    origin_y = (int16_t)((uint16_t)(g.layers[0].scroll_y >> 8) +
                         (g.sprites_take_bg2_shift ? g.bg2_y_shift : g.layer_offset_y));
    origin_fx = (camera_fx() +((uint32_t)(g.sprites_take_bg2_shift ? g.bg2_x_shift : g.bg1_x_shift) << 8)) & 0xFFFFFFu;
    origin_fy = (g.layers[0].scroll_y + ((uint32_t)(g.sprites_take_bg2_shift ? g.bg2_y_shift : g.layer_offset_y) << 8)) & 0xFFFFFFu;
    draw_list(0);
    draw_list(1);
    if (player->state && (player->anim_flags & 0x08))
        draw_actor(player);
    draw_list(2);
    draw_list(3);
}
