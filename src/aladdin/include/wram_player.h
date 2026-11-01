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
#define PLAYER_GROUND_Y 0x001455u    /* the player's Y (pixels) where it last stood (word) */

#endif
