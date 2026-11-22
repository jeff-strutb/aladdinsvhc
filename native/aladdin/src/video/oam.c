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
    list = calloc(budget, sizeof *list);
    if (!list) {
        fprintf(stderr, "oam: no memory for %u sprites\n", (unsigned)budget);
        exit(1);
    }
}

const nshvc_sprite *OamSprites(void)
{
    return list;
}

uint32_t OamSpriteCapacity(void)
{
    return budget;
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

        if (y < -32 || y >= view.h)
            continue;                   /* off the view's height */
        word = (uint16_t)(((p->tile + base) & 0xCFFF) | priority);
        if (mirrored)
            word ^= 0x4000;
        word |= palette;
        s = &list[g.sprite_count++];
        memset(s, 0, sizeof *s);
        s->x = x;
        s->y = y;
        s->width = s->height = (uint16_t)(p->tile & 0x1000 ? 2 : 1);
        if (slot) {
            s->tiles = video_slot_tiles();
            s->tile = (uint16_t)(SLOT_TILES * (slot - 5) + (p->tile & 0x1FF));
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
    draw_list(0);
    draw_list(1);
    if (player->state && (player->anim_flags & 0x08))
        draw_actor(player);
    draw_list(2);
    draw_list(3);
}
