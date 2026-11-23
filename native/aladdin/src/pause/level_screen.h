/*
 * level_screen.h: the level select screen (slot_grid.h with game/beaten.h): the levels the player has beaten, four to a page, each a
 * picture of the level and its number; choosing one starts it. Opened from the title's LEVEL SELECT; the title is held while it is open.
 * Docs: docs/aladdin/native/LEVELSELECT.md.
 */
#ifndef ALADDIN_LEVEL_SCREEN_H
#define ALADDIN_LEVEL_SCREEN_H

#include "nshvc/nshvc.h"

enum { LEVEL_SCREEN_OPEN, LEVEL_SCREEN_BACK, LEVEL_SCREEN_CHOSEN };

/* the title asks for it; the menu's update opens it from the next update */
void level_screen_request_title(void);
int level_screen_requested(void);
int level_screen_active(void);
void level_screen_open(void);
/* in update while open: OPEN while it stays, BACK when shut, CHOSEN when a level was chosen (and the screen is shut) */
int level_screen_update(const nshvc_input *input);
/* the title takes the chosen level's list entry once (game/beaten.h's list), -1 when none */
int level_screen_take(void);

#endif
