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

/*
 * @routine 81:F433 HudRedrawAll
 * WHAT IT DOES: Draws the status line afresh: its text, every health mark
 * place, and then each of its values.
 * HOW: Text 0; HudHealthMarkShow for each mark from the last down to the
 * first; the shown health, $0363, $036D and the two counters zeroed; the
 * digit's refresh at $81:F47F; then on into the refresh at $81:F456.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: as the refresh at $81:F456 leaves them.
 */
void abi_HudRedrawAll(void)
{
    lda8_v(0x00);
    call_routine(TEXT_DRAW, 1);
    lda8(ea_abs(HEALTH_MARKS));
    do {
        pha8();
        abi_HudHealthMarkShow();
        pla8();
        ACC8(dec);
    } while (!IF_Z);
    stz8(ea_abs(HEALTH_SHOWN));
    stz8(ea_abs(HUD_0363));
    stz8(ea_abs(HUD_036D));
    stz8(ea_abs(HUD_COUNT_1_SHOWN));
    stz8(ea_abs(HUD_COUNT_2_SHOWN));
    call_routine(HUD_DIGIT_REFRESH, 0);
    jump_routine(HUD_REFRESH);
}
