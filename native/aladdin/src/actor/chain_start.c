/*
 * chain_start.c: the starts of the nine-piece chain's pieces (kinds $74
 * and $75), and the pattern its eighth piece runs for the whole chain: in
 * rounds, a sweep across at random speeds, or the head's animations with
 * the pieces' lists of children, by random picks per round.
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "data/kinds_e_tables.h"
#include "audio/sound.h"
#include "video/video.h"

#define PIECE_NUMBER OWN_40     /* +$40: the piece's place in the chain (8: the one that runs the pattern) */
#define SWEEP_SPEED OWN_3A      /* the sweep's speed entry (the original's +$1D); negative: still */
#define SWEEP_FRAMES OWN_44     /* +$44: frames left of a sweep */
#define SWEEP_FROM 0x80         /* the sweep keeps between these x pixels (low byte) */
#define SWEEP_TO 0xF0
#define LAST_PIECE 8
#define RANDOM_SWEEP 0x0C       /* the pick lists' bases: the sweep's pick, the head turn's pick, the sweep's speed */
#define RANDOM_TURN 0x14
#define RANDOM_SPEED 0x21
#define OWN_CODE_TURN 2         /* the head turn's pick that runs the head's own code */
#define OWN_CODE_TURNS 4        /* the head's own-code turns before the long one */

enum {
    PIECE_WAIT = ACTOR_RESUME_OWN,  /* each frame until the chain is built */
    PIECE_ON,                       /* the frame after: the eighth runs the pattern, the others stay */
    IDLE_FIRST,                     /* each frame until the first list has finished */
    SWEEP_RUN,                      /* each frame of a sweep */
    HEAD_CUE_WAIT,                  /* each frame until the head is done with its own turn */
    TURN_STEP_DOWN,                 /* the long turn: each frame the count steps down 16 */
    TURN_IDLE_A,
    TURN_AFTER_8,
    TURN_STEP_UP,                   /* each frame the count steps up 16 */
    TURN_IDLE_B,
    TURN_AFTER_3F,
    TURN_IDLE_C,
    PIECE_STAY,
};

/* A random pick from list (round | g.chain_round_or) + base. */
static uint8_t pick(Actor *a, uint8_t base)
{
    (void)a;
    return (uint8_t)RandomFromList((uint8_t)((g.chain_round | g.chain_round_or) + base));
}

/*
 * @routine 84:8982 StepCountUp, 84:8989 StepCountDown (system/step_count.c's record)
 * WHAT IT DOES: The chain's step byte counted up by one (true when it has
 * wrapped to 0), or down by one (true at $F0): the long turn's loops run
 * 16 steps each way. The chain is their only user.
 */
int StepCountUp(void) { return ++g.chain_step == 0; }
int StepCountDown(void) { return --g.chain_step == 0xF0; }

static int next_phase(Actor *a);

/* Phase 0: the sweep across, $FF frames at a random speed, kept between x
   $80 and $F0, stopped when Aladdin is to its right. */
static int sweep_begin(Actor *a)
{
    g.head_anim = 0;
    if (g.first_sweeps)
        g.first_sweeps--;
    else
        g.riser_course = pick(a, RANDOM_SWEEP);
    a->own.bytes[SWEEP_SPEED] = (uint8_t)RandomFromList((uint8_t)(g.chain_round + RANDOM_SPEED));
    a->own.bytes[SWEEP_FRAMES] = 0xFF;
    a->resume = SWEEP_RUN;
    return 1;
}

static int sweep_run(Actor *a)
{
    uint8_t s;

    if ((uint8_t)pixels(g.actors[0].x) >= (uint8_t)pixels(a->x))
        a->own.bytes[SWEEP_SPEED] = 0;
    s = a->own.bytes[SWEEP_SPEED];
    if (!(s & 0x80)) {
        uint16_t low = (uint16_t)((uint16_t)a->x + sweep_step(s));
        uint8_t x = (uint8_t)(low >> 8);

        a->x = (a->x & ~0xFFFFu) | low;
        if (!s && x >= SWEEP_TO) {
            a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0xFF00) | SWEEP_TO));
            a->x &= ~0xFFu;
            a->own.bytes[SWEEP_SPEED] = 0x02;
        }
        if (x < SWEEP_FROM) {           /* 84:88AF, which no script meets */
            a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0xFF00) | SWEEP_FROM));
            a->x &= ~0xFFu;
            a->own.bytes[SWEEP_SPEED] = 0;
        }
    }
    if (--a->own.bytes[SWEEP_FRAMES])
        return 1;
    return next_phase(a);
}

/* The head's long turn (84:88D7): its animations 1 and 6 with the pieces'
   children, sound $49, and back. */
static int long_turn(Actor *a)
{
    g.head_turns = 0;
    g.head_anim = 1;
    ActorListStart(a, CHAIN_SLIDE0, 0x01);
    a->probe_at = 1;
    a->resume = TURN_STEP_DOWN;
    return 1;
}

/* Phase 1: by a random pick, the head's long turn, or (three times in
   four) the head's own code (KIND73_OWN_TURN) until it sets g.head_cue. */
static int head_turn(Actor *a)
{
    Actor *head = g.chain_head;

    if (pick(a, RANDOM_TURN) != OWN_CODE_TURN)
        return long_turn(a);
    if (++g.head_turns == OWN_CODE_TURNS)       /* 84:8958, which no script meets */
        return long_turn(a);
    g.head_cue = 0;
    head->resume = KIND73_OWN_TURN;
    head->own.bytes[OWN_35] = (uint8_t)KIND73_OWN_TURN;
    head->own.bytes[OWN_36] = (uint8_t)(KIND73_OWN_TURN >> 8);
    a->resume = HEAD_CUE_WAIT;
    return 1;
}

/* 84:8826: the next phase; each two, the next round (0-3) */
static int next_phase(Actor *a)
{
    g.chain_phase++;
    g.riser_course = 0;
    g.chain_phase &= 0x01;
    if (!g.chain_phase && ++g.chain_round == 4)         /* 84:8840, which no script meets */
        g.chain_round = 0;
    return g.chain_phase ? head_turn(a) : sweep_begin(a);
}

/* $84:87EC: the eighth piece runs the pattern; the others stay. */
static int piece_on(Actor *a)
{
    if (a->own.bytes[PIECE_NUMBER] != LAST_PIECE) {
        a->resume = PIECE_STAY;
        return 1;
    }
    g.chain_phase = 0;
    g.chain_round = 0;
    g.first_sweeps = 2;
    ActorListStart(a, CHAIN_STREAM0, 0x10);
    a->resume = IDLE_FIRST;
    return 1;
}

/*
 * @routine 84:87C2 Kind74Start
 * WHAT IT DOES: Actor kind $74, one of the five rear pieces of the
 * nine-piece chain: once the chain is built (g.chain_go) it starts and
 * animates by its number (ActorStartBy40, ActorAnimBy40), and the eighth
 * piece runs the chain's pattern for good: its list of children on stream
 * 0, then rounds of a sweep across at random speeds and the head's turn
 * (its long turn with the pieces' children, or the head's own code), by
 * random picks from lists by the round; the others stay.
 * @routine 84:87D1 Kind75Start
 * WHAT IT DOES: Actor kind $75, one of the three front pieces: as
 * Kind74Start, with its look taken from kind $74's (and kept for kind
 * $75's) once it starts; its code holds the pattern the eighth piece runs
 * (84:87EC).
 * Their second entry is ChainPieceGone (chain_piece.c); the resume points
 * from CHAIN_RESUME_FIRST are ChainFollow's.
 */
int ChainPieceStart(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->resume = PIECE_WAIT;
        return 1;
    case ACTOR_START_SECOND:
        return ChainPieceGone(a);
    case PIECE_WAIT:
        if (!g.chain_go)
            return 1;
        ActorStartBy40(a);
        ActorAnimBy40(a);
        if (a->kind == 0x75) {
            a->look = g.kind_look[0x74];
            g.kind_look[0x75] = g.kind_look[0x74];
        }
        a->resume = PIECE_ON;
        return 1;
    case PIECE_ON:
        return piece_on(a);
    case IDLE_FIRST:
        if (!ActorListAllDone(a))
            return 1;
        g.chain_untouchable = 0;
        ActorListStartAD(a);
        g.riser_send++;
        g.riser_course = 0;
        return sweep_begin(a);
    case SWEEP_RUN:
        return sweep_run(a);
    case HEAD_CUE_WAIT:
        if (!g.head_cue)
            return 1;
        return next_phase(a);
    case TURN_STEP_DOWN:
        if (!StepCountDown())
            return 1;
        a->resume = TURN_IDLE_A;
        return 1;
    case TURN_IDLE_A:
        if (!ActorListAllDone(a))
            return 1;
        a->resume = TURN_AFTER_8;
        return 8;
    case TURN_AFTER_8:
        g.head_anim = 6;
        ActorListStart(a, CHAIN_LATE, 0x10);
        SoundQueueAdd(0x49);
        g.head_pose = 4;
        a->resume = TURN_STEP_UP;
        return 1;
    case TURN_STEP_UP:
        if (!StepCountUp())
            return 1;
        a->resume = TURN_IDLE_B;
        return 1;
    case TURN_IDLE_B:
        if (!ActorListAllDone(a))
            return 1;
        g.head_anim = 6;
        a->resume = TURN_AFTER_3F;
        return 0x3F;
    case TURN_AFTER_3F:
        g.head_anim = 0;
        g.head_pose = 0;
        ActorListStart(a, CHAIN_SLIDE2, 0x02);
        a->resume = TURN_IDLE_C;
        return 1;
    case TURN_IDLE_C:
        if (!ActorListAllDone(a))
            return 1;
        ActorListStartAD(a);
        return next_phase(a);
    case PIECE_STAY:
        return 1;
    default:
        if (a->resume >= CHAIN_RESUME_FIRST)
            return ChainFollow(a);
        return ChainPieceGone(a);
    }
}
