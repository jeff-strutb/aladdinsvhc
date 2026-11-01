/*
 * actor.h: an actor's record, as its code reaches it through the direct
 * page (D = the record's address). 35 records of $50 bytes from $092B.
 * Positions and speeds are 24-bit, little-endian: a fraction byte, then a
 * 16-bit whole part (pixels, pixels a frame).
 */
#ifndef ALADDIN_ACTOR_H
#define ALADDIN_ACTOR_H

/* The records: the player's first, then the others */
#define PLAYER 0x0008DBu          /* Aladdin's record */
#define ACTOR_FREE_LIST 0x0015A2u /* addresses of the free records (words), a stack */
#define ACTOR_FREE_TOP 0x0015E4u  /* byte offset of its top entry; negative: none free */

#define ACTOR_STATE 0x00u         /* what the actor is doing ($8C, $CC ...) */
#define ACTOR_07 0x07u
#define ACTOR_08 0x08u            /* low two bits: set from the frame's parity by some actors */
#define ACTOR_09 0x09u            /* bits 4-5: the sprite priority some actors set by height */
#define ACTOR_X 0x0Bu             /* x position: fraction, then pixels (word) */
#define ACTOR_Y 0x0Eu             /* y position: fraction, then pixels (word) */
#define ACTOR_FACING 0x14u        /* 0: facing right (x grows), else left */
#define ACTOR_15 0x15u            /* the player's: nonzero, the reach test's box sits 10 pixels nearer */
#define ACTOR_18 0x18u            /* zero: idle, for the list walk at $84:8A52 */
#define ACTOR_SPEED_X 0x19u       /* horizontal speed, 24-bit, along the facing */
#define ACTOR_ACCEL_X 0x1Cu       /* added to or taken from the horizontal speed (fraction) */
#define ACTOR_SPEED_Y 0x1Du       /* vertical speed, 24-bit (down is positive) */
#define ACTOR_ACCEL_Y 0x20u       /* added to or taken from the vertical speed: gravity (fraction) */

#define ACTOR_NEXT 0x37u          /* the next record in the actor's list (word; 0 ends it) */
#define ACTOR_40 0x40u            /* word */
#define ACTOR_44 0x44u
#define ACTOR_45 0x45u
#define ACTOR_46 0x46u
#define ACTOR_47 0x47u
#define ACTOR_48 0x48u

#endif
