/*
 * menu_art.h: the menu's pictures (menu_art.c). The panel is laid out in game pixels, MENU_W x MENU_H, as the game's own
 * 8 x 8 text is; an image holds it at an whole number D of image pixels to a game pixel. Its text, its hand and its
 * colours are the cartridge's own (the text tiles and the options screen's palettes, through the data layer); the
 * controller is Stunt Race FX's drawing (pad_art.c).
 */
#ifndef ALADDIN_MENU_ART_H
#define ALADDIN_MENU_ART_H

#include <stdint.h>

enum { MENU_PAL_GLOVE, MENU_PAL_CREAM, MENU_PAL_GOLD, MENU_PAL_PINK };   /* the text palettes (the options screen's set) */

#define MENU_W 352
#define MENU_H 282

/* the items, in the order the menu moves among them: the twelve buttons (left column top to bottom, then the right), then
   RESUME and DEFAULTS in a row, SAVE MENU and FULL SCREEN under them, QUIT under those */
enum {
    MENU_TAGS = 12,
    MENU_RESUME = MENU_TAGS, MENU_DEFAULTS, MENU_SAVE, MENU_FULL, MENU_QUIT, MENU_ITEMS
};

/* a button's place: its tag's column (0 left, 1 right) and row, and the pad bit's index in input/controls.h's order */
typedef struct { int right, row, button; const char *name; } MenuTag;
extern const MenuTag menu_tags[MENU_TAGS];

/* what the dynamic picture shows */
typedef struct {
    int chosen;                         /* an item */
    int asking;                         /* the tag waiting for its key or button; -1 none */
    int quit_asked;                     /* QUIT chosen once: it reads REALLY QUIT */
    int full_offered, full_on;          /* FULL SCREEN shown, and whether on */
    char hint[48];                      /* the choose and resume hint in the names of the device in use */
    char action[MENU_TAGS][12];         /* what each tag says the button does in the game */
    char label[MENU_TAGS][12];          /* what each tag shows (the key or button bound), upper case */
} MenuView;

/* Where the panel lies in a window's safe area (l, t, w, h in window pixels): its window pixels a game pixel (a whole
   number from 1 up, below 1 when the area is smaller than the panel), the image pixels a game pixel to draw it at, and
   its top left. */
void menu_art_layout(float l, float t, float w, float h, float *scale, int *d, float *ox, float *oy);

void menu_art_start(void);              /* reads the font and the palettes from the cartridge, once */

/* an image of MENU_W * d by MENU_H * d pixels, 0xAARRGGBB straight alpha */
void menu_art_static(uint32_t *px, int d);                  /* the frame, the title and the controller: never change */
void menu_art_dynamic(uint32_t *px, int d, const MenuView *v);   /* the tags, the items, the hand and the chosen button's ring */

/* the item at game pixel (x, y) in the panel, -1 none */
int menu_art_item_at(float x, float y, int full_offered);

/* the drawing kit other screens use (the save slots, the level select), in game pixels at d image pixels each, over a
   MENU_W x MENU_H image */
void menu_art_panel(uint32_t *px, int d, const char *title);       /* the panel, its frame, its title and the rule under it */
void menu_art_text(uint32_t *px, int d, const char *s, int pal, int x, int y, int scale);
int menu_art_text_w(const char *s);                                  /* in game pixels, at scale 1 */
void menu_art_hand(uint32_t *px, int d, int x, int y);               /* the 24 x 24 hand */
void menu_art_box(uint32_t *px, int d, int x, int y, int w, int h, int bright);   /* a frame in gold, or cream when bright */
void menu_art_blit(uint32_t *px, int d, int x, int y, const uint32_t *src, int w, int h);   /* 1 game pixel to a source pixel */

#endif
