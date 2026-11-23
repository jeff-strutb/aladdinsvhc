/*
 * kind30.c: actor kind $30, the pickups: by type, one with only its
 * sound, a once-only item, a counted item, a health mark, health filled
 * or one more, and the tens of the apples' count. Its wait routine is
 * Kind30Wait (kind_waits.c).
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"
#include "map/map.h"

/* its own byte +$43: nonzero once a shot has hit it (type 7's wait sets it) */
#define SHOT OWN_43

/* its types, $07 bits 0-2 */
enum {
    PLAIN,                      /* only its sound */
    ONCE,                       /* g.hover_on set; once set, a type 1 starts as type 5 */
    COUNTED,                    /* g.lives up, to at most 100 */
    MARK,                       /* a health mark, once a level */
    FILL,                       /* health filled */
    ONE_MORE,                   /* health one more */
    TENS_DROPS,                 /* the apples' tens; drops again and again */
    TENS                        /* the apples' tens; hops once a shot has hit it */
};

enum {
    DROP_FIRST = ACTOR_RESUME_OWN,  /* its first drop, until it lands */
    DROP_SECOND,                /* its second (preset $29), until it lands */
    TURN_DROP,                  /* $07 bit 3 only: turned round, one drop ($2C) */
    WAIT_TAKEN,                 /* types 0-5: until Aladdin takes it */
    TENS_WAIT,                  /* types 6 (with $10) and 7: the same, hopping when shot */
    HOP_FIRST,                  /* shot: its hop up ($2C), turning at a wall */
    HOP_SECOND                  /* and down ($29) */
};

static uint8_t type_of(const Actor *a) { return (uint8_t)(a->spawn_byte & 0x07); }

/* A frame in the air: true once it is on the ground. */
static bool fall(Actor *a)
{
    ActorFly(a);
    (void)ActorOutOfWallProbe4(a);
    return ActorOutOfGround(a);
}

/* How it appears, by $07 bits 3-4: none, it pops up with `preset` and
   drops twice; $10, it waits at once; $08 alone, it turns and drops once.
   Returns the frame's wait. */
static int appear(Actor *a, uint8_t preset)
{
    uint8_t how = (uint8_t)(a->spawn_byte & 0x18);

    if (!how) {
        ActorMotionPreset(a, preset);
        a->resume = DROP_FIRST;
    } else if (how & 0x10) {
        a->resume = type_of(a) >= TENS_DROPS ? TENS_WAIT : WAIT_TAKEN;
    } else {
        a->facing++;
        ActorMotionPreset(a, 0x2C);
        a->resume = TURN_DROP;
    }
    return 1;
}

/* Type 6 or 7 (again for type 6 after each landing): the shot mark
   cleared, then as it appears; the first drop by its spawn point. */
static int tens_appear(Actor *a)
{
    a->own.bytes[SHOT] = 0;
    return appear(a, k30_fall_motion(a->spawn_at));
}

/* Landed: on to its type's wait (type 6 appears again). */
static int landed(Actor *a)
{
    switch (type_of(a)) {
    case TENS_DROPS:
        return tens_appear(a);
    case TENS:
        a->resume = TENS_WAIT;
        return 1;
    default:
        a->resume = WAIT_TAKEN;
        return 1;
    }
}

/* Taken: never placed again; the sound; ended. */
static int taken(Actor *a, uint8_t sound)
{
    SoundQueueAdd(sound);
    g.placed[a->placement] = 0x80;
    return ActorEndPlaced(a);
}

/* Types 0-5 taken: what each does. */
static int take(Actor *a)
{
    switch (type_of(a)) {
    case ONCE:
        g.hover_on = 1;
        return taken(a, 0x09);
    case COUNTED:
        if (g.lives == 100)
            return taken(a, 0x0B);
        g.lives++;
        a->spawn_byte = 0x10;
        a->spawn_at = 0;
        (void)ActorCreateChild8(a, 0x85);
        return taken(a, 0x09);
    case MARK:
        if (g.health_marks != 10)
            g.health_marks++;
        g.level_marks_taken |= k30_level_mark_bit(g.progress_sequence);
        g.health = g.health_marks;
        return taken(a, 0x09);
    case FILL:
        g.health = g.health_marks;
        return taken(a, 0x0B);
    case ONE_MORE:
        if (g.health != g.health_marks)
            g.health++;
        return taken(a, 0x0B);
    default:                            /* PLAIN */
        return taken(a, 0x0B);
    }
}

/*
 * @routine 83:B684 Kind30Start
 * WHAT IT DOES: Actor kind $30, the pickups. Its type (bits 0-2 of $07)
 * picks its picture and what taking it does; bits 3-4 how it appears:
 * none set, it pops up and drops twice (ActorMotionPreset $27, then $29;
 * types 6 and 7 the first by their spawn point); $08, it turns round and
 * drops once ($2C); $10, it is there at once. Then it waits until
 * Aladdin touches it with health left, plays its sound and ends; types
 * 0-5 are not placed again. Type 0: nothing more (sound $0B). Type 1:
 * g.hover_on set (sound 9); once that is set, a type 1 starts as type 5.
 * Type 2: animation 0 and palette 1; taken, g.lives up, to at most 100;
 * below 100 also a puff (kind $85) from it (sound 9; sound $0B at 100).
 * Type 3: a health mark, once a progress sequence (its bit in
 * g.level_marks_taken): the most health up, to at most 10, and health
 * filled to it (sound 9); the bit already set, it starts as type 4. Type
 * 4: health filled (sound $0B). Type 5: health one more, to at most the
 * most (sound $0B). Types 6 and 7: the apples' tens up by one, to at most
 * 99 (sound $0B), and they may be placed again. Type 6 that has dropped
 * drops again; type 7 waits (type 6 too with $10), and once a shot has hit
 * it (its wait sees to that) it hops ($2C, then $29), turning at a wall.
 * Its second entry (state $80-$BF) is 83:96CA ActorEndPlaced: it ends.
 */
int Kind30Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case ACTOR_START:
        a->probes = k30_probes();
        a->look = 0x0400;
        a->anim_flags = 0x03;
        a->flags |= 0x90;
        AnimStart(a, type_of(a));
        a->flip = 0;
        switch (type_of(a)) {
        case ONCE:
            if (g.hover_on) {
                a->spawn_byte |= 0x04;
                AnimStart(a, type_of(a));
            }
            break;
        case COUNTED:
            a->look = (a->look & 0xFFFFF1FFu) | 0x0200;
            AnimStart(a, 0);
            break;
        case MARK:
            if (k30_level_mark_bit(g.progress_sequence) & g.level_marks_taken) {
                AnimStart(a, 4);
                a->spawn_byte = (uint8_t)((a->spawn_byte & 0xF8) + FILL);
            }
            break;
        case TENS_DROPS:
        case TENS:
            return tens_appear(a);
        default:
            break;
        }
        return appear(a, 0x27);
    case DROP_FIRST:
        if (fall(a)) {
            ActorMotionPreset(a, 0x29);
            a->resume = DROP_SECOND;
        }
        return 1;
    case DROP_SECOND:
    case TURN_DROP:
        if (fall(a))
            return landed(a);
        return 1;
    case WAIT_TAKEN:
        if (ActorHitsTestBox(a) && g.health)
            return take(a);
        return 1;
    case TENS_WAIT:
        if (!ActorHitsTestBox(a)) {
            if (a->own.bytes[SHOT]) {
                a->flip = 0;
                ActorMotionPreset(a, 0x2C);
                a->resume = HOP_FIRST;
            }
            return 1;
        }
        if (!g.health)
            return 1;
        if (g.apples != 0x99) {
            g.apples = (uint8_t)(g.apples + 0x10);
            if ((g.apples & 0xF0) == 0xA0)
                g.apples = 0x99;
        }
        SoundQueueAdd(0x0B);
        return ActorEndPlaced(a);
    case HOP_FIRST:
        ActorFly(a);
        if (ActorOutOfWallProbe4(a))
            a->facing ^= 1;
        if (ActorOutOfGround(a)) {
            ActorMotionPreset(a, 0x29);
            a->resume = HOP_SECOND;
        }
        return 1;
    case HOP_SECOND:
        if (fall(a)) {
            a->own.bytes[SHOT] = 0;
            a->resume = TENS_WAIT;
        }
        return 1;
    default:
        return 1;
    }
}
