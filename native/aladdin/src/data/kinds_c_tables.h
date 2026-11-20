/*
 * kinds_c_tables.h: the cartridge's tables the actor kinds of
 * actor/kinds_c.h read (data/kinds_c_tables.c holds their places). Each
 * reader gives an entry by what it holds.
 */
#ifndef ALADDIN_KINDS_C_TABLES_H
#define ALADDIN_KINDS_C_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* ---- kind $2D, the thrower ---- */

/* by facing * 4 (0 right, 4 left): where its reach starts, from its x, and
   how far it reaches */
uint16_t k2d_reach_from(uint8_t at);
uint16_t k2d_reach(uint8_t at);
uint8_t k2d_throw_frames(uint8_t variant);  /* by its +$07: the throw's frames */
uint8_t k2d_throw_leaves(uint8_t variant);  /* by its +$07: the frame left of the throw the copy leaves at */
uint8_t k2d_second_arc(uint8_t arc);        /* a +$07 = 2 thrower's second throw's arc, by the first's */
uint8_t k2d_copy_pose(uint8_t arc);         /* a thrown copy's first pose, by its arc */
DataRef k2d_pictures(void);                 /* its picture list */

/* ---- kind $2F, the riser ---- */

DataRef k2f_probes(void);                   /* its probe offsets */

/* ---- kind $4F, the thing that opens once ---- */

uint8_t k4f_opened_bit(uint8_t n);          /* n 0-3 (its +$07's low bits): its bit in g.opened */

/* ---- kind $90, the stepped flier ---- */

DataRef k90_probes(void);
uint8_t k90_steps(uint8_t slot);            /* by its +$07: steps */
uint8_t k90_preset(uint8_t slot);           /* by its +$07: its motion preset */
uint8_t k90_flights(uint8_t slot);          /* by its +$07: flights a step */

#endif
