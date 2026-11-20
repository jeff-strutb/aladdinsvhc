/*
 * actor_loop.c: the actor loop (docs/aladdin/ACTORS.md). Each frame the
 * player's record and then every other record is looked at: a free one is
 * skipped; a waiting one counts down, and goes back into its kind's code
 * when its wait runs out; one to start goes to its kind's start entry. The
 * kind's code returns its next wait (the original's BRK or COP) or that it
 * ended. A record whose flags have bit 7 set runs its kind's wait routine
 * each frame it waits, and after each turn of its code.
 */
#include "actor/actor_own.h"

/* A record's turn of its code: the start entry its state picks ($0C-$7F,
   $80-$BF, $C0 up), or where it stopped. Returns ACTOR_ENDED, or the wait
   it set. */
static int run_record(Actor *a)
{
    uint8_t was = a->state;
    ActorCode code;
    int r;

    a->state = ACTOR_STATE_RUNNING;
    if (was >= ACTOR_STATE_NEW) {
        uint8_t start = !(was & 0x80) ? ACTOR_START : !(was & 0x40) ? ACTOR_START_SECOND : ACTOR_START_THIRD;

        a->resume = start;
        code = actor_kind_code(a->kind, start);
    } else {
        code = actor_kind_code(a->kind, ACTOR_RESUME_OWN);    /* where it stopped */
    }
    if (!code) {
        /* no code for it yet (reported once): it stays, waiting a frame */
        a->state = ACTOR_STATE_WAITING;
        a->wait = 1;
        return 1;
    }
    r = code(a);
    if (r == ACTOR_ENDED)
        return ACTOR_ENDED;
    a->state = ACTOR_STATE_WAITING;
    a->wait = (uint8_t)r;               /* 256 frames: 0, which the countdown takes round */
    return r;
}

/* After a turn, or a frame of waiting: the wait routine, when bit 7 of its
   flags asks for it. */
static void wait_work(Actor *a)
{
    ActorWaitCode w;

    if (!(a->flags & 0x80))
        return;
    w = actor_kind_wait(a->kind);
    if (w)
        (void)w(a);                     /* a wait routine that ends its actor leaves it free: nothing follows either way */
}

/* One record's turn, as both loops take it. */
static void record_turn(Actor *a)
{
    if (a->state == ACTOR_STATE_FREE)
        return;
    if (a->state == ACTOR_STATE_WAITING && --a->wait != 0) {
        wait_work(a);
        return;
    }
    if (run_record(a) != ACTOR_ENDED)
        wait_work(a);
}

/*
 * @routine 83:9475 PlayerLoop
 * WHAT IT DOES: The player's turn of the actor loop: its record run (its
 * kind's code) when its wait runs out or it starts, else its wait routine.
 * Also 83:948B PlayerLoopBack, the turn after its code yields.
 */
void PlayerLoop(void)
{
    record_turn(&g.actors[0]);
}

/*
 * @routine 83:94A4 ActorLoop
 * WHAT IT DOES: The other actors' turns: each of the 35 records after the
 * player's looked at in turn and run when due. With g.pause_block set, the
 * loop stops before the pool's records (actors[8] on).
 * Also 83:94BF ActorLoopBack and 83:94D3 ActorLoopNext, the loop's re-entries
 * after an actor yields or ends and its step to the next record.
 */
void ActorLoop(void)
{
    int i;

    for (i = 1; i < ACTOR_RECORDS; i++) {
        record_turn(&g.actors[i]);
        if (i + 1 >= ACTOR_RECORDS - ACTOR_POOL_SIZE && g.pause_block)
            return;
    }
}
