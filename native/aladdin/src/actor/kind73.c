/*
 * kind73.c: actor kind $73, the head of the nine-piece chain: it waits for
 * the chain to be built, takes its health, then its turns come from the
 * chain's eighth piece (which sends it to KIND73_OWN_TURN); struck, it
 * flashes and goes back to what it was doing, or, beaten, stays; and its
 * per-frame work while it waits: it follows a path, shows the animation its
 * controller picks, and is touched (stomped, shots turned back) at a place
 * offset from where it is drawn.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "video/video.h"

#define HEALTH_ON_START 7       /* ($7F on a copier whose cartridge RAM reads back: the retail board's open bus fails the test) */

/* its own bytes: +$11, its health: 7; below 0, beaten */
static int8_t *health(Actor *a) { return (int8_t *)&a->own.bytes[OWN_11]; }

enum {
    OWN_TURN = KIND73_OWN_TURN,     /* its own turn: after the chain's piece sends it */
    OT_CHILD_1,                     /* its own turn: after its $14 frames of pose 4 */
    OT_POSE_0A,                     /* after its $1F frames */
    OT_POSE_4B,                     /* after its $18 frames */
    OT_CHILD_2,                     /* after its $14 frames */
    OT_POSE_0B,                     /* after its $1F frames */
    OT_DONE,                        /* after its $81 frames */
    START_WAIT,                     /* each frame until the chain is built */
    HIT_FRAME,                      /* each frame of its show of being struck */
    STAY,
};

/* ActorYieldForever: it waits where it is, for good */
static int yield_forever(Actor *a)
{
    a->resume = STAY;
    return 1;
}

/*
 * @routine 84:8696 Kind73OwnTurn
 * WHAT IT DOES: Kind $73's own turn, where the chain's eighth piece sends
 * it (KIND73_OWN_TURN): twice animation 4 for $14 frames, a child of hit
 * box $A1, a pause, animation 0; then g.head_cue up for the piece, and it
 * waits for good.
 */
static int own_turn(Actor *a)
{
    switch (a->resume) {
    case OWN_TURN:
        g.head_anim = 4;
        a->resume = OT_CHILD_1;
        return 0x14;
    case OT_CHILD_1:
        (void)ActorCreateChild8(a, 0xA1);
        a->resume = OT_POSE_0A;
        return 0x1F;
    case OT_POSE_0A:
        g.head_anim = 0;
        a->resume = OT_POSE_4B;
        return 0x18;
    case OT_POSE_4B:
        g.head_anim = 4;
        a->resume = OT_CHILD_2;
        return 0x14;
    case OT_CHILD_2:
        (void)ActorCreateChild8(a, 0xA1);
        a->resume = OT_POSE_0B;
        return 0x1F;
    case OT_POSE_0B:
        g.head_anim = 0;
        a->resume = OT_DONE;
        return 0x81;
    default:                            /* OT_DONE */
        g.head_cue++;
        return yield_forever(a);
    }
}

/*
 * @routine 84:8766 Kind73Hit
 * WHAT IT DOES: Kind $73 struck (its second entry): below 0 health counted
 * (g.chain_head_struck); it shows being struck (animation 2, flashing) for
 * $1F frames. Still standing: it flashes for $5F frames more, the chain's
 * pick lists go to their second set (g.chain_round_or 4) once its health
 * is below 3, its animation is started again, and it goes back to what it
 * was doing (ChainPieceEnd). Beaten (health negative): animation 3, its
 * wait routine on, and it stays for good.
 */
static int hit(Actor *a)
{
    if (*health(a) < 0)
        g.chain_head_struck++;
    AnimStart(a, 0x02);
    ActorPictureSend(a);
    g.head_hit_frames = 0x1F;
    a->resume = HIT_FRAME;
    return 1;
}

static int hit_frame(Actor *a)
{
    g.head_hit_frames--;
    ActorLookFlash(a);
    PaletteFlash();
    ActorParityBitsSet(a);
    AnimStep(a);
    ActorPictureSend(a);
    if (g.head_hit_frames)
        return 1;
    if (*health(a) >= 0) {
        g.head_flash = 0x5F;
        if ((uint8_t)*health(a) < 0x03)
            g.chain_round_or = 0x04;
        g.head_anim_shown = 0xFF;
        return ChainPieceEnd(a);
    }
    g.head_anim = 3;
    a->flags |= 0x80;
    return yield_forever(a);
}

/*
 * @routine 84:8629 Kind73Start
 * WHAT IT DOES: Actor kind $73, the head of the nine-piece chain: once the
 * chain is built (g.chain_go) it takes its health (7), spawn place $88, no
 * touch tests, touch pose 0, starts and animates by its number
 * (ActorStartBy40, ActorAnimBy40), its picture list, saves the palette
 * reversed, shows no animation yet, and waits for good (its turns come
 * from the chain's eighth piece, which sends it to KIND73_OWN_TURN). Its
 * second entry is Kind73Hit.
 */
int Kind73Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->resume = START_WAIT;
        return 1;
    case START_WAIT:
        if (!g.chain_go)
            return 1;
        *health(a) = HEALTH_ON_START;
        a->spawn_at = 0x88;
        g.chain_untouchable++;
        g.head_pose = 0;
        ActorStartBy40(a);
        ActorAnimBy40(a);
        a->own_pictures = k73_pictures();
        PaletteSaveReversed(2);
        g.palette_upload++;
        a->redraw = 0xFF;
        g.head_anim = 0;
        g.head_anim_shown = 0xFF;
        return yield_forever(a);
    case ACTOR_START_SECOND:
        return hit(a);
    case HIT_FRAME:
        return hit_frame(a);
    case STAY:
        return 1;
    default:
        if (a->resume >= CHAIN_RESUME_FIRST)
            return ChainFollow(a);
        return own_turn(a);
    }
}

/*
 * @routine 84:86F4 Kind73Work
 * WHAT IT DOES: What kind $73 does each frame it waits: it starts the
 * animation g.head_anim names when that changes, animates, shows its
 * picture, steps along its path, counts its flash down (at 0 +$08 bits 4
 * and 7 cleared), and, unless untouchable, is touched at a place offset
 * from it by its pose: Aladdin stomping it and shots turned back there.
 */
int Kind73Work(Actor *a)
{
    uint32_t x, y;

    if (g.head_anim != g.head_anim_shown) {
        g.head_anim_shown = g.head_anim;
        AnimStart(a, g.head_anim);
    }
    AnimStep(a);
    ActorPictureSend(a);
    ActorPathStep(a);
    if (g.head_flash && !--g.head_flash)
        a->anim_flags &= 0x6F;
    x = a->x;
    y = a->y;
    a->x = with_pixels(a->x, (uint16_t)(pixels(a->x) + (uint16_t)k73_touch_dx(g.head_pose)));
    a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + (uint16_t)k73_touch_dy(g.head_pose)));
    if (!g.chain_untouchable) {
        ActorStompedByPlayer(a);
        (void)ActorTurnsShotBack(a);
    }
    a->x = x;
    a->y = y;
    return ACTOR_GOES_ON;
}
