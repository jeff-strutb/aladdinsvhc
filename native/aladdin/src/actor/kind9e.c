/*
 * kind9e.c: actor kind $9E, the figures of the Genie's screen (level $2F)
 * that take their animation from the screen's state; and their wait.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "video/video.h"

enum {
    WAY0_WAIT = ACTOR_RESUME_OWN,   /* way 0: each frame, by the screen's state */
    WAY2_CUE,                       /* way 2: each frame until the screen's end */
    WAY2_FOLLOW,                    /* each frame following the screen's y */
    STAY,
};

/* way 0's test of the screen's end (84:A836): a frame first only when it
   has not come; then the screen's pick */
static int way0(Actor *a)
{
    int8_t step = (int8_t)g.genie_screen_step;

    if (step < 0 || step > 1)
        AnimStart(a, 0x0A);
    else if (step == 1)
        AnimStart(a, 0x08);
    a->resume = WAY0_WAIT;
    return 1;
}

/* a pick taken: its animation (1 when g.genie_pick_one is set), then the
   end test again in the same frame */
static int pick(Actor *a, uint8_t anim)
{
    AnimStart(a, g.genie_pick_one ? 0x01 : anim);
    return way0(a);
}

/*
 * @routine 84:A820 Kind9EStart
 * WHAT IT DOES: Actor kind $9E of the Genie's screen (level $2F), by its
 * +$07 (+$08 bits 0-1 and its wait routine on). Way 0: pose 0, then each
 * frame by the screen's state: its step (g.genie_screen_step) 1 starts
 * pose 8, another nonzero pose $0A; a pick in g.genie_pick (cleared once
 * taken) starts pose 3 (1), 4 (2-3) or 5 (4 up) (1 if g.genie_pick_one is
 * set), or, negative, 7 or 9 by g.genie_pose ($FF) or 0 (other). Way 2:
 * its palette from the ROM ($52), $100 pixels up; once the step is
 * nonzero, pose $0B ($0C when negative); then each frame its y =
 * g.genie_screen_y + $10 and its x $34.
 */
int Kind9EStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x03;
        a->flags |= 0x90;
        if (a->spawn_byte == 0) {
            AnimStart(a, 0x00);
            return way0(a);
        }
        if (a->spawn_byte == 2) {
            ActorPaletteFromRom(a, 0x52);
            a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 0x100));
            a->resume = WAY2_CUE;
            return 1;
        }
        {
            static int told;

            /* past its table of two ways */
            if (!told++)
                fprintf(stderr, "Kind9EStart: way %u (past its table) not converted\n", a->spawn_byte);
            a->resume = STAY;
            return 1;
        }
    case WAY0_WAIT: {
        uint8_t p = g.genie_pick;

        if (!p)
            return way0(a);
        if (p & 0x80) {
            AnimStart(a, p == 0xFF ? (g.genie_pose ? 0x09 : 0x07) : 0x00);
            g.genie_pick = 0;
            return way0(a);
        }
        return pick(a, p < 0x02 ? 0x03 : p < 0x04 ? 0x04 : 0x05);
    }
    case WAY2_CUE:
        if (!g.genie_screen_step)
            return 1;
        AnimStart(a, (g.genie_screen_step & 0x80) ? 0x0C : 0x0B);
        a->resume = WAY2_FOLLOW;
        return 1;
    case WAY2_FOLLOW:
        a->y = with_pixels(a->y, (uint16_t)(g.genie_screen_y + 0x10));
        a->x = with_pixels(a->x, 0x0034);
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 84:A8BD Kind9EWaitWork
 * WHAT IT DOES: Kind $9E's wait (through its stub 83:93FB): its animation
 * steps, and it is drawn in front (flags bits 4-5) on the pictures 7, 8, 9,
 * $0C and $0D, behind (bit 5 cleared) on the others.
 */
int Kind9EWaitWork(Actor *a)
{
    AnimStep(a);
    a->flags &= (uint8_t)~0x20;
    switch (a->picture) {
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0C:
    case 0x0D:
        a->flags |= 0x30;
        break;
    default:
        break;
    }
    return ACTOR_GOES_ON;
}
