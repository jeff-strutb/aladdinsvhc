/*
 * controls.c: pad 1's buttons from the keys and every controller through the player's bindings (the defaults are the
 * game's own, as they were before the menu could change them); pad 2's from the engine's defaults.
 */
#include <string.h>

#include "input/controls.h"
#include "system/system.h"
#include "system/runahead.h"

const uint16_t controls_buttons[CONTROLS_BUTTONS] = {
    PAD_B, PAD_Y, PAD_SELECT, PAD_START, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_A, PAD_X, PAD_L, PAD_R
};
static const uint8_t k_keys[CONTROLS_BUTTONS] = { 'Z', 'A', 0x10, 0x0D, 0x26, 0x28, 0x25, 0x27, 'X', 'S', 'Q', 'W' };
static const uint16_t k_ctl[CONTROLS_BUTTONS] = {
    NSHVC_CTL_SOUTH, NSHVC_CTL_WEST, NSHVC_CTL_BACK, NSHVC_CTL_MENU, NSHVC_CTL_UP, NSHVC_CTL_DOWN, NSHVC_CTL_LEFT,
    NSHVC_CTL_RIGHT, NSHVC_CTL_EAST, NSHVC_CTL_NORTH, NSHVC_CTL_LB, NSHVC_CTL_RB
};
#define KEY_RIGHT_SHIFT 0xA1            /* also Select, until the player gives that key to another button */

static uint8_t key_of[CONTROLS_BUTTONS];
static uint16_t ctl_of[CONTROLS_BUTTONS];
static int ready;
static int source;
RUNAHEAD_KEEP(source)
static uint16_t swallowed;
RUNAHEAD_KEEP(swallowed)

static void start(void)
{
    if (!ready) {
        memcpy(key_of, k_keys, sizeof key_of);
        memcpy(ctl_of, k_ctl, sizeof ctl_of);
        ready = 1;
    }
}

/* ---- the controllers as devices ----
 *
 * Every connected controller plays pad 1 and works the menus, not the engine's first alone: a host lists devices in its
 * own order (a Steam Deck's own controls, Steam Input's or DS4Windows' virtual pad, a wheel or a headset can come first),
 * so a pad that is not controller 0 was ignored everywhere. What a controller holds as it appears is its rest, not a
 * press (a virtual pad's state, a drifting stick), and it counts for nothing until let go: else one such device held a
 * button down for good. A device is known by its index and seq (nshvc.h, 6.11): seq changes when another device takes
 * the index. seq 0 is a device the engine did not see connect (an --input replay, the developer's inputs): no rest.
 * controls_sample keeps this, once an update on the real input, outside the steps: it is the devices', not the world's. */

enum { CONTROLLERS = 8 };
enum { STICK_UP = 1u << 16, STICK_DOWN = 1u << 17, STICK_LEFT = 1u << 18, STICK_RIGHT = 1u << 19 };
#define STICK_ON 0.5f                   /* a direction comes on with the left stick half way over */
#define STICK_OFF 0.35f                 /* and goes off a third of the way back, so a stick near half way does not flicker */

static uint8_t ctl_here[CONTROLLERS];   /* connected at the last sample */
static uint32_t ctl_seq[CONTROLLERS];   /* its seq then */
static uint32_t ctl_was[CONTROLLERS];   /* its buttons and stick directions then */
static uint32_t ctl_rest[CONTROLLERS];  /* what it held as it appeared, until each is let go */
static uint32_t ctl_stick[CONTROLLERS]; /* its left stick as STICK_* */
static int ctl_in_use = -1;             /* the controller pressed last: its names are the tags' */

static uint32_t stick_of(const nshvc_input *in, int c)
{
    uint32_t was = ctl_stick[c], s = 0;
    float x = in->controller[c].lx, y = in->controller[c].ly;

    if (y < (was & STICK_UP ? -STICK_OFF : -STICK_ON)) s |= STICK_UP;
    if (y > (was & STICK_DOWN ? STICK_OFF : STICK_ON)) s |= STICK_DOWN;
    if (x < (was & STICK_LEFT ? -STICK_OFF : -STICK_ON)) s |= STICK_LEFT;
    if (x > (was & STICK_RIGHT ? STICK_OFF : STICK_ON)) s |= STICK_RIGHT;
    return s;
}

void controls_sample(const nshvc_input *in)
{
    int c;

    for (c = 0; c < CONTROLLERS; c++) {
        uint32_t now, fresh;

        if (!in->controller[c].on) {
            ctl_here[c] = 0;
            ctl_stick[c] = 0;
            if (ctl_in_use == c)
                ctl_in_use = -1;
            continue;
        }
        if (ctl_here[c] && in->controller[c].seq != ctl_seq[c]) {
            ctl_here[c] = 0;            /* another device at this index: first seen again */
            ctl_stick[c] = 0;
            if (ctl_in_use == c)
                ctl_in_use = -1;
        }
        ctl_seq[c] = in->controller[c].seq;
        ctl_stick[c] = stick_of(in, c);
        now = in->controller[c].buttons | ctl_stick[c];
        if (!ctl_here[c]) {
            ctl_here[c] = 1;
            ctl_was[c] = in->controller[c].seq ? now : 0;
            ctl_rest[c] = ctl_was[c];
        }
        fresh = now & ~ctl_was[c];
        ctl_rest[c] &= now;
        ctl_was[c] = now;
        if (fresh)
            ctl_in_use = c;
    }
}

/* controller c's buttons and stick directions now, less its rest; 0 when it is not connected. The rest applies only to
   the device it was taken from (the same index and seq), so the developer's own inputs are read whole. */
static uint32_t ctl_state(const nshvc_input *in, int c)
{
    uint32_t now, rest = 0;

    if (!in->controller[c].on)
        return 0;
    now = in->controller[c].buttons;
    if (ctl_here[c] && in->controller[c].seq == ctl_seq[c]) {
        now |= ctl_stick[c];
        rest = ctl_rest[c];
    }
    return now & ~rest;
}

uint16_t controls_ctl_buttons(const nshvc_input *in)
{
    uint32_t all = 0;
    int c;

    for (c = 0; c < CONTROLLERS; c++)
        all |= ctl_state(in, c);
    return (uint16_t)all;
}

int controls_controller(void)
{
    int c;

    if (ctl_in_use >= 0 && ctl_here[ctl_in_use])
        return ctl_in_use;
    for (c = 0; c < CONTROLLERS; c++)
        if (ctl_here[c])
            return c;
    return 0;
}

static uint16_t pad_of(const nshvc_input *input, unsigned pad, int mine)
{
    uint16_t now = 0, ctl;
    int b, alias = 1;

    if (!input)
        return 0;
    start();
    if (pad == 0) {
        for (b = 0; b < CONTROLS_BUTTONS; b++) {
            if (key_of[b] == KEY_RIGHT_SHIFT)
                alias = 0;
            if (NSHVC_KEY_HELD(input, key_of[b])) {
                now |= controls_buttons[b];
                if (mine)
                    source = CONTROLS_KEYBOARD;
            }
        }
        if (alias && NSHVC_KEY_HELD(input, KEY_RIGHT_SHIFT))
            now |= PAD_SELECT;
    }
    /* pad 1: every controller through the bindings; pad 2 (which nothing reads): the second, as the engine's defaults */
    if (pad == 0) {
        uint32_t all = 0;

        for (b = 0; b < CONTROLLERS; b++)
            all |= ctl_state(input, b);
        /* the left stick is the d-pad: each direction as the d-pad's button, so it follows the d-pad's bindings */
        if (all & STICK_UP) all |= NSHVC_CTL_UP;
        if (all & STICK_DOWN) all |= NSHVC_CTL_DOWN;
        if (all & STICK_LEFT) all |= NSHVC_CTL_LEFT;
        if (all & STICK_RIGHT) all |= NSHVC_CTL_RIGHT;
        ctl = (uint16_t)all;
    } else {
        ctl = (uint16_t)ctl_state(input, 1);
    }
    if (ctl)
        for (b = 0; b < CONTROLS_BUTTONS; b++)
            if (ctl & (pad == 0 ? ctl_of[b] : k_ctl[b])) {
                now |= controls_buttons[b];
                if (mine && pad == 0)
                    source = CONTROLS_CONTROLLER;
            }
    return now;
}

uint16_t controls_pad_raw(const nshvc_input *input) { return pad_of(input, 0, 1); }

uint16_t controls_pad(const nshvc_input *input, unsigned pad)
{
    uint16_t now = pad_of(input, pad, 1);

    if (pad == 0) {
        swallowed &= now;
        now &= (uint16_t)~swallowed;
    }
    return now;
}

void controls_swallow(uint16_t held) { swallowed = held; }

int controls_key(int b) { start(); return key_of[b]; }
uint16_t controls_ctl(int b) { start(); return ctl_of[b]; }

void controls_rebind_key(int b, int key)
{
    int j;

    start();
    if (key_of[b] == key)
        return;
    for (j = 0; j < CONTROLS_BUTTONS; j++)
        if (j != b && key_of[j] == key) {
            key_of[j] = key_of[b];
            break;
        }
    key_of[b] = (uint8_t)key;
}

void controls_rebind_ctl(int b, uint16_t ctl)
{
    int j;

    start();
    if (ctl_of[b] == ctl)
        return;
    for (j = 0; j < CONTROLS_BUTTONS; j++)
        if (j != b && ctl_of[j] == ctl) {
            ctl_of[j] = ctl_of[b];
            break;
        }
    ctl_of[b] = ctl;
}

int controls_key_reserved(int key) { return key < 0x08 || key > 0xFE || key == 0x1B || key == 0x09 || key == 0x7A || key == 0x7B; }

int controls_valid(const uint8_t keys[CONTROLS_BUTTONS], const uint16_t ctl[CONTROLS_BUTTONS])
{
    int i, j;

    for (i = 0; i < CONTROLS_BUTTONS; i++) {
        if (controls_key_reserved(keys[i]) || !ctl[i] || (ctl[i] & (ctl[i] - 1)))
            return 0;
        for (j = 0; j < i; j++)
            if (keys[j] == keys[i] || ctl[j] == ctl[i])
                return 0;
    }
    return 1;
}

void controls_set(const uint8_t keys[CONTROLS_BUTTONS], const uint16_t ctl[CONTROLS_BUTTONS])
{
    memcpy(key_of, keys, sizeof key_of);
    memcpy(ctl_of, ctl, sizeof ctl_of);
    ready = 1;
}

void controls_defaults(void)
{
    ready = 0;
    start();
}

int controls_source(void) { return source; }

/* ---- the menus' own input ---- */

#define KEY_ENTER 0x0D
#define KEY_ESCAPE 0x1B
#define KEY_PAGE_UP 0x21
#define KEY_PAGE_DOWN 0x22

static char name_confirm[12] = "START", name_back[12] = "SELECT", name_prev[12] = "L", name_next[12] = "R";

/* a controller's button as its style names it, upper case; "+" and "-" are no letters of the menu's font */
static void ctl_name(char *out, uint16_t ctl, const char *generic)
{
    const char *s = nshvc_controller_label((uint32_t)controls_controller(), ctl);
    size_t n;

    if (s && !strcmp(s, "+"))
        s = "PLUS";
    else if (s && !strcmp(s, "-"))
        s = "MINUS";
    if (!s || !*s)
        s = generic;
    for (n = 0; n < 11 && s[n]; n++)
        out[n] = s[n] >= 'a' && s[n] <= 'z' ? (char)(s[n] - 32) : s[n];
    out[n] = 0;
}

void controls_menu_read(const nshvc_input *in, MenuKeys *k)
{
    uint16_t dirs = 0, ctl = 0, ctl_down, dirs_down;
    uint32_t state = 0;
    int key_any = 0, i;

    for (i = 0; i < 8; i++)
        if (in->keys_pressed[i])
            key_any = 1;
    for (i = 0; i < CONTROLLERS; i++)
        state |= ctl_state(in, i);
    if (state) {
        ctl = (uint16_t)state;
        if ((ctl & NSHVC_CTL_UP) || (state & STICK_UP)) dirs |= PAD_UP;
        if ((ctl & NSHVC_CTL_DOWN) || (state & STICK_DOWN)) dirs |= PAD_DOWN;
        if ((ctl & NSHVC_CTL_LEFT) || (state & STICK_LEFT)) dirs |= PAD_LEFT;
        if ((ctl & NSHVC_CTL_RIGHT) || (state & STICK_RIGHT)) dirs |= PAD_RIGHT;
    }
    {
        ctl_name(name_confirm, NSHVC_CTL_MENU, "START");
        ctl_name(name_back, NSHVC_CTL_BACK, "SELECT");
        ctl_name(name_prev, NSHVC_CTL_LB, "L");
        ctl_name(name_next, NSHVC_CTL_RB, "R");
    }
    ctl_down = (uint16_t)(ctl & ~k->ctl);
    dirs_down = (uint16_t)(dirs & ~k->dirs);
    k->move = dirs_down;
    if (NSHVC_KEY_PRESSED(in, 0x26)) k->move |= PAD_UP;
    if (NSHVC_KEY_PRESSED(in, 0x28)) k->move |= PAD_DOWN;
    if (NSHVC_KEY_PRESSED(in, 0x25)) k->move |= PAD_LEFT;
    if (NSHVC_KEY_PRESSED(in, 0x27)) k->move |= PAD_RIGHT;
    k->confirm = NSHVC_KEY_PRESSED(in, KEY_ENTER) != 0 || (ctl_down & NSHVC_CTL_MENU) != 0;
    k->back = NSHVC_KEY_PRESSED(in, KEY_ESCAPE) != 0 || (ctl_down & NSHVC_CTL_BACK) != 0;
    k->prev = NSHVC_KEY_PRESSED(in, KEY_PAGE_UP) != 0 || (ctl_down & NSHVC_CTL_LB) != 0;
    k->next = NSHVC_KEY_PRESSED(in, KEY_PAGE_DOWN) != 0 || (ctl_down & NSHVC_CTL_RB) != 0;
    k->idle = !k->confirm && !k->back && !NSHVC_KEY_HELD(in, KEY_ENTER) && !NSHVC_KEY_HELD(in, KEY_ESCAPE) &&
              !(ctl & (NSHVC_CTL_MENU | NSHVC_CTL_BACK));
    k->dirs = dirs;
    k->ctl = ctl;
    /* the device last used, whatever its keys and buttons are bound to */
    if (key_any)
        source = CONTROLS_KEYBOARD;
    else if (ctl_down || dirs_down)
        source = CONTROLS_CONTROLLER;
}

const char *controls_confirm_name(void) { return source == CONTROLS_CONTROLLER ? name_confirm : "ENTER"; }
const char *controls_back_name(void) { return source == CONTROLS_CONTROLLER ? name_back : "ESC"; }
const char *controls_prev_name(void) { return source == CONTROLS_CONTROLLER ? name_prev : "PGUP"; }
const char *controls_next_name(void) { return source == CONTROLS_CONTROLLER ? name_next : "PGDN"; }
