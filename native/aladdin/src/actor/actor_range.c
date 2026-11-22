/*
 * actor_range.c: actors of the first pool that move out of range of the
 * camera's point (g.range_x, g.range_y) are ended, and their placement may
 * place them again.
 */
#include "actor/actor_own.h"
#include "video/view.h"
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

/*
 * @routine 83:9738 ActorEndIfLeftBehind
 * WHAT IT DOES: The tail some actors' per-frame routines end in: the actor
 * ends once the camera has left it behind (the view's left edge less 32 not
 * negative and at or past its x pixels), else it goes on.
 */
int ActorEndIfLeftBehind(Actor *a)
{
    uint16_t edge = (uint16_t)(pixels(g.layers[0].scroll_x) - 0x20);

    if (!(edge & 0x8000) && edge >= pixels(a->x))
        return ActorEndPlaced(a);
    return ACTOR_GOES_ON;
}

/*
 * @routine 83:C4AB ActorEndFarWithChildren
 * WHAT IT DOES: Ends an actor and its one or two children for good once it
 * is 256 pixels or more from the range point, across or up and down,
 * unless it can be hit (flags bit 6). Its children are held as their index
 * in g.actors in its own $40 and $42, how many in $44 (1 or 2).
 */
int ActorEndFarWithChildren(Actor *a)
{
    if (a->flags & 0x40)
        return ACTOR_GOES_ON;
    if ((uint16_t)(g.range_x - pixels(a->x) + 0x100 + view.dx) < (uint16_t)(0x200 + 2 * view.dx)
        && (uint16_t)(g.range_y - pixels(a->y) + 0x100 + view.dy) < (uint16_t)(0x200 + 2 * view.dy))
        return ACTOR_GOES_ON;
    ActorFreePlaced(&g.actors[a->own.bytes[OWN_40]]);
    if (a->own.bytes[OWN_44] != 1)
        ActorFreePlaced(&g.actors[a->own.bytes[OWN_42]]);
    return ActorEndPlaced(a);
}
