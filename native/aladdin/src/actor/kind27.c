/*
 * kind27.c: actor kind $27, by its +$07: a figure that crosses the view
 * and closes the level (bit 3), a still pose (bit 4), or one that waits for
 * the first to be touched and then flies about (neither); the moving ones
 * leave a trail.
 */
#include <stddef.h>

#include "actor/actor_own.h"
#include "audio/sound.h"
#include "system/system.h"
#include "system/task.h"
#include "video/video.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40: frames left (the still pose: its frames, from its placement) */
    uint8_t step;               /* +$41: the crosser: the palette fade's step; the waiter: rounds left */
    uint8_t open;               /* +$42: nonzero, it can be touched */
} K27Own;

static inline K27Own *k27(Actor *a) { return (K27Own *)a->own.bytes; }

static int Kind27Waiter(Actor *a);

#define FALL_MOST 0x0300u       /* the crosser's fastest fall (16.8, its low two bytes) */

enum {
    END = ACTOR_RESUME_OWN,     /* the still pose's end */
    CROSS_DOWN,                 /* the crosser: flies down */
    CROSS_CIRCLE,               /* circles, 3 frames a direction */
    CROSS_ACROSS,               /* crosses, 12 frames */
    CROSS_FADE_START,           /* after the leaving figure's 32 frames */
    CROSS_FADE,                 /* the palettes fade, 31 frames */
    WAIT_TOUCH,                 /* the waiter: until a crosser is touched */
    WAIT_RISE,                  /* rises, 48 frames */
    WAIT_CIRCLE,                /* circles in five steps, 8 frames a step */
    WAIT_SPEED_UP,              /* speeds up against the pull */
    WAIT_FLY                    /* flies against the pull; slows to the next round */
};

/*
 * @routine 83:9FDA Kind27Trail
 * WHAT IT DOES: Kind $27's trail: every 16 frames of the frame count
 * ($032D) a new actor of its own kind at its spawn point 0, with +$07 $10
 * (the still pose), $20 frames and its own look; on the counts with bit 4
 * clear also sound $37 when it can be hit.
 */
static void Kind27Trail(Actor *a)
{
    Actor *child;

    if (g.sprite_flicker & 0x0F)
        return;
    if (!(g.sprite_flicker & 0x10) && (a->flags & 0x40))
        SoundQueueAdd(0x37);
    a->spawn_at = 0;
    child = ActorSpawn(a, a->kind);
    if (!child)
        return;
    child->spawn_byte = 0x10;
    k27(child)->frames = 0x20;
    child->look = a->look;
}

/* The start of a run of `frames` frames, each its trail and a move. */
static int run_of(Actor *a, uint16_t resume, uint8_t frames)
{
    k27(a)->frames = frames;
    a->resume = resume;
    return 1;
}

/* The bit 3 figure's first frame: at the view's top left, falling. */
static int crosser_start(Actor *a)
{
    g.figures_touched = 0;
    a->facing = 0;
    a->flip = 0;
    k27(a)->open++;
    a->x = with_pixels(a->x, (uint16_t)(pixels(g.layers[0].scroll_x) - 0x10));
    a->y = with_pixels(a->y, (uint16_t)(pixels(g.layers[0].scroll_y) + 0x20));
    AnimStart(a, 0x0C);
    ActorSpeedXSet(a, 0x2A);
    a->accel_y = 0x38;
    ActorSpeedYClear(a);
    return run_of(a, CROSS_DOWN, 0x18);
}

/* The waiter's start of a round: five steps of a circle. */
static int round_start(Actor *a)
{
    a->facing = 4;
    return run_of(a, WAIT_CIRCLE, 0x08);
}

/*
 * @routine 83:9E3A Kind27Start
 * WHAT IT DOES: Actor kind $27: its wait routine on (flags $90), look
 * $0200, facing and flip 1, not open. By +$07:
 * bit 3, a figure that comes in at the view's top left (16 left, 32 down)
 * and flies down for 24 frames (its fall held to $0300 at most, then no
 * pull), circles through the 16 directions, 3 frames each, crosses for 12
 * frames, leaves a figure (kind $85), and after 32 frames clears the
 * level's tables, saves the palettes and fades them over 31 frames, starts
 * the screen's darkening (task slot 2, BrightnessDownTask with 4) and
 * ends; bit 4, a still pose ($0D, anim_flags bit 0) for its +$40 frames,
 * then the end; neither, Kind27Waiter. Each moving frame it leaves its
 * trail (Kind27Trail).
 */
int Kind27Start(Actor *a)
{
    K27Own *k = k27(a);

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        a->look = 0x0200;
        a->facing = 1;
        a->flip = 1;
        k->open = 0;
        if (a->spawn_byte & 0x08)
            return crosser_start(a);
        if (!(a->spawn_byte & 0x10)) {
            a->anim_flags |= 0x02;
            a->resume = WAIT_TOUCH;
            return 1;
        }
        a->anim_flags |= 0x01;
        AnimStart(a, 0x0D);
        a->resume = END;
        return k->frames ? k->frames : 256;
    case ACTOR_START_SECOND:            /* its second entry (83:A03A): a JML to ActorEndPlaced */
    case END:
        return ActorEndPlaced(a);

    case CROSS_DOWN:
        Kind27Trail(a);
        ActorFly(a);
        if ((uint16_t)a->speed_y > 0x0300) {
            /* the speed's low two bytes held to $0300; its top byte kept */
            a->speed_y = signed24((uint32_t)(a->speed_y >> 16) << 16 | FALL_MOST);
            a->accel_y = 0;
        }
        if (--k->frames)
            return 1;
        a->facing = 0;
        return run_of(a, CROSS_CIRCLE, 0x03);
    case CROSS_CIRCLE:
        Kind27Trail(a);
        ActorMoveByVector(a, 0x1A);
        if (--k->frames)
            return 1;
        a->facing = (uint8_t)((a->facing - 1) & 0x0F);
        if (a->facing)
            return run_of(a, CROSS_CIRCLE, 0x03);
        return run_of(a, CROSS_ACROSS, 0x0C);
    case CROSS_ACROSS:
        Kind27Trail(a);
        ActorMove(a);
        if (--k->frames)
            return 1;
        a->spawn_at = 0;
        (void)ActorCreateChild8(a, 0x85);
        a->resume = CROSS_FADE_START;
        return 0x20;
    case CROSS_FADE_START:
        LevelTablesClear();
        PaletteSave(0x00, 0x10);
        k->step = 0;
        return run_of(a, CROSS_FADE, 0x1F);
    case CROSS_FADE:
        PaletteFadeStep((int8_t)k->step, 0x10);
        k->step++;
        if (--k->frames)
            return 1;
        TaskCreate(2, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
        return ActorEndPlaced(a);

    default:
        return Kind27Waiter(a);
    }
}

/*
 * @routine 83:9F2F Kind27Waiter
 * WHAT IT DOES: Kind $27 with neither bit 3 nor bit 4 of +$07: waits
 * (anim_flags bit 1 set, bit 3 cleared each frame) until a bit 3 figure
 * has been touched (g.figures_touched), then with anim_flags bit 3 and
 * pose $0C rises for 48 frames (speed entry $51), is open to touch, and
 * flies three rounds: five steps of a circle, 8 frames each (directions 4
 * down to 0, set $16); then up and away (preset $81) speeding up by $18
 * against the pull while its vertical speed is not negative, then flying
 * against the pull and slowing until its speed's two low bytes add to 0;
 * after the third round it flies against the pull for good. Each moving
 * frame leaves its trail.
 * The original first counts a byte of copier RAM ($70:2000) up and, when
 * it stays written, clears $08F0-$091B of the player's record; on the
 * cartridge that byte is open bus and never reads back, so it is not
 * converted (@unreachable 83:9F50-9F59, the record's).
 */
static int Kind27Waiter(Actor *a)
{
    K27Own *k = k27(a);

    switch (a->resume) {
    case WAIT_TOUCH:
        a->anim_flags &= (uint8_t)~0x08;
        if (!g.figures_touched) {
            a->anim_flags |= 0x02;
            return 1;
        }
        g.figures_touched = 0;
        a->anim_flags |= 0x08;
        AnimStart(a, 0x0C);
        ActorSpeedYSet(a, 0x51);
        return run_of(a, WAIT_RISE, 0x30);
    case WAIT_RISE:
        Kind27Trail(a);
        ActorMoveY(a);
        if (--k->frames)
            return 1;
        k->open++;
        k->step = 3;
        return round_start(a);
    case WAIT_CIRCLE:
        Kind27Trail(a);
        ActorMoveByVector(a, 0x16);
        if (--k->frames)
            return 1;
        if (((a->facing - 1) & 0x07) != 0x07) {
            a->facing = (uint8_t)((a->facing - 1) & 0x07);
            return run_of(a, WAIT_CIRCLE, 0x08);
        }
        a->facing = 0;
        ActorMotionPreset(a, 0x81);
        a->accel_x = 0x18;
        a->resume = WAIT_SPEED_UP;
        return 1;
    case WAIT_SPEED_UP:
        Kind27Trail(a);
        ActorAccelerateX(a);
        ActorFlyAgainst(a);
        if (a->speed_y >= 0)
            return 1;
        a->resume = WAIT_FLY;
        return 1;
    case WAIT_FLY:
        Kind27Trail(a);
        ActorFlyAgainst(a);
        if (!k->step)
            return 1;
        ActorDecelerateX(a);
        if ((uint8_t)(a->speed_x + (a->speed_x >> 8)) != 0)
            return 1;
        k->step--;
        return round_start(a);
    default:
        return 1;
    }
}


