/*
 * wram_player.h: work RAM the bank-$83 player code keeps. The same names
 * as asm/aladdin/include/wram_player.inc, in lower case there. Addresses
 * as in wram.h.
 */
#ifndef ALADDIN_WRAM_PLAYER_H
#define ALADDIN_WRAM_PLAYER_H

#define PLAYER_X 0x0008E7u         /* word: Aladdin's X position */
#define PLAYER_Y 0x0008EAu         /* word: Aladdin's Y position */
#define PLAYER_08F0 0x0008F0u      /* nonzero: the near test's reach is 8, not 18 */
#define POINT_145D 0x00145Du       /* word: X of a point an object sets (Y at $145F) */
#define POINT_145F 0x00145Fu

#endif
