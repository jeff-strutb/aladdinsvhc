/*
 * ground_mark.c: actor kind $60, the mark at Aladdin's feet while the
 * ground pushes him or he slides on it, shown only then (the record's
 * player/ground_push_marks.c). Kind $5F's use of the same code (its third
 * state $4C) is left aside with its other entries (kinds-d-shared-kinds.md,
 * section 2).
 */
#include "actor/actor_own.h"
#include "player/player_own.h"

#define MARK_ANIM_WANTED(a) ((a)->own.bytes[OWN_40])    /* +$40: the animation it is to show */
#define MARK_ANIM(a) ((a)->own.bytes[OWN_41])           /* +$41: the animation it shows */

enum { MARK_FRAME = ACTOR_RESUME_OWN };

/*
 * @routine 83:E480 GroundPushMark
 * WHAT IT DOES: Kind $60: a mark that follows Aladdin's feet (his x, 24
 * below his y) for good; shown animated only while he is pushed or slides:
 * animation 9 while the ground pushes him (2), $0A while he slides (1).
 * Its wait routine (83:E4E7 ActorWaitAnimB) steps its animation.
 */
int Kind60Start(Actor *a)
{
    const Actor *p = &g.actors[0];
    uint8_t anim;

    if (a->resume == ACTOR_START) {
        a->flags |= 0x90;
        a->look = 0x0600;
        MARK_ANIM_WANTED(a) = 0x0A;
        MARK_ANIM(a) = 0x0A;
        AnimStart(a, 0x0A);
        a->anim_flags = (uint8_t)((a->anim_flags | 0x01) & 0xF7);
        a->resume = MARK_FRAME;
        return 1;
    }
    set_x_pixels(a, x_pixels(p));
    set_y_pixels(a, (uint16_t)(y_pixels(p) + 0x0018));
    if (!g.player_ground_pushed) {
        a->anim_flags &= 0xF7;
        return 1;
    }
    a->anim_flags |= 0x08;
    anim = g.player_ground_pushed == 1 ? 0x0A : 0x09;      /* sliding, pushed */
    MARK_ANIM_WANTED(a) = anim;
    if (anim != MARK_ANIM(a)) {
        MARK_ANIM(a) = anim;
        AnimStart(a, anim);
    }
    return 1;
}
