/*
 * kind23_25.c: actor kinds $23 and $25. Kind $23 makes four children of its
 * kind across the view, each of which shows itself, waits and makes a kind
 * $6D, then ends; kind $25 plays sound $3E every 291 frames until the level
 * is being left. Their waits are Kind23Wait and ActorWaitNothing
 * (kind_waits_rest.c). Kind $23's second entry ends it.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

#define K23_FACING 0x46         /* the facing its children get: the loop's X, twice their kind, which ActorSpawnFacing takes as it is */

enum {
    K23_SHOWN = ACTOR_RESUME_OWN,   /* a child: after its 26 frames */
    K25_QUIET,                      /* kind $25: after its 255 frames */
    K25_SOUND,                      /* after its 36 more */
};

/*
 * @routine 83:9CEB Kind23Start
 * WHAT IT DOES: Actor kind $23's start: with +$07 0 it makes four children
 * of its kind (parts 4 down to 1) and ends; a child takes its x from a
 * table by its part, y $40 (its low byte), look $0600, shows animation $19
 * for $1A frames, makes a kind $6D with the +$07 its part gives, and ends.
 */
static int start23(Actor *a)
{
    uint8_t part = a->spawn_byte;

    if (!part) {
        a->spawn_at = 0;
        for (part = 4; part; part--)
            (void)ActorSpawnFacing(a, 0x23, K23_FACING, part);
        return ActorEndPlaced(a);
    }
    a->x = with_pixels(a->x, k23_x(part - 1));
    a->look = 0x0600;
    a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | 0x40));
    a->flags |= 0x90;
    AnimStart(a, 0x19);
    a->resume = K23_SHOWN;
    return 0x1A;
}

int Kind23Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start23(a);
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case K23_SHOWN:
        a->spawn_byte = k23_next_byte07(a->spawn_byte - 1);
        a->spawn_at = 0;
        (void)ActorCreateChild8(a, 0x6D);
        return ActorEndPlaced(a);
    default:
        return kind_lost(a, "Kind23Code");
    }
}

/*
 * @routine 83:9D4B Kind25Start
 * WHAT IT DOES: Actor kind $25's start: for good, every 291 frames (255,
 * then 36) it plays sound $3E unless the level is being left.
 */
int Kind25Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0;
        a->resume = K25_QUIET;
        return 0xFF;
    case K25_QUIET:
        a->resume = K25_SOUND;
        return 0x24;
    case K25_SOUND:
        if (!g.level_exiting)
            SoundQueueAdd(0x3E);
        a->resume = K25_QUIET;
        return 0xFF;
    default:
        return kind_lost(a, "Kind25Code");
    }
}
