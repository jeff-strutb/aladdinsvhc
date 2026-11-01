/*
 * wram_state.h: work RAM that holds the game's settings and progress, and
 * the frame count. The same names as
 * asm/aladdin/include/wram_state.inc, in upper case. Addresses as in wram.h.
 */
#ifndef ALADDIN_WRAM_STATE_H
#define ALADDIN_WRAM_STATE_H

/* Kept across a reset: power-on finds $0123 at $0000 and the cartridge
   title copied at $B600 when the work RAM still holds them. */
#define KEPT_OPTIONS 0x000003u        /* three option bytes the options screen sets */
#define KEPT_OPTIONS_SUM 0x000006u    /* their 8-bit sum */

#define FRAME_COUNT 0x00032Eu         /* counts NMIs (8-bit) */
#define CONTINUES 0x00035Bu           /* continues left (3 at the start of a game) */

#endif
