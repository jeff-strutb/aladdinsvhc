/*
 * chain_followers.c: what the actors of the nine-piece chain (the head,
 * kind $73, and the pieces, kinds $74 and $75) do once a list start
 * (ActorListStart) sends them on: each waits the frames the start gave it
 * in +$44 (its leader's step apart), then starts its byte stream
 * (ActorStreamStart) or slides its +$42 to a target, and waits for good.
 * The record's own code (whichever of the three kinds it is) hands such a
 * resume point to ChainFollow.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"

#define INDEX OWN_40            /* +$40: its place in the chain */
#define WAIT OWN_44             /* +$44: frames left before it goes on */
#define DONE(a) ((a)->probe_at) /* +$18: counted up once it has finished (the list's test) */

enum {
    STREAM0_COUNT = CHAIN_FOLLOW_AFTER, /* each frame of its wait, then stream 0 */
    STREAM2_COUNT,
    STREAM14_COUNT,
    LATE_COUNT,
    SLIDE_WAIT,                 /* each frame of its wait before it slides */
    SLIDE_STEP,                 /* each frame of its slide */
    FOLLOW_STAY,                /* finished: it waits for good (ActorYieldForever) */
};

static uint16_t own_word(const Actor *a, int at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void set_own_word(Actor *a, int at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

/*
 * @routine 84:8D41 ActorListStart
 * WHAT IT DOES: Starts every actor in the list this actor owns (the records
 * its link leads through, from the first, until a link of none): each is
 * sent on at resume point `point` (kept also in its +$35-$36, where
 * ChainPieceEnd finds it), its counters at +$18, +$48, +$49 and +$4C
 * cleared, to go on next frame, and its +$44 a value that steps by `step`
 * down the list from `step`.
 */
void ActorListStart(Actor *a, uint16_t point, uint8_t step)
{
    uint8_t value = step;
    Actor *r;

    for (r = a->next; r; r = r->next) {
        r->resume = point;
        set_own_word(r, OWN_35, point);
        DONE(r) = 0;
        r->own.bytes[OWN_4C] = 0;
        r->own.bytes[OWN_48] = 0;
        r->own.bytes[OWN_49] = 0;
        r->wait = 1;
        r->own.bytes[WAIT] = value;
        value = (uint8_t)(value + step);
    }
}

/*
 * @routine 84:8996 ActorListStartAD
 * WHAT IT DOES: Starts every actor in this actor's list on stream 2 (84:89AD),
 * their waits stepping by 8.
 */
void ActorListStartAD(Actor *a)
{
    ActorListStart(a, CHAIN_STREAM2, 8);
}

/*
 * @routine 84:8A52 ActorListFindIdle
 * WHAT IT DOES: Walks the actor's list from the record its link points at
 * to the first whose +$18 is 0 (not finished); ActorListAllDone is that
 * test the other way: true when none is, the end of the list reached. Its
 * callers wait a frame at a time until it is.
 */
int ActorListAllDone(const Actor *a)
{
    const Actor *r;

    for (r = a->next; r; r = r->next)
        if (!DONE(r))
            return 0;
    return 1;
}

/*
 * @routine 84:8DBB ActorStreamStart
 * WHAT IT DOES: Starts the actor on byte stream `x`: its horizontal speed
 * and the word at +$1C from the entry its flip picks (4 bytes), then from
 * the stream's head a byte for +$45, +$46 and +$47, a count less one for
 * +$4D, and a word for +$48 (EORed with the speed's low word, plus the
 * +$1C word); the stream goes on from the word after (Actor.stream), and
 * the counter at +$4C steps.
 */
void ActorStreamStart(Actor *a, uint8_t x)
{
    uint8_t i = (uint8_t)(a->flip << 2);
    uint16_t w1 = stream_speed(i, 1), step;
    DataRef s = stream_ref(x);

    a->speed_x = signed24(((uint32_t)a->speed_x & ~0xFFFFu) | stream_speed(i, 0));
    a->accel_x = (uint8_t)w1;
    a->speed_y = signed24(((uint32_t)a->speed_y & ~0xFFu) | (w1 >> 8));
    a->own.bytes[OWN_45] = path_byte(s, 0);
    a->own.bytes[OWN_46] = a->own.bytes[OWN_45];
    a->own.bytes[OWN_47] = a->own.bytes[OWN_45];
    a->own.bytes[OWN_4D] = (uint8_t)(path_byte(s, 1) - 1);
    step = (uint16_t)((path_word(s, 2) ^ (uint16_t)a->speed_x) + (uint16_t)(a->accel_x | (a->speed_y & 0xFF) << 8));
    set_own_word(a, OWN_48, step);
    a->stream = path_skip(s, 4);
    a->own.bytes[OWN_4C]++;
}

/* the actor's wait over: it stays, for good */
static int stay(Actor *a)
{
    a->resume = FOLLOW_STAY;
    return 1;
}

/* the wait before a stream: each frame its +$44 down, after one more first */
static int stream_wait(Actor *a, int next)
{
    a->resume = (uint16_t)next;
    return 1;
}

/* the slide's row, then its wait */
static int slide_start(Actor *a, uint8_t row)
{
    set_own_word(a, OWN_16, row);
    a->resume = a->own.bytes[WAIT] ? SLIDE_WAIT : SLIDE_STEP;
    return 1;
}

/*
 * @routine 84:89D5 ChainSlideRow0
 * WHAT IT DOES: (the slide's body, with 84:89D1 ChainSlideRow2, 84:89CD
 * ChainSlideRow4 and 84:89C9 ChainSlideRow6, which take its row first) A
 * chain actor that waits its turn, then each frame moves its value +$42
 * toward its place's target in its row (all at once when nearer than the
 * row's step, else by the step), and once there marks itself finished
 * (+$18) and waits for good.
 */
static int slide_step(Actor *a)
{
    uint16_t row = own_word(a, OWN_16), target = slide_target((uint8_t)row, a->own.bytes[INDEX]);
    uint16_t at = own_word(a, OWN_42), diff = (uint16_t)(target - at), step;
    unsigned y = (unsigned)row * 2, side = 0, dist = diff;

    if (diff & 0x8000) {
        side = 2;
        dist = (uint16_t)-diff;
    }
    /* the row's steps are words at an odd place in the table (84:89E5 reads the threshold at +1): the original's step load adds
       the compare's carry, set when the distance is not less than the threshold, to the side */
    if (dist >= slide_step_word(y + 1))
        step = slide_step_word(y + side + 1);
    else
        step = diff;
    at = (uint16_t)(at + step);
    set_own_word(a, OWN_42, at);
    if (at != target)
        return 1;
    DONE(a)++;
    return stay(a);
}

/*
 * @routine 84:89A0 ChainFollowStream0, 84:89AD ChainFollowStream2,
 * 84:89BB ChainFollowStream14, 84:8A2E ChainFollowLate
 * WHAT IT DOES: The actors a list start sends on at these points wait
 * their +$44 frames, then start a byte stream and wait for good: stream 0
 * (89A0), 2 (89AD), $14 (89BB: when its wait was one frame, else as 89AD)
 * or, by the frames its place gives, the stream its place picks (8A2E).
 * @routine 84:89C9 ChainSlideRow6 and its kin: ChainSlide below.
 */
int ChainFollow(Actor *a)
{
    switch (a->resume) {
    case CHAIN_STREAM0:
        return stream_wait(a, STREAM0_COUNT);
    case STREAM0_COUNT:
        if (--a->own.bytes[WAIT])
            return 1;
        ActorStreamStart(a, 0x00);
        return stay(a);
    case CHAIN_STREAM2:
        return stream_wait(a, STREAM2_COUNT);
    case STREAM2_COUNT:
        if (--a->own.bytes[WAIT])
            return 1;
        ActorStreamStart(a, 0x02);
        return stay(a);
    case CHAIN_STREAM14:
        return stream_wait(a, STREAM14_COUNT);
    case STREAM14_COUNT:
        if (--a->own.bytes[WAIT])
            return stream_wait(a, STREAM2_COUNT);
        ActorStreamStart(a, 0x14);
        return stay(a);
    case CHAIN_SLIDE6:
        return slide_start(a, 6);
    case CHAIN_SLIDE4:
        return slide_start(a, 4);
    case CHAIN_SLIDE2:
        return slide_start(a, 2);
    case CHAIN_SLIDE0:
        return slide_start(a, 0);
    case SLIDE_WAIT:
        if (--a->own.bytes[WAIT])
            return 1;
        a->resume = SLIDE_STEP;
        return 1;
    case SLIDE_STEP:
        return slide_step(a);
    case CHAIN_LATE:
        a->own.bytes[WAIT] = follow_wait(a->own.bytes[INDEX]);
        return stream_wait(a, LATE_COUNT);
    case LATE_COUNT:
        if (--a->own.bytes[WAIT])
            return 1;
        ActorStreamStart(a, (uint8_t)((a->own.bytes[INDEX] + 2) << 1));
        return stay(a);
    default:                            /* FOLLOW_STAY: ActorYieldForever */
        return 1;
    }
}
