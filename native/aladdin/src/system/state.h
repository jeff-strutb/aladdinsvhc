/*
 * state.h: the game's shared state, g (plan section 3, "Work RAM").
 *
 * Each field is one the original kept in work RAM and more than one area
 * reads, typed by what it holds. Its comment gives the original's address
 * (and, where the field holds a part of a byte or a value the original kept
 * scaled, how), so the state comparison (tools/state_fields.csv) can line it
 * up with a recording of the original. Fields are grouped by the area that
 * mostly writes them (counted in src/aladdin).
 *
 * Not here: work RAM whose purpose the record does not yet establish (the
 * V_xxxx names), sizes the record does not establish, and what only one
 * file uses. The session that converts the code reading such bytes names
 * them by what they do and adds them here, under its area, with a small
 * exact edit; a variable only one file uses stays static in that file. The
 * direct page's scratch ($000C up) and the task stacks are locals and
 * arguments, never fields.
 */
#ifndef ALADDIN_STATE_H
#define ALADDIN_STATE_H

#include <stdint.h>

#include "nshvc/nshvc.h"
#include "data/data.h"

/* ---- records ---- */

/* A task (docs/aladdin/TASKS.md): one of 8 slots of $18 bytes from $006F.
   The original's saved S and stack top (+$02-$05) are gone: a task's place
   is its entry and its resume point (decision 1: every wait a state). */
typedef enum {
    TASK_FREE = 0x00, TASK_WAITING = 0x01, TASK_READY = 0x04, TASK_RUNNING = 0x08, TASK_NEW = 0x0C
} TaskState;

typedef struct Task {
    uint8_t state;              /* +$00 ($006F + $18 n): TaskState */
    uint8_t wait;               /* +$01: frames left to wait; the NMI's countdown makes it ready at 0 */
    uint8_t param;              /* +$06: the byte its creator gave it */
    uint16_t entry;             /* the task's entry, a TaskEntry of system/ (stage 4); the original's entry address */
    uint16_t resume;            /* where its chain stopped, in that entry's own numbering; 0 at its start */
    uint16_t calls[5];          /* where each waiting routine its entry called stopped, one to five calls down
                                   (system/task.h, task_enter); 0 not entered */
    union {
        uint8_t bytes[17];      /* +$07-$17: the task's own data; each entry's named view is stage 4's */
        struct {                /* slot 0 in the opening story (game/opening_story.c) */
            uint16_t frames_left;   /* $0076: frames the scene still runs */
            uint8_t unused_0078;
            uint16_t part3_done;    /* $0079: counted up by the story's third part at its end */
            uint16_t part2_done;    /* $007B: counted up by its second part */
        } story;
        struct {                /* slot 0 on the title (game/title_task.c) */
            uint8_t fade;           /* $0076: the colour fade's step, counted up to 0 */
        } title;
        struct {                /* the colour fades and the palette sequence (video/tasks.c): the register the
                                   original kept on the task's stack across its waits, so no address (native only) */
            int8_t step;            /* the fade's grey level, or the sequence's step */
        } video;
        struct {                /* the story's text task (game/story_text.c); its frames between characters
                                   are the slot's parameter, as the original's */
            uint8_t palette;        /* +$07: the palette bits of the tile's high byte */
            uint8_t column;         /* +$08 */
            uint8_t row;            /* +$09: each two of the text layer's rows */
            uint16_t origin;        /* +$0A: the text layer's byte offset of the first row and column */
            uint16_t at;            /* the next byte of the text (the original kept it on the task's stack) */
        } story_text;
        struct {                /* ShakeTask (video/shake.c): its record at $75-$78 of its slot, and the count the
                                   original kept in Y across its waits (native only) */
            uint8_t layer;          /* $0075 (slot 0's; +$06 of the slot): the layer, an offset from $1A1C */
            uint8_t sways;          /* +$07 */
            uint8_t frames;         /* +$08: frames a half-sway */
            uint8_t step;           /* +$09: pixels a frame */
            uint8_t left;           /* frames left of the half-sway (Y) */
        } shake;
        struct {                /* Bg1WobbleTask (video/shake.c) */
            int16_t step_x;         /* +$07 of the slot (word) */
            int16_t step_y;         /* +$09 (word) */
            uint8_t swing;          /* +$0B: frames a swing */
            uint8_t swing_left;     /* +$0C */
            uint8_t left;           /* +$0D: frames in all left */
        } wobble;
        struct {                /* WindowEffectTask's waves (video/window_effects.c) */
            uint16_t phase;         /* +$06 of the slot (word): the sine's start */
            uint8_t step;           /* +$08: lines a step of the sine lasts */
            uint8_t count;          /* +$09 */
        } waves;
        struct {                /* ColourBandTask (video/colour_bands.c) */
            uint8_t step;           /* +$06 of the slot: how far the lower bands are dimmed */
            uint8_t buffer;         /* +$07: the buffer the bands are written to next (0 or 1) */
        } bands;
    } own;
} Task;

/* An actor's record (docs/aladdin/ACTORS.md, src/aladdin/include/actor.h,
   actor_anim.h): the player's at $08DB, then 35 of $50 bytes from $092B.
   Positions and speeds are 24 bits, a fraction byte and then a 16-bit
   whole part, kept here as one value 16.8 that wraps at 24 bits as the
   original's. The original's resume address (+$02-$05: P, the PC, the
   bank) is a resume point of the kind's code. */
typedef struct Actor {
    uint8_t state;              /* +$00: 0 free, 1 waiting, 8 running, $0C and up to start */
    uint8_t wait;               /* +$01: frames left to wait */
    uint16_t resume;            /* +$02-$05, as a resume point of its kind's code */
    uint8_t kind;               /* +$06: picks its hit box (the table at $C29A), its animation set, its KIND_COUNT entry */
    uint8_t spawn_byte;         /* +$07: the byte its spawner gave it */
    uint8_t anim_flags;         /* +$08: bit 3 animated; bits 0-1 the frame's parity, for some kinds */
    uint8_t flags;              /* +$09: bit 7 a routine of its kind runs each frame it waits; bit 6 it can be hit; bits 4-5 its sprite priority */
    uint32_t x;                 /* +$0B-$0D: 16.8, pixels */
    uint32_t y;                 /* +$0E-$10: 16.8, pixels */
    uint8_t view_box;           /* +$12: its box for ActorsInView, 8 bytes a box (0, 8, 16, 24, 32) */
    uint8_t facing;             /* +$14: 0 right (x grows), else left */
    uint8_t flip;               /* +$15: the facing's opposite: bit 0 set when facing right */
    DataRef probes;             /* +$16-$17: its probe list (map/'s collision tests read it: map/map.h) */
    uint8_t probe_at;           /* +$18: the probe the next test uses, a byte offset in the list (4 a probe) */
    int32_t speed_x;          /* +$19-$1B: 16.8, along the facing */
    uint8_t accel_x;            /* +$1C: added to or taken from speed_x (fraction) */
    int32_t speed_y;            /* +$1D-$1F: 16.8, down positive */
    uint8_t accel_y;            /* +$20: gravity (fraction) */
    uint8_t spawn_at;           /* +$21: where it spawns a child, an index into the spawn offsets */
    uint16_t picture_gfx;       /* +$22: the frame's picture's place in g.actor_pictures (the original kept $2000 more: its work-RAM address) */
    uint16_t anim_table;        /* +$24: its set's animation table, in the animation data */
    uint8_t frame_time;         /* +$26: frames the frame has left */
    uint8_t picture;            /* +$27: the frame's picture; at a start, the animation's number */
    uint8_t redraw;             /* +$28: $FF when its picture changed */
    uint16_t look;              /* +$2B-$2C: sprite tile base and palette (actor_anim.h's ACTOR_2C is its high byte) */
    uint8_t anim;               /* +$2D: the animation's offset in its set's table (its number times 2) */
    uint8_t anim_high;          /* +$2E: AnimStart adds it as the high byte of anim's offset; kind $41 keeps its own byte here */
    uint8_t frame;              /* +$2F: the frame in the animation */
    uint8_t loop_back;          /* +$30: frames the animation steps back at its end */
    uint16_t gfx_base;          /* +$31: its set's graphics, in the picture list */
    uint8_t pool_number;        /* +$33: in the 28-record pool, its number (28 down to 1) */
    uint8_t placement;          /* +$34: the placement it came from (g.placed); $FF a child, which has none */
    struct Actor *next;         /* +$37-$38: the next record in its list; NULL ends it (0 in the original) */
    uint8_t tile_slot;          /* +$39: the animated-tile slot it holds (1-4), 0 none */
    DataRef stream;             /* +$4A: where its byte stream goes on */
    uint16_t call_at;           /* native: where a shared routine that waits inside it goes on (actor.h), 0 none;
                                   the original kept its caller's return at +$4E and its own place in +$02-$05 */
    DataRef own_pictures;       /* +$29-$2A, for the kinds that send their own pictures: the list ActorPictureSend sends them
                                   from (the original's word in the data bank); those kinds' +$29 is this, the others' own.bytes */
    union {
        /* the bytes each kind keeps for itself; each kind's named view is
           in actor/ (actor_own.h; tools/state_fields.csv lists them) */
        uint8_t bytes[32];      /* +$0A, $11, $13, $16-$18, $29-$2A, $35-$36, $3A-$49, $4C-$4F, $4A-$4B: in record order, then the stream's two bytes, which a kind that keeps no stream uses for its own */
    } own;
} Actor;

#define ACTOR_RECORDS 36        /* the player's and 35 more: $08DB to $141A */
#define ACTOR_POOL_SIZE 28      /* the records from $0B5B */
#define ACTOR_POOL_B_SIZE 5     /* the records from $09CB */

/* A record of the VRAM queue ($085A, 8 bytes each): one copy to video
   memory the NMI made. 16 fit before the player's record ($08DB). */
typedef struct VramQueueEntry {
    uint16_t to;                /* +$00: the VRAM word address, which video/ turns into its object */
    const uint8_t *from;        /* +$02-$04: the source, the game's own array or cartridge data */
    uint16_t bytes;             /* +$05: how many */
    uint8_t step;               /* +$07: how the address steps: bit 7 after the high byte, bits 0-1 by how much */
} VramQueueEntry;

#define VRAM_QUEUE_SIZE 16

/* A record of the tile-block queue ($1B44, 10 bytes each): four map words
   at a VRAM address. 16 fit before its length at $1BE4. */
typedef struct TileBlock {
    uint16_t to;                /* +$00: the VRAM word address */
    uint16_t words[4];          /* +$02: the four map words */
} TileBlock;

#define TILE_BLOCK_QUEUE_SIZE 16

/* A queued change of a map tile ($071D, 8 bytes each), made by
   TileChangesApply: 32 fit before $0821; the original checks no bound. */
typedef struct TileChange {
    uint16_t x, y;              /* +$00, +$02: the point, pixels (bit 3 set: the tile's middle) */
    uint16_t tile;              /* +$04: the new map tile (an offset in the level's metatiles) */
} TileChange;

#define TILE_CHANGES_SIZE 32

#define LEVEL_TILES_WORDS 0x8000    /* the original's bank $7F, a word a tile, as far as a screen's offset reaches */
#define SCREEN_MAP_PLACES 1024      /* 32 x 32 */
#define BLOCK_SOURCES_SIZE 40       /* the most screens a level builds (40, from the map headers) */
/* One of a scroll layer's four edges (+$00 left, +$0A right, +$14 top,
   +$1E bottom of its record, 10 bytes each): a place in its video map and
   the same place in the level's map. Its +$06 is read and written by
   nothing. */
typedef struct ScrollEdge {
    uint16_t tilemap;           /* +$00: its map word address: column bits 0-4, row 5-9, the second screen bit 10, the layer's base */
    uint16_t map;               /* +$02: its screen's place in the screen map (g.block_map), a byte offset: 2 across, $40 down */
    uint16_t block;             /* +$04: its metatile in the screen, a byte offset: 2 across, $20 down */
    uint16_t half;              /* +$08: its tile in the metatile: 2 the right half, 4 the bottom */
} ScrollEdge;

/* A scroll layer's record ($156 bytes: the play field's at $1738, the
   backdrop's at $188E). Offsets in the record. +$3D, +$41 and +$47-$49
   are read by nothing. */
typedef struct ScrollLayer {
    ScrollEdge edge[4];         /* +$00: left, right, top, bottom */
    uint16_t column_length;     /* +$28: tiles a column draws */
    uint16_t row_length;        /* +$2A: tiles a row draws */
    uint16_t tilemap_base;      /* +$2E: bits every map word of the layer carries */
    uint16_t tilemap_mask;      /* +$30: the map word bits a position gives */
    uint16_t screen_flip;       /* +$32: map bits a row step past the top or bottom flips */
    uint16_t drawn_x;           /* +$34: the x its drawn tiles reach to */
    uint16_t drawn_y;           /* +$36: the y its drawn tiles reach to */
    uint8_t column_side;        /* +$38: the side of the column being drawn: 0 right, 1 left */
    uint8_t row_side;           /* +$39: the side of the row being drawn: 0 down, 1 up */
    uint32_t scroll_x;          /* +$3A-$3C: 16.8, pixels */
    uint32_t scroll_y;          /* +$3E-$40: 16.8, pixels */
    uint16_t map_base;          /* +$42: the layer's block map in the level map, a word offset */
    uint16_t block;             /* +$44-$45: a screen's metatile list, its offset in the level tiles (+$46, its bank: always the original's $7F) */
    uint16_t metatile;          /* +$4A-$4B: a metatile's four tiles, its address in the level's metatiles' bank */
    DataRef metatiles;          /* +$4C: the level's metatiles' bank (LayerSetup: the bank LevelMapBuild left at $005C) */
    uint8_t edges;              /* +$4F: the edges it draws: 1 right, 2 left, 4 bottom, 8 top */
    uint8_t column_parts;       /* +$50: a drawn column to copy: 1, or 2 past 32 tiles */
    uint16_t column_to;         /* +$51: its map word address */
    uint16_t column[32];        /* +$53: its tiles, a ring from its row */
    uint8_t row_parts;          /* +$D3: a drawn row to copy: 2, the two screens */
    uint16_t row_to;            /* +$D4: its map word address */
    uint16_t row[64];           /* +$D6: its tiles, a ring from its column */
} ScrollLayer;

/* A palette cycle's record ($15F1 + 14 n; its running byte, +$00, is
   g.palette_cycle_running[n]): a script of colour frames played into the
   palette, round and round. */
typedef struct PaletteCycle {
    uint8_t cycle;              /* +$01: its number, in the cycle table */
    uint8_t frames;             /* +$02: the frames in its script */
    uint8_t colours;            /* +$03: the colours in a frame */
    uint8_t wait;               /* +$05: frames before the next frame */
    uint16_t first;             /* +$06: the script's first frame, an offset in the frames' data */
    uint16_t next;              /* +$08: the next frame, the same */
    uint16_t at;                /* +$0A: the first colour it sets, a byte offset in the palette */
    uint8_t step;               /* +$0C: 0 find its script, 1 restart, 2 a frame, 3 wait */
    uint8_t left;               /* +$0D: frames left before a restart */
} PaletteCycle;

/* A column or row queued to draw ($1A48, $1A68: rings of 8 of 4 bytes). */
typedef struct LayerEdge {
    uint8_t layer;              /* +$00: the layer's record, as 0 (play field) or 1 (backdrop) */
    uint8_t side;               /* +$02: 0 right or down, 1 left or up */
} LayerEdge;

/* The maps and tile sets a layer can show; video/ makes them (video.h
   gives the video addresses of each). */
typedef enum {
    VIDEO_MAP_BACK,             /* the original's $4000, 64 x 32: BG2's */
    VIDEO_MAP_FRONT,            /* $4800, 64 x 32: BG1's */
    VIDEO_MAP_TEXT,             /* $5C00, 32 x 32: BG3's, BG1's on the title */
    VIDEO_MAP_KINDS
} VideoMap;

typedef enum {
    VIDEO_TILES_LOW,            /* 4bpp from video address 0 (a base nibble of 0) */
    VIDEO_TILES_HIGH,           /* 4bpp from $5000 (nibble 5); the sprites' are its tiles 256 on */
    VIDEO_TILES_TEXT,           /* 2bpp from $5000: BG3's (nibble 5) */
    VIDEO_TILE_KINDS
} VideoTiles;

/* A record of the single video words list ($0619, 4 bytes each). */
typedef struct VramWord {
    uint16_t to;                /* +$00: the VRAM word address */
    uint16_t word;              /* +$02 */
} VramWord;

#define VRAM_WORDS_SIZE 64      /* the upload sends count * 4 bytes as one byte's length (80:884C), so 63 at most */
/* A value sequence ($1645 + 16 n, ten; level/sequences.c): a list of
   words, each held for some frames. Its new-word byte (+$04) and its word
   (+$08), the VRAM list it names, are video's g.vram_list_pending[n] and
   g.vram_list[n], which the NMI's upload reads and clears. */
#define SEQUENCES 10
typedef struct Sequence {
    uint8_t running;            /* +$00: nonzero while it runs */
    uint8_t number;             /* +$01: its list in the sequences' table (an offset, 3 bytes an entry) */
    uint8_t length;             /* +$02: its list's length in bytes */
    uint8_t index;              /* +$03: its entry's offset in the list */
    uint8_t wait;               /* +$05: frames left of the word */
    DataRef list;               /* +$06: its list */
    uint8_t started;            /* +$0C: nonzero once started */
} Sequence;

/* A per-line effect (the original's HDMA channel 1-7): what it changes and
   where its table is. The table is the game's, in g.line_data (the original's
   $7E:F400-$7E:F9FF), as the original builds it: runs of a line count (bit 7:
   a value each line; 0 ends) and either the value (direct) or a pointer to
   it in $7E (indirect); video_draw reads it each picture, as the original's
   NMI restarted every channel at its table each frame. */
typedef enum {
    LINE_TM, LINE_TS, LINE_PALETTE, LINE_BG1_Y, LINE_BG2_X, LINE_BG3_Y, LINE_WINDOW1, LINE_WINDOW2,
    LINE_MODE, LINE_M7_A, LINE_FIXED, LINE_WINDOW_USE_BG34, LINE_TARGETS
} LineTarget;

typedef struct LineChannel {
    uint8_t target;             /* LineTarget */
    uint8_t direct;             /* the values are in the table itself (else it points at them) */
    uint16_t table_at;          /* the table's offset in g.line_data; channels 4-7 take g.line_table_at's each frame */
} LineChannel;

#define LINE_DATA_SIZE 0x600

/* ---- the state ---- */

typedef struct GameState {
    /* -- system -- */
    uint8_t frame_count;        /* $032E: counts NMIs (8-bit) */
    uint16_t random;            /* $032B: stepped by RandomNext */
    Task tasks[8];              /* $006F */
    uint8_t task_current;       /* $0327: the running task's slot (the original: its byte offset, $18 a slot) */
    uint8_t task_slots;         /* $0328: how many slots the scheduler scans */
    uint8_t kept_options[3];    /* $0003: the options screen's settings, kept across a reset */
    uint8_t kept_options_sum;   /* $0006: their 8-bit sum */
    uint16_t pad_held[2];       /* $0377, every 2 bytes: each pad's buttons held, as the SNES's word (B bit 15 ... R bit 4) */
    uint16_t pad_pressed[2];    /* $037B, every 2 bytes: each pad's buttons newly pressed this frame */
    uint16_t pad1_held;         /* $037F: pad 1's held buttons as the actions were read from them */
    uint16_t pad1_pressed;      /* $0381: pad 1's newly pressed buttons, the same */
    uint16_t control_masks[4];  /* $0348: per action, the buttons the options give it */
    uint8_t action_held[4];     /* $0383: per action, 1 while one of its buttons is held */
    uint8_t action_pressed[4];  /* $0387: per action, 1 when one of its buttons was just pressed */
    uint8_t play_mode;          /* $0331: 0 the pads; 2 a demo, the pad read from its script (1, the recorder, never on retail) */
    uint16_t demo_input_at;     /* $033B: the demo's next pad entry, a byte offset into its script */
    uint8_t demo_buttons;       /* $033D: the demo's pad 1 high byte */
    uint8_t demo_input_frames;  /* $033E: frames left on that entry */
    uint8_t demo_number;        /* $033F: the demo being played; also the stage per-stage tables are indexed by (wram.h's V_033F) */

    /* -- audio: the S-CPU's sound queue -- */
    uint8_t sound_queue_read;   /* $03BB: the next command to send (0-31) */
    uint8_t sound_queue_write;  /* $03BD: where the next command goes (0-31) */
    uint8_t sound_queue[32];    /* $03BF: the ring of command bytes */
    uint8_t sound_special;      /* $03F8: the last special command queued ($F0, $F2, $F5, $F6) */

    /* -- video: the picture draw reads (the NMI's shadows, by meaning) -- */
    uint8_t brightness;         /* $03B8 bits 0-3 */
    uint8_t blank;              /* $03B8 bit 7 */
    uint8_t nmi_on;             /* $03B7 bit 7: the NMI's work runs */
    uint8_t split_on;           /* $03B7 bits 4-5 nonzero: the screen turns on at the IRQ's line */
    uint8_t line_effects_on;    /* $03B6: a bit per effect, by the original's HDMA channel (1-7) */
    uint8_t main_layers;        /* $0399: bits 0-3 BG1-BG4, bit 4 sprites, on the main screen */
    uint8_t sub_layers;         /* $039A: on the sub screen */
    uint8_t window_main;        /* $038B: the layers the windows mask on the main screen */
    uint8_t window_sub;         /* $038C: on the sub screen */
    uint8_t mode;               /* $039D bits 0-2: the picture mode */
    uint8_t bg3_priority;       /* $039D bit 3 */
    uint8_t tile_size_16;       /* $039D bits 4-7: BG1-BG4 with 16 x 16 tiles */
    uint8_t window_edges[4];    /* $03A6-$03A9: window 1 left, right, window 2 left, right */
    uint8_t window_use[6];      /* $03AA, a nibble each: per BG1-4, sprites, colour: bit 0 window 1 inverted, 1 on, 2 window 2 inverted, 3 on */
    uint8_t window_logic[6];    /* $03AD, 2 bits each: per BG1-4, sprites, colour: 0 or, 1 and, 2 xor, 3 xnor */
    uint8_t blend_prevent;      /* $03AF bits 4-5 */
    uint8_t blend_clip;         /* $03AF bits 6-7 */
    uint8_t blend_use_sub;      /* $03AF bit 1: the sub screen, else the fixed colour */
    uint8_t blend_direct_colour;/* $03AF bit 0 */
    uint8_t blend_layers;       /* $03B0 bits 0-5: BG1-4, sprites, backdrop */
    uint8_t blend_half;         /* $03B0 bit 6 */
    uint8_t blend_subtract;     /* $03B0 bit 7 */
    uint8_t fixed_red;          /* $03B2 bits 0-4 */
    uint8_t fixed_green;        /* $03B3 bits 0-4 */
    uint8_t fixed_blue;         /* $03B4 bits 0-4 */
    uint8_t mosaic_size;        /* $03B1 bits 4-7 */
    uint8_t mosaic_layers;      /* $03B1 bits 0-3 */
    LineChannel line_channel[8]; /* by the original's HDMA channel; g.line_effects_on's bit n turns channel n on */
    uint8_t line_data[LINE_DATA_SIZE]; /* $7E:F400-$7E:F9FF: the per-line effects' tables and data (the window edges at
                                   $F800 among them; g.band_pos is the parallax's view of $F800 on) */
    uint8_t line_channels_kept; /* $03B5 high nibble: channels 4-7 other code keeps, not for the window effects */
    uint32_t bg_x[3];           /* $19EC, every 8 bytes: BG1-3's x, 24.8: the pixels the middle two bytes */
    uint32_t bg_y[3];           /* $19F0, every 8 bytes: BG1-3's y, 24.8, as bg_x */
    uint16_t bg_map_at[3];      /* $03A0-$03A2 bits 2-7: BG1-3's map, as its VRAM word address */
    uint8_t bg_map_size[3];     /* $03A0-$03A2 bits 0-1: 0 32 x 32, 1 64 wide, 2 64 high, 3 both */
    uint16_t bg_tiles_at[4];    /* $03A4-$03A5, a nibble each: BG1-4's tiles, as their VRAM word address */
    int16_t m7_matrix[4];       /* $038D, $038F, $0391, $0393: Mode 7's a, b, c, d, 8.8 */
    uint16_t m7_centre_x;       /* $0395 */
    uint16_t m7_centre_y;       /* $0397 */
    uint16_t scroll_x_min;      /* $1A40: the play field's scroll x at its least */
    uint16_t scroll_x_max;      /* $1A42: and at its most */
    uint8_t scroll_kind;        /* $083D: the level's scroll handler (ScrollUpdate), times 2 */
    uint8_t camera_window_step; /* $1A09: added to camera_window for the window the camera uses */
    uint16_t camera_aim_x;      /* $03F1: added to the scroll x for the point the camera keeps the player from */
    uint16_t camera_push;       /* $1A0A: how the camera moved across this frame: 0 not, 1 right, 2 left */
    uint16_t scroll_x_held;     /* $1A10: nonzero, no following and no stop at empty blocks across */
    uint16_t scroll_y_held;     /* $1A12: the same up and down */
    int8_t camera_window;       /* $1A08: with $1A09, the camera's window in the scroll's table; negative, the
                                   camera does not follow the player */
    uint8_t screen_signal;      /* $146B: a byte the screens' code signals with; on the title, bit 7 set when the
                                   Mode 7 zoom (Mode7ZoomInTask) has ended; in play, the escort's and guards' marks */
    uint8_t split_line;         /* $1C7D: the IRQ's line; 0: line 8 */
    uint8_t mode7_how;          /* $0829: 0 the matrix not sent, 1 each element scaled first, 2 as it is */
    uint8_t mode7_scale[4];     /* $0825-$0828: a's, b's, c's and d's scale, 256ths (negated as the original's multiplier took it) */
    uint8_t frame_ready;        /* $0857: nonzero, the frame's sprites, VRAM queue and layer edges go to the picture */
    uint8_t uploads_held;       /* $0403: nonzero, the value sequences' lists wait (the NMI clears it) */
    uint8_t vram_words_pending; /* $0616: nonzero, the single words go */
    uint8_t vram_words_count;   /* $0617 */
    VramWord vram_words[VRAM_WORDS_SIZE]; /* $0619 */
    uint8_t unpacked[57344];    /* $7F:0000-$7F:DFFF: graphics unpacked from the cartridge, at the original's places */
    uint8_t sprites_take_bg2_shift; /* $1A16: 8, the sprites move with BG2's shifts ($1A20, $1A24); 0 BG1's */
    uint16_t unpack_next;       /* $006B: where in unpacked the next kept part or actor set's graphics go */
    uint16_t bg1_x_shift;       /* $1A18: added to BG1's x as it goes to the picture */
    uint16_t layer_offset_y;    /* $1A1C: added to BG1's y */
    uint16_t bg2_x_shift;       /* $1A20 */
    uint16_t bg2_y_shift;       /* $1A24 */
    uint8_t bg2_x_held;         /* $083C: nonzero, BG2's x is not sent */
    uint8_t bg1_held;           /* $1A14: nonzero, BG1's position is not taken from the play field */
    uint8_t bg2_follows;        /* $1A15: 0 the backdrop layer; $40 its y only; $80 the play field's x; else neither */
    uint16_t backdrop_offset_y; /* $1A2B: the backdrop's y less the play field's, 8.8 */
    uint16_t palette[256];      /* $7E:FD00: the colours, 15-bit */
    uint8_t palette_upload;     /* $0844: nonzero, the palette goes to the picture */
    uint16_t palette_saved[256];/* $0406: colours kept to fade from */
    uint16_t palette_saved_at;  /* $0606: the first colour kept, a byte offset */
    uint16_t text_map[32 * 32]; /* $7E:D800: the text layer's map */
    uint8_t text_map_upload;    /* $0835: nonzero, the text map goes to its object */
    uint16_t text_upload_to;    /* $03DF: the VRAM word address it goes to */
    uint16_t text_upload_bytes; /* $03E1 */
    uint8_t text_parts_pending[3]; /* $0836: nonzero, that part of the text map goes */
    nshvc_sprite sprites[128];  /* $7E:FA80 and $7E:FC80, the sprite table, as the engine's sprites */
    uint16_t sprite_count;      /* replaces the sprite table's free count and cursors ($084D, $0852, $0854) */
    uint8_t vram_upload_number; /* $0834: nonzero, the number of a fixed upload to make */
    uint8_t vram_queue_pending; /* $0858: nonzero, the queue is ready */
    uint8_t vram_queue_count;   /* $0859 */
    VramQueueEntry vram_queue[VRAM_QUEUE_SIZE]; /* $085A */
    uint8_t tile_blocks_pending;/* $1BE6 */
    uint16_t tile_blocks_count; /* $1BE4: the original keeps its length in bytes, 10 a record */
    TileBlock tile_blocks[TILE_BLOCK_QUEUE_SIZE]; /* $1B44 */
    uint8_t vram_list_pending[10]; /* $1649 + $10 n: the value sequence's new-word byte ($04 of its record at $1645 + $10 n): nonzero, upload its list */
    DataRef vram_list[10];      /* $164D + $10 n: its word ($08), the list of uploads (bank $80) */
    ScrollLayer layers[2];      /* $1738 the play field, $188E the backdrop */
    LayerEdge column_queue[8];  /* $1A48 */
    LayerEdge row_queue[8];     /* $1A68 */
    uint16_t column_queue_head; /* $1A88 */
    uint16_t column_queue_tail; /* $1A8A */
    uint16_t row_queue_head;    /* $1A8C */
    uint16_t row_queue_tail;    /* $1A8E */
    uint8_t fill_layers_left;   /* $1A90: while a level's maps fill */
    uint8_t fill_columns_left;  /* $1A91 */
    uint16_t scroll_x_before;   /* $19E5: the play field's scroll before this frame's move */
    uint16_t scroll_y_before;   /* $19E9 */
    uint16_t camera_stopped_x;  /* $1A0C: nonzero, an empty block stopped the scroll across */
    uint16_t camera_stopped_y;  /* $1A0E */
    uint32_t band_pos[11];      /* $7E:F800: the drifting backdrop bands' positions, 4 bytes each */
    uint32_t band_drift;        /* $1A2F-$1A31: 24 bits from its fraction byte */
    uint8_t palette_cycle_running[6]; /* $15F1 + 14 n: nonzero while palette cycle record n runs (its other
                                         bytes are video/'s to add) */
    PaletteCycle palette_cycle[6]; /* $15F1 + 14 n: the rest of palette cycle record n (video/palette_cycles.c) */

    /* -- video: the parallax (video/parallax.c) -- */
    uint16_t backdrop_stage;    /* $0841: PlayField3Stages's stage, times 2 */
    uint16_t backdrop_offset_y_high; /* $1A2D-$1A2E: backdrop_offset_y's high word (BackdropFollowOffsetY adds 4 bytes) */
    uint16_t band_table_buffer; /* $03FC: which of the two band tables BandTable wrote last (0 or 2) */
    uint16_t line_table_at[4];  /* $0350: channels 4-7's tables, as their offset in g.line_data (the original's
                                   $F400 + it), which the NMI restarts each channel at */

    /* -- level and map -- */
    uint16_t level;             /* $035E: the level being played */
    uint16_t scroll_limit_x;    /* $1A04 */
    uint16_t scroll_limit_y;    /* $1A06 */
    uint8_t level_spot_count;   /* $0821: the level's trigger spots */
    DataRef level_spots;        /* $0822: their list */
    uint8_t tile_behaviour[4096]; /* $7E:C800: per map tile, how it behaves */
    uint16_t map_level;         /* the level LevelMapBuild last built: the banks it noted for its block lists,
                                   block tiles and metatiles ($0056, $0059, $005C, and the pointer $005A-$005C
                                   at $8000 of the metatiles) are that level's (g.level changes after) */
    uint16_t level_tiles[LEVEL_TILES_WORDS]; /* $7F:0000: the level's map, a map word a 16 x 16 tile, each screen
                                   16 x 16 tiles ($200 bytes) one after another as LevelMapBuild built them.
                                   Read at the byte offsets the screen map gives (up to $FFFF); past the screens
                                   built the original reads whatever else its bank $7F holds there (the graphics
                                   buffer from $5800, the screen map from $F000), which this array does not */
    union {                     /* $7F:F000-$F7FF: two uses of the same bytes in the original */
        uint16_t places[SCREEN_MAP_PLACES]; /* the screen map: 32 x 32 places, row by row; a place's high byte the
                                   byte offset of its screen's tiles in level_tiles over 256, its low byte 0 */
        uint8_t anim_set_of_kind[256];      /* $7F:F000 + kind: each kind's animation set (AnimSetsLoad, $FF none);
                                   LevelMapBuild clears these bytes and lays the screen map over them */
    } screen_map;
    uint16_t block_sources[BLOCK_SOURCES_SIZE]; /* $1BF5: per screen built, its block list (screen number * $80) */
    uint16_t block_sources_end; /* $0016 while a level's map is built: the entries listed (the original: their
                                   byte offset); LevelMapBuild empties the list */
    uint16_t tile_changes_count;    /* $0719: tile changes queued */
    uint16_t tile_changes_end;  /* $071B: the next record to fill (the original: its byte offset, 8 a record) */
    TileChange tile_changes[TILE_CHANGES_SIZE]; /* $071D */
    Sequence sequences[SEQUENCES]; /* $1645 + 16 n: the value sequences (their +$04 and +$08 are video's
                                      vram_list_pending[n] and vram_list[n]) */

    /* -- player -- */
    uint16_t player_top_speed;  /* $141D: 8.8 */
    int16_t player_run_accel;   /* $1440 */
    int16_t player_run_decel;   /* $1442: negative */
    int16_t player_turn_decel;  /* $1444: negative */
    uint8_t player_move;        /* $1420: standing, running, jumping ...: picks its animation */
    uint8_t player_dash;        /* $1459: 1 while the dash is held */
    uint8_t player_slowed;      /* $1457: frames the speed was held up to its least */
    uint16_t player_saved_x;    /* $145D: a position kept for the player, pixels */
    uint16_t player_saved_y;    /* $145F */
    uint16_t player_ground_y;   /* $1455: where it last stood, pixels */
    uint16_t player_no_actor_landing; /* $1427: nonzero, Aladdin lands on no actor (ScrollUpdate's kind 2 reads its low 4 bits) */
    uint8_t player_box_wanted;  /* $1448 */
    uint8_t player_box;         /* $1449 */
    uint16_t test_box_x;        /* $1484: the box actors are tested against (the player's, PlayerBoxSet): its centre */
    uint16_t test_box_y;        /* $1486 */
    uint16_t test_box_half_w;   /* $1488 */
    uint16_t test_box_half_h;   /* $148A */
    uint8_t player_hurt_count;  /* $141B: counted up on entering state $8C; nonzero, he cannot land or be hit */
    uint8_t player_in_air;      /* $141C: set by his jumps, cleared as he lands; zero, he cannot land on an actor */
    uint8_t player_no_landing;  /* $145A: nonzero, frames with no hold on a ledge; bit 7, no landing on an actor */
    uint8_t player_landed;      /* $144E: 1 once he has landed on an actor */
    uint8_t player_hurt_time;   /* $03F5: frames before it can be hit again */
    uint8_t player_hurts;       /* $141B: counted up as he enters state $8C (hurt); nonzero, no level exit */
    uint8_t player_held;        /* $141C: nonzero while an actor holds him; no level exit then */
    uint8_t view_edge_rule;     /* $145B: what the view's sides do to him (level/view_edges.c: 0-4) */
    uint8_t health;             /* $0367 */

    /* -- game: progress -- */
    uint8_t health_marks;       /* $0365: the most health */
    uint8_t continues;          /* $035B: continues left */
    uint8_t cheat_start;        /* $0009: nonzero, play starts with 10 health marks (the title acts on it too) */
    uint8_t pause_block;        /* $0404: nonzero, no pause, and the actor loop runs only actors[1..7] past the player */
    uint8_t paused;             /* $0405: counted up while the game is paused */
    uint8_t play_starts;        /* $03FA: counted up each time a level's play is set up */
    uint8_t layer_setup_mode;   /* $083F: the level's layer set-up mode */
    uint32_t frame_start_x;     /* $143A-$143C: the player's x as the frame began, 16.8 */
    uint32_t frame_start_y;     /* $143D-$143F: its y */
    uint8_t play_frames;        /* $147C: frames of the play time's second, 0-59 */
    uint8_t play_seconds;       /* $147D: counted up each second, unbounded */
    uint8_t play_digits[6];     /* $147E: the play time's digits, seconds' units up to the hours' tens */
    uint8_t game_begun;         /* $036F: nonzero once a game has begun */
    uint8_t level_restart;      /* $0360: the restart point the level starts at (0 or 1) */
    uint16_t play_countdown;    /* $0343: a countdown the title and the play loop step down each frame */
    uint8_t demo_was_last;      /* $0345: nonzero, the title's last idle show was the demo, so the next is the story */
    uint8_t player_flags;       /* $0347: bit 0 no hurt from the ground; bit 1 set in state $8C; bit 7 a ride */
    uint8_t level_marks_taken;  /* $0358: a bit per level whose health mark was taken */
    uint8_t level_flags;        /* $0359: eight flags the level's actors set */
    uint8_t opened_flags;       /* $035A: flags of things already opened */
    uint8_t progress_sequence;  /* $035C: the progress sequence (an offset in the progress table) */
    uint8_t progress_stage;     /* $035D: the stage in it */
    uint8_t lives_start;        /* $0362: the lives a level's play starts with */
    uint8_t lives;              /* $0364: lives left (the status line shows it, less 1) */
    uint8_t health_shown;       /* $0366: the health the status line's marks show (video/hud.c) */
    uint8_t lives_shown;        /* $0363: the lives as the status line shows them */
    uint8_t apples_shown;       /* $0368 */
    uint8_t hud_count_2_shown;  /* $036A */
    uint8_t hover_shown;        /* $036D */
    uint8_t apples;             /* $0369: apples left, two decimal digits */
    uint8_t hud_count_2;        /* $036B: the status line's second counter */
    uint8_t figures_touched;    /* $036C: counted up as kind $27's figures are touched */
    uint8_t hover_on;           /* $036E: nonzero, the fourth action makes him hover */
    uint16_t gem_sets_found;    /* $0373: a bit per set gem found */
    uint8_t gem_sets;           /* $0375: set gems found */
    uint8_t gem_sets_total;     /* $0376: gem_sets added up at each level end */
    uint8_t continue_frames;    /* $142E: counted up while the continue screen shows */
    uint8_t menu_blink_hidden;  /* $0847: nonzero, the menu's chosen line is blanked */
    uint8_t menu_blink_frames;  /* $0848: frames the line has been shown, or frames left blanked */
    uint8_t menu_choice;        /* $0849: the title menu's choice (0 start, 1 password, 2 options) */
    uint8_t menu_choice_was;    /* $084A: the choice the mark was drawn at */
    uint8_t menu_cursor_moves;  /* $084B: counted up at each move of the cursor */
    uint8_t scene_cue;          /* $00BE: a scene's cue: its task or figures count it up or set it as the scene goes on */
    uint8_t scene_cue_2;        /* $00C0: a scene's second cue (level 5's closing scene: set as its map is rebuilt) */
    uint16_t range_x;           /* $1A44: the point actors are kept in range of (FrameStep: the view's corner + $80) */
    uint16_t range_y;           /* $1A46 */
    uint16_t demo_frames_left;  /* $0341: frames the demo still plays */
    uint8_t pause_cooldown;     /* $000B: frames before Start can pause again */
    uint8_t level_exiting;      /* $1423: nonzero once the player has reached the level's exit (his exit walk) */
    uint8_t stages_ended;       /* $1454: counted up as a stage's exit is reached */

    /* -- actors -- */
    Actor actors[ACTOR_RECORDS];    /* $08DB: actors[0] the player's */
    Actor *free_pool[ACTOR_POOL_SIZE];  /* $15A2: the free records of the pool, a stack */
    int8_t free_pool_top;       /* $15E4: its top entry; -1 none (the original: its byte offset, negative none) */
    Actor *free_pool_b[ACTOR_POOL_B_SIZE]; /* $15DA: the second pool's */
    int8_t free_pool_b_top;     /* $15E6 */
    uint8_t kind_count[256];    /* $7E:FF00: by kind, how many are live */
    uint8_t flier_slots[3];     /* $1478: a guard's three kind $90 fliers, by their +$07: low four bits nonzero, fly on; how each ended ($80 the ground, $40 a wall, 0 its steps ran out) */
    Actor *draw_list[4][32];    /* $1492: the four lists of the actors drawn this frame, $40 bytes each (ActorsInView) */
    uint8_t draw_count[4];      /* $1592, 2 apart: each list's length */
    uint8_t sprite_flicker;     /* $032D: bit 0 set, actors with +$08 bit 4 not drawn this frame; bit 1, those with bit 5 flash */
    uint8_t placed[256];        /* $7E:C600: by placement, bit 0 placed, bit 7 never placed again; [255] $FF, children's */
    uint8_t anim_set_of_kind[256];  /* $7F:F000: by kind, its animation set ($FF none); LevelActorGraphicsLoad's marks meanwhile */
    uint8_t anim_set_place[256];    /* $7F:E000: by kind, its set's place in the level's list of sets (4: not listed) */
    uint16_t anim_set_pictures[16]; /* $7F:E100: by place, its picture list's start in actor_pictures */
    uint16_t anim_set_table[16];    /* $7F:E300: by place, its animation table's start in anim_data, in bytes */
    uint8_t actor_pictures[24576]; /* $7E:2000: the level's sets' picture lists (at most 22,329 B, levels 0-63); picture_gfx is an offset here */
    uint16_t anim_data[2048];   /* $7E:B000: the level's sets' animation tables and frames (at most 1,219 words): time low, picture high */
    /* by kind, its sprite graphics in the level */
    uint16_t kind_gfx_at[256];  /* $7F:E500: where its graphics were unpacked, in video's unpack buffer */
    uint16_t kind_look[256];    /* $7F:E700: its sprite attributes (palette in bits 9-11); bit 15 set once loaded */
    uint8_t kind_extra_slots[256]; /* $7F:E900: the palette slots it needs past its first (0-2) */
    uint8_t kind_palette[256];  /* $7F:EA00: its palette among the actors' palettes, by its placements' variant */
    uint8_t kind_slot[256];     /* $7F:EB00: the palette slot it holds (1-4) */
    uint8_t palette_slot_users[4];    /* $15E9: per sprite palette slot (1-4 as 0-3), the actors holding it */
    uint8_t palette_slot_kind[4];     /* $15ED: per slot, the kind that claimed it */
} GameState;

extern GameState g;

#endif
