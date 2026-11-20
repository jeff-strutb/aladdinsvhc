/*
 * kind90.c: actor kind $90, a flier a guard sends to try a path: it flies
 * by the preset its slot (its +$07) names, in steps of several flights each
 * made within one frame, and reports in its slot of g.flier_slots how it
 * ended: $80 when it came down onto ground, $40 when it was moved out of a
 * wall whose tile actors do not pass through, 0 when its steps ran out.
 *
 * Its second entry in the original's table (A5:F533, LongReturn) is an RTL
 * the loop's jump would return through into its own saved D: no code sets
 * a kind $90's state past $7F, and it has none. Its wait routine
 * (ActorKind90Wait, 83:9366, a JSL to that RTL and an RTS) does nothing:
 * its row names none.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "map/map.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t flights;            /* +$40: flights left in this step */
    uint8_t steps;              /* +$41: steps left */
} Kind90Own;

_Static_assert(sizeof(Kind90Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind90Own *own(Actor *a) { return (Kind90Own *)a->own.bytes; }

enum {
    STEP = ACTOR_RESUME_OWN,    /* after a step: the next */
};

static int report(Actor *a, uint8_t how)
{
    g.flier_slots[a->spawn_byte] = how;
    return ActorEndPlaced(a);
}

/* A step: its flights, all in this frame, while its slot stays live; then
   the wall and the ground tested. */
static int step(Actor *a)
{
    MapPoint wall;

    own(a)->flights = k90_flights(a->spawn_byte);
    if (!(g.flier_slots[a->spawn_byte] & 0x0F))
        return ActorEndPlaced(a);
    do
        ActorFly(a);
    while (--own(a)->flights);
    if (ActorOutOfWallProbe4At(a, &wall) && !(wall.behaviour & TILE_THROUGH))
        return report(a, 0x40);
    if (ActorOutOfGround(a))
        return report(a, 0x80);
    a->resume = STEP;
    return 1;
}

/*
 * @routine A5:F4D2 Kind90Start
 * WHAT IT DOES: Actor kind $90: flies by the preset its +$07 names, in
 * steps of several flights each made within one frame, while its slot in
 * g.flier_slots stays live; it ends, reporting in the slot $80 when it is
 * in the ground, $40 when it is moved out of a wall whose tile has no
 * TILE_THROUGH (which no script meets), 0 when its steps run out.
 */
static int start(Actor *a)
{
    a->flags |= 0x80;
    a->probes = k90_probes();
    own(a)->steps = k90_steps(a->spawn_byte);
    ActorMotionPreset(a, k90_preset(a->spawn_byte));
    return step(a);
}

int Kind90Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case STEP:
        if (!--own(a)->steps)
            return report(a, 0x00);
        return step(a);
    default:
        return kind_lost(a, "Kind90Code");
    }
}
