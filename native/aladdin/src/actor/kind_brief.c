/*
 * kind_brief.c: actors that play one animation for a few frames and end:
 * kind $11 (animation 8, 8 frames) and kind $13 (sound 8, animation $0B,
 * 6 frames).
 */
#include "actor/actor_own.h"
#include "audio/sound.h"

/* the own bytes these kinds use: +$40, the frames left */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t frames;             /* +$40 */
} BriefOwn;

static inline BriefOwn *brief(Actor *a) { return (BriefOwn *)a->own.bytes; }

enum { STEP = ACTOR_RESUME_OWN };

/* The look's high byte (the record's +$2C): the sprites' palette and priority bits. */
static void look_high(Actor *a, uint8_t high)
{
    a->look = (uint16_t)((a->look & 0x00FF) | high << 8);
}

/* Each frame after the first: the animation one frame on; when its frames
   run out the actor ends. */
static int step(Actor *a)
{
    AnimStep(a);
    if (--brief(a)->frames)
        return 1;
    return ActorEnd(a);
}

/*
 * @routine 81:EB90 ActorBriefAnim8
 * WHAT IT DOES: Actor kind $11 (through 83:9216 Kind11Start): drawn with
 * flags bit 4 (its sprite priority), look's high byte 4, animation 8 for 8
 * frames, then it ends (ActorEnd).
 */
int ActorBriefAnim8(Actor *a)
{
    if (a->resume == STEP)
        return step(a);
    a->flags |= 0x10;
    AnimStart(a, 0x08);
    look_high(a, 0x04);
    brief(a)->frames = 0x08;
    a->resume = STEP;
    return 1;
}

/*
 * @routine 81:EBB2 ActorBriefAnimB
 * WHAT IT DOES: Actor kind $13 (through 83:9222 Kind13Start): sound 8,
 * anim_flags bit 0 set, flags $10, animation $0B with look's high byte 4
 * for 6 frames, then it ends (ActorEnd).
 */
int ActorBriefAnimB(Actor *a)
{
    if (a->resume == STEP)
        return step(a);
    SoundQueueAdd(0x08);
    a->anim_flags |= 0x01;
    a->flags = 0x10;
    AnimStart(a, 0x0B);
    look_high(a, 0x04);
    brief(a)->frames = 0x06;
    a->resume = STEP;
    return 1;
}

