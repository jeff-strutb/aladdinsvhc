/*
 * game_tables.h: the screens' tables from the cartridge (game/), read
 * through data/ (data.h). Owned by the stage 4 system session.
 */
#ifndef ALADDIN_GAME_TABLES_H
#define ALADDIN_GAME_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#include "data/data.h"

/* ---- the opening story (game/opening_story.c) ---- */

#define OPENING_SCENES 6
#define OPENING_ACTORS 22

typedef struct OpeningScene {
    uint8_t gfx_set;            /* its packed graphics set; 0 none of its own */
    uint8_t copy;               /* its entry of the second VRAM copy table (the record's X / 7) */
    uint8_t palette;            /* its palette set */
} OpeningScene;

typedef struct OpeningActor {
    uint8_t kind;               /* the actor's kind */
    uint8_t spawn_byte;         /* its $07 */
    uint16_t x, y;              /* its place, pixels */
} OpeningActor;

uint16_t opening_scene_frames(unsigned scene);  /* the frames scene 0-5 runs */
OpeningScene opening_scene(unsigned scene);     /* scene 0-5 */
OpeningActor opening_actor(unsigned entry);     /* entry 0-21 */

/* ---- the ending's story (game/story_scenes.c): the same forms, from the tables at $80:DB3F-$DB5D ---- */

#define ENDING_SCENES 6
uint16_t ending_scene_frames(unsigned scene);   /* the frames scene 0-5 runs */
OpeningScene ending_scene(unsigned scene);      /* scene 0-5 */
OpeningActor ending_actor(unsigned entry);      /* entry 0-7: kind 0 is the player */

/* ---- the title's menu (game/menu_mark.c, menu_mark_draw.c) ---- */

#define MENU_CHOICES 3

uint8_t menu_line_layout(unsigned choice);      /* the text layout of choice 0-2's line */
uint16_t menu_mark_place(unsigned choice);      /* the text map's word index of choice 0-2's mark */

/* ---- the story's texts (game/story_text.c) ---- */

/* where text `text` of set `set` (0 the opening story's, 1 the later
   scenes') starts, as a place story_text_byte reads from */
uint16_t story_text_start(unsigned set, uint8_t text);
/* the text's byte at place `at`: a character (below $FB), a code ($FB place,
   $FC palette, $FD wait, $FE page, $FF next row), or 0, its end; the codes'
   bytes follow at at + 1 on */
uint8_t story_text_byte(uint16_t at);

/* ---- the level's value sequences (level/sequences.c) ---- */

typedef struct SequenceList {
    uint8_t length;             /* its length in bytes, 3 an entry */
    DataRef list;
} SequenceList;
typedef struct SequenceEntry {
    uint8_t frames;             /* frames the word is held; bit 7 set: the list ends here */
    DataRef vram_list;          /* the word: the VRAM list it names */
} SequenceEntry;
SequenceList sequence_list(uint8_t number);     /* number: the table's offset, 3 an entry */
SequenceEntry sequence_entry(DataRef list, uint8_t index);  /* index: the entry's offset in the list */

/* ---- the map's tile changes (map/tile_changes.c) ---- */

/* a map tile's quarter (its 8 x 8 tiles: 0 top left, 1 top right, 2 bottom
   left, 3 bottom right), as TileChangeQueueQuarter writes it */
typedef struct TileQuarter {
    int16_t vram_offset;        /* from the VRAM word of the tile's bottom right quarter */
    uint8_t place;              /* its word in the metatile: 0-3 */
} TileQuarter;
TileQuarter tile_quarter(unsigned quarter);     /* quarter 0-3 */
/* the word `offset` bytes into the metatiles `metatiles` names (from the
   start of their bank, level_metatiles in data.h) */
uint16_t metatile_word(DataRef metatiles, uint16_t offset);

/* ---- the level select (game/level_select.c) ---- */
uint8_t level_select_last_stage(uint8_t part);          /* by part (0-6): its last stage ($80:CE2E) */

/* ---- the options screen (game/options.c; $80:CE35 on) ---- */
#define OPTION_ITEMS 4
uint16_t option_setting_word(unsigned line, unsigned setting);  /* the map word of line 0-4 of a setting 0-3 */
uint16_t option_cursor_place(unsigned item);            /* the item's place in the text map, a byte offset */
uint16_t option_code_press(unsigned i);                 /* the cheat code's i'th press (pad 2's word), i 1-7 */

/* ---- the story card between acts (game/story_card.c) ---- */

/* part 1-7's list of commands (the record's offset in bank $80), and a
   byte of any list or text at an offset */
uint16_t story_card_list_start(uint8_t part);
uint8_t story_card_list_byte(uint16_t at);
uint16_t story_card_text_start(uint8_t text);       /* text 0 up: where its lines are, for story_card_text_byte */
uint8_t story_card_text_byte(uint16_t at);
uint16_t story_card_text_word(uint16_t at);

/* ---- the map's collision tests (map/collision.c, tile_behaviour.c) ---- */

/* by a tile behaviour's low 3 bits (the tables at $80:BF0A, $BF12, $BF1A,
   $BF22): a wall actors are pushed out of, a floor they settle on, ground
   the player does not stand firm on, and the tile's kind (map.h) */
bool tile_is_wall(uint8_t low3);
bool tile_is_floor(uint8_t low3);
bool tile_is_soft(uint8_t low3);
uint8_t tile_kind(uint8_t low3);
/* the word `at` bytes into a probe list (Actor.probes): at a probe's
   offset its x offset, at + 2 its y offset */
uint16_t probe_word(DataRef list, uint16_t at);
/* byte i (0-4095) of the tile behaviour table of the map whose metatiles
   `metatiles` names (level_metatiles in data.h) */
uint8_t tile_behaviour_byte(DataRef metatiles, uint16_t i);

#endif
