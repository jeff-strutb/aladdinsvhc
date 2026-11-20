/*
 * game_tables.c: the screens' tables (game_tables.h). In bank $80: the
 * opening story's scene frames at $DA9D (6 words), its scenes' graphics
 * sets, second-table copies and palettes at $DAA9, $DAAF and $DAB5 (6 bytes
 * each), and its 22 actor placements at $DABB (6 bytes each: kind, the byte
 * for $07, x word, y word).
 */
#include "data/data_internal.h"
#include "data/game_tables.h"

#define STORY_BANK 0x80
#define SCENE_FRAMES_ADDR 0xDA9D
#define SCENE_GFX_ADDR 0xDAA9
#define SCENE_COPY_ADDR 0xDAAF
#define SCENE_PALETTE_ADDR 0xDAB5
#define STORY_ACTORS_ADDR 0xDABB
#define STORY_ACTOR_BYTES 6

uint16_t opening_scene_frames(unsigned scene)
{
    return cart16(cart_offset(STORY_BANK, (uint16_t)(SCENE_FRAMES_ADDR + 2 * scene)));
}

OpeningScene opening_scene(unsigned scene)
{
    OpeningScene s;

    s.gfx_set = cart8(cart_offset(STORY_BANK, (uint16_t)(SCENE_GFX_ADDR + scene)));
    s.copy = (uint8_t)(cart8(cart_offset(STORY_BANK, (uint16_t)(SCENE_COPY_ADDR + scene))) / 7);
    s.palette = cart8(cart_offset(STORY_BANK, (uint16_t)(SCENE_PALETTE_ADDR + scene)));
    return s;
}

OpeningActor opening_actor(unsigned entry)
{
    uint16_t at = (uint16_t)(STORY_ACTORS_ADDR + STORY_ACTOR_BYTES * entry);
    OpeningActor a;

    a.kind = cart8(cart_offset(STORY_BANK, at));
    a.spawn_byte = cart8(cart_offset(STORY_BANK, (uint16_t)(at + 1)));
    a.x = cart16(cart_offset(STORY_BANK, (uint16_t)(at + 2)));
    a.y = cart16(cart_offset(STORY_BANK, (uint16_t)(at + 4)));
    return a;
}
