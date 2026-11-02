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

/* Demos and the start of play (game/progress.c). */
#define CHEAT_0009 0x000009u          /* nonzero: play starts with 10 health marks and 99 at $0369; the title acts on it too */
#define PLAY_MODE 0x000331u           /* 2: a demo plays, the pad read from its stream */
#define DEMO_INPUT_AT 0x00033Bu       /* the demo's next pad entry: byte offset from $84:F700 (word) */
#define DEMO_INPUT_FRAMES 0x00033Eu   /* frames left on its current pad entry */
#define DEMO_NUMBER 0x00033Fu         /* the demo being played */
#define HEALTH_MARKS 0x000365u        /* the player's health marks (most health) */
#define HEALTH 0x000367u              /* the player's health */

#endif
