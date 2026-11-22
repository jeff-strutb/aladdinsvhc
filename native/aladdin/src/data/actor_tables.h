/*
 * actor_tables.h: the cartridge's tables the actors read (data/actor_tables.c
 * holds their places). Each reader gives a table's entry by what it holds.
 */
#ifndef ALADDIN_ACTOR_TABLES_H
#define ALADDIN_ACTOR_TABLES_H

#include <stdint.h>

/* ---- a level's actor placements ---- */

/* one placement, as the level's list holds it: the kind, a byte whose low 5
   bits the actor gets at +$07 and whose bit 7 means never placed again
   (bits 5-6 its graphics' variant), and its place as words whose low 14
   bits are pixels */
typedef struct Placement {
    uint8_t kind;
    uint8_t byte;
    uint16_t x_word;
    uint16_t y_word;
} Placement;

unsigned placement_count(uint16_t level);
Placement placement_at(uint16_t level, unsigned n);    /* n < placement_count(level) */

/* ---- creating an actor ---- */

/* how a kind's set-up runs: 0 nothing, 2 its graphics shared or loaded, 4
   its graphics in a new palette slot; other bytes are past the table's
   kinds */
uint8_t kind_setup_class(uint8_t kind);
uint16_t kind_graphics_bytes(uint8_t kind);    /* its graphics' length in the unpack buffer (0: none to upload) */
uint16_t palette_slot_look(uint8_t slot);      /* slot 1-4: the sprite attributes an actor in it takes */
uint8_t level_escort(uint16_t level);          /* the level's escort: its +$07 plus 1; 0 none */

/* ---- animation ---- */

uint8_t kind_anim_set(uint8_t kind);           /* the kind's animation set; $FF none */
/* a set's picture list: `*bytes` bytes, contiguous in the cartridge */
const uint8_t *anim_set_pictures(uint8_t set, uint16_t *bytes);
/* a set's animation table: `*words` little-endian words */
const uint8_t *anim_set_table(uint8_t set, uint16_t *words);

/* ---- motion: speeds as 24-bit values, a fraction byte then the pixels ---- */

uint32_t speed_x_entry(uint8_t at);            /* the horizontal speed table, entry at byte offset `at` */
uint32_t speed_y_entry(uint8_t at);            /* the vertical one */
uint32_t speed_triple(uint8_t at);             /* the story figures' speeds, entry at byte offset `at` */
typedef struct MotionPreset {
    uint32_t speed_x, speed_y;
    uint8_t gravity;
} MotionPreset;
MotionPreset motion_preset(uint8_t preset);

/* vectors: a direction set's first vector (`at`: the set's byte offset;
   the next vector is the set's for facing left), and a vector's 24-bit x
   and y parts (a fraction byte, then pixels) */
uint16_t vector_set_first(uint8_t at);
uint32_t vector_x(uint16_t v);
uint32_t vector_y(uint16_t v);

/* ---- spawning and view ---- */

uint16_t spawn_offset_x(uint8_t at);           /* a child's place from its spawner: across, at byte offset `at` */
uint16_t spawn_offset_y(uint8_t at);           /* down */
typedef struct ViewBox {
    uint16_t x_off, x_span, y_off, y_span;
} ViewBox;
ViewBox view_box_screen(uint8_t at);           /* the same, the original's (not grown with the view) */
ViewBox view_box(uint8_t at);                  /* at: an actor's view_box (0, 8, 16, 24, 32) */

/* ---- the story figures (kinds $95, $96) ---- */

uint8_t k95_flight_frames(uint8_t step);       /* part 8: each step's frames */
typedef struct Kind95Hops {
    uint8_t pose, delay, count;
    uint8_t up_frames, up_speed, down_frames, down_speed;  /* speeds: speed_triple offsets */
} Kind95Hops;
Kind95Hops k95_hops(uint8_t set);              /* set 0-3 */
uint8_t k96_first_wait(uint8_t variant);       /* kind $96: its first wait's frames, by its +$07 */

#endif
