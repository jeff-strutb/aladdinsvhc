/*
 * controls.c: pad 1's buttons from the keys and the first controller through the player's bindings (the defaults are the
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

static uint16_t pad_of(const nshvc_input *input, unsigned pad, int mine)
{
    uint16_t now = 0;
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
    if (input->controller[pad].on)
        for (b = 0; b < CONTROLS_BUTTONS; b++)
            if (input->controller[pad].buttons & (pad == 0 ? ctl_of[b] : k_ctl[b])) {
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

uint16_t controls_pressed_keys(const nshvc_input *input)
{
    uint16_t now = 0;
    int b;

    start();
    for (b = 0; b < CONTROLS_BUTTONS; b++)
        if (NSHVC_KEY_PRESSED(input, key_of[b]))
            now |= controls_buttons[b];
    return now;
}

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
