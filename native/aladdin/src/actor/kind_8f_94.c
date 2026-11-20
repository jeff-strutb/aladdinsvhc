/*
 * kind_8f_94.c: actor kinds $8F (a figure that crosses the screen and back,
 * a song playing, its x kept for the scene) and $94 (a spawner that starts
 * one more of its kind at each step of the scene's count), and their waits.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "audio/sound.h"
#include "data/kinds_e_tables.h"
#include "video/video.h"

enum {
    SONG = ACTOR_RESUME_OWN,    /* $8F: after its first $3A frames */
    POSE,                       /* after 6 more */
    CROSS,                      /* each frame of a crossing */
    SPARK_END,                  /* $94's spark: after its $20 frames */
    SPAWNER,                    /* $94's spawner: each frame */
};

/* its own bytes: +$40 the crossing's frames left; $94's spawner +$40-$42 */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: $8F: frames left of its crossing; $94: the count it last acted on */
    uint8_t spark_x;            /* +$41: $94: the next spark's x pixels (low byte) */
    uint8_t spark_y;            /* +$42: its y pixels (low byte) */
} Kind8FOwn;

static inline Kind8FOwn *own(Actor *a) { return (Kind8FOwn *)a->own.bytes; }

/* a crossing: $56 frames from the next */
static int cross(Actor *a)
{
    own(a)->frames = 0x56;
    a->resume = CROSS;
    return 1;
}

/*
 * @routine 83:FA4F Kind8FWalk
 * WHAT IT DOES: Actor kind $8F: starts 32 pixels off the screen's left
 * edge, its wait routine on, waits $3A frames, plays song $27, waits 6,
 * then (pose 0, its picture sent, speed $5D) walks right for $56 frames,
 * comes back left with its y's low byte $80 for as long, and ends; its x
 * is kept in g.scene_walker_x all the while. Its second entry (83:FAB7)
 * ends it.
 */
int Kind8FWalk(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        a->own_pictures = k8f_pictures();
        a->x = with_pixels(a->x, 0xFFE0);
        g.scene_walker_x = 0xFFE0;
        a->resume = SONG;
        return 0x3A;
    case ACTOR_START_SECOND:
        return ActorEndPlaced(a);
    case SONG:
        SongPlay(0x27);
        a->resume = POSE;
        return 6;
    case POSE:
        AnimStart(a, 0x00);
        ActorPictureSend(a);
        ActorSpeedXSet(a, 0x5D);
        return cross(a);
    case CROSS:
        ActorMoveX(a);
        g.scene_walker_x = pixels(a->x);
        if (--own(a)->frames)
            return 1;
        if (a->facing)
            return ActorEndPlaced(a);
        a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | 0x80));
        a->facing = 1;
        a->flip = 1;
        return cross(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:FAAE KindAnimateSendFAAE
 * WHAT IT DOES: Kind $8F's wait (through its stub 83:9359): its animation
 * on and its picture sent. The same code as KindAnimateSend (kind_steps.c),
 * which kind $8F's row names: one kind's piece, written as that.
 */

/*
 * @routine 83:FABB Kind94Spark
 * WHAT IT DOES: Actor kind $94. With +$07 zero, one spark: +$08 bit 0 and
 * its wait routine on, its x and y's top bytes cleared, pose 0, shown $20
 * frames, then ended. Else the spawner: each frame g.scene_count has a new
 * value, it starts a spark (its own kind, +$07 0, its own look) at the
 * place the spark table gives for it. Its second entry, KindA8Start, is
 * another kind's routine: left aside (kinds-d-shared-kinds.md section 2).
 */
int Kind94Spark(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        if (!a->spawn_byte) {
            a->anim_flags |= 0x01;
            a->flags |= 0x90;
            a->x &= 0xFFFFu;
            a->y &= 0xFFFFu;
            AnimStart(a, 0x00);
            a->resume = SPARK_END;
            return 0x20;
        }
        own(a)->frames = 0;
        a->resume = SPAWNER;
        return 1;
    case SPARK_END:
        return ActorEndPlaced(a);
    case SPAWNER: {
        uint8_t count = g.scene_count;
        Actor *spark;

        if (count == own(a)->frames)
            return 1;
        own(a)->frames = count;
        own(a)->spark_x = k94_spark_x((uint8_t)(count << 1));
        own(a)->spark_y = k94_spark_y((uint8_t)(count << 1));
        spark = ActorSpawn(a, a->kind);
        if (spark) {
            spark->look = a->look;
            spark->x = with_pixels(spark->x, (uint16_t)((pixels(spark->x) & 0xFF00) | own(a)->spark_x));
            spark->y = with_pixels(spark->y, (uint16_t)((pixels(spark->y) & 0xFF00) | own(a)->spark_y));
            spark->spawn_byte = 0;
        }
        return 1;
    }
    default:
        return 1;
    }
}

/*
 * @routine 83:FB15 Kind94Wait
 * WHAT IT DOES: Kind $94's wait (through its stub 83:939A): its animation
 * on. The same code as the core's AnimateWhileWaiting, which kind $94's
 * row names: one kind's piece, written as that.
 */
