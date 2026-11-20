/*
 * display.c: switching the NMI's work and the screen. The original set the
 * interrupt's enable and the screen's blank and brightness in the PPU and
 * kept copies of them; here they are the fields the loop and draw read.
 */
#include "system/state.h"
#include "system/system.h"

/*
 * @routine 81:969F NmiEnable
 * WHAT IT DOES: Turns the vertical-blank interrupt on: from the next update
 * the NMI's work (system/nmi.c) runs again.
 */
void NmiEnable(void)
{
    g.nmi_on = 1;
}

/*
 * @routine 81:96A8 NmiDisable
 * WHAT IT DOES: Turns the vertical-blank interrupt off while a screen is
 * set up. No task waits while it is off (it would wait for ever in the
 * original), so no update starts with it off on the boot path.
 */
void NmiDisable(void)
{
    g.nmi_on = 0;
}

/*
 * @routine 81:96B1 ScreenBlank
 * WHAT IT DOES: Blanks the screen now (forced blank), keeping the
 * brightness.
 */
void ScreenBlank(void)
{
    g.blank = 1;
}

/*
 * @routine 81:96C5 ScreenFullBright
 * WHAT IT DOES: Shows the screen at full brightness from the next picture.
 */
void ScreenFullBright(void)
{
    g.blank = 0;
    g.brightness = 15;
}
