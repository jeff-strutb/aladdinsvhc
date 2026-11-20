/*
 * kind6b.c: actor kind $6B, which the camera trigger task starts with
 * +$07 = 1: shown in its first pose; with +$07 nonzero it waits for the
 * task's cue $00BF, then the second cue $00BE, and walks off to the right
 * with a song, a pause and a second walk, and ends. Its wait routine is
 * the core's AnimateWhileWaiting (83:ECCF ActorWaitAnimC is AnimStep and an
 * RTS).
 */
#include "actor/kinds_c.h"
#include "audio/sound.h"
#include "data/kinds_c_tables.h"

#define K6B_SONG 0x31           /* the song it starts as it walks off */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left of a walk */
} Kind6BOwn;

_Static_assert(sizeof(Kind6BOwn) <= OWN_BYTES, "a view fits the own bytes");

static Kind6BOwn *own(Actor *a) { return (Kind6BOwn *)a->own.bytes; }

enum {
    STAY = ACTOR_RESUME_OWN,    /* with +$07 0: for good */
    WAIT_CUE,                   /* each frame until the camera trigger's third cue */
    WAIT_WALK_CUE,              /* each frame until the scene's cue to walk off */
    WALK_OFF,                   /* after its 64 frames: the song and the first walk */
    WALK_FIRST,                 /* each frame of the first walk */
    WALK_SECOND_SET,            /* after its pause */
    WALK_SECOND,                /* each frame of the second walk */
};

/* A frame of a walk: across and onto the ground. */
static void walk(Actor *a)
{
    ActorMoveX(a);
    (void)ActorSnapToGround(a, NULL);
}

/*
 * @routine 83:EC2C Kind6BWait
 * WHAT IT DOES: Actor kind $6B (its start entry: the record's name for it
 * is misleading): shown facing right in animation 0. With +$07 zero it
 * stays so; else it waits for the third cue, moves $20 pixels right in
 * animation 1, waits for the scene's cue, then after $40 frames starts
 * the song $31 and walks right ($70 frames, speed $3F), pauses ($30
 * frames) and walks on ($60 frames), and ends.
 */
static int start(Actor *a)
{
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    a->look = (uint16_t)((a->look & 0xF1FF) | 0x0200);
    a->probes = k6b_probes();
    a->facing = 0;
    a->flip = 0;
    AnimStart(a, 0);
    a->resume = a->spawn_byte ? WAIT_CUE : STAY;
    return 1;
}

int Kind6BCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case STAY:
        return 1;
    case WAIT_CUE:
        if (!g.scene_cue_bf)
            return 1;
        a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + 0x20));
        AnimStart(a, 1);
        a->anim_flags &= (uint8_t)~0x03;
        a->resume = WAIT_WALK_CUE;
        return 1;
    case WAIT_WALK_CUE:
        if (!g.scene_cue)
            return 1;
        a->resume = WALK_OFF;
        return 0x40;
    case WALK_OFF:
        SongPlay(K6B_SONG);
        AnimStart(a, 3);
        own(a)->frames = 0x70;
        ActorSpeedXSet(a, 0x3F);
        a->resume = WALK_FIRST;
        return 1;
    case WALK_FIRST:
        walk(a);
        if (--own(a)->frames)
            return 1;
        AnimStart(a, 4);
        a->resume = WALK_SECOND_SET;
        return 0x30;
    case WALK_SECOND_SET:
        AnimStart(a, 3);
        own(a)->frames = 0x60;
        a->resume = WALK_SECOND;
        return 1;
    case WALK_SECOND:
        walk(a);
        if (--own(a)->frames)
            return 1;
        return ActorEndPlaced(a);
    default:
        return kind_lost(a, "Kind6BCode");
    }
}
