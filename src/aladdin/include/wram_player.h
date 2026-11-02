/*
 * wram_player.h: work RAM the player's code keeps (player/), by name, as
 * wram.h. The same names as asm/aladdin/include/wram_player.inc.
 */
#ifndef ALADDIN_WRAM_PLAYER_H
#define ALADDIN_WRAM_PLAYER_H

#define PLAYER_TOP_SPEED 0x00141Du   /* the fastest the player runs (word, the speed's whole and fraction) */
#define PLAYER_RUN_ACCEL 0x001440u   /* added to the speed while the pad holds the facing (word) */
#define PLAYER_RUN_DECEL 0x001442u   /* added while no direction is held (word, negative) */
#define PLAYER_TURN_DECEL 0x001444u  /* added while the pad holds the other way (word, negative) */
#define PLAYER_MOVE 0x001420u        /* the player's move (standing, running, jumping ...): picks its animation */
#define PLAYER_DASH 0x001459u        /* 1 while the dash is held: picks the faster speed rows */
#define PLAYER_SLOWED 0x001457u      /* counts the frames the speed was held up to its least */
#define PLAYER_POS_SAVED 0x00145Du   /* a position kept for the player: X, then Y at +2 (pixels, words) */
#define V_0012 0x000012u             /* direction bits the player's code reads */
#define V_1428 0x001428u             /* nonzero: the player's direction comes from V_1434 */
#define V_1434 0x001434u
#define V_141C 0x00141Cu
#define V_142D 0x00142Du
#define V_144D 0x00144Du
#define V_1A08 0x001A08u
#define PLAYER_GROUND_Y 0x001455u    /* the player's Y (pixels) where it last stood (word) */

/* The player's test box and hurt time (player/player_box.c,
   player/player_bounds.c) */
#define PLAYER_BOX_WANTED 0x001448u  /* the box of the player's box table the code wants the test box set from */
#define PLAYER_BOX 0x001449u         /* the box the test box was last set from */
#define PLAYER_HURT_TIME 0x0003F5u   /* frames left before the player can be hit again */

#endif
