/*
 * actor_end.c: an actor's end: its record cleared and given back, its
 * palette slot let go. A kind's code that ends its actor calls one of these
 * and returns ACTOR_ENDED to the loop.
 */
#include <stdio.h>

#include "actor/actor_own.h"

/* A record pushed on a full free stack: the original writes past the
   stack's end (into the next stack's words), which only a record freed
   twice can do. Reported, and the push left out. */
static int stack_full(const char *which)
{
    static int told;

    if (!told++)
        fprintf(stderr, "actor: a record freed onto the full %s free stack (freed twice?)\n", which);
    return 1;
}

/*
 * @routine 83:975B ActorClear
 * WHAT IT DOES: Clears an actor's record: it is free, still, drawn no
 * more, and one fewer of its kind is live.
 */
void ActorClear(Actor *a)
{
    if (a->outside_play != 1)           /* native: 1, not counted with its kind since a scene's clear kept it */
        g.kind_count[a->kind]--;
    a->outside_play = 0;
    ActorClearFields(a);
}

/*
 * @routine 83:9768 ActorClearFields
 * WHAT IT DOES: ActorClear's fields part: the record is free, still and
 * drawn no more, its kind's count left as it is. Its kind, place and
 * speeds stay.
 */
void ActorClearFields(Actor *a)
{
    a->placement = 0;
    a->state = ACTOR_STATE_FREE;
    a->anim_flags = 0;
    a->flags = 0;
    a->own.bytes[OWN_0A] = 0;
    a->flip = 0;
    a->facing = 0;
    a->view_box = 0;
    a->look = 0;
    a->next = NULL;
    a->own.bytes[OWN_13] = 0;
    a->redraw = 0;
    a->pool_number = 0;
}

/*
 * @routine 83:95E5 ActorFree
 * WHAT IT DOES: Ends an actor of the second pool: clears its record and
 * puts it back on the pool's free stack.
 */
void ActorFree(Actor *a)
{
    ActorClear(a);
    if (g.free_pool_b_top + 1 < ACTOR_POOL_B_SIZE || !stack_full("second pool's"))
        g.free_pool_b[++g.free_pool_b_top] = a;
}

/*
 * @routine 83:9706 ActorTileSlotRelease
 * WHAT IT DOES: An ending actor lets go of its palette slot; when it was
 * the slot's last user, the kind that claimed it loses its look, and the
 * slots after it the kind also held are freed.
 */
void ActorTileSlotRelease(Actor *a)
{
    uint16_t slot = a->tile_slot;

    if (slot && --g.palette_slot_users[slot - 1] == 0) {
        uint8_t kind = g.palette_slot_kind[slot - 1];

        g.kind_look[kind] = 0;
        if (g.kind_extra_slots[kind]) {
            if (g.kind_extra_slots[kind] != 1)
                g.palette_slot_users[slot + 1] = 0;
            g.palette_slot_users[slot] = 0;
        }
    }
    a->tile_slot = 0;
}

/*
 * @routine 83:96E2 ActorFreePlaced
 * WHAT IT DOES: Ends an actor of the pool: its placement may place it
 * again (unless marked never), its record is cleared and goes back on the
 * free stack, and its palette slot is let go.
 * Also 83:96D4 ActorFreePlacedAtY and 83:96D0 ActorFreePlacedAtYLong, the
 * same for another actor's record.
 */
void ActorFreePlaced(Actor *a)
{
    if (a->state != ACTOR_STATE_FREE && a->placed_here)
        PlacementConsoleForget(a->placement);   /* native: the original's placing may reach it again */
    if (!(g.placed[a->placement] & 0x80))
        g.placed[a->placement] = 0;
    ActorClear(a);
    if (g.free_pool_top + 1 < ACTOR_POOL_SIZE || !stack_full("pool's"))
        g.free_pool[++g.free_pool_top] = a;
    ActorTileSlotRelease(a);
}

/*
 * @routine 83:96CA ActorEndPlaced
 * WHAT IT DOES: Ends an actor of the pool for good: its placement may
 * place it again, and the actor loop goes on to the next record.
 * Also 83:96C6 ActorEndDropReturn, the same from a routine its code called.
 */
int ActorEndPlaced(Actor *a)
{
    ActorFreePlaced(a);
    return ACTOR_ENDED;
}

/*
 * @routine 83:95B8 ActorEnd
 * WHAT IT DOES: Ends an actor of the second pool for good: its record
 * cleared and back on the pool's free stack, its palette slot let go.
 */
int ActorEnd(Actor *a)
{
    ActorFree(a);
    ActorTileSlotRelease(a);
    return ACTOR_ENDED;
}
