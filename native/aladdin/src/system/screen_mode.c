/*
 * screen_mode.c: full screen or a window (screen_mode.h). The player's choice is the settings'. A switch made by the
 * system's own keys or menu (macOS's View menu, F11 and Alt+Enter on Windows and Linux) is saved as the choice too, so
 * the game starts as the player last left it. Where the window cannot switch there is nothing to do and the menu offers
 * no FULL SCREEN.
 */
#include "nshvc/nshvc.h"
#include "system/screen_mode.h"
#include "system/settings.h"

static int started;

int screen_mode_can_switch(void) { return nshvc_display_info().full_screen_switch != 0; }

int screen_mode_full(void) { return nshvc_display_info().full_screen != 0; }

void screen_mode_set(int on)
{
    if (!screen_mode_can_switch())
        return;
    nshvc_full_screen(on != 0);
    settings_full_screen_set(on);
}

void screen_mode_start(void)
{
    started = 1;
    if (screen_mode_can_switch() && screen_mode_full() != settings_full_screen())
        nshvc_full_screen(settings_full_screen());
}

void screen_mode_update(void)
{
    if (started && screen_mode_can_switch())
        settings_full_screen_set(screen_mode_full());
}
