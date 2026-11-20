/*
 * story_actor_9b.c: actor kind $9B, a figure of the story's scenes: by its
 * $07 one of four parts, each moving and posing on the story's cues and
 * starting the story's text task (slot 4) with the next scene's number.
 * The cues are g.story_part1, story_part2, story_part3_done, story_in_place
 * and story_fly (the original's $0076-$007B), and the story's text task
 * (slot 4) being busy ($00CF).
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "system/task.h"

#define FRAMES OWN_40           /* frames left of a step */
#define STORY_SLOT 4            /* the story's text task's slot */

enum {
    P0_WAIT = ACTOR_RESUME_OWN, P0_END,
    P1_A, P1_B, P1_WALK, P1_C, P1_D, P1_E, P1_F, P1_G, P1_H, P1_I, P1_J, P1_K, P1_L, P1_STAY,
    P2_A, P2_FLY1, P2_B, P2_C, P2_D, P2_E, P2_F, P2_G, P2_H, P2_I, P2_J, P2_K, P2_L, P2_M, P2_N, P2_O, P2_P, P2_FLY,
    P3_A, P3_B, P3_C, P3_STAY
};

/* Its pose `n`. */
static void pose(Actor *a, uint8_t n) { AnimStart(a, n); }

/* The step of a flight with the pull reversed that stops its rise: the
   original's A5... 84:A7D4 ActorFlyAgainstNoRise (only kind $9B calls it):
   ActorFlyAgainst, then, its speed upward, its pull and vertical speed
   cleared. */
static void fly_against_no_rise(Actor *a)
{
    ActorFlyAgainst(a);
    if ((a->speed_y >> 16) & 0x80) {
        a->accel_y = 0;
        a->speed_y = 0;
    }
}

static void place(Actor *a, uint16_t x, uint16_t y)
{
    a->x = with_pixels(a->x, x);
    a->y = with_pixels(a->y, y);
}

/*
 * @routine 84:A509 Kind9BStory
 * WHAT IT DOES: Actor kind $9B, a figure of the story's scenes (the
 * ending's): part 0 waits for story_part1 to go negative, poses (8) and
 * ends; part 1 turns, walks, turns back and starts scenes 7, $10 and $11
 * between the story's cues, counting story_part1 up and clearing it at the
 * end; part 2 flies in and out on set motions, starts scenes 8, $0C and
 * $0D, counts story_in_place and story_part2 up, and flies on for good;
 * part 3 waits for story_part2, walks across, starts scene $0F and counts
 * story_part3_done up. Parts 1-3 stay for good at their ends.
 */
int Kind9BStory(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x10;
        a->look &= 0xF1FF;
        a->own.bytes[OWN_42] = 0;
        a->own.bytes[OWN_44] = 0;
        switch (a->spawn_byte) {
        case 0:
            a->look |= 0x0C00;
            a->resume = P0_WAIT;
            return 1;
        case 1:
            a->look |= 0x0800;
            a->anim_flags |= 0x02;
            a->flags |= 0x80;
            pose(a, 1);
            a->resume = P1_A;
            return 1;
        case 2:
            a->look |= 0x0C00;
            a->anim_flags |= 0x02;
            a->flags |= 0x80;
            a->facing++;
            a->flip++;
            pose(a, 4);
            a->resume = P2_A;
            return 1;
        case 3:
            a->look |= 0x0A00;
            a->anim_flags |= 0x02;
            a->flags |= 0x80;
            pose(a, 3);
            a->resume = P3_A;
            return 1;
        default:
            a->resume = P1_STAY;
            return 1;
        }

    case P0_WAIT:
        if (!(g.story_part1 & 0x80))
            return 1;
        a->flags |= 0x80;
        pose(a, 8);
        a->resume = P0_END;
        return 0x30;
    case P0_END:
        return ActorEndPlaced(a);

    /* part 1 */
    case P1_A:
        if (task_busy(STORY_SLOT))
            return 1;
        pose(a, 2);
        a->resume = P1_B;
        return 0x18;
    case P1_B:
        pose(a, 1);
        a->facing++;
        a->flip++;
        a->own.bytes[FRAMES] = 0x20;
        ActorSpeedXSet(a, 0x66);
        a->resume = P1_WALK;
        return 1;
    case P1_WALK:
        ActorMoveX(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->resume = P1_C;
        return 0x20;
    case P1_C:
        pose(a, 2);
        a->resume = P1_D;
        return 0x18;
    case P1_D:
        pose(a, 1);
        a->facing = a->flip = 0;
        TaskCreate(STORY_SLOT, 7, TASK_ENTRY_STORY_TEXT1);
        a->resume = P1_E;
        return 1;
    case P1_E:
        if (task_busy(STORY_SLOT))
            return 1;
        a->resume = P1_F;
        return 0x30;
    case P1_F:
        g.story_part1++;
        a->resume = P1_G;
        return 1;
    case P1_G:
        if (!g.story_part3_done)
            return 1;
        pose(a, 2);
        a->resume = P1_H;
        return 0x18;
    case P1_H:
        pose(a, 1);
        a->flip++;
        a->resume = P1_I;
        return 1;
    case P1_I:
        if (task_busy(STORY_SLOT))
            return 1;
        TaskCreate(STORY_SLOT, 0x10, TASK_ENTRY_STORY_TEXT1);
        a->resume = P1_J;
        return 1;
    case P1_J:
        if (task_busy(STORY_SLOT))
            return 1;
        a->resume = P1_K;
        return 0x20;
    case P1_K:
        pose(a, 2);
        a->resume = P1_L;
        return 0x18;
    case P1_L:
        pose(a, 1);
        a->flip = 0;
        TaskCreate(STORY_SLOT, 0x11, TASK_ENTRY_STORY_TEXT1);
        g.story_part1 = 0;
        a->resume = P1_STAY;
        return 1;
    case P1_STAY:
        return 1;

    /* part 2 */
    case P2_A:
        if (!g.story_part1)
            return 1;
        ActorMotionPreset(a, 0x6E);
        a->own.bytes[FRAMES] = 0x40;
        a->resume = P2_FLY1;
        return 1;
    case P2_FLY1:
        fly_against_no_rise(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->resume = P2_B;
        return 0x20;
    case P2_B:
        TaskCreate(STORY_SLOT, 8, TASK_ENTRY_STORY_TEXT1);
        g.story_in_place++;
        a->resume = P2_C;
        return 1;
    case P2_C:
        if (!(g.story_part1 & 0x80))
            return 1;
        pose(a, 6);
        a->resume = P2_D;
        return 0x20;
    case P2_D:
        pose(a, 7);
        a->resume = P2_E;
        return 0x20;
    case P2_E:
        TaskCreate(STORY_SLOT, 0x0C, TASK_ENTRY_STORY_TEXT1);
        a->resume = P2_F;
        return 1;
    case P2_F:
        if (task_busy(STORY_SLOT))
            return 1;
        pose(a, 5);
        a->resume = P2_G;
        return 0x10;
    case P2_G:
        ActorSpeedYSet(a, 0x87);
        a->own.bytes[FRAMES] = 0x50;
        a->resume = P2_H;
        return 1;
    case P2_H:
        ActorMoveY(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        pose(a, 9);
        place(a, 0x00E0, 0x0030);
        ActorMotionPreset(a, 0x6F);
        a->own.bytes[FRAMES] = 0x60;
        a->resume = P2_I;
        return 1;
    case P2_I:
        fly_against_no_rise(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->resume = P2_J;
        return 0x50;
    case P2_J:
        place(a, 0x0020, 0x0030);
        a->facing = a->flip = 0;
        ActorMotionPreset(a, 0x6F);
        a->own.bytes[FRAMES] = 0x60;
        a->resume = P2_K;
        return 1;
    case P2_K:
        fly_against_no_rise(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->resume = P2_L;
        return 0x30;
    case P2_L:
        place(a, 0x00E8, 0x0050);
        pose(a, 5);
        ActorSpeedXSetAccel(a, 0x40, 0x69);
        a->facing = a->flip = 1;
        a->own.bytes[FRAMES] = 0x20;
        a->resume = P2_M;
        return 1;
    case P2_M:
        ActorMoveX(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->resume = P2_N;
        return 1;
    case P2_N:
        ActorAccelerateXBack(a);
        ActorMoveX(a);
        if (!((a->speed_x >> 16) & 0x80))
            return 1;
        a->resume = P2_O;
        return 0x20;
    case P2_O:
        TaskCreate(STORY_SLOT, 0x0D, TASK_ENTRY_STORY_TEXT1);
        g.story_part2++;
        a->resume = P2_P;
        return 1;
    case P2_P:
        if (g.story_part1 || task_busy(STORY_SLOT))
            return 1;
        g.story_fly[0] = 0x40;
        g.story_fly[1] = 0;
        ActorMotionPreset(a, 0x71);
        a->resume = P2_FLY;
        return 1;
    case P2_FLY:
        fly_against_no_rise(a);
        return 1;

    /* part 3 */
    case P3_A:
        if (!g.story_part2 || task_busy(STORY_SLOT))
            return 1;
        ActorSpeedXSet(a, 0x66);
        a->own.bytes[OWN_44] = 0xFB;
        a->own.bytes[OWN_42]++;
        a->own.bytes[FRAMES] = 0x60;
        a->resume = P3_B;
        return 1;
    case P3_B:
        ActorMoveX(a);
        if (--a->own.bytes[FRAMES])
            return 1;
        a->own.bytes[OWN_42] = 0;
        a->resume = P3_C;
        return 0x20;
    case P3_C:
        TaskCreate(STORY_SLOT, 0x0F, TASK_ENTRY_STORY_TEXT1);
        g.story_part3_done++;
        a->resume = P3_STAY;
        return 1;
    default:                            /* P3_STAY */
        return 1;
    }
}

/*
 * @routine 84:A7AD ActorBob
 * WHAT IT DOES: Bobs the actor up and down a pixel a frame, five frames
 * each way, while its +$42 is nonzero; stops at the bottom once it is 0.
 * Its count (+$44): 0, nothing; negative, y up a pixel and the count up, at
 * 0 it becomes 5; positive, y down a pixel and the count down, at 0 it
 * becomes -5 when +$42 is nonzero. Only kind $9B's wait calls it.
 * Also 84:A7A1 ActorBobAnimate, below: the wait routine of kind $9B (83:93E9
 * ActorKind9BWait): its animation steps, and it bobs while +$42 is nonzero.
 */
static void bob_step(Actor *a)
{
    int8_t count = (int8_t)a->own.bytes[OWN_44];

    if (count < 0) {
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) - 1));
        if (++count == 0)
            count = 5;
    } else if (count) {
        a->y = with_pixels(a->y, (uint16_t)(pixels(a->y) + 1));
        if (--count == 0 && a->own.bytes[OWN_42])
            count = -5;
    }
    a->own.bytes[OWN_44] = (uint8_t)count;
}

int ActorKind9BWait(Actor *a)
{
    AnimStep(a);
    if (a->own.bytes[OWN_42])
        bob_step(a);
    return ACTOR_GOES_ON;
}
