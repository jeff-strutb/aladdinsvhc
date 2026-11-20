/*
 * kind35.c: actor kind $35, a gem: touched, it counts on the HUD, plays its
 * sound and leaves sparkles. Its wait routine is ActorWaitAnimEnd10
 * (wait_step.c, the actor core's).
 */
#include "actor/actor_own.h"
#include "data/kinds_e_tables.h"

#define SPARKLE_KIND 0x27
#define SET_GEM 0x10            /* +$07 bit 4: a set gem; its low digit picks its bit */
#define NO_SET_STAGE 2          /* the stage (g.demo_number) with no set gems kept */
#define LEVEL_6_LOOKS 6         /* the progress sequence whose gems look and sound otherwise */

/* its own bytes: +$40, the sparkles left to leave */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t sparkles;           /* +$40 */
} Kind35Own;

static inline Kind35Own *own(Actor *a) { return (Kind35Own *)a->own.bytes; }

enum {
    UNTOUCHED = ACTOR_RESUME_OWN,   /* each frame until Aladdin touches it */
    SPARKLE_NEXT,                   /* after a sparkle's 7 frames */
};

/* a set gem's bit in g.gem_sets_found, by its +$07's low digit */
static uint16_t set_bit(const Actor *a)
{
    return k35_set_bit((uint8_t)(a->spawn_byte & 0x0F));
}

/* The touch counted on the HUD's gem counter (two decimal digits): three
   for a set gem; every tenth adds a health mark, or at ten marks refills
   health and counts g.lives up to 100. Returns the sound to play. */
static uint8_t count(Actor *a)
{
    if (a->spawn_byte & SET_GEM) {
        if (g.demo_number != NO_SET_STAGE) {
            g.gem_sets++;
            g.gem_sets_found |= set_bit(a);
        }
        g.hud_count_2 = (uint8_t)(g.hud_count_2 + 2);
    }
    g.hud_count_2++;
    if ((g.hud_count_2 & 0x0F) >= 0x0A) {
        g.hud_count_2 = (uint8_t)(g.hud_count_2 - 0x0A + 0x10);
        if ((g.hud_count_2 & 0xF0) == 0xA0) {
            /* 100 gems (no script reaches it) */
            if (g.health_marks == 0x0A) {
                if (g.lives != 100)
                    g.lives++;
                g.health = 0x0A;
            } else {
                g.health_marks++;
                g.health = g.health_marks;
            }
            g.hud_count_2 &= 0x0F;
            return 0x09;
        }
    }
    return g.progress_sequence == LEVEL_6_LOOKS ? 0x32 : 0x31;
}

/* A sparkle (kind $27) where Aladdin touched it, then 7 frames. */
static int sparkle(Actor *a)
{
    Actor *child;

    a->x = with_pixels(a->x, g.test_box_x);
    a->y = with_pixels(a->y, g.test_box_y);
    child = ActorSpawn(a, SPARKLE_KIND);
    if (child) {
        child->spawn_byte = 0x10;
        child->look = a->look;
        figure(child)->frames = 0x10;
    }
    a->resume = SPARKLE_NEXT;
    return 7;
}

/*
 * @routine 83:BBF2 Kind35Start
 * WHAT IT DOES: Actor kind $35, a gem: when Aladdin (with health) touches
 * it, it counts on the HUD's gem counter (three for a set gem, by bit 4 of
 * +$07, kept as found in g.gem_sets_found), every tenth refills his health
 * or adds a mark (no script has that), plays its sound, is never placed
 * again, and leaves three sparkles (kind $27) where he touched it, 7 frames
 * apart; then it ends. Its look is $04E0 in progress sequence 6, else
 * $0440. A set gem not found yet (or any on stage 2) shows pose $1D; any
 * other is a plain gem (its +$07 cleared) in pose $1C. It tests the touch
 * each frame from the next. Its second entry (state $80 up) ends it.
 */
int Kind35Start(Actor *a)
{
    uint8_t pose;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x02;
        a->flags |= 0x90;
        a->flip = 0;
        a->look = g.progress_sequence == LEVEL_6_LOOKS ? 0x04E0 : 0x0440;
        if ((a->spawn_byte & SET_GEM)
            && (g.demo_number == NO_SET_STAGE || !(set_bit(a) & g.gem_sets_found))) {
            pose = 0x1D;
        } else {
            a->spawn_byte = 0;
            pose = 0x1C;
        }
        AnimStart(a, pose);
        a->resume = UNTOUCHED;
        return 1;
    case UNTOUCHED:
        if (!ActorHitsTestBox(a) || !g.health)
            return 1;
        SoundQueueAdd(count(a));
        g.placed[a->placement] = 0x80;
        a->anim_flags &= (uint8_t)~0x08;
        a->spawn_at = 0;
        own(a)->sparkles = 3;
        return sparkle(a);
    case SPARKLE_NEXT:
        if (--own(a)->sparkles)
            return sparkle(a);
        return ActorEndPlaced(a);
    default:                            /* ACTOR_START_SECOND: 83:BD1E */
        return ActorEndPlaced(a);
    }
}

/*
 * @routine 83:BD0B ActorWaitAnimEnd10
 * WHAT IT DOES: Kind $35's wait routine: each frame it waits, its
 * animation on; then, in progress sequence 6, it ends once the camera has
 * left it behind (the tail at 83:9738); elsewhere once out of range by box
 * $10.
 */
int ActorWaitAnimEnd10(Actor *a)
{
    AnimStep(a);
    if (g.progress_sequence == LEVEL_6_LOOKS)
        return ActorEndIfLeftBehind(a);
    return ActorEndOutOfRange(a, 0x10);
}
