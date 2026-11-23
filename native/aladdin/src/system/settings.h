/*
 * settings.h: what the player chooses in the menu and the game keeps between runs (settings.c): full screen or a window,
 * and the keys and controller buttons of pad 1. Kept in the engine's save folder as "options", apart from the original's
 * passwords (plan decision 4: passwords are the game's progress, there is no save file for it).
 */
#ifndef ALADDIN_SETTINGS_H
#define ALADDIN_SETTINGS_H

void settings_load(void);               /* in init: the saved bindings put on the controls, the saved window choice read */
int settings_full_screen(void);         /* 1 full screen (until the player chooses a window), 0 a window */
void settings_full_screen_set(int on);  /* and saved */
void settings_save(void);               /* the choice and the controls as they are now */

#endif
