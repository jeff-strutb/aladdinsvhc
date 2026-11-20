/*
 * kind8a.c: actor kind $8A, a pad Aladdin bounces on (the record's
 * player_landing.c and actor_waits_small.c; the native player_landing.c
 * is the actor core's).
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "player/player.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t rest;               /* +$40: frames before it can be landed on again */
    uint8_t pressed;            /* +$41: frames it shows pressed; at 1 it rises again */
} PadOwn;

static inline PadOwn *pad(Actor *a) { return (PadOwn *)a->own.bytes; }

enum { FRAME = ACTOR_RESUME_OWN, STAND };

#define LAND_ANIM 0x13          /* its animation at its start and at each landing */
#define AFTER_ANIM 0x14         /* from 9 frames after a landing */
#define TOP 0x24                /* his y plus this below its y: he comes from above */
#define STAND_ABOVE 0x2C        /* he stands this many pixels above its y */

/*
 * @routine 83:F9BA LandingPad
 * WHAT IT DOES: Actor kind $8A (the kinds table's start entry for $8A is
 * this routine itself): a pad Aladdin can land on.
 * anim_flags bit 1, its wait routine on (flags $90), look's high byte 6,
 * animation $13. Each frame: its pressed count down (animation $14 as it
 * goes from 1 to 0); while resting, its rest count down. Else when he
 * meets it (ActorHitsTestBoxIfActive) and may land on actors ($145A bit 7
 * clear): from above (his y + $24 above its y) animation $13 and
 * g.player_in_air 1; then, his hurt count 0, a rest of 20 frames, pressed
 * 10, sound $12, and his code goes on at PlayerBounce; the next frame he
 * stands $2C pixels above it, at its x, landed on it (g.player_landed).
 * From the side or with his hurt count set he is put beside it
 * (PlayerPushAside).
 * The original copies the byte at $0001 into g.player_landed: the high
 * byte of the power-on signature $0123 at $0000 (81:8000's PowerOn), which
 * nothing else writes, so 1 (every frame end of the attract recordings
 * holds 1 there).
 * Also 83:FA40 PadFromSide.
 */
int LandingPad(Actor *a)
{
    PadOwn *k = pad(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->look = (uint16_t)((a->look & 0x00FF) | 0x06 << 8);
        AnimStart(a, LAND_ANIM);
        k->rest = 0;
        k->pressed = 0;
        a->resume = FRAME;
        return 1;
    case STAND:
        g.actors[0].y = with_pixels(g.actors[0].y, (uint16_t)(pixels(a->y) - STAND_ABOVE));
        g.actors[0].x = with_pixels(g.actors[0].x, pixels(a->x));
        g.player_landed = 1;
        a->resume = FRAME;
        return 1;
    default:                            /* FRAME */
        break;
    }
    if (k->pressed) {
        if (k->pressed == 1)
            AnimStart(a, AFTER_ANIM);
        k->pressed--;
    }
    if (k->rest) {
        k->rest--;
        return 1;
    }
    if (!ActorHitsTestBoxIfActive(a) || (g.player_no_landing & 0x80))
        return 1;
    if ((uint16_t)(pixels(g.actors[0].y) + TOP) >= pixels(a->y)) {
        PlayerPushAside(a);
        return 1;
    }
    AnimStart(a, LAND_ANIM);
    g.player_in_air = 1;
    if (g.player_hurt_count) {
        PlayerPushAside(a);
        return 1;
    }
    k->rest = 20;
    k->pressed = 10;
    SoundQueueAdd(0x12);
    g.actors[0].resume = PLAYER_AT_BOUNCE;
    a->resume = STAND;
    return 1;
}

/*
 * @routine 83:FA45 Kind8AWait
 * WHAT IT DOES: Kind $8A's wait routine: its animation, then its end when
 * it is out of range by box 0.
 */
int Kind8AWait(Actor *a)
{
    AnimStep(a);
    return ActorEndOutOfRange(a, 0);
}
