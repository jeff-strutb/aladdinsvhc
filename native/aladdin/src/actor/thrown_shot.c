/*
 * thrown_shot.c: actor kinds $10 and $14-$17 of the second pool: a shot
 * (an apple Aladdin throws) that flies its course, splashing (kind $13)
 * where it lands; kind $10's end when it hits (kind $11), which is also
 * kind $B9's whole code; and the two ends other actors send a shot to
 * (shot_hit.c): at once, or turned back and flying off.
 */
#include "actor/actor_a.h"
#include "audio/sound.h"
#include "data/actor_a_tables.h"
#include "map/map.h"

#define SHOT_LAUNCH OWN_40          /* frames before bit 7 of its $08 (not yet hitting) is cleared */
#define TURNED_BACK_PRESET 0x75

/* The shot is gone: below the level's floor (levels 8 and $10: its y's
   high byte negative; level $0E with the camera left of x $0700: at the
   top), or it can no longer be hit. */
static bool shot_gone(const Actor *a)
{
    uint8_t level = (uint8_t)g.level, y_high = (uint8_t)(a->y >> 16);

    if (level == 0x08 || level == 0x10) {
        if (y_high & 0x80)
            return true;
    } else if (level == 0x0E) {
        if ((uint8_t)(g.layers[0].scroll_x >> 16) < 0x07 && y_high == 0)
            return true;
    }
    return !(a->flags & 0x40);
}

/*
 * @routine 81:EABA ShotFly
 * @actor
 * WHAT IT DOES: A shot (kinds $10 and $14-$17) started new: it flies the
 * course its $07 picks (a motion preset), animation 7, until it lands in
 * the ground (not one it passes through), where it splashes (an actor of
 * kind $13 in its place) and ends; it ends at once when it is gone
 * (shot_gone). Its first 2 frames it does not hit yet ($08 bit 7).
 */
static int shot_fly(Actor *a)
{
    MapPoint p;

    if (a->resume == ACTOR_START) {
        a->x &= ~0xFFu;
        a->y &= ~0xFFu;
        a->flags = 0x50;
        AnimStart(a, 0x07);
        a->look = (uint16_t)((a->look & 0x00FF) | 0x0400);
        ActorMotionPreset(a, a->spawn_byte);
        a->x &= ~0xFFu;
        a->y &= ~0xFFu;
        a->probes = shot_probes();
        a->own.bytes[OWN_11] = 0;
        a->probe_at = 0;
        a->anim_flags |= 0x80;
        a->own.bytes[SHOT_LAUNCH] = 0x02;
        a->resume = SHOT_AT_FLY;
        return 1;
    }
    if (shot_gone(a))
        return ActorEnd(a);
    AnimStep(a);
    ActorMove(a);
    if (a->own.bytes[SHOT_LAUNCH]) {
        if (--a->own.bytes[SHOT_LAUNCH])
            return 1;
        a->anim_flags &= 0x7F;
    }
    if (!ActorInGround(a, &p) || (p.behaviour & TILE_THROUGH))
        return 1;
    Spawn13AtY(a);
    return ActorEnd(a);
}

/*
 * @routine 81:EB44 ShotHit
 * @actor
 * WHAT IT DOES: Actor kind $10 in state $4C (it hit something), and kind
 * $B9 started: a frame later, sound 8 and an actor of kind $11 in its
 * place from the second pool, then its end.
 */
static int shot_hit(Actor *a)
{
    Actor *c;

    if (a->resume != SHOT_AT_HIT) {
        a->resume = SHOT_AT_HIT;
        return 1;
    }
    SoundQueueAdd(0x08);
    a->anim_flags |= 0x80;
    c = ActorAllocB();
    if (c) {
        c->state = ACTOR_STATE_NEW;
        c->kind = 0x11;
        c->spawn_byte = 0;
        c->anim_flags = 0x80;
        c->x = with_pixels(c->x, pixels(a->x));
        c->y = with_pixels(c->y, pixels(a->y));
    }
    return ActorEnd(a);
}

/* The shot's code, for each of its kinds' entries and where it goes on. */
int ShotCode(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return a->kind == 0xB9 ? shot_hit(a) : shot_fly(a);
    case ACTOR_START_SECOND:
    case SHOT_AT_HIT:
        return shot_hit(a);
    case SHOT_AT_FLY:
        return shot_fly(a);
    case SHOT_AT_ENDS:
        return ActorEnd(a);
    case SHOT_AT_TURNED_BACK:
        /* turned back (ActorTurnsShotBack): preset $75, then it flies a
           frame at a time until it can no longer be hit, and ends */
        ActorMotionPreset(a, TURNED_BACK_PRESET);
        a->resume = SHOT_AT_FLY_OFF;
        return 1;
    case SHOT_AT_FLY_OFF:
        ActorFly(a);
        if (a->flags & 0x40)
            return 1;
        return ActorEnd(a);
    default:
        unconverted("ShotCode: a resume point it does not have");
        return 1;
    }
}
