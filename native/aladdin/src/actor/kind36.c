/*
 * kind36.c: actor kind $36, a figure of the menu and story screens that
 * stands, follows a cursor, or walks, by its +$07.
 */
#include <stdio.h>

#include "actor/actor_own.h"

enum {
    WALK_SET = ACTOR_RESUME_OWN,    /* after the walk's first wait: a frame before it moves */
    WALK,                           /* each frame of the walk */
    STAY,
    PUFF_WAIT,                      /* move 1: after pose 3's 16 frames */
    RISE_SET,                       /* move 1: after pose 4's 16 frames */
    RISE,                           /* move 1: each frame of the rise, for good */
    PICK_WAIT,                      /* menu parts: each frame until the pick is made */
    SPEED_OFF,                      /* menu parts: each frame of the speed-off */
};

/*
 * 83:B0C7, kind $36's and $9D's second entry (state $80-$BF): a JML to
 * ActorEndPlaced, so the actor ends.
 */
int Kind36End(Actor *a)
{
    return ActorEndPlaced(a);
}

/*
 * @routine 83:AF8E Kind36Start
 * WHAT IT DOES: Actor kind $36, a figure of the menu and story screens. Its
 * wait routine on (flags $90), facing right. With +$07 bit 4 set (the
 * opening story's) it walks for $16 frames after $18 frames of waiting, by
 * bits 0-2: 0 (turned round with bit 3, drawn in list 3) and stays; 1 with
 * pose 1, then sound $42, a puff (kind $85, ActorCreateChild8), poses 3 and
 * 4 for 16 frames each, then rises for good; 2 turned round with pose 2,
 * then ends. With bit 4 clear it plays one of six parts the menus use
 * 4, 6, 8 and $A (the Genie's screen's figures, +$07 as the record's table
 * indexes it): the pose (4 facing left, 6 right, 8 left with pose 2, $A pose
 * 1, the others pose 0) until the Genie's pick (g.genie_pick) is made, then
 * a speed-off sideways for 13 frames with sound $42, and it stays. Parts 0
 * and 2 (it stands; it follows a cursor) have no placement and no creator
 * in the ROM, and the odd values lie past the table's words: each reports
 * itself and stays if reached.
 */
int Kind36Start(Actor *a)
{
    FigureOwn *f = figure(a);
    uint8_t move = (uint8_t)(a->spawn_byte & 0x07);

    switch (a->resume) {
    case ACTOR_START:
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        if (!(a->spawn_byte & 0x10)) {
            static int told;

            switch (a->spawn_byte) {
            case 4:
                a->facing = 1;
                a->flip = 1;
                AnimStart(a, 0);
                break;
            case 6:
                AnimStart(a, 0);
                break;
            case 8:
                a->facing = 1;
                AnimStart(a, 2);
                break;
            case 10:
                AnimStart(a, 1);
                break;
            default:
                if (!told++)
                    fprintf(stderr, "Kind36Start: +$07 %u (no placement or creator; or past its table) not converted\n",
                            a->spawn_byte);
                a->resume = STAY;
                return 1;
            }
            a->resume = PICK_WAIT;
            return 1;
        }
        f->frames = 0x16;
        ActorSpeedXSet(a, 0x2D);
        if (move == 0) {
            a->anim_flags |= 0x03;
            if (a->spawn_byte & 0x08) {
                a->facing++;
                a->flip++;
            }
            AnimStart(a, 0);
        } else if (move == 1) {
            AnimStart(a, 1);
        } else if (move == 2) {
            AnimStart(a, 2);
            a->facing++;
        } else {
            static int told;

            /* past the table of three moves: the original jumps where the
               next words point */
            if (!told++)
                fprintf(stderr, "Kind36Start: move %u (past its table) not converted\n", move);
            a->resume = STAY;
            return 1;
        }
        a->resume = WALK_SET;
        return 0x18;
    case WALK_SET:
        a->resume = WALK;
        return 1;
    case WALK:
        ActorMoveX(a);
        if (--f->frames)
            return 1;
        if (move == 0) {
            a->resume = STAY;
            return 1;
        }
        if (move == 2)
            return ActorEndPlaced(a);
        SoundQueueAdd(0x42);
        a->spawn_at = 0;
        (void)ActorCreateChild8(a, 0x85);
        AnimStart(a, 3);
        a->resume = PUFF_WAIT;
        return 0x10;
    case PICK_WAIT:
        if (!g.genie_pick)
            return 1;
        a->speed_x = 0;
        ActorSpeedXSetAccel(a, 0x40, 0x60);
        f->frames = 0x0D;
        a->resume = SPEED_OFF;
        return 1;
    case SPEED_OFF:
        ActorAccelerateX(a);
        ActorMoveX(a);
        if (--f->frames)
            return 1;
        SoundQueueAdd(0x42);
        a->resume = STAY;
        return 1;
    case PUFF_WAIT:
        ActorSpeedYSet(a, 0x57);
        AnimStart(a, 4);
        a->resume = RISE_SET;
        return 0x10;
    case RISE_SET:
        a->resume = RISE;
        return 1;
    case RISE:
        ActorMoveY(a);
        return 1;
    default:                            /* STAY */
        return 1;
    }
}
