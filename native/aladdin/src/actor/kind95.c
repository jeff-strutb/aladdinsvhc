/*
 * kind95.c: actor kind $95, figures of the opening story's scenes: by its
 * +$07 one of fifteen parts, each a short piece of motion (a fall, a
 * fly-past, a bounce, hops in place) with its poses and sounds, some
 * starting the story's text task.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"

#define STORY_TEXT_SLOT 4      /* the story's text task's slot */

enum {
    STAY = ACTOR_RESUME_OWN,
    P0_SPEED,       /* part 0: after pose 0's 16 frames */
    P0_RUN,         /* each frame of a pose's 16 */
    P2_SET,         /* part 2: after its 40 frames */
    P2_RUN,
    P2_FALL,
    P4_SET,         /* part 4: after its 136 frames */
    P4_RUN,
    PAGE_THEN_6,    /* part 3: each frame until the story's page is down */
    HOPS_SET,       /* the hops: after their first wait */
    HOP_UP,
    HOP_DOWN,
    P8_TEXT,        /* part 8: after its 240 frames */
    PAGE_THEN_FLY,
    P8_FLY,
    P8_STEPS,       /* after its 128 frames */
    STEP_FLY,       /* each frame of a step's flight */
    STEP_BACK,      /* step 3: each frame of its 96 back */
    STEP_POSE,      /* step 3: after its 96 frames' wait */
    P12_SOUND,
    P13_SOUND,
    P13_AGAIN,
    P14_HIDDEN,     /* part 14: each frame of its $06F0 */
};

static int pose_wait(Actor *a, uint8_t pose, uint8_t frames, int next)
{
    AnimStart(a, pose);
    a->resume = (uint16_t)next;
    return frames;
}

/* Part 0's speed-off, from pose `n`: 16 frames of it. */
static int p0_pose(Actor *a, uint8_t n)
{
    figure(a)->step = n;
    AnimStart(a, n);
    figure(a)->frames = 0x10;
    a->resume = P0_RUN;
    return 1;
}

/* The speed's low 16 bits as the original's word at +$19 or +$1D. */
static uint16_t low16(int32_t speed) { return (uint16_t)speed; }

/* The hops of `set` (84:9E78 Kind95Hops): its pose, a wait, then its count
   of hops, each a rise and a fall for the set's frames at the set's
   speeds; then the part goes on (parts 5-7 stay; part 8 goes on). */
static int hops_start(Actor *a, uint8_t set)
{
    Kind95Hops h = k95_hops(set);

    figure(a)->hop = set;
    AnimStart(a, h.pose);
    figure(a)->step = h.count;
    a->resume = HOPS_SET;
    return h.delay ? h.delay : 256;
}

static int hop_up(Actor *a)
{
    Kind95Hops h = k95_hops((uint8_t)(figure(a)->hop >> 1));

    figure(a)->frames = h.up_frames;
    ActorSpeedYFrom84(a, h.up_speed);
    a->resume = HOP_UP;
    return 1;
}

static int hops_done(Actor *a)
{
    if ((a->spawn_byte & 0x0F) == 8) {
        a->resume = P8_TEXT;
        return 0xF0;
    }
    a->resume = STAY;
    return 1;
}

/* Part 8's flight in steps: pose `n`, then the step's frames flying with
   the speed's fraction held to $E0. */
static int step_pose(Actor *a, uint8_t n)
{
    AnimStart(a, n);
    figure(a)->frames = k95_flight_frames(figure(a)->step);
    a->resume = STEP_FLY;
    return 1;
}

/* What follows a step of part 8, by the step just flown. */
static int step_after(Actor *a)
{
    uint8_t was = figure(a)->step;

    if (was < 5)
        figure(a)->step++;
    switch (was) {
    case 0:
        return step_pose(a, 0x0A);
    case 1:
        a->speed_x = (int32_t)(((uint32_t)a->speed_x & ~0xFFu) | 0x10);
        return step_pose(a, 0x0B);
    case 2:
        a->speed_x = (int32_t)(((uint32_t)a->speed_x & ~0xFFu) | 0x20);
        return step_pose(a, 0x0C);
    case 3:
        a->anim_flags |= 0x03;
        figure(a)->frames = 0x60;
        a->resume = STEP_BACK;
        return 1;
    case 4:
        a->anim_flags = (uint8_t)((a->anim_flags & 0xFC) ^ 0x02);
        return step_pose(a, 0x0E);
    default:
        return ActorEndPlaced(a);
    }
}

/* The start: shown in front, its wait routine on, facing right; unless
   +$07 bit 4 is set, palette 5 (look bits 9-11). */
static int start(Actor *a)
{
    uint8_t part = (uint8_t)(a->spawn_byte & 0x0F);

    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    a->facing = 0;
    a->flip = 0;
    if (!(a->spawn_byte & 0x10))
        a->look = (a->look & ~0x0E00u) | 0x0A00;
    switch (part) {
    case 0:
        return pose_wait(a, 0x00, 0x10, P0_SPEED);
    case 1:
        a->facing++;
        return pose_wait(a, 0x00, 0x28, P2_SET);
    case 2:
        return pose_wait(a, 0x00, 0x28, P2_SET);
    case 3:
        a->facing++;
        return pose_wait(a, 0x00, 0x88, P4_SET);
    case 4:
        return pose_wait(a, 0x00, 0x88, P4_SET);
    case 5:
    case 6:
    case 7:
        return hops_start(a, (uint8_t)(part - 5));
    case 8:
        return hops_start(a, 3);
    case 9:
        AnimStart(a, 0x08);
        a->resume = STAY;
        return 1;
    case 10:
        AnimStart(a, 0x05);
        a->resume = STAY;
        return 1;
    case 11:
        AnimStart(a, 0x06);
        a->resume = STAY;
        return 1;
    case 12:
        a->resume = P12_SOUND;
        return 0x10;
    case 13:
        a->resume = P13_SOUND;
        return 0x2D;
    case 14:
        a->flags &= (uint8_t)~0x80;
        a->x = with_pixels(a->x, 0x0468);
        a->y = with_pixels(a->y, 0x0116);
        figure(a)->frames = 0xF0;       /* $06F0 frames: +$40 the low byte, +$41 the high */
        figure(a)->step = 0x06;
        a->resume = P14_HIDDEN;
        return 1;
    default: {
        static int told;

        /* 15: past the table of fifteen parts; no +$07 names it */
        if (!told++)
            fprintf(stderr, "Kind95Start: part 15 (past its table) not converted\n");
        a->resume = STAY;
        return 1;
    }
    }
}

/*
 * @routine 84:9BE3 Kind95Start
 * WHAT IT DOES: Actor kind $95, a figure of the story's scenes: by its +$07
 * (low four bits) one of fifteen parts: a figure that speeds off through
 * four poses (0, or 14 after a long wait off screen); one that bounces
 * along and falls (2, 1 facing left); one that flies up (4, 3 facing left,
 * which then starts the story's text task with scene 6); hops in place,
 * then still (5-7); hops, the text task with scene 1, then a flight in six
 * steps (8); a still pose (9-11); a sound after 16 frames (12); a sound
 * every $12C frames (13). Unless its +$07 bit 4 is set it takes palette 5.
 * The parts that end do so by ActorEndPlaced. (Reached through 83:93A3
 * Kind95Enter; 84:9E78 Kind95Hops is its hops.)
 */
int Kind95Start(Actor *a)
{
    FigureOwn *f = figure(a);

    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case P0_SPEED:
        SoundQueueAdd(0x43);
        a->accel_x = 0x40;
        ActorSpeedXFrom84(a, 0x0C);
        ActorSpeedYFrom84(a, 0x0F);
        return p0_pose(a, 0x01);
    case P0_RUN:
        ActorSpeedUpMove(a);
        if (--f->frames)
            return 1;
        if (f->step + 1 < 0x05)
            return p0_pose(a, (uint8_t)(f->step + 1));
        return ActorEndPlaced(a);
    case P2_SET:
        f->frames = 0xD8;
        ActorSpeedYFrom84(a, 0x1E);
        ActorSpeedXFrom84(a, 0x21);
        a->accel_x = 0x09;
        a->resume = P2_RUN;
        return 1;
    case P2_RUN:
        ActorSpeedUpMove(a);
        if (a->speed_x >= 0 && low16(a->speed_x) >= 0x00C0) {
            /* bounced: the vertical speed's word negated and its high byte
               turned over, as the original does it */
            uint32_t v = (uint32_t)a->speed_y;

            a->speed_y = signed24((((v >> 16) ^ 0xFF) & 0xFF) << 16 | (uint16_t)-low16(a->speed_y));
            ActorSpeedXFrom84(a, 0x21);
            a->facing ^= 0x01;
        }
        if (--f->frames)
            return 1;
        ActorSpeedYFrom84(a, 0x15);
        f->frames = 0x58;
        a->resume = P2_FALL;
        return 1;
    case P2_FALL:
        ActorMoveY(a);
        if (--f->frames)
            return 1;
        return ActorEndPlaced(a);
    case P4_SET:
        ActorSpeedYFrom84(a, 0x1B);
        ActorSpeedXFrom84(a, 0x12);
        a->accel_y = 0x20;
        f->frames = 0x30;
        a->resume = P4_RUN;
        return 1;
    case P4_RUN:
        ActorFly(a);
        if (a->speed_y >= 0 && low16(a->speed_y) > 0x0200)
            a->speed_y = (int32_t)((uint32_t)a->speed_y >> 16 << 16 | 0x0200);
        if (--f->frames)
            return 1;
        if (a->spawn_byte != 0x03)
            return ActorEndPlaced(a);
        a->resume = PAGE_THEN_6;
        return 1;
    case PAGE_THEN_6:
        if (task_busy(STORY_TEXT_SLOT))  /* the story's text task: its page still up */
            return 1;
        TaskCreate(STORY_TEXT_SLOT, 6, TASK_ENTRY_STORY_TEXT);
        return ActorEndPlaced(a);
    case HOPS_SET:
        f->hop = (uint8_t)(f->hop << 1);    /* from here the set's pair offset, set * 2 */
        return hop_up(a);
    case HOP_UP: {
        Kind95Hops h;

        ActorMoveY(a);
        if (--f->frames)
            return 1;
        h = k95_hops((uint8_t)(f->hop >> 1));
        f->frames = h.down_frames;
        ActorSpeedYFrom84(a, h.down_speed);
        a->resume = HOP_DOWN;
        return 1;
    }
    case HOP_DOWN:
        ActorMoveY(a);
        if (--f->frames)
            return 1;
        if (--f->step)
            return hop_up(a);
        return hops_done(a);
    case P8_TEXT:
        TaskCreate(STORY_TEXT_SLOT, 1, TASK_ENTRY_STORY_TEXT);
        a->resume = PAGE_THEN_FLY;
        return 1;
    case PAGE_THEN_FLY:
        if (task_busy(STORY_TEXT_SLOT))
            return 1;
        ActorMotionPreset(a, 0x61);
        f->frames = 0x1D;
        a->facing++;
        a->resume = P8_FLY;
        return 1;
    case P8_FLY:
        ActorFly(a);
        if (--f->frames)
            return 1;
        a->x = with_pixels(a->x, 0x0090);
        a->y = with_pixels(a->y, 0x0090);
        a->resume = P8_STEPS;
        return 0x80;
    case P8_STEPS:
        a->facing = 0;
        ActorMotionPreset(a, 0x66);
        f->step = 0;
        return step_pose(a, 0x09);
    case STEP_FLY:
        ActorFly(a);
        if ((uint8_t)a->speed_y > 0xE0)
            a->speed_y = (int32_t)(((uint32_t)a->speed_y & ~0xFFu) | 0xE0);
        if (--f->frames)
            return 1;
        return step_after(a);
    case STEP_BACK:
        ActorStepYBack(a);
        if (--f->frames)
            return 1;
        a->resume = STEP_POSE;
        return 0x60;
    case STEP_POSE:
        AnimStart(a, 0x0D);
        a->x = with_pixels(a->x, 0x00A0);
        a->y = with_pixels(a->y, 0x006C);
        a->facing++;
        return step_pose(a, 0x0D);
    case P12_SOUND:
        SoundQueueAdd(0x05);
        return ActorEndPlaced(a);
    case P13_SOUND:
        SoundQueueAdd(0x3E);
        a->resume = P13_AGAIN;
        return 0xFF;
    case P13_AGAIN:
        a->resume = P13_SOUND;
        return 0x2D;
    case P14_HIDDEN: {
        uint16_t left = (uint16_t)((f->frames | f->step << 8) - 1);

        if (left) {
            f->frames = (uint8_t)left;
            f->step = (uint8_t)(left >> 8);
            return 1;
        }
        a->flags |= 0x80;
        return pose_wait(a, 0x00, 0x10, P0_SPEED);
    }
    default:                            /* STAY */
        return 1;
    }
}
