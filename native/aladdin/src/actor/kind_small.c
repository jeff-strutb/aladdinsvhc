/*
 * kind_small.c: actor kinds that show one pose and stay, or show it a
 * while and end: $4A and $82 on the title, $85 (kind $36's puff), $96 in
 * the opening story.
 */
#include "actor/actor_own.h"
#include "video/video.h"
#include "data/actor_tables.h"

enum { STAY = ACTOR_RESUME_OWN, K85_END, K96_SECOND_WAIT, K96_SHOW, K96_STAY_C0, K96_STAY_C1 };

/*
 * @routine 84:951D Kind82Show
 * WHAT IT DOES: Actor kind $82: shown in front (anim_flags bit 1), its
 * wait routine on and its priority bits set (flags $B0), animation 0, and
 * it stays. (Reached through 83:92CF Kind82Start.)
 */
int Kind82Show(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->anim_flags |= 0x02;
        a->flags |= 0xB0;
        AnimStart(a, 0);
    }
    a->resume = STAY;
    return 1;
}

/*
 * @routine 83:B496 Kind4AShow
 * WHAT IT DOES: Actor kind $4A: facing right, look $0600, palette set $0A
 * loaded, animation 6, and it stays.
 */
int Kind4AShow(Actor *a)
{
    if (a->resume == ACTOR_START) {
        a->flags |= 0x90;
        a->facing = 0;
        a->flip = 0;
        a->look = 0x0600;
        PaletteLoad(0x0A);
        AnimStart(a, 6);
    }
    a->resume = STAY;
    return 1;
}

/*
 * @routine 84:9602 Kind85Start
 * WHAT IT DOES: Actor kind $85: in front, its wait routine on, sound $36
 * unless its +$07 bit 4 is set, pose 0 for 16 frames, then it ends.
 * (Reached through 83:92F2 Kind85Enter.)
 */
int Kind85Start(Actor *a)
{
    if (a->resume == K85_END)
        return ActorEndPlaced(a);
    a->anim_flags |= 0x02;
    a->flags |= 0x80;
    if (!(a->spawn_byte & 0x10))
        SoundQueueAdd(0x36);
    AnimStart(a, 0);
    a->resume = K85_END;
    return 0x10;
}

/*
 * @routine 84:9EF5 SteppedShowActor
 * WHAT IT DOES: Actor kind $96. With +$07 bit 4 clear: waits the frames
 * its first-wait table gives for +$07, then as long again, then shows
 * itself (sound $44, animation +$07 * 2) and stays, its wait routine
 * playing the animation. Bit 4 set: shown at once (animation 1) with its
 * priority bits set, and stays. (Reached through 83:93AC Kind96Start.)
 * It stays by COP: the first with A = $FF as AnimStart leaves it, each
 * after with A as the actor loop leaves it, the P byte the first COP
 * saved: N and Z from the data bank $80 (AnimStart's PLB), M and X 8-bit,
 * the carry of AnimStart's last add (the picture's place). So $B0 or $B1.
 */
int SteppedShowActor(Actor *a)
{
    uint8_t wait;

    switch (a->resume) {
    case ACTOR_START:
        a->look &= (uint16_t)~0x0E00;
        if (a->spawn_byte & 0x10) {
            a->look |= 0x0E00;
            a->anim_flags |= 0x02;
            a->flags |= 0xB0;
            AnimStart(a, 1);
            a->resume = STAY;
            return 1;
        }
        a->flags |= 0x10;
        a->look |= 0x0C00;
        wait = k96_first_wait(a->spawn_byte);
        figure(a)->frames = wait;
        a->resume = K96_SECOND_WAIT;
        return wait ? wait : 256;
    case K96_SECOND_WAIT:
        wait = figure(a)->frames;
        a->resume = K96_SHOW;
        return wait ? wait : 256;
    case K96_SHOW:
        a->anim_flags |= 0x03;
        a->flags |= 0x80;
        SoundQueueAdd(0x44);
        AnimStart(a, (uint8_t)(a->spawn_byte << 1));
        /* the carry of the original's last add: its list's word + $2000, then + the set's base */
        a->resume = ((uint32_t)(uint16_t)(a->picture_gfx - a->gfx_base + 0x2000) + a->gfx_base) >> 16 ? K96_STAY_C1 : K96_STAY_C0;
        return 0xFF;
    case K96_STAY_C0:
        return 0xB0;
    case K96_STAY_C1:
        return 0xB1;
    default:                            /* STAY */
        return 1;
    }
}
