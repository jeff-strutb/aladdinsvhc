/*
 * game_tables.c: the screens' tables (game_tables.h). In bank $80: the
 * opening story's scene frames at $DA9D (6 words), its scenes' graphics
 * sets, second-table copies and palettes at $DAA9, $DAAF and $DAB5 (6 bytes
 * each), and its 22 actor placements at $DABB (6 bytes each: kind, the byte
 * for $07, x word, y word). The title menu's line layouts at $CE25 (3
 * bytes) and its mark's places at $CE28 (3 words, byte offsets in the text
 * map); the record's comment puts the layouts in bank $81, but the menu
 * runs with DB = $80 and $80:CE25 holds the title's layouts $0E, $10, $12.
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
#define MENU_BANK 0x80
#define MENU_LINES_ADDR 0xCE25
#define MENU_PLACES_ADDR 0xCE28

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

uint8_t menu_line_layout(unsigned choice)
{
    return cart8(cart_offset(MENU_BANK, (uint16_t)(MENU_LINES_ADDR + choice)));
}

uint16_t menu_mark_place(unsigned choice)
{
    return (uint16_t)(cart16(cart_offset(MENU_BANK, (uint16_t)(MENU_PLACES_ADDR + 2 * choice))) / 2);
}

#define STORY_SETS_ADDR 0xFD8D
#define STORY_TEXT_BANK 0xA5
#define SEQUENCES_BANK 0x80
#define SEQUENCES_TABLE_ADDR 0xCA17

uint16_t story_text_start(unsigned set, uint8_t text)
{
    uint8_t list = cart8(cart_offset(STORY_BANK, (uint16_t)(STORY_SETS_ADDR + set)));
    uint8_t at = (uint8_t)(list + 2 * text);        /* the original's 8-bit index */

    return cart16(cart_offset(STORY_BANK, (uint16_t)(STORY_SETS_ADDR + at)));
}

uint8_t story_text_byte(uint16_t at)
{
    return cart8(cart_offset(STORY_TEXT_BANK, at));
}

/* a place in bank $80, as the sequences' words name them */
static DataRef ref_in_sequences_bank(uint16_t addr)
{
    DataRef r;

    r.at = cart_offset(SEQUENCES_BANK, addr);
    return r;
}

SequenceList sequence_list(uint8_t number)
{
    uint16_t at = (uint16_t)(SEQUENCES_TABLE_ADDR + number);
    SequenceList l;

    l.length = cart8(cart_offset(SEQUENCES_BANK, at));
    l.list = ref_in_sequences_bank(cart16(cart_offset(SEQUENCES_BANK, (uint16_t)(at + 1))));
    return l;
}

SequenceEntry sequence_entry(DataRef list, uint8_t index)
{
    SequenceEntry e;

    e.frames = cart8(list.at + index);
    e.vram_list = ref_in_sequences_bank(cart16(list.at + index + 1u));
    return e;
}

/* The map's tile quarters at $80:BA46 (TileChangeQueueQuarter, read with
   DB = $80; the record's comment names bank $81, where the bytes are code):
   per quarter, 8 bytes apart, the VRAM offset (word) and the quarter's
   byte offset in the metatile (word). The other four bytes of each 8 are
   another table's. */
#define QUARTERS_BANK 0x80
#define QUARTERS_ADDR 0xBA46

TileQuarter tile_quarter(unsigned quarter)
{
    uint16_t at = (uint16_t)(QUARTERS_ADDR + 8u * quarter);
    TileQuarter q;

    q.vram_offset = (int16_t)cart16(cart_offset(QUARTERS_BANK, at));
    q.place = (uint8_t)(cart16(cart_offset(QUARTERS_BANK, (uint16_t)(at + 2))) / 2);
    return q;
}

uint16_t metatile_word(DataRef metatiles, uint16_t offset)
{
    return cart16(metatiles.at + offset);
}

/* ---- the map's collision tests: four tables of 8 bytes in bank $80 (the
   record's comments say $81; the routines run with DB = $80, and $81:BF0A
   on is code), and the behaviour tables in bank $82: $E000 for the map
   whose metatiles are in bank $9C, the last 4 KB of $E000-$FFFF for the
   others (TileBehaviourLoad) ---- */

#define TILE_TABLES_BANK 0x80
#define WALL_KINDS_ADDR 0xBF0A
#define FLOOR_KINDS_ADDR 0xBF12
#define SOFT_KINDS_ADDR 0xBF1A
#define TILE_KINDS_ADDR 0xBF22
#define BEHAVIOUR_BANK 0x82
#define BEHAVIOUR_FIRST_ADDR 0xE000
#define BEHAVIOUR_OTHERS_ADDR 0xF000
#define BEHAVIOUR_FIRST_SET_BANK 0x9C

static uint8_t tile_table(uint16_t addr, uint8_t low3)
{
    return cart8(cart_offset(TILE_TABLES_BANK, (uint16_t)(addr + (low3 & 7))));
}

bool tile_is_wall(uint8_t low3) { return tile_table(WALL_KINDS_ADDR, low3) != 0; }
bool tile_is_floor(uint8_t low3) { return tile_table(FLOOR_KINDS_ADDR, low3) != 0; }
bool tile_is_soft(uint8_t low3) { return tile_table(SOFT_KINDS_ADDR, low3) != 0; }
uint8_t tile_kind(uint8_t low3) { return tile_table(TILE_KINDS_ADDR, low3); }

uint16_t probe_word(DataRef list, uint16_t at)
{
    return cart16(list.at + at);
}

uint8_t tile_behaviour_byte(DataRef metatiles, uint16_t i)
{
    uint16_t from = metatiles.at == cart_offset(BEHAVIOUR_FIRST_SET_BANK, 0x8000) ? BEHAVIOUR_FIRST_ADDR
                                                                                  : BEHAVIOUR_OTHERS_ADDR;

    return cart8(cart_offset(BEHAVIOUR_BANK, (uint16_t)(from + (i & 0x0FFF))));
}
