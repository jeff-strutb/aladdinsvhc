/*
 * kinds_e_tables.h: the cartridge's tables kinds E's actors read
 * (data/kinds_e_tables.c holds their places). Each reader gives an entry by
 * what it holds.
 */
#ifndef ALADDIN_KINDS_E_TABLES_H
#define ALADDIN_KINDS_E_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* ---- picture lists (an actor's +$29: the list ActorPictureSend sends its
   own pictures from) ---- */

DataRef k21_pictures(void);     /* kind $21's */
DataRef k98_pictures(void);     /* kind $98's */
DataRef k8f_pictures(void);     /* kind $8F's */
DataRef k5a_pictures(void);     /* kind $5A's */

/* ---- kind $2E, level 5's closing scene ---- */

/* part 2's place beside Aladdin: by `at` (his frame times 2, a byte), the
   offsets added to his x and y, pixels */
uint16_t k2e_beside_x(uint8_t at);
uint16_t k2e_beside_y(uint8_t at);

/* ---- probe lists (an actor's +$16: the map's probes read them) ---- */

DataRef k31_probes(void);       /* kind $31's */
DataRef k98_probes(void);       /* kind $98's */
DataRef k54_probes(void);       /* kind $54's */
DataRef k55_probes(void);       /* kind $55's */
DataRef k56_probes(void);       /* kind $56's */

/* ---- kind $3B's track (part 2), by `at` a byte offset ---- */

uint16_t k3b_ahead_x(uint8_t at);   /* by direction times 4: the pixels ahead it reads the map at, across */
uint16_t k3b_ahead_y(uint8_t at);   /* and down */
uint8_t k3b_turn(uint8_t at);       /* the direction after a turn, by the quarter's bits 7-6 and its direction's bit 0 */
uint8_t k3b_spot_dir(uint8_t at);   /* the direction out of a spot, by its direction * 2 and the flag's bit 0 */
uint8_t k3b_spot_turn(uint8_t at);  /* the turn added at each of its steps to a spot, the same */

/* ---- kind $2A ---- */

uint8_t k2a_reach(uint8_t at);      /* the leaper's distance below it Aladdin must be, by its +$07 */
uint16_t k2a_speed(uint8_t at);     /* the pacer's speed by its variant times 2 (16.8) */
uint8_t k2a_leg(uint8_t variant);   /* the frames of its first pace by its variant (each later one twice that) */

/* ---- kinds $54 and $55 ---- */

uint8_t k54_rise(uint8_t way);      /* kind $54's speed up (ActorSpeedYSet's entry) by +$07 bits 0-1 */
uint8_t k55_across(uint8_t way);    /* kind $55's speed across (ActorSpeedXSet's entry) by +$07 bits 0-1 */

/* ---- kind $97: the windows of the range point's x it acts in, a low and a
   high word each, by `at` a byte offset ---- */

#define K97_DROP_WINDOW 0       /* the dropper's */
#define K97_PAIR_WINDOW_A 4     /* the sender's, +$07 AND $11 nonzero */
#define K97_PAIR_WINDOW_B 8     /* the sender's, +$07 AND $11 zero */
uint16_t k97_window(uint8_t at);

/* ---- kind $28 ---- */

uint8_t k28_wave(uint8_t at);       /* its wave: the step down (a signed byte) at place `at` (0 to $23) */

/* ---- kind $94's sparks: by `at` (the scene's count times 2), x and y
   pixels' low bytes ---- */

uint8_t k94_spark_x(uint8_t at);
uint8_t k94_spark_y(uint8_t at);

/* ---- kind $A8 ---- */

uint8_t k_a8_delay(uint8_t at);     /* the frames before it poses again, by its +$07 (0 is 256) */

/* ---- kind $58, by its +$07 ---- */

uint8_t k58_leg(uint8_t at);        /* the frames of its first leg */
uint8_t k58_direction(uint8_t at);  /* its direction (the vector set's index) */

/* ---- kind $73, the chain's head ---- */

DataRef k73_pictures(void);
int16_t k73_touch_dx(uint8_t pose);     /* its touch place's offset from it by its pose (index by 4) */
int16_t k73_touch_dy(uint8_t pose);

/* ---- a path (ActorPathStep): its segments' words and bytes at offsets of
   the DataRef Actor.stream names; `path_at` the same list's bank at
   `addr` ---- */

uint16_t path_word(DataRef r, unsigned off);
uint8_t path_byte(DataRef r, unsigned off);
DataRef path_at(DataRef r, uint16_t addr);

/* ---- kind $35, the gem ---- */

/* a set gem's bit in g.gem_sets_found, by `at` (its +$07's low digit, a
   byte offset into the table of words) */
uint16_t k35_set_bit(uint8_t at);

#endif
