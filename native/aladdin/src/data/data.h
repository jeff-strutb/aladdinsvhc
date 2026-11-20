/*
 * data.h: the cartridge's data as typed tables (plan section 3, "ROM data").
 *
 * data_load() reads assets/cart.dat (tools/extract_assets.py: the player's
 * own cartridge, checked by SHA-1, with the code left out) once, at init,
 * through nshvc_asset_*. Every table's place in the cartridge is written
 * once, in data/ only; the rest of the game sees each table by what it holds,
 * through a reader, and never a bank, an address or a raw read. Data that
 * names other data in its own format is passed on as a DataRef, which only
 * data/ looks inside.
 *
 * Who writes a reader (the master's work split, 2026-11-19, PLAN.md): this
 * header holds the loader and the formats more than one area reads (sound
 * lists, palettes, packed graphics, level graphics, unpack sets); each area's
 * own tables are in data/<area>_tables.{c,h}, written by the session that
 * converts the routine reading them, from that routine's code.
 */
#ifndef ALADDIN_DATA_H
#define ALADDIN_DATA_H

#include <stdint.h>

/* where more of the cartridge's data is, as the data itself names it; opaque
   outside data/ */
typedef struct DataRef { uint32_t at; } DataRef;

void data_load(void);           /* at init; ends the run with a message when cart.dat is missing or short */

/* ---- sound: the upload lists (audio/spc_upload.c's SpcUpload, the table
   of 3-byte entries the decomp lists in config/aladdin/spclists.csv) ---- */

/* one block of a list: bytes for the sound memory at `to`. Blocks of the
   SPC700 driver's own code come back as zeros (cart.dat leaves code out;
   the native driver is C). */
typedef struct SoundBlock {
    uint16_t to;                /* where the original put it in the sound memory */
    uint16_t length;            /* bytes */
    const uint8_t *bytes;
} SoundBlock;

/* list `list` (the original's X divided by 3: 0 the driver, 1 and 2 the
   sample sets, then the songs). Returns its block count, or -1 for an
   entry the cartridge leaves empty. */
int sound_list_blocks(unsigned list);
SoundBlock sound_list_block(unsigned list, unsigned block);  /* block < sound_list_blocks(list) */
uint16_t sound_list_start(unsigned list);   /* the address its end names: where the driver goes on */

/* ---- graphics ---- */

/* the palette loads (video/palette.c's PaletteLoad): load `set` copies
   palette_load_count(set) colours to the palette from colour
   palette_load_first(set) on. The count is the original's: 0 copies 65,536. */
#define PALETTE_LOADS 98
uint16_t palette_load_count(uint8_t set);
uint8_t palette_load_first(uint8_t set);     /* a colour index, 0-255 */
uint16_t palette_load_colour(uint8_t set, uint16_t i);  /* its i'th colour, 15-bit */

/* packed graphics (video/unpack.c's UnpackGroup): groups of 8 bytes, each
   a flag byte and a fill byte, then a source byte for each flag bit set
   (from the top) and the fill for each clear */
typedef struct PackedGraphics {
    DataRef src;
    uint16_t groups;            /* 8-byte groups it unpacks to */
} PackedGraphics;
uint32_t unpack_graphics(PackedGraphics p, uint8_t *out);   /* writes p.groups * 8 bytes; returns them */

/* a level's graphics (video/unpack.c's LevelGraphicsLoad): parts unpacked and
   sent to video memory, then parts kept unpacked one after another in the
   graphics buffer (from its $5800 on) */
typedef struct LevelVramPart {
    PackedGraphics packed;
    uint16_t bytes;             /* how many of the unpacked bytes go */
    uint16_t to;                /* the VRAM word address they go to */
} LevelVramPart;
#define LEVEL_GRAPHICS 66       /* the levels the table has (its first list starts after 66 words) */
unsigned level_vram_parts(uint16_t level);
LevelVramPart level_vram_part(uint16_t level, unsigned i);
unsigned level_kept_parts(uint16_t level);
PackedGraphics level_kept_part(uint16_t level, unsigned i);

/* the packed sets (video/unpack.c's UnpackSetBegin): set `set` unpacks to
   the graphics buffer at `to` */
typedef struct UnpackSetEntry {
    uint16_t to;                /* its place in the graphics buffer (the original's bank $7F) */
    PackedGraphics packed;
} UnpackSetEntry;
#define UNPACK_SETS 50          /* $80:F039 on, 7 bytes each; from set 50 the bytes are not sets (banks below $80) */
UnpackSetEntry unpack_set(uint8_t set);    /* set < UNPACK_SETS */

/* an animation set's graphics (video/unpack.c's AnimSetGraphicsUnpack): the
   set's entry gives its packed graphics; a length of 0 has none. The
   record's traces read entries only for sets 32 and up (the words for sets
   0-31 would lie in the code at $82:AC18-$AC57, which no trace reads as
   data), so `set` is 32 or more. */
typedef struct AnimSetGraphics {
    PackedGraphics packed;      /* groups: the length's low 14 bits / 8, as the original's (not rounded up) */
    uint16_t length;            /* the entry's word as it is: bits 0-13 the length, bits 14-15 kept */
} AnimSetGraphics;
AnimSetGraphics anim_set_graphics(uint8_t set);

/* ---- level maps (map/level_map.c's LevelMapBuild, ScreenMapLayout) ----

   A level's map is screens of 32 x 16 tiles, each 8 x 8 blocks of 2 x 2
   tiles; its header gives the screens and the banks their data is in; its
   layouts place the screens in the screen map. */
#define LEVEL_MAPS 66           /* $80:E3DC's words up to the layouts' table at $80:E460 */

unsigned level_map_screens(uint16_t level);             /* screens it builds (the original's count byte: 0 builds 256) */
uint8_t level_map_layers(uint16_t level);               /* the layers on the main and sub screens it sets */
uint8_t level_map_screen(uint16_t level, unsigned i);   /* the i'th screen built: its number in the block lists */
/* screen `screen`'s block list: 64 block words, then 4 more bytes (the
   original copies 132 bytes); `i` a byte offset < 132 */
uint8_t level_screen_list_byte(uint16_t level, uint8_t screen, unsigned i);
/* a block's 2 x 2 tiles: `block` the word from the list, `k` 0 top left,
   1 top right, 2 bottom left, 3 bottom right; a map word */
uint16_t level_block_tile(uint16_t level, uint16_t block, unsigned k);
DataRef level_metatiles(uint16_t level);                /* the start of its metatiles' bank, for the map's readers */

/* a layout: the screens of a `columns` x `rows` area of the screen map,
   row by row. Layout 0 is every level's; layout 1 only where
   level_map_layout_count says 2. */
unsigned level_map_layout_count(uint16_t level);
unsigned level_map_layout_columns(uint16_t level, unsigned layout);
unsigned level_map_layout_rows(uint16_t level, unsigned layout);
uint8_t level_map_layout_screen(uint16_t level, unsigned layout, unsigned i);   /* i < columns * rows */

#endif
