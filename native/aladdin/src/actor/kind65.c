/*
 * kind65.c: actor kind $65, the continues counter shown in play: its
 * picture is the count of continues left, changed as the count changes;
 * with none left as it starts, it ends. (Kinds $61, $64 and $6C go on here
 * in their second state: those entries are left aside for now, see
 * kinds_e.c.) Its wait, 83:E5EC Kind65Wait (only AnimStep), is the core's
 * AnimateWhileWaiting, which its row names.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"

#define FIRST_PICTURE 0x07      /* the animation for 0 continues; the count is added */

enum {
    WATCH = ACTOR_RESUME_OWN,   /* each frame until the count changes */
};

/* its own bytes: +$40, the count it shows */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t shown;              /* +$40 */
} Kind65Own;

static inline Kind65Own *own(Actor *a) { return (Kind65Own *)a->own.bytes; }

static int show(Actor *a)
{
    own(a)->shown = g.continues;
    AnimStart(a, (uint8_t)(g.continues + FIRST_PICTURE));
    a->resume = WATCH;
    return 1;
}

/*
 * @routine 83:E5BB Kind65Start
 * WHAT IT DOES: Actor kind $65: +$08 bit 1 and its wait routine on, its
 * look $0600, facing right; with no continues left it ends; else it shows
 * the continues left as its animation (7 plus the count), and each frame
 * from the next shows it again when the count differs.
 */
int Kind65Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->look = 0x0600;
        a->facing = 0;
        a->flip = 0;
        if (!g.continues)
            return ActorEndPlaced(a);
        return show(a);
    case WATCH:
        if (g.continues == own(a)->shown)
            return 1;
        return show(a);
    default:
        return 1;
    }
}
