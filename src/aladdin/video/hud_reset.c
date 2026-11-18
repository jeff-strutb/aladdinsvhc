/*
 * hud_reset.c: the status line drawn afresh: its text, every health mark
 * place, and each value marked as not yet shown so the refresh draws it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_TextLayoutShow(void);

void abi_HudHealthMarkShow(void);
void abi_HudDigitRefresh(void);
void abi_HudRefresh(void);

#define HEALTH_MARKS 0x0365u        /* how many health marks the line has */
#define HEALTH_SHOWN 0x0366u        /* the health the marks show */
#define HUD_0363 0x0363u            /* a value as shown (its value at $0364), a digit */
#define HUD_COUNT_1_SHOWN 0x0368u   /* the first counter as shown (its value at $0369) */
#define HUD_COUNT_2_SHOWN 0x036Au   /* the second counter as shown (its value at $036B) */
#define HUD_036D 0x036Du            /* a value as shown (its value at $036E) */

/*
 * @routine 81:F433 HudRedrawAll
 * WHAT IT DOES: Draws the status line afresh: its text, every health mark
 * place, and then each of its values.
 * HOW: Text 0; HudHealthMarkShow for each mark from the last down to the
 * first; the shown health, $0363, $036D and the two counters zeroed; then
 * HudDigitRefresh, and on into HudRefresh.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: as HudRefresh leaves them.
 */

void abi_HudRedrawAll(void)
{
    uint8_t mark;

    a8_load(0x00);
    jsl_frame(0x81F435u); abi_TextLayoutShow(); cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(rd8(dba(HEALTH_MARKS)));
    do {                                  /* each mark place, the last first */
        pha8();
        call_frame(0xF43F, abi_HudHealthMarkShow);   /* 81F43D jsr */
        pla8();
        mark = (uint8_t)(cpu.a - 1);
        a8_load(mark);
    } while (mark);
    wr8(dba(HEALTH_SHOWN), 0);
    wr8(dba(HUD_0363), 0);
    wr8(dba(HUD_036D), 0);
    wr8(dba(HUD_COUNT_1_SHOWN), 0);
    wr8(dba(HUD_COUNT_2_SHOWN), 0);
    call_frame(0xF455, abi_HudDigitRefresh);     /* 81F453 jsr */
    abi_HudRefresh();                            /* on into it */
}
