/*
 * kinda7.c: actor kind $A7, a figure of the ending: it waits (for a cue,
 * by its $07), is placed, fades its palette up out of black with colour
 * math, and then stays, or (with $07 set) rises 64 pixels and stays.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"
#include "video/slot_gfx.h"
#include "video/video.h"

#define FADE OWN_40             /* the fade's step: -$20 up to 8 */
#define TOP OWN_42              /* a $07 figure: the y it rises to (word) */

enum {
    CUE = ACTOR_RESUME_OWN,     /* a $07 figure: until the boss is beaten (g.boss_beaten) */
    CUE_DONE,                   /* after 96 frames */
    WAIT_2,                     /* $07 0: the second and third waits of 255 */
    WAIT_3,
    FADE_UP,                    /* the fade's steps */
    STAY_FIRST,                 /* $07 0: after 255 frames */
    STAY_SECOND,                /* after 192 frames: for good */
    RISE_FIRST,                 /* $07 set: after 255 frames */
    RISE_SECOND,                /* after 152 frames */
    RISE                        /* each frame up half a pixel until it reaches its height */
};

/* Its place and the fade's start (after its waits). */
static int place_and_fade(Actor *a)
{
    uint8_t v07 = a->spawn_byte;

    AnimStart(a, 0);
    if (v07) {
        a->x = with_pixels(a->x, (uint16_t)(pixels(g.layers[0].scroll_x) + ka7_place_x(v07)));
        a->y = with_pixels(a->y, (uint16_t)(pixels(g.layers[0].scroll_y) + ka7_place_y(v07)));
    } else {
        a->x = with_pixels(a->x, ka7_place_x(0));
        a->y = with_pixels(a->y, ka7_place_y(0));
    }
    a->flags = 0x20;
    PaletteSave((uint16_t)look_palette_number(a->look), 1);
    a->own.bytes[FADE] = 0xE0;
    PaletteFadeStep((int8_t)0xE0, 1);
    g.blend_use_sub = 1;
    g.blend_layers = 0x12;
    g.blend_half = 0;
    g.blend_subtract = 0;
    g.sub_layers = 0x10;
    a->resume = FADE_UP;
    return v07 ? 4 : 6;
}

/*
 * @routine 84:AC66 KindA7Start
 * WHAT IT DOES: Actor kind $A7, a figure of the ending. $07 0: three waits
 * of 255 frames; $07 set: g.boss_beaten cleared, then awaited, and 96
 * frames. Then pose 0 at its place ($80:D99E by $07, 4 bytes: x, y; for
 * $07 set from the play field's scroll), its sprite palette faded up from
 * black in steps of 6 frames (4 for $07 set) under colour math; then $07 0
 * waits 255 and 192 frames and stays; $07 set waits 255 and 152 frames,
 * rises 64 pixels half a pixel a frame, and stays.
 */
int KindA7Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        if (a->spawn_byte) {
            g.boss_beaten = 0;
            a->resume = CUE;
            return 1;
        }
        a->resume = WAIT_2;
        return 0xFF;
    case CUE:
        if (!g.boss_beaten)
            return 1;
        a->resume = CUE_DONE;
        return 0x60;
    case CUE_DONE:
        return place_and_fade(a);
    case WAIT_2:
        a->resume = WAIT_3;
        return 0xFF;
    case WAIT_3:
        a->resume = CUE_DONE;
        return 0xFF;
    case FADE_UP: {
        uint8_t v07 = a->spawn_byte;

        a->own.bytes[FADE]++;
        PaletteFadeStep((int8_t)a->own.bytes[FADE], 1);
        if (a->own.bytes[FADE] != 0x08)
            return v07 ? 4 : 6;
        g.blend_prevent = 3;
        g.blend_use_sub = 0;
        g.blend_layers = 0;
        a->flags &= 0xCF;
        a->resume = v07 ? RISE_FIRST : STAY_FIRST;
        return 0xFF;
    }
    case STAY_FIRST:
        a->resume = STAY_SECOND;
        return 0xC0;
    case RISE_FIRST:
        a->resume = RISE_SECOND;
        return 0x98;
    case RISE_SECOND:
        a->own.bytes[TOP] = (uint8_t)(pixels(a->y) - 0x40);
        a->own.bytes[TOP + 1] = (uint8_t)((uint16_t)(pixels(a->y) - 0x40) >> 8);
        a->resume = RISE;
        return 1;
    case RISE:
        a->y = wrap24(a->y - 0x80);
        if (pixels(a->y) >= (uint16_t)(a->own.bytes[TOP] | a->own.bytes[TOP + 1] << 8))
            return 1;
        a->resume = STAY_SECOND;
        return 1;
    default:                            /* STAY_SECOND: for good */
        return 1;
    }
}
