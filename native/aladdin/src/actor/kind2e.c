/*
 * kind2e.c: actor kind $2E, the figures of level 5's closing scene (the
 * scene's task places them): by its +$07, one that walks a few steps on a
 * cue, starts the window waves and sinks away; one that stands; one that
 * keeps beside Aladdin on a cue; one that turns up at its place on another.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "data/kinds_e_tables.h"

/* its own bytes: part 0's +$40 and +$41 */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t face;               /* +$40: part 0's facing, flipped each step; then the frames left of its sink */
    uint8_t steps;              /* +$41: part 0's steps left */
} Kind2EOwn;

static inline Kind2EOwn *own(Actor *a) { return (Kind2EOwn *)a->own.bytes; }

enum {
    STAY = ACTOR_RESUME_OWN,
    P0_CUE,                     /* part 0: each frame until g.screen_signal is set */
    P0_STEP,                    /* each of its six steps */
    P0_SINK_SET,                /* after its 42 frames' wait */
    P0_SINK,                    /* each frame of its 144 down */
    P2_CUE,                     /* part 2: each frame until g.scene_cue is set */
    P2_BESIDE,                  /* each frame beside Aladdin */
    P3_CUE,                     /* part 3: each frame until g.scene_cue_2 is set */
};

static void set_facing(Actor *a, uint8_t f)
{
    a->facing = f;
    a->flip = f;
}

static void place(Actor *a, uint16_t x, uint16_t y)
{
    a->x = with_pixels(a->x, x);
    a->y = with_pixels(a->y, y);
}

/* part 2: at Aladdin's place plus the offsets his frame picks */
static int beside(Actor *a)
{
    const Actor *p = &g.actors[0];
    uint8_t at = (uint8_t)(p->frame << 1);

    a->x = with_pixels(a->x, (uint16_t)(k2e_beside_x(at) + pixels(p->x)));
    a->y = with_pixels(a->y, (uint16_t)(k2e_beside_y(at) + pixels(p->y)));
    return 1;
}

/*
 * @routine 83:AADC Kind2EStart
 * WHAT IT DOES: Actor kind $2E, a figure of level 5's closing scene, by its
 * +$07: 0 faces right and, once g.screen_signal is set (cleared as it
 * starts), takes six steps across at speed 2 turning each step (its facing
 * only), starts window effect 6 (slot 6, parameter $0C), waits 42 frames
 * and sinks away at speed 1 for 144; 1 faces left and stays; 2 faces left
 * and, on g.scene_cue, is put at ($0E80, $0148) and then each frame keeps
 * beside Aladdin by the offsets his frame picks; 3 faces left in pose 1
 * and, on g.scene_cue_2, turns up at ($0DA0, $0238) in pose 2 with sound 5.
 * Each part waits for its cue a frame at a time, from the next frame.
 * Its start sets +$08 bit 1, its wait routine on ($09 bits $90), and +$16
 * to $D14E: a probe list's place no routine reads for this kind (it neither
 * probes the map nor is probed), not kept.
 */
int Kind2EStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        switch (a->spawn_byte) {
        case 0:
            set_facing(a, 1);
            own(a)->face = 0;
            AnimStart(a, 0x00);
            g.screen_signal = 0;
            a->resume = P0_CUE;
            return 1;
        case 1:
            set_facing(a, 0);
            AnimStart(a, 0x00);
            a->resume = STAY;
            return 1;
        case 2:
            set_facing(a, 0);
            AnimStart(a, 0x00);
            a->resume = P2_CUE;
            return 1;
        case 3:
            set_facing(a, 0);
            AnimStart(a, 0x01);
            a->resume = P3_CUE;
            return 1;
        default: {
            static int told;

            /* past the table's four parts: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "Kind2EStart: part %u (past its table) not converted\n", a->spawn_byte);
            a->resume = STAY;
            return 1;
        }
        }
    case P0_CUE:
        if (!g.screen_signal)
            return 1;
        a->speed_x = 0x200;            /* 2 pixels a frame */
        own(a)->steps = 6;
        a->resume = P0_STEP;
        return 1;
    case P0_STEP:
        ActorMoveX(a);
        own(a)->face ^= 0x01;
        a->facing = own(a)->face;
        if (--own(a)->steps)
            return 1;
        TaskCreate(6, 0x0C, TASK_ENTRY_WINDOW_EFFECT);
        a->resume = P0_SINK_SET;
        return 0x2A;
    case P0_SINK_SET:
        own(a)->face = 0x90;
        a->speed_y = 0x100;            /* 1 pixel a frame */
        a->resume = P0_SINK;
        return 1;
    case P0_SINK:
        ActorMoveY(a);
        if (--own(a)->face)
            return 1;
        a->resume = STAY;
        return 1;
    case P2_CUE:
        if (!g.scene_cue)
            return 1;
        place(a, 0x0E80, 0x0148);
        a->resume = P2_BESIDE;
        return 1;
    case P2_BESIDE:
        return beside(a);
    case P3_CUE:
        if (!g.scene_cue_2)
            return 1;
        place(a, 0x0DA0, 0x0238);
        AnimStart(a, 0x02);
        SoundQueueAdd(0x05);
        a->resume = STAY;
        return 1;
    default:                            /* STAY */
        return 1;
    }
}

/*
 * @routine 83:ABD8 ActorWaitEndLowKinds
 * WHAT IT DOES: Kind $2E's wait routine: each frame it waits, its
 * animation on; a figure whose +$07 has a low nibble below 2 (parts 0 and
 * 1) ends once out of range by box 0.
 */
int ActorWaitEndLowKinds(Actor *a)
{
    AnimStep(a);
    if ((a->spawn_byte & 0x0F) >= 2)
        return ACTOR_GOES_ON;
    return ActorEndOutOfRange(a, 0x00);
}
