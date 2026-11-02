/*
 * hud_reset.c: the status line drawn afresh: its text, every health mark
 * place, and each value marked as not yet shown so the refresh draws it.
 */
#include "machine.h"
#include "ops65.h"

void abi_HudHealthMarkShow(void);

#define HEALTH_MARKS 0x0365u        /* how many health marks the line has */
#define HEALTH_SHOWN 0x0366u        /* the health the marks show */
#define HUD_0363 0x0363u            /* a value as shown (its value at $0364), a digit */
#define HUD_COUNT_1_SHOWN 0x0368u   /* the first counter as shown (its value at $0369) */
#define HUD_COUNT_2_SHOWN 0x036Au   /* the second counter as shown (its value at $036B) */
#define HUD_036D 0x036Du            /* a value as shown (its value at $036E) */

/* Not yet filed */
#define TEXT_DRAW 0x81972Du         /* text A drawn in the text layer, its upload asked for */
#define HUD_DIGIT_REFRESH 0x81F47Fu /* the digit at $0363 drawn when it differs from $0364 */
#define HUD_REFRESH 0x81F456u       /* each value drawn that differs from the one shown */



