/*
 * kind48_5a.c: two still actors. Kind $48 (placed in no level, spawned by
 * no code) shows pose 0 for good; in its second state it ends. Kind $5A
 * (the Genie's screen, level $2F) shows the pose the scene asks for in
 * g.genie_screen_step. Their waits, Kind48Wait (83:CE25, only AnimStep:
 * the core's AnimateWhileWaiting) and Kind5AWait (83:DEF3), follow.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "video/video.h"

enum {
    STAY = ACTOR_RESUME_OWN,
    SHOW,                       /* $5A: each frame */
};

/*
 * @routine 83:CE14 Kind48Start
 * WHAT IT DOES: Actor kind $48: its wait routine on only (flags $80), pose
 * 0, still, for good (its wait animates it). Placed in no level and
 * spawned by no code. Its second entry Kind48Second (83:CE22) ends it.
 */
int Kind48Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags = 0x80;
        AnimStart(a, 0x00);
        a->resume = STAY;
        return 1;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:DEBE Kind5AStart
 * WHAT IT DOES: Actor kind $5A of the Genie's screen: +$08 bit 1, its wait
 * routine on and its priority bits 4-5 and 7 ($B0), its picture marked to
 * be sent, its picture list, pose 0; then each frame the pose its scene
 * asks for in g.genie_screen_step (0: as it is, 1: pose 1, 2 up: pose 2).
 */
int Kind5AStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0xB0;
        a->redraw = 0xFF;
        a->own_pictures = k5a_pictures();
        AnimStart(a, 0x00);
        a->resume = SHOW;
        return 1;
    case SHOW:
        if (g.genie_screen_step)
            AnimStart(a, g.genie_screen_step == 1 ? 0x01 : 0x02);
        return 1;
    default:
        return 1;
    }
}

/*
 * @routine 83:DEF3 Kind5AWait
 * WHAT IT DOES: Kind $5A's wait: its animation and picture; its priority
 * bit 5 cleared, or bits 4-5 set (priority 3) when its picture is 0, 7, 8
 * or 9.
 */
int Kind5AWait(Actor *a)
{
    AnimStep(a);
    ActorPictureSend(a);
    a->flags &= (uint8_t)~0x20;
    if (a->picture == 0 || a->picture == 7 || a->picture == 8 || a->picture == 9)
        a->flags |= 0x30;
    return ACTOR_GOES_ON;
}
