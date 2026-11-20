/*
 * kind21.c: actor kind $21: by its +$07, one of two figures that play out
 * a little scene, or a third that stands facing left.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "data/kinds_e_tables.h"

/* its own bytes: +$40, the frames left of a step */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40 */
} Kind21Own;

static inline Kind21Own *own(Actor *a) { return (Kind21Own *)a->own.bytes; }

enum {
    STAY = ACTOR_RESUME_OWN,
    P0_WAIT,                    /* part 0: each of its 64 first frames */
    P0_WALK,                    /* each frame of its walk left */
    P0_POSE,                    /* each frame of pose 3's 128 */
    P0_TURN,                    /* each frame of pose $1E's 16 */
    P0_RUN,                     /* each frame of its walk right */
    P1_CLEAR,                   /* part 1: waiting for bit 7 of Aladdin's x to set */
    P1_SET,                     /* waiting for it to clear */
};

/* the speed's pixel byte (+$1A) set to `px`, the other two kept */
static void speed_x_pixels(Actor *a, uint8_t px)
{
    a->speed_x = signed24(((uint32_t)a->speed_x & ~0xFF00u) | (uint32_t)px << 8);
}

static void set_facing(Actor *a, uint8_t f)
{
    a->facing = f;
    a->flip = f;
}

/* The low byte of Aladdin's x pixels has bit 7 set. */
static int player_x_bit7(void)
{
    return (pixels(g.actors[0].x) & 0x80) != 0;
}

static int p1_set(Actor *a);

/* part 1, pose $0F: every 8 frames until bit 7 of Aladdin's x sets */
static int p1_clear(Actor *a)
{
    if (!player_x_bit7()) {
        a->resume = P1_CLEAR;
        return 8;
    }
    AnimStart(a, 0x04);
    return p1_set(a);
}

/* part 1, pose 4: every 8 frames until it clears */
static int p1_set(Actor *a)
{
    if (player_x_bit7()) {
        a->resume = P1_SET;
        return 8;
    }
    AnimStart(a, 0x0F);
    return p1_clear(a);
}

/* `n` frames of a step from the next frame, at resume point `next` */
static int frames_of(Actor *a, uint8_t n, int next)
{
    own(a)->frames = n;
    a->resume = (uint16_t)next;
    return 1;
}

/*
 * @routine A5:F309 Kind21Start
 * WHAT IT DOES: Actor kind $21: by its +$07, part 0 waits 64 frames, walks
 * left for 128, poses (3) for 128, counts g.screen_signal up, turns right
 * (pose $1E) for 16 frames, then walks right faster (2) for 128 and stays;
 * part 1 watches Aladdin's x: pose $0F while bit 7 of its low byte is
 * clear, pose 4 while it is set, checking every 8 frames; part 2 takes
 * pose 0 facing left and stays. Each frame of a step is a one-frame wait,
 * so its wait routine runs only in part 1's waits of 8.
 * Its start sets +$08 bit 1, its wait routine on ($09 bits $90), +$28 $FF,
 * its look $0220, its picture list (+$29) and +$16 to $D06A: a probe list's
 * place no routine reads for this kind (it neither probes the map nor is
 * probed), not kept.
 */
int Kind21Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->redraw = 0xFF;
        a->look = 0x0220;
        a->own_pictures = k21_pictures();
        switch (a->spawn_byte) {
        case 0:
            g.screen_signal = 0;
            AnimStart(a, 0x01);
            set_facing(a, 1);
            a->speed_x = 0x100;            /* 1 pixel a frame */
            return frames_of(a, 0x40, P0_WAIT);
        case 1:
            set_facing(a, 0);
            AnimStart(a, 0x0F);
            return p1_clear(a);
        case 2:
            AnimStart(a, 0x00);
            own(a)->frames = 0;
            set_facing(a, 1);
            a->resume = STAY;
            return 1;
        default: {
            static int told;

            /* past the table of three parts: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "Kind21Start: part %u (past its table) not converted\n", a->spawn_byte);
            a->resume = STAY;
            return 1;
        }
        }
    case P0_WAIT:
        if (--own(a)->frames)
            return 1;
        return frames_of(a, 0x80, P0_WALK);
    case P0_WALK:
        ActorMoveX(a);
        if (--own(a)->frames)
            return 1;
        AnimStart(a, 0x03);
        return frames_of(a, 0x80, P0_POSE);
    case P0_POSE:
        if (--own(a)->frames)
            return 1;
        g.screen_signal++;
        set_facing(a, 0);
        AnimStart(a, 0x1E);
        return frames_of(a, 0x10, P0_TURN);
    case P0_TURN:
        if (--own(a)->frames)
            return 1;
        AnimStart(a, 0x02);
        speed_x_pixels(a, 0x03);
        return frames_of(a, 0x80, P0_RUN);
    case P0_RUN:
        ActorMoveX(a);
        if (--own(a)->frames)
            return 1;
        a->resume = STAY;
        return 1;
    case P1_CLEAR:
        return p1_clear(a);
    case P1_SET:
        return p1_set(a);
    default:                            /* STAY */
        return 1;
    }
}
