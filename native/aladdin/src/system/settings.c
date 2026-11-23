/*
 * settings.c: the "options" save (settings.h). Bytes: size, version, full screen, then the twelve keys, then the twelve
 * controller buttons as 16-bit little endian. A file of another size or with bindings that are not valid (controls_valid)
 * is left out whole and the defaults stand.
 */
#include <string.h>

#include "input/controls.h"
#include "nshvc/nshvc.h"
#include "system/settings.h"

#define SIZE (3 + CONTROLS_BUTTONS + 2 * CONTROLS_BUTTONS)
#define VERSION 1

static const char NAME[] = "options";
static int full_screen = 1;             /* full screen until the player chooses a window (Jeff, 2026-11-22) */
static int loaded;

void settings_save(void)
{
    uint8_t f[SIZE];
    int b;

    f[0] = SIZE;
    f[1] = VERSION;
    f[2] = (uint8_t)full_screen;
    for (b = 0; b < CONTROLS_BUTTONS; b++) {
        uint16_t c = controls_ctl(b);

        f[3 + b] = (uint8_t)controls_key(b);
        f[3 + CONTROLS_BUTTONS + 2 * b] = (uint8_t)c;
        f[4 + CONTROLS_BUTTONS + 2 * b] = (uint8_t)(c >> 8);
    }
    nshvc_save_write(NAME, f, sizeof f);
}

void settings_load(void)
{
    uint8_t f[SIZE], keys[CONTROLS_BUTTONS];
    uint16_t ctl[CONTROLS_BUTTONS];
    int b;

    loaded = 1;
    if (nshvc_save_read(NAME, f, sizeof f) != (int)sizeof f || f[0] != SIZE || f[1] != VERSION)
        return;
    full_screen = f[2] != 0;
    memcpy(keys, f + 3, sizeof keys);
    for (b = 0; b < CONTROLS_BUTTONS; b++)
        ctl[b] = (uint16_t)(f[3 + CONTROLS_BUTTONS + 2 * b] | f[4 + CONTROLS_BUTTONS + 2 * b] << 8);
    if (controls_valid(keys, ctl))
        controls_set(keys, ctl);
}

int settings_full_screen(void)
{
    if (!loaded)
        settings_load();
    return full_screen;
}

void settings_full_screen_set(int on)
{
    if (!loaded)
        settings_load();
    if (full_screen == (on != 0))
        return;
    full_screen = on != 0;
    settings_save();
}
