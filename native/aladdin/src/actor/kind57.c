/*
 * kind57.c: actor kind $57 of level 3, the guard at the market's end: it
 * waits until Aladdin is right of x $0528, shows itself, then in rounds
 * stands, and when he is near steps at him or walks; stomped it hops back
 * and flashes, with no health left it is knocked away; shot it stops a
 * round.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"
#include "map/map.h"
#include "player/player.h"

/* its own bytes ($11 too: its health, own.bytes[OWN_11]) */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t steps;              /* +$40: frames left of a move */
    uint8_t unused_41;          /* +$41 */
    uint8_t touch;              /* +$42: its wait: 0 touching it hurts Aladdin; positive, he stomps it or is hurt; negative, nothing */
    uint8_t mode;               /* +$43: its wait: 0 in play; 1 still; 2 up, posed by its vertical speed's sign */
    uint8_t faces;              /* +$44: its wait: nonzero, it turns to face Aladdin */
} K57Own;

static inline K57Own *k57(Actor *a) { return (K57Own *)a->own.bytes; }
static inline uint8_t *k57_health(Actor *a) { return &a->own.bytes[OWN_11]; }   /* +$11: stomps left */

enum {
    WAIT_PLAYER = ACTOR_RESUME_OWN, /* until Aladdin is right of x $0528 */
    SHOW_POSE2,                 /* after $18 frames: pose 2 */
    SHOW_POSE1,                 /* after $28: pose 1 */
    ROUND,                      /* after $90, and the rounds' rests: a round */
    LOOK_PAUSE,                 /* after the stand's $18 frames: a frame more */
    LOOK,                       /* each frame: looks for him */
    STEP_WIND,                  /* after pose 5's $18 frames */
    STEP,                       /* the step at him */
    WALK,                       /* the walk */
    HURT,                       /* hurt: walks back */
    STOMPED,                    /* after pose 7's 8 frames */
    HOP_1,                      /* the first hop back */
    HOP_2,                      /* the second */
    FLASH,                      /* flashes $20 frames */
    KNOCKED                     /* knocked away, for good */
};

#define SHOW_X 0x0528           /* Aladdin's x (pixels) it waits for: high byte 5 and low $28 up */
#define WALK_SPEED 0x33         /* ActorSpeedXSet's entry for its walk */
#define STEP_SPEED 0x3F         /* and for its step at him */

/* One frame of a move: across, out of a wall at probe 4, onto the ground. */
static void move_frame(Actor *a)
{
    ActorMoveX(a);
    (void)ActorOutOfWallProbe4(a);
    (void)ActorSnapToGround(a, NULL);
}

/*
 * @routine 83:DA14 Kind57Round
 * WHAT IT DOES: A round of kind $57: it stands facing Aladdin (pose 3, 24
 * frames); then each frame while he has health it looks for him: near in
 * height (span 8) and across (span 6) it steps at him (pose 5 for $18
 * frames, pose 6, sound $20, 8 frames of moving), rests 32 frames and
 * starts a round again; else it walks (pose 4, 48 frames) and starts a
 * round again. (Its start: this frame's part, up to the stand's wait.)
 */
static int round_start(Actor *a)
{
    k57(a)->faces = 1;
    k57(a)->touch = 0;
    AnimStart(a, 3);
    a->resume = LOOK_PAUSE;
    return 0x18;
}

/*
 * @routine 83:DAA0 Kind57Hurt
 * WHAT IT DOES: Kind $57 hurt: it walks back in pose $0C for 128 frames
 * with sound $21, facing Aladdin; again while the scene's cue at $00C0 is
 * set; then a round again. (Its start: this frame's part.)
 */
static int hurt_start(Actor *a)
{
    k57(a)->faces = 1;
    k57(a)->touch = 0;
    ActorSpeedXSet(a, WALK_SPEED);
    k57(a)->steps = 0x80;
    SoundQueueAdd(0x21);
    AnimStart(a, 0x0C);
    a->resume = HURT;
    return 1;
}

/*
 * @routine 83:D9BA Kind57Start
 * WHAT IT DOES: Actor kind $57: anim_flags bit 1, its wait routine on
 * (flags $90), redrawn, 4 stomps of health, its probe list and its own
 * picture list, facing right, touching hurts, not turning, in play; pose
 * 0 until Aladdin's x is $0528-$05FF (high byte 5, low $28 up); then
 * after $18 frames pose 2, after $28 pose 1, after $90 its rounds
 * (Kind57Round).
 * Its second entry (83:DB43 Kind57Second, stomped) and third (83:DB9A
 * Kind57Third, shot) are below in the same function.
 */
int Kind57Start(Actor *a)
{
    K57Own *k = k57(a);
    uint16_t px;
    int r;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->redraw = 0xFF;
        *k57_health(a) = 4;
        a->probes = k57_probes();
        a->own_pictures = k57_own_pictures();
        a->facing = 0;
        a->flip = 0;
        k->touch = 0;
        k->faces = 0;
        k->mode = 0;
        AnimStart(a, 0);
        a->resume = WAIT_PLAYER;
        return 1;
    case WAIT_PLAYER:
        px = pixels(g.actors[0].x);
        if (px >> 8 != SHOW_X >> 8 || (px & 0xFF) < (SHOW_X & 0xFF))
            return 1;
        a->resume = SHOW_POSE2;
        return 0x18;
    case SHOW_POSE2:
        AnimStart(a, 2);
        a->resume = SHOW_POSE1;
        return 0x28;
    case SHOW_POSE1:
        AnimStart(a, 1);
        a->resume = ROUND;
        return 0x90;
    case ROUND:
        return round_start(a);
    case LOOK_PAUSE:
        a->resume = LOOK;
        return 1;
    case LOOK:
        if (!g.health)
            return 1;
        if (ActorNearPlayerY(a, 0x08) && ActorNearPlayerX(a, 0x06)) {
            k->touch = 1;
            AnimStart(a, 5);
            a->resume = STEP_WIND;
            return 0x18;
        }
        k->steps = 0x30;
        ActorSpeedXSet(a, WALK_SPEED);
        AnimStart(a, 4);
        k->faces = 0;
        a->resume = WALK;
        return 1;
    case STEP_WIND:
        k->faces = 0;
        AnimStart(a, 6);
        SoundQueueAdd(0x20);
        k->steps = 0x08;
        ActorSpeedXSet(a, STEP_SPEED);
        a->resume = STEP;
        return 1;
    case STEP:
        move_frame(a);
        if (--k->steps)
            return 1;
        a->resume = ROUND;
        return 0x20;
    case WALK:
        move_frame(a);
        if (--k->steps)
            return 1;
        return round_start(a);
    case HURT:
        move_frame(a);
        if (--k->steps)
            return 1;
        if (g.scene_cue_2)
            return hurt_start(a);
        return round_start(a);

    /*
     * @routine 83:DB43 Kind57Second
     * WHAT IT DOES: Kind $57 stomped (its second entry): with no health
     * left g.scene_cue_first ($00BD) is counted up and it is knocked away
     * (ActorKnockedAway: pose 9, sound $45, preset 3). Else: still (mode
     * 1), sound $0F, its wait routine on, state $0C (the yield's wait
     * replaces it at once, as in the original), pose 7 for 8 frames; up
     * (mode 2), hops back by the hop table's entries $23 and $28 (sound $22
     * between); mode 1, flashing $20 frames (anim_flags bit 3 turned each
     * frame, then set); in play, and on hurt (Kind57Hurt).
     */
    case ACTOR_START_SECOND:
        if (!*k57_health(a)) {
            g.scene_cue_first++;
            a->resume = KNOCKED;
            return ActorKnockedAway(a, 0x09, 0x45, 0x03);
        }
        k->mode++;
        SoundQueueAdd(0x0F);
        a->flags |= 0x80;
        a->state = ACTOR_STATE_NEW;
        AnimStart(a, 7);
        a->resume = STOMPED;
        return 0x08;
    case STOMPED:
        k->mode++;
        a->resume = HOP_1;
        __attribute__((fallthrough));   /* the hop starts this frame */
    case HOP_1:
        r = ActorHopFromTable(a, 0x23);
        if (r != ACTOR_RETURNS)
            return r;
        SoundQueueAdd(0x22);
        a->resume = HOP_2;
        __attribute__((fallthrough));
    case HOP_2:
        r = ActorHopFromTable(a, 0x28);
        if (r != ACTOR_RETURNS)
            return r;
        k->mode--;
        k->steps = 0x20;
        a->resume = FLASH;
        return 1;
    case FLASH:
        a->anim_flags ^= 0x08;
        if (--k->steps)
            return 1;
        a->anim_flags |= 0x08;
        k->mode = 0;
        return hurt_start(a);
    case KNOCKED:
        return ActorKnockedAway(a, 0x09, 0x45, 0x03);

    /*
     * @routine 83:DB9A Kind57Third
     * WHAT IT DOES: Kind $57 shot (its third entry): sound $17; while the
     * scene's cue at $00C0 is set it goes on hurt (Kind57Hurt); else it
     * stands in pose $0A for 32 frames, not turning, touching it doing
     * nothing, and a round again.
     */
    case ACTOR_START_THIRD:
        SoundQueueAdd(0x17);
        if (g.scene_cue_2)
            return hurt_start(a);
        k->faces = 0;
        k->touch = 0xFF;
        AnimStart(a, 0x0A);
        a->resume = ROUND;
        return 0x20;
    default:
        return 1;
    }
}

/*
 * @routine 83:DAD6 Kind57Wait
 * WHAT IT DOES: Kind $57's wait: g.scripted_cue ($146C) = 1
 * when actors[1] is at or right of it (pixels), else 0. In play (mode 0):
 * animated and its picture sent; in animation $0C (offset $18), or $06 at
 * its first frame, Aladdin in its zone $38 is hurt (turned from it); a
 * shot that reaches it puts it in state $CC (ActorHitByShotCC); turning,
 * it faces him; touching it, by +$42: 0 hurts him, positive he stomps it
 * or is hurt, negative nothing. Still (mode 1): animated and drawn. Up
 * (mode 2): offset $10 (vertical speed negative) or $16, started (as
 * animation offset / 2) when its frame is not that number, then animated
 * and drawn.
 */
int Kind57Wait(Actor *a)
{
    K57Own *k = k57(a);
    uint8_t pose;

    g.scripted_cue = pixels(g.actors[1].x) >= pixels(a->x);
    if (k->mode == 0) {
        AnimStep(a);
        ActorPictureSend(a);
        if ((a->anim == 0x18 || (a->anim == 0x0C && a->frame == 0)) && ActorZoneHasTestBox(a, 0x38))
            (void)PlayerEnter8CFacing(a);
        (void)ActorHitByShotCC(a);
        if (k->faces) {
            a->facing = ActorSideOfPlayer(a);
            a->flip = a->facing;
        }
        if (k->touch == 0)
            (void)ActorHurtsPlayerOnTouch(a);
        else if (!(k->touch & 0x80))
            (void)ActorStompOrHurt(a);
        return ACTOR_GOES_ON;
    }
    if (k->mode != 1) {
        pose = a->speed_y < 0 ? 0x10 : 0x16;
        if (pose != a->frame)
            AnimStart(a, pose >> 1);
    }
    AnimStep(a);
    ActorPictureSend(a);
    return ACTOR_GOES_ON;
}
