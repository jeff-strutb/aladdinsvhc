/*
 * kind88.c: actor kinds $88 (a thing that flies up a course and back down
 * each time it is sent) and $89 (sets Aladdin's sprite priority by where
 * he is).
 */
#include "actor/actor_a.h"
#include "data/actor_a_tables.h"
#include "video/video.h"

#define FLOOR_Y 0x0100          /* the y pixels it rests at */

#define FLOOR OWN_40            /* +$40 (word): the y pixels it rests at */
#define COURSE OWN_42           /* +$42: the course it was sent on */

enum { REST = ACTOR_RESUME_OWN, UP, DOWN, SIDE };

int Kind89Start(Actor *a);

static void set_floor(Actor *a)
{
    a->own.bytes[FLOOR] = (uint8_t)FLOOR_Y;
    a->own.bytes[FLOOR + 1] = (uint8_t)(FLOOR_Y >> 8);
}

static uint16_t floor_y(const Actor *a)
{
    return (uint16_t)(a->own.bytes[FLOOR] | a->own.bytes[FLOOR + 1] << 8);
}

/* sent again as long as g.riser_send is set: up on its course's motion; else at rest */
static int send_or_rest(Actor *a)
{
    if (g.riser_send) {
        a->own.bytes[COURSE] = g.riser_course;
        ActorMotionPreset(a, riser_up(g.riser_course));
        a->resume = UP;
        return 1;
    }
    a->y = with_pixels(a->y, FLOOR_Y);
    set_floor(a);
    a->resume = REST;
    return 1;
}

/*
 * @routine 84:8E6A RiserActor
 * WHAT IT DOES: Actor kind $88: rests at y 256 until it is sent
 * (g.riser_send), then flies up its course (g.riser_course) until it
 * starts down, and comes back down to y 256; its heights are kept in the y
 * history (ActorYHistoryPush) all the while. Sent again as long as
 * g.riser_send stays set. Its second entry (83:9322) goes on as kind $89's
 * start (Kind89Start).
 */
int Kind88Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags = 0x80;
        a->y = with_pixels(a->y, FLOOR_Y);
        set_floor(a);
        a->resume = REST;
        return 1;
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind89Start, ACTOR_START);
    case UP:
        ActorFlyAgainst(a);
        ActorYHistoryPush(a);
        if (pixels(a->y) >= floor_y(a))
            return 1;
        ActorMotionPreset(a, riser_down(a->own.bytes[COURSE]));
        a->resume = DOWN;
        return 1;
    case DOWN:
        ActorFly(a);
        ActorYHistoryPush(a);
        if (pixels(a->y) < floor_y(a))
            return 1;
        return send_or_rest(a);
    default:                            /* REST */
        return send_or_rest(a);
    }
}

/*
 * RiserFuture (native only, not the original's): the heights (y less 256) the riser (kind $88) will push next, as a copy of
 * it flies on by the steps above, with no effect on the game: up and down on its course, and sent again on the course g
 * holds while g.riser_send stays set (the course a later sweep picks is not known). Fills `out` with up to n heights and
 * returns how many; it stops where the riser would rest unsent (no height is pushed while it rests). Level $11's floor
 * shows them left of the original's screen, so the wave comes in from there as it moves (video/line_history.c).
 */
int RiserFuture(int16_t *out, int n)
{
    const Actor *live = NULL;
    Actor c;
    int k = 0;

    for (unsigned i = 0; i < ACTOR_RECORDS && !live; i++)
        if (g.actors[i].state && g.actors[i].kind == 0x88 && (g.actors[i].resume == REST || g.actors[i].resume == UP || g.actors[i].resume == DOWN))
            live = &g.actors[i];
    if (!live)
        return 0;
    c = *live;
    for (int frame = 0; k < n && frame < 8 * n; frame++) {
        switch (c.resume) {
        case UP:
            ActorFlyAgainst(&c);
            out[k++] = (int16_t)(uint16_t)((uint16_t)(c.y >> 8) - 0x0100);
            if (pixels(c.y) < floor_y(&c)) {
                ActorMotionPreset(&c, riser_down(c.own.bytes[COURSE]));
                c.resume = DOWN;
            }
            break;
        case DOWN:
            ActorFly(&c);
            out[k++] = (int16_t)(uint16_t)((uint16_t)(c.y >> 8) - 0x0100);
            if (pixels(c.y) >= floor_y(&c)) {
                if (!g.riser_send)
                    return k;
                c.own.bytes[COURSE] = g.riser_course;
                ActorMotionPreset(&c, riser_up(g.riser_course));
                c.resume = UP;
            }
            break;
        default:                        /* REST: sent again, or at rest unsent (nothing more is pushed) */
            if (!g.riser_send)
                return k;
            c.own.bytes[COURSE] = g.riser_course;
            ActorMotionPreset(&c, riser_up(g.riser_course));
            c.resume = UP;
            break;
        }
    }
    return k;
}

/*
 * @routine 84:8F9C Kind89PlayerPriority
 * WHAT IT DOES: Actor kind $89: each frame sets Aladdin's sprite priority
 * (bits 4-5 of his flags) by whether he is left of x $0934 or not, for good.
 */
int Kind89Start(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->resume = SIDE;
        return 1;
    }
    g.actors[0].flags = (uint8_t)((g.actors[0].flags & 0xCF) | k89_priority(pixels(g.actors[0].x) >= 0x0934));
    return 1;
}
