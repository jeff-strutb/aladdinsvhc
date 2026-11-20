/*
 * kinds_c_tables.h: the cartridge's tables the actor kinds of
 * actor/kinds_c.h read (data/kinds_c_tables.c holds their places). Each
 * reader gives an entry by what it holds.
 */
#ifndef ALADDIN_KINDS_C_TABLES_H
#define ALADDIN_KINDS_C_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* ---- kind $2B, the walker: by way (its surface * 2 + its direction) ---- */

uint8_t k2b_direction(uint8_t way);         /* its facing byte: the direction set's way */
uint8_t k2b_flip(uint8_t way);
uint8_t k2b_walk_anim(uint8_t way);
uint8_t k2b_turn_anim(uint8_t way);
/* the point it tests, from its place: ahead (on_surface 0), or ahead on its
   surface (1) */
uint16_t k2b_point_x(uint8_t way, int on_surface);
uint16_t k2b_point_y(uint8_t way, int on_surface);
DataRef k2b_probes(void);
uint8_t k2b_leg(uint8_t n);                 /* n: its +$07's low two bits */
uint8_t k2b_range(uint8_t n);
uint16_t k2b_partner_half(void);            /* how far from its partner's x it keeps */
uint16_t k2b_partner_span(void);            /* twice that */

/* ---- kind $33, the swinging platform ---- */

uint8_t k33_frame(uint8_t n);               /* the placed one's frame by the platform's direction / 8 */
/* its graphics' two uploads, `at` 0 or 6: the first tile (from its look's),
   the source (from where its graphics were unpacked), the bytes */
uint16_t k33_upload_tile(uint8_t at);
uint16_t k33_upload_from(uint8_t at);
uint16_t k33_upload_bytes(uint8_t at);

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

/* ---- kind $59, the Genie's screen's figures ---- */

uint16_t k59_lamp_x(uint8_t at);            /* a row lamp's place by `at` (4 times its number less 1) */
uint16_t k59_lamp_y(uint8_t at);            /* its y below the screen's */

/* ---- kind $63, the thrower above ---- */

DataRef k63_probes(void);
DataRef k63_pictures(void);
uint16_t k63_reach_y(uint8_t at);           /* by its +$07 * 2: added to its y before his is taken off */
uint8_t k63_aim_near(uint8_t at);           /* by its +$07 * 2: below this, aim 2 */
uint8_t k63_aim_mid(uint8_t at);            /* then below this, aim 1; else 0 */

/* ---- kind $6B ---- */

DataRef k6b_probes(void);

/* ---- kind $83: by its +$07 ---- */

DataRef k83_probes(void);
uint8_t k83_delay(uint8_t variant);         /* frames before it shows (0: 256) */
uint8_t k83_top_speed(uint8_t variant);     /* the most its vertical speed's fraction may reach */
uint8_t k83_preset(uint8_t variant);        /* its motion preset */

/* ---- kind $3A, the hold ---- */

int16_t k3a_hold_x(uint8_t picture);        /* where he is held, from its x, by its picture */
int16_t k3a_hold_y(uint8_t picture);
uint8_t k3a_frame(uint8_t frame, int facing_left);  /* his frame while held, by its frame */

/* ---- kinds $7E, $7F ---- */

uint16_t k7e_first_angle(uint8_t n);        /* a circler's first angle, entry n */
DataRef k7f_probes(void);

/* ---- kind $90, the stepped flier ---- */

DataRef k90_probes(void);
uint8_t k90_steps(uint8_t slot);            /* by its +$07: steps */
uint8_t k90_preset(uint8_t slot);           /* by its +$07: its motion preset */
uint8_t k90_flights(uint8_t slot);          /* by its +$07: flights a step */

#endif
