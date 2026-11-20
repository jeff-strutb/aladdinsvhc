/*
 * kind9a.c: actor kinds $9A and $99 (act 5): kind $9A, dropped by kind
 * $98, falls to the ground and bursts into 32 kind $99: two that run off
 * along the ground, the rest flashing out in a spread and gone. Each
 * kind's second entry (84:A434, 84:A4DF, untagged JMLs) is ActorEndPlaced.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* kind $9A's own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t pairs;              /* +$40: pairs of children left to make */
    uint8_t bounces;            /* +$41: nonzero once burst: Aladdin bounces off it */
} K9AOwn;

static inline K9AOwn *k9a(Actor *a) { return (K9AOwn *)a->own.bytes; }

#define K99_KIND 0x99

enum {
    K99_RUN_SET = ACTOR_RESUME_OWN, /* kind $99, $07 0 or 1: after 8 frames */
    K99_RUN,                    /* runs along the ground while it can be hit */
    K99_SPREAD,                 /* the others: after 8 frames, put across */
    K99_END,                    /* after 16 more */
    K9A_FALL,                   /* kind $9A: falls until it is in the ground */
    K9A_BURST,                  /* after 8 frames on the ground */
    K9A_SPAWN,                  /* every 2 frames, two kind $99 */
    K9A_HIDE,                   /* after $80 frames */
    K9A_END                     /* after $80 more */
};

/*
 * @routine 84:A3B7 Kind99Start
 * WHAT IT DOES: Actor kind $99, a piece of a kind $9A's burst, by its +$07
 * (0-31, from its maker): anim_flags bit 1, flags $90, facing and flip
 * +$07 bit 0, its animation from kind $99's table by +$07. 0 and 1, after
 * 8 frames, run off along the ground at 8 pixels a frame, right or left,
 * until they can no longer be hit, and end. The others (anim_flags bit 3
 * clear) show for 8 frames, are put across by their offset from the
 * spread table (right for even +$07, left for odd), hide (anim_flags bit
 * 3) for 16 frames and end.
 */
int Kind99Start(Actor *a)
{
    uint16_t spread;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->facing = (uint8_t)(a->spawn_byte & 0x01);
        a->flip = a->facing;
        AnimStart(a, k99_anim(a->spawn_byte));
        if (!(a->spawn_byte & 0xFE)) {
            a->resume = K99_RUN_SET;
            return 0x08;
        }
        a->anim_flags &= (uint8_t)~0x08;
        a->resume = K99_SPREAD;
        return 0x08;
    case K99_RUN_SET:
        a->speed_x = 0x0800;            /* 8 pixels a frame */
        a->resume = K99_RUN;
        return 1;
    case K99_RUN:
        ActorMoveX(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEndPlaced(a);
    case K99_SPREAD:
        spread = k99_spread(a->spawn_byte & 0x1F);
        if (!(a->spawn_byte & 0x01))
            a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + spread));
        else
            a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) - spread));
        a->anim_flags |= 0x08;
        a->resume = K99_END;
        return 0x10;
    case ACTOR_START_SECOND:            /* 84:A434: ActorEndPlaced */
    case K99_END:
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 84:A4D4 Kind9ASpawn
 * WHAT IT DOES: One kind $99 of a kind $9A's burst, at its spawn point 0
 * with the burst's +$07; then +$07 counts up for the next.
 */
static void Kind9ASpawn(Actor *a)
{
    a->spawn_at = 0;
    (void)ActorCreateChild8(a, K99_KIND);
    a->spawn_byte++;
}

/*
 * @routine 84:A446 Kind9AStart
 * WHAT IT DOES: Actor kind $9A, dropped by kind $98: Aladdin does not
 * bounce off it yet; anim_flags bit 1, flags $90, its probe list
 * ($80:D96E), palette bits $0600 in its look, falling from 1 pixel a frame
 * with gravity $30 (animation $17) until it is in the ground
 * (ActorOutOfGround raises it); then 4 pixels lower (its y's low pixel
 * byte), animation $18 and sound $48; after 8 frames animation $1E and
 * sound $3E, and it bursts: two kind $99 every 2 frames, 32 in all (+$07
 * 0 to 31). Then Aladdin bounces off it (+$41); animation $1F; after $80
 * frames it hides (anim_flags bit 4), after $80 more it ends.
 */
int Kind9AStart(Actor *a)
{
    K9AOwn *k = k9a(a);

    switch (a->resume) {
    case ACTOR_START:
        k->bounces = 0;
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->probes = k9a_probes();
        a->look |= 0x0600;
        a->speed_x = 0;
        a->speed_y = 0x0100;            /* 1 pixel a frame down */
        a->accel_y = 0x30;
        AnimStart(a, 0x17);
        a->resume = K9A_FALL;
        return 1;
    case K9A_FALL:
        ActorFly(a);
        if (!ActorOutOfGround(a))
            return 1;
        /* its y's low pixel byte 4 more (no carry into the high byte) */
        a->y = (a->y & ~0xFF00u) | (uint32_t)(uint8_t)(pixels(a->y) + 4) << 8;
        AnimStart(a, 0x18);
        SoundQueueAdd(0x48);
        a->resume = K9A_BURST;
        return 0x08;
    case K9A_BURST:
        AnimStart(a, 0x1E);
        SoundQueueAdd(0x3E);
        k->pairs = 0x10;
        a->spawn_byte = 0;
        a->resume = K9A_SPAWN;
        return 0x02;
    case K9A_SPAWN:
        Kind9ASpawn(a);
        Kind9ASpawn(a);
        if (--k->pairs)
            return 0x02;
        k->bounces++;
        AnimStart(a, 0x1F);
        a->resume = K9A_HIDE;
        return 0x80;
    case K9A_HIDE:
        a->anim_flags |= 0x10;
        a->resume = K9A_END;
        return 0x80;
    case ACTOR_START_SECOND:            /* 84:A4DF: ActorEndPlaced */
    case K9A_END:
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}

/*
 * @routine 84:A438 Kind99WaitWork
 * WHAT IT DOES: Kind $99's wait routine (through its stub, ActorKind99Wait):
 * its animation plays on, Aladdin touching it is hurt, and it is freed
 * once out of range by box $18.
 * Also 83:93CF ActorKind99Wait, the stub the wait table names (a JSL to it).
 */
int Kind99Wait(Actor *a)
{
    AnimStep(a);
    (void)ActorHurtsPlayerOnTouch(a);
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}

/*
 * @routine 84:A4E3 Kind9AWaitWork
 * WHAT IT DOES: Kind $9A's wait routine (through its stub, ActorKind9AWait):
 * its animation plays on; before it bursts Aladdin touching it is hurt,
 * after it, coming down on it bounces him (sound $12, animation $18); it
 * is freed once out of range by box $18.
 * Also 83:93DC ActorKind9AWait, the stub the wait table names (a JSL to it).
 */
int Kind9AWait(Actor *a)
{
    AnimStep(a);
    if (!k9a(a)->bounces) {
        (void)ActorHurtsPlayerOnTouch(a);
    } else if (ActorBouncesPlayer(a)) {
        SoundQueueAdd(0x12);
        AnimStart(a, 0x18);
    }
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
