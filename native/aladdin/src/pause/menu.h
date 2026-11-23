/*
 * menu.h: the pause menu (menu.c), in place of the original's PAUSED. Start during play (where the original paused) and
 * Escape on any screen open it; Start or Escape shuts it. While it is open the game is not stepped: its tasks, its
 * frame counts and its picture stand where they were, as the original's pause stood them, and the sound goes on (the
 * music paused, the menu's sounds sent).
 */
#ifndef ALADDIN_MENU_H
#define ALADDIN_MENU_H

#include "nshvc/nshvc.h"

/* In update, before the game's step: 1 when the menu holds this update (the game is not stepped), 0 when it does not. */
int menu_update(const nshvc_input *input);

/* The original's pause check (game/pause.c), when Start is pressed where it would have paused: the menu opens from the
   next update. */
void menu_request(void);

#endif
