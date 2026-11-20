/*
 * kinda5.c: actor kind $A5 (stage a5 s2), by its $07: 0, the bomb kind $98
 * throws after each hit, which falls to the ground, bursts and ends; 1 to
 * 3, a column of them set off from one of three places across the top, a
 * child a frame; $10 and up, one of the column's pieces, at a height by
 * its low bits, which poses and ends. Its wait routine is ActorKindA5Wait.
 */
#include "actor/actor_own.h"
#include "actor/kinds_d.h"
#include "data/kinds_d_tables.h"

#define KIND 0xA5               /* the column's children are of this kind */
#define PIECE 0x10              /* $07 bit 4: a piece of the column */
#define LAST_PIECE 0x1C         /* the last piece's $07 plus one */

enum {
    BOMB_FALL = ACTOR_RESUME_OWN,   /* the bomb: each frame of its fall */
    BOMB_BURST,                 /* after 8 frames of its burst: it ends */
    COLUMN,                     /* the column: each frame a piece */
    PIECE_POSE                  /* a piece: after $12 frames it ends */
};

/*
 * @routine 84:AB9C KindA5Start
 * WHAT IT DOES: Actor kind $A5, by its $07: 0, a bomb (thrown by kind $98)
 * that falls (speed entry $93) until it is in the ground, then bursts 8
 * pixels lower (animation 5, 8 frames) and ends; 1 to 3, a column set off
 * from the x of entry $07 - 1 of its places, at y 0: a bomb ($07 0) and then
 * a child of this kind a frame ($07 $10 to $1B), then it ends; $10 up, a
 * piece of a column put at the y pixels' low byte its low four bits give,
 * posing (animation $07 AND 3) for $12 frames, then it ends.
 */
int KindA5Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        if (!a->spawn_byte) {
            a->probes = ka5_probes();
            AnimStart(a, 4);
            ActorSpeedYSet(a, 0x93);
            a->resume = BOMB_FALL;
            return 1;
        }
        if (a->spawn_byte & PIECE) {
            a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | ka5_height(a->spawn_byte & 0x0F)));
            AnimStart(a, a->spawn_byte & 0x03);
            a->resume = PIECE_POSE;
            return 0x12;
        }
        a->x = with_pixels(a->x, ka5_place_x((uint8_t)((a->spawn_byte & 0x03) - 1)));
        a->y = with_pixels(a->y, 0);
        a->spawn_at = 0;
        a->spawn_byte = 0;
        (void)ActorCreateChild8(a, KIND);
        a->spawn_byte = PIECE;
        a->resume = COLUMN;
        return 1;
    case BOMB_FALL:
        ActorMoveY(a);
        if (!ActorOutOfGround(a))
            return 1;
        a->y = with_pixels(a->y, (uint16_t)((pixels(a->y) & 0xFF00) | (uint8_t)(pixels(a->y) + 8)));
        AnimStart(a, 5);
        a->resume = BOMB_BURST;
        return 8;
    case COLUMN:
        a->spawn_at = 0;
        (void)ActorCreateChild8(a, KIND);
        if (++a->spawn_byte != LAST_PIECE)
            return 1;
        return ActorEndPlaced(a);
    default:                            /* BOMB_BURST, PIECE_POSE */
        return ActorEndPlaced(a);
    }
}

/*
 * @routine 83:9439 ActorKindA5Wait
 * WHAT IT DOES: Kind $A5's wait routine, through 84:AC31 KindA5WaitWork:
 * its animation steps, Aladdin touching it is hurt, and it is freed once
 * out of range (box $18).
 */
int ActorKindA5Wait(Actor *a)
{
    AnimStep(a);
    (void)ActorHurtsPlayerOnTouch(a);
    return ActorFreeOutOfRange(a, 0x18) ? ACTOR_ENDED : ACTOR_GOES_ON;
}
