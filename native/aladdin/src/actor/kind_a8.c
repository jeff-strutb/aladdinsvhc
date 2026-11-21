/*
 * kind_a8.c: actor kind $A8, a figure of the level-end screen that waits
 * for its cue and, when its +$07 is below the count of set gems found,
 * poses again after its own delay with a sound; then it keeps animating.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

enum {
    CUE = ACTOR_RESUME_OWN,     /* each frame until its cue */
    POSE,                       /* after its delay */
    ANIMATE,                    /* each frame, for good */
};

/*
 * @routine 83:FB1A KindA8Start
 * WHAT IT DOES: Actor kind $A8: +$08 bit 1 and +$09 bit 4 set, shown in
 * pose 1; once the screen's cue (g.screen_cue) is set, when its +$07 is
 * below g.gem_sets it waits its delay (by +$07), takes pose 0 and plays
 * sound $28; then it animates for good. Kind $94's second entry runs it
 * (ActorRunAs).
 */
int KindA8Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x10;
        AnimStart(a, 0x01);
        a->resume = CUE;
        return 1;
    case CUE:
        if (!g.screen_cue)
            return 1;
        if (a->spawn_byte < g.gem_sets) {
            a->resume = POSE;
            return k_a8_delay(a->spawn_byte) ? k_a8_delay(a->spawn_byte) : 256;
        }
        a->resume = ANIMATE;
        return 1;
    case POSE:
        AnimStart(a, 0x00);
        SoundQueueAdd(0x28);
        a->resume = ANIMATE;
        return 1;
    case ANIMATE:
        AnimStep(a);
        return 1;
    default:
        return 1;
    }
}
