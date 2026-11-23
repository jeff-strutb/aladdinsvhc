/*
 * controls.h: the keys and controller buttons that make each of the SNES pad's twelve buttons for pad 1 (controls.c),
 * which the menu (pause/menu.c) lets the player change. Pad 2 is read as the engine's defaults: the game reads pad 1 alone.
 */
#ifndef ALADDIN_CONTROLS_H
#define ALADDIN_CONTROLS_H

#include <stdint.h>

#include "nshvc/nshvc.h"

#define CONTROLS_BUTTONS 12
/* the pad bits (system.h PAD_*) in the order the settings file keeps their bindings */
extern const uint16_t controls_buttons[CONTROLS_BUTTONS];

/* pad `pad`'s buttons as the devices hold them now: for pad 1 the bindings, and any button held when the menu closed kept
   out until let go (controls_swallow); `raw` leaves that out (the menu's own reading). */
uint16_t controls_pad(const nshvc_input *input, unsigned pad);
uint16_t controls_pad_raw(const nshvc_input *input);
void controls_swallow(uint16_t held);

/* the bindings: button b (an index of controls_buttons); a key by its nshvc_key, a controller button by its one NSHVC_CTL_*
   bit. A key or button another had goes over to the one whose own was taken, so each button keeps one of each. */
int controls_key(int b);
uint16_t controls_ctl(int b);
void controls_rebind_key(int b, int key);
void controls_rebind_ctl(int b, uint16_t ctl);
void controls_set(const uint8_t keys[CONTROLS_BUTTONS], const uint16_t ctl[CONTROLS_BUTTONS]);   /* all at once, as saved */
void controls_defaults(void);
int controls_key_reserved(int key);     /* the menu's own keys, and keys that are no key */
int controls_valid(const uint8_t keys[CONTROLS_BUTTONS], const uint16_t ctl[CONTROLS_BUTTONS]);

/* the device pad 1 last used: its names are the tags' */
enum { CONTROLS_KEYBOARD, CONTROLS_CONTROLLER };
int controls_source(void);

/* The menus' own input (the pause menu, the slot grid): physical keys and buttons that never go through the bindings, so
   remapping a game button cannot change how a menu works. The arrows, the d-pad and the left stick move; Enter or the
   controller's Start (NSHVC_CTL_MENU) confirms; Escape or the controller's Back (NSHVC_CTL_BACK) goes back; Page Up and
   Page Down or the controller's LB and RB turn a page. A reader holds what it saw last, so each menu keeps its own. */
typedef struct {
    uint16_t move;                      /* PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT gone down since its last read */
    int confirm, back, prev, next;      /* gone down since its last read */
    int idle;                           /* neither confirm nor back is held by any device, nor went down */
    uint16_t dirs;                      /* what it saw last: the directions held */
    uint16_t ctl;                       /* and the controller's buttons */
} MenuKeys;
void controls_menu_read(const nshvc_input *input, MenuKeys *k);   /* once an update; it also sets controls_source */
/* the menus' hints as the device in use names them (a controller by its own style's labels), upper case */
const char *controls_confirm_name(void);
const char *controls_back_name(void);
const char *controls_prev_name(void);
const char *controls_next_name(void);

#endif
