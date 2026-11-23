/*
 * dev_level.c: ALADDIN_DEV_LEVEL=PART-STAGE (a developer's setting, e.g. 0-3 for level 3): the game's own level select is
 * played by keys from power-on, the same keys at the same updates as native/checks/plays/level_select_input.py PART STAGE
 * (Start at 2400, Options, the cheat code on pad 2, Exit, R held with Up PART times and Right STAGE times, Start at 3900),
 * and the person's keys and controllers take over after the last of them (update 3903); the level plays from about update
 * 4000. Until then the person's input is not used. The keys go through ALADDIN_DEV_RECORD like any other, so a recording
 * made with this set replays from power-on with --input alone.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/system.h"

#define LINES_MAX 64

typedef struct {
    uint32_t at;
    uint8_t key;                /* 0: none */
    uint8_t key2;               /* 0: none (R held with Up or Right) */
    uint8_t pad1;               /* controller 1's buttons; the line names controller 1 when pad1_on */
    uint8_t pad1_on;
} Line;

static Line lines[LINES_MAX];
static int count, next;
static uint32_t held[8];
static Line now;

static void add(uint32_t at, uint8_t key, uint8_t key2, uint8_t pad1, uint8_t pad1_on)
{
    lines[count].at = at;
    lines[count].key = key;
    lines[count].key2 = key2;
    lines[count].pad1 = pad1;
    lines[count].pad1_on = pad1_on;
    count++;
}

static void press(uint32_t at, uint8_t key)
{
    add(at, key, 0, 0, 0);
    add(at + 3, 0, 0, 0, 0);
}

/* the lines level_select_input.py writes, in its order */
static void build(unsigned part, unsigned stage)
{
    static const uint8_t code[8] = { 0x10, 0x20, 0x80, 0x40, 0x08, 0x04, 0x02, 0x01 };
    uint32_t t;
    unsigned i;

    add(2400, 0x0D, 0, 0, 0);
    add(2402, 0, 0, 0, 0);
    press(2700, 0x28);
    press(2715, 0x28);
    press(2730, 0x0D);
    for (t = 3100, i = 0; i < 8; i++, t += 10) {
        add(t, 0, 0, code[i], 1);
        add(t + 3, 0, 0, 0, 1);
    }
    press(3300, 0x28);
    press(3310, 0x28);
    press(3320, 0x28);
    press(3340, 0x58);
    add(3700, 0x57, 0, 0, 0);
    for (t = 3720, i = 0; i < part + stage; i++, t += 10) {
        add(t, 0x57, i < part ? 0x26 : 0x27, 0, 0);
        add(t + 3, 0x57, 0, 0, 0);
    }
    add(t + 5, 0, 0, 0, 0);
    press(3900, 0x0D);
}

static void set_key(nshvc_input *in, uint8_t k)
{
    if (k)
        in->keys[k >> 5] |= 1u << (k & 31);
}

bool DevLevelInput(uint64_t u, nshvc_input *in)
{
    static int known, on;
    int k;

    if (!known) {
        const char *v = getenv("ALADDIN_DEV_LEVEL");
        unsigned part, stage;

        known = 1;
        if (v && sscanf(v, "%u-%u", &part, &stage) == 2 && part < 8 && stage < 8) {
            build(part, stage);
            on = 1;
        } else if (v && *v) {
            fprintf(stderr, "ALADDIN_DEV_LEVEL=%s: give PART-STAGE, e.g. 0-3\n", v);
        }
    }
    if (!on || u > lines[count - 1].at)
        return false;
    /* the devices held from each line's update on, as --input holds them */
    while (next < count && lines[next].at <= u)
        now = lines[next++];
    memset(in, 0, sizeof *in);
    set_key(in, now.key);
    set_key(in, now.key2);
    if (now.pad1_on) {
        in->controller[1].on = 1;
        in->controller[1].buttons = now.pad1;
    }
    for (k = 0; k < 8; k++) {
        in->keys_pressed[k] = in->keys[k] & ~held[k];
        in->keys_released[k] = held[k] & ~in->keys[k];
        held[k] = in->keys[k];
    }
    return true;
}
