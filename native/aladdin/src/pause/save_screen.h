/*
 * save_screen.h: the screen of save slots (slot_grid.h with system/saves.h), from the pause menu's SAVE MENU (saves and loads)
 * and from the title's SAVED GAMES (loads only). The game is held while it is open. Docs: docs/aladdin/native/SAVES.md.
 */
#ifndef ALADDIN_SAVE_SCREEN_H
#define ALADDIN_SAVE_SCREEN_H

#include "nshvc/nshvc.h"

typedef enum { SAVE_FROM_GAME, SAVE_FROM_TITLE } SaveFrom;
enum { SAVE_SCREEN_OPEN, SAVE_SCREEN_BACK, SAVE_SCREEN_LOADED };

/* when the pause menu opens (in update, before the menu is over the picture): the game's picture and play values taken */
void save_screen_capture(void);
void save_screen_open(SaveFrom from);
/* the title asks for it; the menu's update opens it from the next update */
void save_screen_request_title(void);
int save_screen_requested(void);
/* in update while open: SAVE_SCREEN_OPEN while it stays, BACK when shut, LOADED when a slot's game was put in */
int save_screen_update(const nshvc_input *input);

#endif
