/*
 * kinds_b_tables.h: the cartridge's tables kinds B's actor kinds read
 * (data/kinds_b_tables.c holds their places). Each reader gives an entry
 * by what it holds. Every kind runs with the data bank $80 (the actor
 * loop's); each place below was confirmed in the traces.
 */
#ifndef ALADDIN_KINDS_B_TABLES_H
#define ALADDIN_KINDS_B_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* kind $38 */
DataRef k38_probes(void);               /* its probe list (Actor.probes) */

/* kind $42 */
DataRef k42_probes(void);               /* its probe list */
uint16_t k42_ahead_x(uint8_t facing);   /* the x offset of the block ahead of it, by facing (0 right, 1 left) */

/* kind $52's wait routine: the pictures on which it hurts, and each one's zone */
#define K52_HURT_PICTURES 5
uint8_t k52_hurt_picture(uint8_t i);    /* i < K52_HURT_PICTURES */
uint8_t k52_hurt_zone(uint8_t i);

/* kind $5B */
uint8_t k5b_delay(uint8_t variant);     /* frames before it is thrown, by its +$07 (bit 7 clear) */
uint8_t k5b_step_sound(uint8_t at);     /* its wait routine's step sounds, by its x pixels' low 4 bits */

/* kind $76 */
DataRef k76_pictures(void);             /* its picture list (Actor.own_pictures) */

/* kinds $99, $9A */
DataRef k9a_probes(void);               /* kind $9A's probe list */
uint8_t k99_anim(uint8_t piece);        /* kind $99: its animation by its +$07 (0-31) */
uint16_t k99_spread(uint8_t piece);     /* kind $99: its offset across by its +$07 (0-31), pixels */

/* kind $A2 */
uint8_t ka2_heading(uint8_t at);        /* its heading (a direction) by +$07, plus 3 facing left */

#endif
