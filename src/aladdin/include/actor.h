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

#define ACTOR_X 0x0Bu             /* x position: fraction, then pixels (word) */
#define ACTOR_Y 0x0Eu             /* y position: fraction, then pixels (word) */
#define ACTOR_FACING 0x14u        /* 0: facing right (x grows), else left */
#define ACTOR_SPEED_X 0x19u       /* horizontal speed, 24-bit, along the facing */
#define ACTOR_ACCEL_X 0x1Cu       /* added to or taken from the horizontal speed (fraction) */
#define ACTOR_SPEED_Y 0x1Du       /* vertical speed, 24-bit (down is positive) */
#define ACTOR_ACCEL_Y 0x20u       /* added to or taken from the vertical speed: gravity (fraction) */

#endif
