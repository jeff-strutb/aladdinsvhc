/*
 * slot_grid.h: a pageable grid of cells, each with a picture (shown 1 game pixel to 1 picture pixel) and a two-line
 * caption, in the pause menu's panel and look (menu_art.h). The save slots and the level select are built on it; it holds
 * no save or level logic: the owner gives the cells, takes the events, and asks its prompts through it.
 *
 * Shape is the spec's: cols x rows cells a page, the picture's size, the number of pages. The grid is drawn by slot_grid_draw
 * (in draw, from its own state outside the game's), moved by slot_grid_update (in update). It queues the menu's move sound;
 * the owner sends the sound queue.
 *
 * Pad: the directions move among the cells, Left at the first column and Right at the last turn the page, L and R turn it,
 * B or A chooses, Start or Escape go back (or, with a prompt up, put the prompt away). The pointer chooses the cell it is on,
 * turns the page on the "<" and ">" of the page line, and chooses a prompt's answer.
 */
#ifndef ALADDIN_SLOT_GRID_H
#define ALADDIN_SLOT_GRID_H

#include <stdint.h>

#include "nshvc/nshvc.h"

typedef struct {
    int used;                           /* 0: the cell is empty, drawn as an empty frame (EMPTY, no picture) */
    const uint32_t *picture;            /* pic_w x pic_h, 0xAARRGGBB; NULL: none */
    char line1[24], line2[24];          /* the caption, upper case, at most 12 letters each to fit a 104-wide cell */
} SlotCell;

typedef struct {
    const char *title;                  /* in the panel's title place, 2x text */
    int cols, rows;                     /* cells in a page: 3 x 2 for the save slots */
    int pages;
    int pic_w, pic_h;                   /* the picture's size in game pixels; cols * (pic_w + 8) + gaps must fit in 336 and
                                           rows * (pic_h + 30) in 210 */
    const SlotCell *(*cell)(void *ctx, int index);   /* cell number `index` (page * cols * rows + place); never NULL */
    void *ctx;
} SlotGridSpec;

typedef enum { SLOT_NONE, SLOT_CHOSEN, SLOT_ANSWER, SLOT_BACK } SlotEvent;
typedef struct { SlotEvent what; int index; int answer; } SlotResult;   /* index: the cell chosen; answer: the prompt's choice */

/* Opens it (the spec is copied; the first cell is chosen). */
void slot_grid_open(const SlotGridSpec *spec);
void slot_grid_close(void);
int slot_grid_is_open(void);
/* The cells' contents changed (draw them again). */
void slot_grid_changed(void);
/* A prompt in place of the hint: `line` over the `n` (1 to 3) choices, the first chosen; SLOT_ANSWER or the prompt put away. */
void slot_grid_ask(const char *line, const char *const *choices, int n);
int slot_grid_asking(void);
int slot_grid_selected(void);

/* In update, while it is open. */
SlotResult slot_grid_update(const nshvc_input *input);
/* In draw: the overlays set, or taken away when it has shut. */
void slot_grid_draw(void);

/* For the checks: the panel image of the grid as it now is, MENU_W * d by MENU_H * d (static and dynamic over each other). */
void slot_grid_render(uint32_t *px, int d);

#endif
