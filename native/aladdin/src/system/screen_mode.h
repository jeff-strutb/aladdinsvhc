/*
 * screen_mode.h: full screen or a window (screen_mode.c), through the engine's switch (nativeshvc 6.5). Full screen until
 * the player chooses a window; the choice is kept in the settings.
 */
#ifndef ALADDIN_SCREEN_MODE_H
#define ALADDIN_SCREEN_MODE_H

int screen_mode_can_switch(void);       /* a window that can switch (not a phone's whole screen, not a headless run) */
int screen_mode_full(void);             /* 1: full screen now */
void screen_mode_set(int on);           /* switched, and the choice saved */
void screen_mode_start(void);           /* in init: the saved choice put on the window */
void screen_mode_update(void);          /* each update: a switch the system's own keys or menu made, saved */

#endif
