/*
 * kind4c.c: actor kind $4C, the five marks at the end of levels $12 and $1C:
 * each shows its animation when the gem sets found reach its number, then
 * goes; with $07 bit 4 set (no placement has it) it waits, sets the level
 * 0 and shows its animation for ever. It has no wait routine.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"

#define COUNT OWN_40            /* frames of its animation left */
#define WAIT_LO OWN_41          /* its long wait's two counters */
#define WAIT_HI OWN_42
#define FRAMES 0x88             /* frames its animation shows */
#define SOUND_AT 0x50           /* the count at which its sound plays */
#define SOUND 0x4C
#define PALETTE_ROW 0x5A        /* ActorPaletteFromRom's row for it */

enum {
    SHOW = ACTOR_RESUME_OWN,    /* each frame of its animation: then it goes */
    WAITING,                    /* its long wait ($07 bit 4) */
    SHOW_AGAIN,                 /* each frame of the animation after it */
    AGAIN                       /* after the 32-frame wait */
};

/* The top of its show loop: the sound at the count, then a frame's wait. */
static int show_top(Actor *a, uint16_t next)
{
    if (a->own.bytes[COUNT] == SOUND_AT)
        SoundQueueAdd(SOUND);
    a->resume = next;
    return 1;
}

/*
 * @routine A5:F43D Kind4CStart
 * WHAT IT DOES: Actor kind $4C, one of the five placed at the end of levels
 * $12 and $1C ($07 0-4 numbers them). Its anim_flags bit 1 and flags bit 4
 * set; with $07 bit 0 clear its palette 2 higher, and with the low four bits
 * clear the palette of row $5A from the ROM. $07 bit 4 clear: it ends at
 * once unless the gem sets found less 5 are at least its $07; else animation
 * 0 for 136 frames, sound $4C at the 50th from the last, then it ends. $07
 * bit 4 set: it waits $A0 and $8C counts, sets the level's low byte 0, and
 * shows animation 0 again and again with a 32-frame wait between.
 */
int Kind4CStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        if (!(a->spawn_byte & 0x01)) {
            a->look = (uint16_t)((a->look & 0x00FF) | ((((a->look >> 8) + 2) & 0xFF) << 8));
            if (!(a->spawn_byte & 0x0F))
                ActorPaletteFromRom(a, PALETTE_ROW);
        }
        if (a->spawn_byte & 0x10) {
            a->own.bytes[WAIT_LO] = 0xA0;
            a->own.bytes[WAIT_HI] = 0x8C;
            a->resume = WAITING;
            return 1;
        }
        a->own.bytes[COUNT] = FRAMES;
        if (g.gem_sets < 5 || (uint8_t)(g.gem_sets - 5) < a->spawn_byte)
            return ActorEndPlaced(a);
        AnimStart(a, 0);
        return show_top(a, SHOW);
    case SHOW:
        AnimStep(a);
        if (--a->own.bytes[COUNT])
            return show_top(a, SHOW);
        return ActorEndPlaced(a);
    case WAITING:
        if (--a->own.bytes[WAIT_LO])
            return 1;
        if (--a->own.bytes[WAIT_HI])
            return 1;
        g.level &= 0xFF00;
        /* fall through */
    case AGAIN:
        a->own.bytes[COUNT] = FRAMES;
        AnimStart(a, 0);
        return show_top(a, SHOW_AGAIN);
    case SHOW_AGAIN:
        AnimStep(a);
        if (--a->own.bytes[COUNT])
            return show_top(a, SHOW_AGAIN);
        a->resume = AGAIN;
        return 0x20;
    default:
        return 1;
    }
}
