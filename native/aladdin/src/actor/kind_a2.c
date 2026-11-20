/*
 * kind_a2.c: actor kind $A2: a shot another actor sends out (made by the
 * core's ActorSpawnA2, 84:8145), which turns back to its sender and is
 * taken in, or falls away once its sender is hit.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"

/* its sender: its record's index in g.actors, in its +$4C-$4D (the
   original's word there is the sender's record address) */
static Actor *sender_of(const Actor *a)
{
    return &g.actors[a->own.bytes[OWN_4C] | a->own.bytes[OWN_4D] << 8];
}

/* in the sender's own bytes: +$42-$43 its shots out, +$47 set once it is hit */
#define SENDER_OUT_LOW OWN_42
#define SENDER_OUT_HIGH OWN_43
#define SENDER_HIT OWN_47

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t time;               /* +$40: frames left of its way out */
} KA2Own;

static inline KA2Own *ka2(Actor *a) { return (KA2Own *)a->own.bytes; }

#define HEADING_SET 0x22        /* the direction set its heading's vectors are in */

enum { OUT = ACTOR_RESUME_OWN, BACK, FALL };

/* A frame of its flight along its heading (+$14). */
static void fly_heading(Actor *a)
{
    ActorSpeedAddVector(a, HEADING_SET);
    ActorMoveRight(a);
    ActorMoveY(a);
}

/* Its fall away: motion preset $6D, its sprite priority cleared, then each
   frame until it can no longer be hit. */
static int fall_away(Actor *a)
{
    ActorMotionPreset(a, 0x6D);
    ActorPriorityClear(a);
    a->resume = FALL;
    return 1;
}

/* Its sender within $30 below and 16 above, and $20 across (16 each way). */
static bool at_sender(const Actor *a, const Actor *s)
{
    uint16_t dy = (uint16_t)(pixels(s->y) + 0x10 - pixels(a->y));
    uint16_t dx = (uint16_t)(pixels(s->x) + 0x10 - pixels(a->x));

    return dy < 0x30 && dx < 0x20;
}

/*
 * @routine 84:8249 KindA2Sent
 * WHAT IT DOES: Actor kind $A2 started new: anim_flags bit 1, flags $90,
 * look $0600, sound $2F, animation $1A, both speeds 0; its heading from
 * kind $A2's heading table by +$07 plus 3 when it faces left. Each frame
 * it flies along its heading (direction set $22's vector added to its
 * speeds, then moved right and down by them); after $20 frames the
 * heading turns back (bit 3), and each frame after: its sender hit
 * (+$47): animation $1B, flags bit 7 clear, and it falls away (as
 * KindA2Hit); else at its sender: the sender's shots out (+$42-$43 word)
 * one fewer, and it ends.
 * The record's DB = $84 is not what the code runs with: every trace that
 * runs it reads the headings at $80:D501, none at $84:D501.
 *
 * @routine 84:82F5 KindA2Hit
 * WHAT IT DOES: Kind $A2's second entry (state $4C, hit): sound $12,
 * animation $1B, its sender's shots out one fewer (+$42, a byte), and it
 * falls away: motion preset $6D, its sprite priority cleared, each frame
 * its animation and its flight while it can be hit, then the end.
 */
int KindA2Sent(Actor *a)
{
    KA2Own *k = ka2(a);
    Actor *s;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->look = 0x0600;
        SoundQueueAdd(0x2F);
        AnimStart(a, 0x1A);
        ActorSpeedXClear(a);
        ActorSpeedYClear(a);
        a->facing = ka2_heading((uint8_t)((a->facing ? 3 : 0) + a->spawn_byte));
        k->time = 0x20;
        a->resume = OUT;
        return 1;
    case OUT:
        fly_heading(a);
        if (--k->time)
            return 1;
        a->facing ^= 0x08;
        a->resume = BACK;
        return 1;
    case BACK:
        fly_heading(a);
        s = sender_of(a);
        if (s->own.bytes[SENDER_HIT]) {
            AnimStart(a, 0x1B);
            a->flags &= (uint8_t)~0x80;
            return fall_away(a);
        }
        if (!at_sender(a, s))
            return 1;
        {
            uint16_t out = (uint16_t)((s->own.bytes[SENDER_OUT_LOW] | s->own.bytes[SENDER_OUT_HIGH] << 8) - 1);

            s->own.bytes[SENDER_OUT_LOW] = (uint8_t)out;
            s->own.bytes[SENDER_OUT_HIGH] = (uint8_t)(out >> 8);
        }
        return ActorEndPlaced(a);

    case ACTOR_START_SECOND:            /* KindA2Hit */
        SoundQueueAdd(0x12);
        AnimStart(a, 0x1B);
        s = sender_of(a);
        s->own.bytes[SENDER_OUT_LOW]--;
        return fall_away(a);
    case FALL:
        AnimStep(a);
        ActorFly(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEndPlaced(a);
    default:
        return 1;
    }
}
