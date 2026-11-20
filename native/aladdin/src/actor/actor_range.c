/*
 * actor_range.c: actors of the first pool that move out of range of the
 * camera's point (g.range_x, g.range_y) are ended, and their placement may
 * place them again.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine 83:962C ActorInRange
 * WHAT IT DOES: Tells whether the actor is in range of the camera's point
 * by box `at`: on each axis the point less its pixels, plus the box's
 * offset, is below the box's span (unsigned). (The original answers C set
 * when out of range.)
 */
bool ActorInRange(const Actor *a, uint8_t at)
{
    ViewBox b = range_box(at);

    if ((uint16_t)(g.range_x - pixels(a->x) + b.x_off) >= b.x_span)
        return false;
    return (uint16_t)(g.range_y - pixels(a->y) + b.y_off) < b.y_span;
}

/*
 * @routine 83:96BE ActorEndOutOfRange
 * WHAT IT DOES: Ends the actor for good when it is out of range of the
 * camera's point by box `at` (ActorEndPlaced: its placement may place it
 * again); otherwise it goes on.
 * Also 83:9659 ActorEndOutOfRangeLong, the same for callers in other
 * banks. Its way out is ActorEndDropReturn's (actor_end.c).
 */
int ActorEndOutOfRange(Actor *a, uint8_t at)
{
    if (ActorInRange(a, at))
        return ACTOR_GOES_ON;
    a->call_at = 0;
    return ActorEndPlaced(a);
}

/*
 * @routine 83:966C ActorFreeOutOfRange
 * WHAT IT DOES: Frees the actor (ActorFreePlaced) when it is out of range
 * of the camera's point by box `at`, and says so; its code goes on either
 * way.
 * Also 83:9668 ActorFreeOutOfRangeLong, for callers in other banks.
 */
bool ActorFreeOutOfRange(Actor *a, uint8_t at)
{
    if (ActorInRange(a, at))
        return false;
    ActorFreePlaced(a);
    return true;
}
