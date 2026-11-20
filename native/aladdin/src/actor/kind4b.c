/*
 * kind4b.c: actor kind $4B, a figure that sticks to another record for 24
 * frames, its animation following that record's. Its second entry and its
 * wait routine in the record's table are kind $4D's start, which its
 * flags ($10: no wait routine) never reach: not registered.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"

#define FRAMES OWN_40           /* frames left */
#define SHOWN OWN_41            /* the $07 its animation was last set from */
#define FOLLOWED_40 OWN_40      /* the followed record's byte its $07 follows */
#define ANIM_0 0x05             /* its animations start here, by $07 */

enum { STICK = ACTOR_RESUME_OWN };

/* One frame of it: the place of the record Actor.next names, its $07 from
   that record's +$40, its animation by it. */
static void follow(Actor *a)
{
    const Actor *f = a->next;

    a->x = with_pixels(a->x, pixels(f->x));
    a->y = with_pixels(a->y, pixels(f->y));
    if (f->own.bytes[FOLLOWED_40] != a->spawn_byte)
        a->spawn_byte = f->own.bytes[FOLLOWED_40];
    if (a->spawn_byte != a->own.bytes[SHOWN]) {
        a->own.bytes[SHOWN] = a->spawn_byte;
        AnimResume(a, (uint8_t)(a->spawn_byte + ANIM_0));
    }
    AnimStep(a);
}

/*
 * @routine 83:CE2A Kind4BStart
 * WHAT IT DOES: Actor kind $4B, a figure that sticks to the record Actor.next
 * names for 24 frames, then ends: each frame it takes that record's place,
 * and its animation follows that record's +$40 (animation 5 plus it).
 */
int Kind4BStart(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->flags = 0x10;
        AnimStart(a, (uint8_t)(a->spawn_byte + ANIM_0));
        a->own.bytes[SHOWN] = 0xFF;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0600);
        a->own.bytes[FRAMES] = 0x18;
        follow(a);
        a->resume = STICK;
        return 1;
    }
    if (!--a->own.bytes[FRAMES])
        return ActorEndPlaced(a);
    follow(a);
    return 1;
}
