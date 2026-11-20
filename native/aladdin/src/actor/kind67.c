/*
 * kind67.c: actor kind $67, which moves the backdrop layer up and down the
 * screen by its speed a frame, in turns, and catches Aladdin while he and
 * his test box are at the backdrop's height.
 */
#include "actor/actor_a.h"
#include "player/player.h"

#define FRAMES OWN_40           /* +$40 (word): frames left of the drift at full speed */
#define SPEED OWN_42            /* +$42 (word): fractions of a pixel a frame */

#define FULL_FRAMES 0x0080
#define FULL_SPEED 0x0080
#define REST_FRAMES 0x30

enum { DOWN_FULL = ACTOR_RESUME_OWN, DOWN_SLOW, DOWN_REST, UP_FULL, UP_SLOW, UP_REST };

static uint16_t own_word(const Actor *a, int at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own_word_set(Actor *a, int at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

/* the backdrop's y offset, 32 bits from $1A2B */
static uint32_t offset_get(void)
{
    return g.backdrop_offset_y | (uint32_t)g.backdrop_offset_y_high << 16;
}

static void offset_set(uint32_t v)
{
    g.backdrop_offset_y = (uint16_t)v;
    g.backdrop_offset_y_high = (uint16_t)(v >> 16);
}

/*
 * @routine 83:E653 BackdropDriftDown
 * WHAT IT DOES: Moves the backdrop down the screen by the actor's speed:
 * its offset against the play field's scroll falls.
 */
static void BackdropDriftDown(const Actor *a)
{
    offset_set(offset_get() - own_word(a, SPEED));
}

/*
 * @routine 83:E666 BackdropDriftUp
 * WHAT IT DOES: Moves the backdrop up the screen by the actor's speed.
 */
static void BackdropDriftUp(const Actor *a)
{
    offset_set(offset_get() + own_word(a, SPEED));
}

/*
 * @routine 83:E645 ActorCount42Down
 * WHAT IT DOES: Counts the actor's word at +$42 down by 1, not below 0.
 */
static void ActorCount42Down(Actor *a)
{
    uint16_t v = own_word(a, SPEED);

    own_word_set(a, SPEED, v ? (uint16_t)(v - 1) : 0);
}

/* a drift's start: its frames and speed */
static void drift_start(Actor *a)
{
    own_word_set(a, FRAMES, FULL_FRAMES);
    own_word_set(a, SPEED, FULL_SPEED);
}

/*
 * @routine 83:E5F1 Kind67Start
 * WHAT IT DOES: Actor kind $67: drifts the backdrop down the screen for 128
 * frames at $80 fractions of a pixel a frame, slows to a stop by 1 a frame,
 * rests 48 frames; then the same upwards; for good. Its second entry
 * (83:8DFD's table) goes on as kind $68's start: left aside.
 */
int Kind67Start(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->flags = 0x80;
        drift_start(a);
        a->resume = DOWN_FULL;
        return 1;
    case DOWN_FULL:
        BackdropDriftDown(a);
        own_word_set(a, FRAMES, (uint16_t)(own_word(a, FRAMES) - 1));
        if (own_word(a, FRAMES))
            return 1;
        a->resume = DOWN_SLOW;
        return 1;
    case DOWN_SLOW:
        ActorCount42Down(a);
        BackdropDriftDown(a);
        if (own_word(a, SPEED))
            return 1;
        a->resume = DOWN_REST;
        return REST_FRAMES;
    case DOWN_REST:
        drift_start(a);
        a->resume = UP_FULL;
        return 1;
    case UP_FULL:
        BackdropDriftUp(a);
        own_word_set(a, FRAMES, (uint16_t)(own_word(a, FRAMES) - 1));
        if (own_word(a, FRAMES))
            return 1;
        a->resume = UP_SLOW;
        return 1;
    case UP_SLOW:
        ActorCount42Down(a);
        BackdropDriftUp(a);
        if (own_word(a, SPEED))
            return 1;
        a->resume = UP_REST;
        return REST_FRAMES;
    default:                            /* UP_REST */
        drift_start(a);
        a->resume = DOWN_FULL;
        return 1;
    }
}

/*
 * @routine 83:E679 Kind67Wait
 * WHAT IT DOES: Kind $67's wait: puts Aladdin into state $8C while he is
 * across x $04F8-$0E47 and his test box is within its zone (from the
 * backdrop's offset, the word at $1A2C) up and down.
 */
int Kind67Wait(Actor *a)
{
    uint16_t half, backdrop, d;

    if ((uint16_t)(pixels(g.actors[0].x) - 0x09A0 + 0x04A8) >= 0x0950)
        return ACTOR_GOES_ON;
    half = (uint16_t)(g.test_box_half_h + 0x004C);
    backdrop = (uint16_t)(offset_get() >> 8);
    d = (uint16_t)((uint16_t)(0 - backdrop) + 0x00C0 - g.test_box_y + half);
    if (d < (uint16_t)(half << 1))
        (void)PlayerEnter8CFacing(a);
    return ACTOR_GOES_ON;
}
